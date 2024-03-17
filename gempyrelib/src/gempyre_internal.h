#ifndef GEMPYRE_INTERNAL
#define GEMPYRE_INTERNAL

#include "gempyre.h"
#include "gempyre_utils.h"
#include "server.h"
#include "semaphore.h"
#include "eventqueue.h"
#include "base64.h"
#include "timequeue.h"
#include <cassert>
#include <unordered_map>


namespace Gempyre {


    constexpr auto SERVER_ADDRESS{"http://localhost"};

    constexpr auto BROWSER_KEY{"browser"};
    constexpr auto BROWSER_PARAMS_KEY{"params"};
    constexpr auto WIDTH_KEY{"width"};
    constexpr auto HEIGHT_KEY{"height"};
    constexpr auto TITLE_KEY{"title"};
    constexpr auto FLAGS_KEY{"flags"};

  enum class State {NOTSTARTED, RUNNING, RETRY, EXIT, CLOSE, RELOAD, PENDING, SUSPEND};

  // There are 3 Ui c'tors
  // 'legacy' with just map + index, that is using AUTO
  //  with browser using BROWSER
  //  with title, width, height, that is using PYTHON
  // AUTO depends on USE_PYTHON_UI 
  enum class WindowType {AUTO, BROWSER, PYTHON};

  inline
  std::string_view str(State state) {
    const char* s[] = 
    {"NOTSTARTED", "RUNNING", "RETRY", "EXIT", "CLOSE", "RELOAD", "PENDING", "SUSPEND"};
    return s[static_cast<unsigned>(state)]; 
  } 

// this class methods follows snake style
// intention is to change eveything to snake for time being
// by time being style is a mess
class GempyreInternal {
    struct Event {
        Element element;
        const Server::Object properties;
    };

    struct HandlerEvent {
        std::string element;
        std::string handler;
        Server::Object data;
        };

    using HandlerFunction = std::function<void (const Event& el)>;
    using HandlerMap = std::unordered_map<std::string, HandlerFunction>;

    template<size_t SZ, typename T>
    void emplace_in(const T& tuple, json& map) {
        if constexpr (SZ > 1) {
            const auto key = std::string(std::get<SZ - 2>(tuple));
            const auto value = std::get<SZ - 1>(tuple);
            map.emplace(std::move(key), std::move(value));
            emplace_in<SZ - 2, T>(tuple, map);
        }
    }    
    

public:
    class LoopWatch {
        public:
        LoopWatch(GempyreInternal& gi, bool is_main) : m_gi(gi) {
            gempyre_utils_assert_x(!is_main || m_gi.m_loop == 0,
             std::to_string(static_cast<unsigned int>(is_main)) + " " + std::to_string(m_gi.m_loop));
            m_gi.m_loop++;
        }
        ~LoopWatch() {
            m_gi.m_loop--;
            gempyre_utils_assert(m_gi.m_loop >= 0);
        }
        private:
        GempyreInternal& m_gi;
    };

    GempyreInternal(
        Ui* ui, 
        const Ui::Filemap& filemap,
        const std::string& indexHtml,
        unsigned short port,
        const std::string& root,
        const std::unordered_map<std::string, std::string>& parameters,
        WindowType windowType
        );

     GempyreInternal(const Gempyre::GempyreInternal&) = delete;
     GempyreInternal& operator=(const Gempyre::GempyreInternal&) = delete;  

    bool operator==(State state) const {return m_status == state;}
    bool operator!=(State state) const {return m_status != state;}

    template<class T>
    std::optional<T> query(const std::string& elId, const std::string& queryString, const std::vector<std::string>& queryParams = {});

    void eventLoop(bool is_main);

    void send(const DataPtr& data, bool droppable);

    template<typename T>
    void send_unique(const Element& el, const std::string& type, const T& value) {
        Server::Value params {
            {"element", el.m_id},
            {"type", type},
            {type, value},
            {"msgid", next_msg_id()}
            };
        add_request([this, params = std::move(params)]() mutable {        
            return send_to(Server::TargetSocket::Ui, std::move(params));
        });    
    }

    template<typename K, typename V, typename... P>
    void send_unique(const Element& el, const std::string& type, const K& key, const V& value, const P&... pairs) {
        json params {
            {"element", el.m_id},
            {"type", type},
            {"msgid", next_msg_id()},
            {key, value}
            };
        constexpr auto count = sizeof...(pairs);
        static_assert((count & 0x1) == 0, "Expect is even");
        emplace_in<count>(std::forward_as_tuple(pairs...), params);
        add_request([this, params = std::move(params)]() mutable {        
            return send_to(Server::TargetSocket::Ui, std::move(params));
        });    
    }

    template<typename T>
    void send(const Element& el, const std::string& type, const T& value) {
         json params {
            {"element", el.m_id},
            {"type", type},
            {type, value}
            };
        add_request([this, params = std::move(params)]() mutable {    
            return send_to(Server::TargetSocket::Ui, std::move(params));
        });    
    }


    template<typename K, typename V, typename... P>
    void send(const Element& el, const std::string& type, const K& key, const V& value, const P&... pairs) {
        json params {
            {"element", el.m_id},
            {"type", type},
            {key, value}
            };
        constexpr auto count = sizeof...(pairs);
        static_assert((count & 0x1) == 0, "Expect is even");
        emplace_in<count>(std::forward_as_tuple(pairs...), params);
        add_request([this, params = std::move(params)]() mutable {        
            return send_to(Server::TargetSocket::Ui, std::move(params));
        });        
    }

    void add_request(std::function<bool()>&& f) {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_requestqueue.emplace_back(f);
        signal_pending();
    }


    std::string_view state_str() const {
        return str(m_status.load());
    }

    void set(State state) {
        //TODO non-execute logging
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "log level", str(m_status), "->", str(state));
        m_status = state;
    }

    bool has_server() const {
        return m_server != nullptr;
    }

    bool is_connected() const {
        return has_server() && m_server->isConnected();
    }

    bool is_running() const {
        return has_server() && m_server->isRunning();
    }


    void add_handler(const std::string& id, const std::string& name, const Element::SubscribeFunction& handler) {
        HandlerFunction hf = [handler](const Event& event) {
            std::unordered_map<std::string, std::string> property_map;
            for(const auto& [k, v] : event.properties) {
                //const auto str_value = GempyreUtils::to_str(v.value());
                std::string value = to_string(v);
                property_map.emplace(k, std::move(value));
            }
            Gempyre::Event ev{event.element, std::move(property_map)};
            handler(ev);
            };

        m_elements[id].emplace(name, std::move(hf));
    }

    void clear_handlers() {
        m_elements.clear();
    }

    void ensure_element_exists(const std::string& id) {
        if(m_elements.find(id) == m_elements.end())
            m_elements.emplace(std::make_pair(id, HandlerMap{}));
    }    


    void signal_pending() {
            m_sema.signal();    // there may be some pending requests
    }

    void push_event(HandlerEvent&& event) {
        m_eventqueue.push(std::move(event));
    }


    std::optional<Server::Value> take_response(const std::string& queryId) {
         if(m_responsemap.contains(queryId)) {
            const auto item = m_responsemap.take(queryId);
            return std::make_optional(item);
        }
        return std::nullopt;
    }

    void push_response(std::string&& id, Server::Value&& response) {
         m_responsemap.push(std::move(id), std::move(response));
    }

    void call_error(const std::string& src, std::string err) {
        if(m_onError) {
                m_onError(src, err);
            }
    }

    std::optional<std::string> file(std::string_view name) const {
        if(name.empty()) return std::nullopt;
        const std::string key{name};
        if(const auto it = m_filemap.find(key); it != m_filemap.end()) {
            return std::make_optional(it->second);
        }
        // if path is "not absolute" - older versions of Gempyre required that filemap
        // paths are absolute (issues inherited from uws) but now we can be more
        // liberal with paths
        if(name[0] != '/') {
            if(const auto it = m_filemap.find('/' + key); it != m_filemap.end())
                return std::make_optional(it->second);
        }
        return std::nullopt; 
    }


    void add_file(const std::string& url, const std::string& file_name) {
        const auto data = GempyreUtils::slurp<Base64::Byte>(file_name);
        add_data(url, data);
    }

    void add_data(const std::string& url, const std::vector<uint8_t>& data) {
        const auto string = Base64::encode(data);
        m_filemap.insert_or_assign(url, std::move(string));
    }

    std::string query_id() const {
        return std::to_string(m_server->queryId());
    }

    
    [[nodiscard]]
    bool send_to(Server::TargetSocket target, Server::Value&& value) {
        return m_server->send(target, std::move(value));
    }

    unsigned port() const {
        return m_server->port();
    }

    void set_hold(bool on_hold) {
        m_hold = on_hold;
    }

    bool hold() const {
        return m_hold;
    }

    std::string next_msg_id() {
        return std::to_string(m_msgId++);
    }

    void flush_timers(bool do_run) {
        m_timers.flush(do_run);
    }

    int append_timer(const TimerMgr::TimeType& ms, bool singleShot, TimerMgr::Callback&& cb) {
        return m_timers.append(ms, singleShot, std::move(cb));        
    }

    bool remove_timer(int id) {
        return m_timers.remove(id);
    } 

    void close_server() {
        if(m_server)
            m_server->close(true);
    }


    bool begin_batch() {
        return m_server->beginBatch();
    }

    bool end_batch() {
        return m_server->endBatch();
    }

    bool has_requests() const {
        return !m_requestqueue.empty();
    }

    bool has_timers() const {
        return !m_timerqueue.empty();
    }

    bool has_responses() const {
        return !m_responsemap.empty();
    }
    
    bool has_events() const {
         return !m_eventqueue.empty();
    }

    Gempyre::Ui::ExitFunction set_on_exit(const Gempyre::Ui::ExitFunction& onUiExitFunction) {
        auto old = std::move(m_onUiExit);
        m_onUiExit = onUiExitFunction;
        return old;
    }

    Gempyre::Ui::ReloadFunction set_on_reload(const Gempyre::Ui::ReloadFunction& onReloadFunction) {
        auto old = std::move(m_onReload);
        m_onReload = onReloadFunction;
        return old;
    }


    Gempyre::Ui::ErrorFunction set_on_error(const Gempyre::Ui::ErrorFunction& onErrorFunction) {
        auto old = std::move(m_onError);
        m_onError = onErrorFunction;
        return old;
    }


    Gempyre::Ui::OpenFunction set_on_open(const Gempyre::Ui::OpenFunction& onOpenFunction) {
        auto old = std::move(m_onOpen);
        m_onOpen = onOpenFunction;
        return old;
    }


    void start_server() {
        gempyre_utils_assert_x(!m_server, "You shall not run more than once");
        m_startup();
    }

    void handle_requests() {
        while(has_timers() && *this != State::EXIT && !m_onOpen && !m_hold) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "Do timer request", m_timerqueue.size());
            std::list<std::function<void ()>> timer(1);
            timer.splice(timer.begin(), m_timerqueue, m_timerqueue.begin());
            const auto& timerfunction = timer.front();
            if(!timerfunction) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer queue miss",
                                state_str(), !m_timerqueue.empty() && *this != State::EXIT);
                continue;
            }
            timerfunction();
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "Dod timer request", m_timerqueue.size(),
                            state_str(), !m_timerqueue.empty() && *this != State::EXIT);
        }
    }

    bool has_open() const {
        return m_onOpen != nullptr;
    }

    bool has_reload() const {
        return m_onReload != nullptr;
    }

    void on_reload() {
        m_onReload();
    }

    std::function<void()> take_open() {
         const auto fptr = std::move(m_onOpen);
         m_onOpen = nullptr;
         return fptr;
    }

    void do_exit() {
        if(m_onUiExit) { // what is point? Should this be here
            m_onUiExit();
        }
        GEM_DEBUG("requests:", m_requestqueue.size(), "timers:", m_timerqueue.size());
        m_requestqueue.clear(); // we have exit, rest of requests get ignored
        GEM_DEBUG("run, exit event loop");
        m_server->close(true);
        assert(!m_server->isJoinable());
        m_server.reset(); // so the run can be recalled
        m_timers.flush(false);
     
        // clear or erase calls destructor and that seems to be issue in raspberry
        while(!m_timerqueue.empty())
            m_timerqueue.pop_front();

        m_sema.undo();    
    
        while (!m_sema.empty()) {
            m_sema.signal();
        }
  //      assert(m_eventqueue.empty());
        assert(m_requestqueue.empty());
        assert(!m_timers.isValid());
        assert(!m_server);
        if(!m_sema.empty()) {
            [[maybe_unused]] auto wait_ok = m_sema.wait(500ms);
            assert(wait_ok);
        }
        assert(m_sema.empty());
        m_elements.clear();
        m_status = State::NOTSTARTED; // reset the state
    }

    void add_timer(std::function<void ()>&& call) {
         m_timerqueue.emplace_back(std::move(call));
    }

    bool retry_start() {
        return m_server->retryStart();
    }

    const std::function<bool ()> take_request() {
        m_requestMutex.lock();
        const std::function<bool ()> topRequest = m_requestqueue.front();
        if(!m_requestqueue.empty())
            m_requestqueue.pop_front();
        m_requestMutex.unlock();
        return topRequest;
    }

    void put_request(std::function<bool ()>&& topRequest ) {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_requestqueue.push_back(std::move(topRequest));
    }

    void wait_events() {
        const auto start = std::chrono::steady_clock::now();
        if(*this == State::CLOSE || *this == State::EXIT)
            m_sema.wait(300ms);
        else    
            m_sema.wait();
        const auto end = std::chrono::steady_clock::now();
        const auto duration = end - start;
        GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "Eventloop is waited", duration.count());
    }

    void clear() {
        m_eventqueue.clear();
        m_requestqueue.clear();
        m_timerqueue.clear();
    }

    bool loop() const {
        return m_loop;
    }

    void messageHandler(Server::Object&& msg);
    void openHandler();
    void closeHandler(CloseStatus closeStatus, int code);
    std::optional<std::string> getHandler(const std::string_view& name);
    void pendingClose();
    bool startListen(const std::string& indexHtml, const std::unordered_map<std::string, std::string>& parameters, int listen_port);
    static std::string to_string(const nlohmann::json& js);
    std::function<void(int)> makeCaller(const std::function<void (Ui::TimerId id)>& function);
    void consume_events();
    void shoot_requests(); 
private:
    Ui* m_app_ui;
    std::atomic<State> m_status = State::NOTSTARTED;
    EventQueue<HandlerEvent> m_eventqueue{};
    EventMap<std::string, Server::Value> m_responsemap{};
    Semaphore  m_sema{};
    TimerMgr m_timers{};
    std::unordered_map<std::string, HandlerMap> m_elements{};
    std::list<std::function<bool ()>> m_requestqueue{};
    std::list<std::function<void ()>> m_timerqueue{};
    std::function<void ()> m_onUiExit{nullptr};
    std::function<void ()> m_onReload{nullptr};
    std::function<void ()> m_onOpen{nullptr};
    std::function<void (const std::string& element, const std::string& info)> m_onError{nullptr};
    Ui::Filemap m_filemap{};
    WindowType m_windowType{};
    std::function<void ()> m_startup{};
    std::unique_ptr<Server> m_server{};
    // protect request_queue
    std::mutex m_requestMutex{};
    bool m_hold{false};
    unsigned m_msgId{1};
    int m_loop{0};
};
}

#endif // GEMPYRE_INTERNAL