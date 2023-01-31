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

  enum class State {NOTSTARTED, RUNNING, RETRY, EXIT, CLOSE, RELOAD, PENDING};
  inline
  std::string_view str(State state) {
    const char* s[] = 
    {"NOTSTARTED", "RUNNING", "RETRY", "EXIT", "CLOSE", "RELOAD", "PENDING"};
    return s[static_cast<unsigned>(state)]; 
  } 

// this class methods follows snake style
// intention is to change eveything to snake for time being
// by time being style is a mess
class GempyreInternal {
    struct Event {
        Element element;
        const std::unordered_map<std::string, std::any> properties;
    };
    using HandlerFunction = std::function<void (const Event& el)>;
    using HandlerMap = std::unordered_map<std::string, HandlerFunction>;

public:
    GempyreInternal(
        Ui* ui, 
        const Ui::Filemap& filemap,
        const std::string& indexHtml,
        unsigned short port,
        const std::string& root,
        const std::unordered_map<std::string, std::string>& parameters);

    bool operator==(State state) const {return m_status == state;}
    bool operator!=(State state) const {return m_status != state;}


    void add_request(std::function<bool()>&& f) {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_requestqueue.emplace_back(f);
        m_sema->signal();
    }

    std::string state_str() const {
    const std::unordered_map<State, std::string> m{
        {State::NOTSTARTED, "NOTSTARTED"},
        {State::RUNNING, "RUNNING"},
        {State::RETRY, "RETRY"},
        {State::EXIT, "EXIT"},
        {State::CLOSE, "CLOSE"},
        {State::RELOAD, "RELOAD"},
        {State::PENDING, "PENDING"}};
    return m.at(m_status.load());
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


    void add_handler(const std::string& id, const std::string& name, std::function<void(const Gempyre::Event&)> handler) {
        HandlerFunction hf = [handler](const Event& event) {
            std::unordered_map<std::string, std::string> property_map;
            for(const auto& [k, v] : event.properties)
                property_map.emplace(k, std::any_cast<std::string>(v));
            Gempyre::Event ev{event.element, std::move(property_map)};
            handler(ev);
            };

        m_elements[id].emplace(name, std::move(hf));
    }


    void ensure_element_exists(const std::string& id) {
        if(m_elements.find(id) == m_elements.end())
            m_elements.emplace(std::make_pair(id, HandlerMap{}));
    }    


    void signal_pending() const {
        if(m_sema) {
            m_sema->signal();    // there may be some pending requests
        }
    }

    void push_event(Ui::InternalEvent&& event) {
        m_eventqueue->push(std::move(event));
    }


    std::optional<std::any> take_response(const std::string& queryId) {
         if(m_responsemap->contains(queryId)) {
            const auto item = m_responsemap->take(queryId);
            return std::make_optional(item);
        }
        return std::nullopt;
    }

    void push_response(std::string&& id, std::any&& response) {
         m_responsemap->push(std::move(id), std::move(response));
    }

    void call_error(const std::string src, std::string err) {
        if(m_onError) {
                m_onError(src, err);
            }
    }

    std::optional<std::string> file(std::string_view name) const {
         const auto it = m_filemap.find(std::string(name));
         if(it != m_filemap.end()) {
            return std::make_optional(it->second);
         }
         return std::nullopt; 
    }


    void add_file(const std::string& url, const std::string& file_name) {
        const auto data = GempyreUtils::slurp<Base64::Byte>(file_name);
        const auto string = Base64::encode(data);
        m_filemap.insert_or_assign(url, std::move(string));
    }


    std::string query_id() const {
        return std::to_string(m_server->queryId());
    }

    
    bool send(const std::unordered_map<std::string, std::string>& object, const std::any& values = std::any()) {
        return m_server->send(object, values);
    }

    
    bool send(const char* data, size_t len) {
        return m_server->send(data, len);
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

    TimerMgr& timers() {
        return *m_timers;
    }

    void close_server() {
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
        return !m_responsemap->empty();
    }
    
    bool has_events() const {
         return !m_eventqueue->empty();
    }

    void set_on_exit(std::function<void ()>&& onUiExitFunction) {
        m_onUiExit = std::move(onUiExitFunction);
    }

    void set_on_reload(std::function<void ()>&& onReloadFunction) {
        m_onReload = std::move(onReloadFunction);
    }


    void set_on_error(std::function<void (const std::string&, const std::string&)>&& onErrorFunction) {
        m_onError = std::move(onErrorFunction);
    }


    void set_on_open(std::function<void ()>&& onOpenFunction) {
        m_onOpen = std::move(onOpenFunction);
    }


    void start_server() {
        gempyre_utils_assert_x(!m_server, "You shall not run more than once");
        m_startup();
    }

    void handle_requests() {
        while(has_timers() && *this != State::EXIT && !m_onOpen && !m_hold) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Do timer request", m_timerqueue.size());
            const auto timerfunction = std::move(m_timerqueue.front());
            m_timerqueue.pop_front();
            if(!timerfunction) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer queue miss",
                                state_str(), !m_timerqueue.empty() && *this != State::EXIT);
                continue;
            }
            timerfunction();
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Dod timer request", m_timerqueue.size(),
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
        timers().flush(false);
    
        // clear or erase calls destructor and that seems to be issue in raspberry
        while(!m_timerqueue.empty())
            m_timerqueue.pop_front();
    
        assert(m_requestqueue.empty());
        assert(!timers().isValid());
    }

    void consume_events(Ui& ui) {
        while(has_events() && *this == State::RUNNING) {
            const auto it = m_eventqueue->take();
            const auto element = m_elements.find(it.element);
            if(element != m_elements.end()) {
                const auto handlerName = it.handler;
                const auto handlers = std::get<1>(*element);
                const auto h = handlers.find(handlerName);

                if(h != handlers.end()) {
                    h->second(Event{Element(ui, std::move(element->first)), std::move(it.data)});
                } else {
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Cannot find a handler", handlerName, "for element", it.element);
                }
            } else {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Cannot find", it.element, "from elements");
            }
        }
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
        gempyre_utils_assert_x(topRequest, "Request is null");
        m_requestqueue.pop_front();
        m_requestMutex.unlock();
        return topRequest;
    }

    void put_request(std::function<bool ()>&& topRequest ) {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_requestqueue.push_back(std::move(topRequest));
    }

    void wait_events() {
        if(!m_sema->empty()) {
            const auto start = std::chrono::steady_clock::now();

            m_sema->wait();

            const auto end = std::chrono::steady_clock::now();
            const auto duration = end - start;
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "Eventloop is waited", duration.count());
        }
    }

    inline void addRequest(std::function<bool()>&& f) {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_requestqueue.emplace_back(f);
        m_sema->signal();
}

private:
        std::atomic<State> m_status = State::NOTSTARTED;
        std::unique_ptr<EventQueue<Ui::InternalEvent>> m_eventqueue;
        std::unique_ptr<EventMap<std::string, std::any>> m_responsemap;
        std::unique_ptr<Semaphore>  m_sema;
        std::unique_ptr<TimerMgr> m_timers;
        std::unordered_map<std::string, HandlerMap> m_elements;
        std::deque<std::function<bool ()>> m_requestqueue;
        std::deque<std::function<void ()>> m_timerqueue;
        std::function<void ()> m_onUiExit{nullptr};
        std::function<void ()> m_onReload{nullptr};
        std::function<void ()> m_onOpen{nullptr};
        std::function<void (const std::string& element, const std::string& info)> m_onError{nullptr};
        Ui::Filemap m_filemap;
        std::function<void ()> m_startup;
        std::unique_ptr<Server> m_server;
        std::mutex m_requestMutex;
        bool m_hold{false};
        unsigned m_msgId{1};
};
}

#endif // GEMPYRE_INTERNAL