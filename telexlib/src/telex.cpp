#include <iostream>
#include <condition_variable>
#include <thread>
#include <sstream>
#include <future>


#include "semaphore.h"

#include "telex.h"
#include "server.h"
#include "telex_utils.h"
#include "eventqueue.h"

#include "base64.h"
#include "generated/telex.js.h"

#include "idlist.h"
#include "timer.h"
#include "json.h"

#include "telex_internal.h"

using namespace std::chrono_literals;
using namespace Telex;

const std::string SERVER_ADDRESS = "http://localhost";

const std::string DefaultBrowser =
#if defined(UNIX_OS) //maybe works only on Debian derivatives
    "x-www-browser"
#elif defined(MAC_OS)
    "open"
#elif defined(WINDOWS_OS)
    "start /max"
#else
    ""
#endif
        ;

#define CHECK_FATAL(x) if(ec) {error(ec, merge(x, " at ", __LINE__)); return;}  std::cout << x << " - ok" << std::endl;
constexpr char Name[] = "Telex";

void Telex::setDebug() {
    TelexUtils::setLogLevel(TelexUtils::LogLevel::Debug, false);
}

/**
 * The server assumes that file are found at root, therefore we add a '/' if missing
 */
static Ui::Filemap normalizeNames(const Ui::Filemap& files) {
    Ui::Filemap normalized;
    for(const auto& [k, v] : files) {
        if(k.length() > 0 && k[0] != '/') {
            normalized.emplace('/' + k, v);
        } else {
            normalized.emplace(k, v);
        }
    }
    return normalized;
}

 std::string Ui::toStr(const std::atomic<Telex::Ui::State>& s) {
     const std::unordered_map<Telex::Ui::State, std::string> m{
         {Ui::State::NOTSTARTED, "NOTSTARTED"},
         {Ui::State::RUNNING, "RUNNING"},
         {Ui::State::RETRY, "RETRY"},
         {Ui::State::EXIT, "EXIT"},
         {Ui::State::CLOSE, "CLOSE"},
         {Ui::State::RELOAD, "RELOAD"},
         {Ui::State::PENDING, "PENDING"}};
     return m.at(s.load());
}

Ui::Ui(const std::string& indexHtml, unsigned short port, const std::string& root) : Ui(indexHtml,
#if defined(UNIX_OS) //maybe works only on Debian derivatives
               "x-www-browser"
#elif defined(MAC_OS)
               "open"
#elif defined(WINDOWS_OS)
	"start /max"
#else
#error "I have no idea what browser should be spawned, please use other constructor"
	""
#endif
    ,"", port, root) {}

Ui::Ui(const Filemap& filemap, const std::string& indexHtml, unsigned short port, const std::string& root)
    : Ui(filemap, indexHtml, DefaultBrowser, "", port, root) {}

Ui::Ui(const std::string& indexHtml, const std::string& browser, const std::string& extraParams, unsigned short port, const std::string& root) :
    Ui({}, indexHtml, browser, extraParams, port, root){}

Ui::Ui(const Filemap& filemap, const std::string& indexHtml, const std::string& browser, const std::string& extraParams, unsigned short port, const std::string& root) :
    m_eventqueue(std::make_unique<EventQueue<std::tuple<std::string, std::string, std::unordered_map<std::string, std::any>>>>()),
    m_responsemap(std::make_unique<EventMap<std::string, std::any>>()),
    m_sema(std::make_unique<Semaphore>()),
    m_timers(std::make_unique<TimerMgr>()),
    m_onUiExit([this](){exit();}),
    m_filemap(normalizeNames(filemap)) {
    TelexUtils::init();

    m_startup = [this, port, indexHtml, browser, extraParams, root](){
        m_server = std::make_unique<Server>(
                 port,
                 root.empty() ? TelexUtils::workingDir() : root,
                 Name,
                 [this](int){ //open
                    TelexUtils::log(TelexUtils::LogLevel::Debug, "Opening", toStr(m_status));
                    if(m_status == State::CLOSE || m_status == State::PENDING) {
                        TelexUtils::log(TelexUtils::LogLevel::Debug, "Request reload, Status change --> Reload");
                        m_status = State::RELOAD;
                    }
                 //   setLogging(Utils::logLevel() == Utils::LogLevel::Debug);
                    if(m_sema)
                        m_sema->signal(); // there may be some pending requests
                },
                [this](const std::unordered_map<std::string, std::any>& params) { //message
                    const auto kit = params.find("type");
                    if(kit != params.end())  {
                        const auto type = std::any_cast<std::string>(kit->second);
                        TelexUtils::log(TelexUtils::LogLevel::Debug, "message", type);
                        if(type == "event") {
                            const auto element = std::any_cast<std::string>(params.at("element"));
                            const auto event = std::any_cast<std::string>(params.at("event"));
                            const auto properties = std::any_cast<std::unordered_map<std::string, std::any>>(params.at("properties"));
                            m_eventqueue->push({element, event, properties});
                        } else if(type == "query") {
                            const auto key = std::any_cast<std::string>(params.at("query_value"));
                            const auto id = std::any_cast<std::string>(params.at("query_id"));
                            auto k = params.at(key);
                            m_responsemap->push(id, std::move(k));
                        } else if(type == "extension_response") {
                            const auto id = std::any_cast<std::string>(params.at("extension_id"));
                            const auto key = std::any_cast<std::string>(params.at("extension_call"));
                            auto k = params.at(key);
                            m_responsemap->push(id, std::move(k));
                        } else if(type == "error") {
                            TelexUtils::log(TelexUtils::LogLevel::Error, "JS says at:", std::any_cast<std::string>(params.at("element")),
                                       "error:" , std::any_cast<std::string>(params.at("error")));
                           if(m_onError)
                                m_onError(std::any_cast<std::string>(params.at("element")), std::any_cast<std::string>(params.at("error")));
                        } else if(type == "exit_request") {
                            TelexUtils::log(TelexUtils::LogLevel::Debug, "client kindly asks exit --> Status change Exit");
                            m_status = State::EXIT;
                        }
                        m_sema->signal();
                    }
                },
                [this](Server::Close closeStatus, int code) { //close
                    if(!m_server) {
                        TelexUtils::log(TelexUtils::LogLevel::Debug, "Close, Status change --> Exit");
                        m_status = State::EXIT;
                        m_sema->signal();
                        return;
                    }
                    TelexUtils::log(TelexUtils::LogLevel::Debug, "Telex close",  toStr(m_status), static_cast<int>(closeStatus), m_server->isConnected(), code);

                    if(m_status != State::EXIT && (closeStatus != Server::Close::EXIT  &&  (closeStatus == Server::Close::CLOSE && !m_server->isConnected()))) {
                            pendingClose();
                    }
                    else if(closeStatus == Server::Close::FAIL) {
                        TelexUtils::log(TelexUtils::LogLevel::Debug, "Fail, Status change --> Retry");
                        m_status = State::RETRY;
                    }

                    if(m_status == State::EXIT || m_status == State::RETRY) {
                        m_sema->signal();
                }
                },
                [this](const std::string& name)->std::optional<std::string> { //get
                    if(name == "/telex.js") {
                        const auto encoded = Base64::decode(Telexjs);
                        const auto page = TelexUtils::join(encoded.begin(), encoded.end());
                        return std::optional<std::string>(page);
                    }
                    const auto it = m_filemap.find(name);
                    if(it != m_filemap.end()) {
                        const auto encoded = Base64::decode(it->second);
                        const auto page = TelexUtils::join(encoded.begin(), encoded.end());
                        return std::optional<std::string>(page);
                    }
                    return std::nullopt;
                },
                [this, indexHtml, browser, extraParams](auto port)->bool { //listening
                    if(m_status == State::EXIT)
                        return false; //we are on exit, no more listening please
                    TelexUtils::log(TelexUtils::LogLevel::Debug, "Listening, Status change --> Running");
                    m_status = State::RUNNING;
                    const auto appPage = TelexUtils::split<std::vector<std::string>>(indexHtml, '/').back();
                    telex_utils_assert_x(!browser.empty() || !DefaultBrowser.empty(), "I have no idea what browser should be spawned, please use other constructor");
                    const auto cmdLine = (browser.empty() ? DefaultBrowser : browser)
                            + " " + SERVER_ADDRESS + ":"
                            + std::to_string(port) + "/"
                            + (appPage.empty() ? "index.html" : appPage)
                            + " " + extraParams;
                    const auto result = std::system((cmdLine + "&").c_str() );
                    if(result != 0) {
                        TelexUtils::log(TelexUtils::LogLevel::Fatal,"Cannot open:", cmdLine);
                    } else {
                        TelexUtils::log(TelexUtils::LogLevel::Debug,"Opening:", cmdLine);
                    }
                    return true;
                }
                );};
}

Ui::~Ui() {
    TelexUtils::log(TelexUtils::LogLevel::Debug, "Ui Destructor");
    exit();
}

void Ui::pendingClose() {
    TelexUtils::log(TelexUtils::LogLevel::Debug, "Pending close, Status change --> Pending");
    m_status = State::PENDING;
    m_timers->flush(false); //all timers are run here
    TelexUtils::log(TelexUtils::LogLevel::Debug, "Start 1s wait for pending");
    startTimer(1000ms, true, [this]() { //delay as a get may come due page chage
    if(m_status == State::PENDING) {
        TelexUtils::log(TelexUtils::LogLevel::Debug, "Pending close, Status change --> Exit");
        m_status = State::CLOSE;
        m_sema->signal();
    } else {
         TelexUtils::log(TelexUtils::LogLevel::Debug, "Pending cancelled", toStr(m_status));
    }
    });
}

void Ui::close() {
        addRequest([this](){
            return m_server->send({{"type", "close_request"}});
        });
}

void Ui::exit() {
    if(m_status == State::RUNNING) {
        addRequest([this](){
            return m_server->send({{"type", "exit_request"}});
        });
        //Utils::log(Utils::LogLevel::Debug, "Status change -> CLOSE");
        //m_status = State::CLOSE;
        m_timers->flush(true);
        TelexUtils::log(TelexUtils::LogLevel::Debug, "exit - wait in eventloop", toStr(m_status));
        eventLoop();
        TelexUtils::log(TelexUtils::LogLevel::Debug, "exit - wait in eventloop done, back in mainloop", toStr(m_status));
        m_sema->signal();
    } else if(m_status == State::CLOSE) {
        TelexUtils::log(TelexUtils::LogLevel::Debug, "Status change -> EXIT");
        m_status = State::EXIT;  //there will be no one
        m_sema->signal();
    }
   // } else
   //     m_status = State::EXIT;
  //   m_sema->signal();
}

#ifndef ENSURE_SEND
#define ENSURE_SEND 65536
#endif


//DIRECT_DATA is MAYBE ok
//#define DIRECT_DATA

void Ui::send(const DataPtr &data) {
#ifndef DIRECT_DATA
    const auto clonedBytes = data->clone();
    addRequest([this, clonedBytes]() {
         const auto [bytes, len] = clonedBytes->payload();
#else
     const auto [bytes, len] = data->payload();
#endif
            const auto ok = m_server->send(bytes, len);
            if(ok && len > ENSURE_SEND) {           //For some reason the DataPtr MAY not be send (propability high on my mac), but his cludge seems to fix it
                send(root(), "nil", "");     //correct fix may be adjust buffers and or send Data in several smaller packets .i.e. in case of canvas as
            }                                        //multiple tiles
            return ok;
#ifndef DIRECT_DATA
    });
#endif
}

/*
void Ui::send(const Element& el, const std::string& type, const std::string& data) {
    m_requestqueue.emplace_back([this, el, type, data](){
        m_server->send({{"element", el.m_id}, {"type", type}, {type, data}});
    });
    m_sema->signal();

    if(type != "nil" && data.length() > ENSURE_SEND) {
                                 // Im not sure this workaround is needed, but DataPtr messages may not get send immediately and
        send(root(), "nil", ""); // therefore I have to push another message :-( maybe works without, dunno  - bug in uWs? See above in another send
        TelexUtils::log(TelexUtils::LogLevel::Debug, "send data", type);
    }
}

void Ui::send(const Element& el, const std::string& type, const std::vector<std::pair<std::string, std::string>>& values) {
    TelexUtils::log(TelexUtils::LogLevel::Debug, "send values", TelexUtils::joinPairs(values.begin(), values.end()));
    std::unordered_map<std::string, std::string> params {{"element", el.m_id}, {"type", type}};
    for(const auto& [k, v] : values) {
        params.emplace(k, v);
    }
    m_requestqueue.emplace_back([this, params](){
        m_server->send(params);
    });
    m_sema->signal();
}
*/

void Ui::beginBatch() {
    addRequest([this](){
        return m_server->beginBatch();
    });
}

void Ui::endBatch() {
    addRequest([this](){
       return m_server->endBatch();
    });
}

void Ui::send(const Element& el, const std::string& type, const std::any& values) {
    std::unordered_map<std::string, std::string> params {{"element", el.m_id}, {"type", type}};
    if(const auto s = std::any_cast<std::string>(&values)) {
        params.emplace(type, *s);
        addRequest([this, params](){
            return m_server->send(params);
        });
    }
    else if(const auto* c = std::any_cast<const char*>(&values)) {
        params.emplace(type, std::string(*c));
        addRequest([this, params](){
            return m_server->send(params);
        });
    } else {
        addRequest([this, params, values](){
            return m_server->send(params, values);
        });
    }
}


Ui::TimerId Ui::startTimer(const std::chrono::milliseconds &ms, bool singleShot, const std::function<void ()>& timerFunc) {
    return startTimer(ms, singleShot, [timerFunc](TimerId){
        return timerFunc();
    });
}


Ui::TimerId Ui::startTimer(const std::chrono::milliseconds &ms, bool singleShot, const std::function<void (TimerId)>& timerFunc) {
    const int id = m_timers->append(ms, [this, timerFunc, singleShot](int id) {
        TelexUtils::log(TelexUtils::LogLevel::Debug, "Timer added to run", id, toStr(m_status));
        m_timerqueue.emplace_back([timerFunc, id, singleShot, this]() {
            if(!m_timers->blessed(id))
                return;
            m_timers->takeBless(id);
            TelexUtils::log(TelexUtils::LogLevel::Debug, "Timer running", id);
            timerFunc(id);
            if(singleShot)   {
                TelexUtils::log(TelexUtils::LogLevel::Debug, "Timer", id, "decided to finish");
                stopTimer(id);
            } else {
                TelexUtils::log(TelexUtils::LogLevel::Debug, "Timer bless", id);
                m_timers->bless(id);
            }
        });
        m_sema->signal();
    });
    TelexUtils::log(TelexUtils::LogLevel::Debug, "Start Timer", ms.count(), id);
    return id;
}


bool Ui::stopTimer(TimerId id) {
    TelexUtils::log(TelexUtils::LogLevel::Debug, "Stop Timer", id);
    return m_timers->remove(id);
}


Ui& Ui::onExit(std::function<void ()> onUiExitFunction) {
    m_onUiExit = std::move(onUiExitFunction);
    return *this;
}

Ui& Ui::onReload(std::function<void ()> onReloadFunction) {
    m_onReload = std::move(onReloadFunction);
    return *this;
}

Ui& Ui::onOpen(std::function<void ()> onOpenFunction ) {
    m_onOpen = std::move(onOpenFunction);
    return *this;
}

Ui& Ui::onError(std::function<void (const std::string &, const std::string &)> onErrorFunction) {
    m_onError = std::move(onErrorFunction);
    return *this;
}

void Ui::run() {
    telex_utils_assert_x(!m_server, "You shall not run more than once");
    m_startup();
    TelexUtils::log(TelexUtils::LogLevel::Debug, "run, Status change --> RUNNING");
    m_status = State::RUNNING;
    eventLoop();
    TelexUtils::log(TelexUtils::LogLevel::Debug, "run, Status change --> EXIT");
    m_status = State::EXIT;
    if(m_server)
        m_server->close();
    TelexUtils::log(TelexUtils::LogLevel::Debug, "run has exit, server is gone");
}

void Ui::eventLoop() {
    while(m_server && m_server->isRunning()) {
        TelexUtils::log(TelexUtils::LogLevel::Debug_Trace, "Eventloop is waiting");
        m_sema->wait();
        if(m_status == State::EXIT) {
            TelexUtils::log(TelexUtils::LogLevel::Debug, "Eventloop is exiting");
            break;
        }

        if(m_status == State::RETRY) {
            TelexUtils::log(TelexUtils::LogLevel::Debug, "Eventloop will retry");
            m_server->retryStart();
            continue;
        }

        if(m_status == State::CLOSE) {
            TelexUtils::log(TelexUtils::LogLevel::Debug, "Eventloop is Close", m_server && m_server->isRunning());
            if(m_onUiExit) {
                m_onUiExit();
            }
            if(!m_server->isConnected()) {
                m_server->close(true);
            }
            continue;
        }

        if(m_status == State::RELOAD) {
            TelexUtils::log(TelexUtils::LogLevel::Debug, "Eventloop is Reload");
            if(m_onReload)
                addRequest([this](){
                    m_onReload();
                    return true;
                });
            TelexUtils::log(TelexUtils::LogLevel::Debug, "Reload, Status change --> Running");
            m_status = State::RUNNING;
        }

        if(!m_requestqueue.empty() && m_status == State::EXIT)
            TelexUtils::log(TelexUtils::LogLevel::Debug, "skip timerqueue", toStr(m_status));


        //shoot pending requests
        while(!m_timerqueue.empty() && m_status != State::EXIT) {
            TelexUtils::log(TelexUtils::LogLevel::Debug, "Do timer request", m_timerqueue.size());
            const auto timerfunction = std::move(m_timerqueue.front());
            m_timerqueue.pop_front();
            timerfunction();
            TelexUtils::log(TelexUtils::LogLevel::Debug, "Dod timer request", m_timerqueue.size(),
                       m_timerqueue.empty(), toStr(m_status), !m_timerqueue.empty() && m_status != State::EXIT);
        }

        if(m_status == State::PENDING)
            continue;

        if(m_onOpen && m_status == State::RUNNING && m_server->isConnected()) {
            const auto fptr = m_onOpen;
            addRequest([fptr](){fptr(); return true;}); //we try to keep logic call order
            m_onOpen = nullptr; //as the function may reset the function, we do let that happen
        }

        if(!m_requestqueue.empty() && m_status != State::RUNNING)
            TelexUtils::log(TelexUtils::LogLevel::Debug, "skip requestqueue", toStr(m_status));

        //shoot pending requests
        while(!m_requestqueue.empty() && m_status == State::RUNNING && m_server->isConnected()) {
            TelexUtils::log(TelexUtils::LogLevel::Debug_Trace, "do request");
            m_mutex.lock();
            const std::function<bool ()> topRequest = m_requestqueue.front();
            telex_utils_assert_x(topRequest, "Request is null");
            m_requestqueue.pop_front();
            m_mutex.unlock();
            if(!topRequest()) { //yes I wanna  mutex to be unlocked
                std::lock_guard<std::mutex> lock(m_mutex);
                m_requestqueue.push_back(std::move(topRequest));
            }
        }

        //if there are responses they must be handled
        if(!m_responsemap->empty()) {
            return; //handle query elsewhere
        }

        if(!m_eventqueue->empty() && m_status != State::RUNNING)
            TelexUtils::log(TelexUtils::LogLevel::Debug, "skip eventqueue", toStr(m_status));

        //events must be last as they may generate more requests or responses
        while(!m_eventqueue->empty() && m_status == State::RUNNING) {
            const auto it = m_eventqueue->take();
            const auto element = m_elements.find(std::get<0>(it));
            if(element != m_elements.end()) {
                const auto handlerName = std::get<1>(it);
                const auto handlers = std::get<1>(*element);
                const auto h = handlers.find(handlerName);
                if(h != handlers.end()) {
                    h->second(Element::Event{std::make_shared<Element>(*this, std::move(element->first)), std::move(std::get<2>(it))});
                } else {
                    TelexUtils::log(TelexUtils::LogLevel::Debug, "Cannot find a handler", handlerName, "for element", std::get<0>(it));
                }
            } else {
                TelexUtils::log(TelexUtils::LogLevel::Debug, "Cannot find", std::get<0>(it), "from elements");
            }
        }
    }
}

void Ui::setLogging(bool logging) {
    send(root(), "logging", logging ? "true" : "false");
}

void Ui::eval(const std::string &eval) {
    send(root(), "eval", eval);
}

void Ui::debug(const std::string &msg) {
    send(root(), "debug", msg);
}

void Ui::alert(const std::string &msg) {
    send(root(), "alert", msg);
}



void Ui::open(const std::string &url, const std::string& name) {
    send(root(), "open", std::unordered_map<std::string, std::string>{{"url", url}, {"view", name}});
}

std::optional<std::pair<std::chrono::microseconds, std::chrono::microseconds>> Ui::ping() const {
    const auto ms = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    const clock_t begin_time = ::clock();
    const auto pong = const_cast<Ui*>(this)->query<std::string>(std::string(), "ping");
    if(pong.has_value() && !pong->empty()) {
        auto full = double(::clock () - begin_time ) /  (CLOCKS_PER_SEC / 1000000.0);
        auto half = (stod(*pong) * 1000) - ms.count();
        return std::make_pair(
                    std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double, std::ratio<1, 1000000>>(full)),
                    std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double, std::ratio<1, 1000000>>(half)));
    }
    return std::nullopt;
}

Element Ui::root() const {
    return Element(*const_cast<Ui*>(this), "");
}


std::string Ui::addressOf(const std::string& filepath) const {
    return std::string(SERVER_ADDRESS) + ":" + std::to_string(m_server->port()) +
            "?file=" + TelexUtils::hexify(TelexUtils::absPath(filepath), R"([^a-zA-Z0-9-,.,_~])");
}

std::optional<Element::Elements> Ui::byClass(const std::string& className) const {
    Element::Elements childArray;
    const auto childIds = const_cast<Ui*>(this)->query<std::vector<std::string>>(className, "classes");
    if(!childIds.has_value())
        return std::nullopt;
    for(const auto& cid : *childIds) {
        childArray.push_back(Element(*const_cast<Ui*>(this), cid));
    }
    return m_status == Ui::State::RUNNING ? std::make_optional(childArray) : std::nullopt;
}

std::optional<Element::Elements> Ui::byName(const std::string& className) const {
    Element::Elements childArray;
    const auto childIds = const_cast<Ui*>(this)->query<std::vector<std::string>>(className, "names");
    if(!childIds.has_value())
        return std::nullopt;
    for(const auto& cid : *childIds) {
        childArray.push_back(Element(*const_cast<Ui*>(this), cid));
    }
    return m_status == Ui::State::RUNNING ? std::make_optional(childArray) : std::nullopt;
}

std::optional<std::any> Ui::extension(const std::string& callId, const std::unordered_map<std::string, std::any> &parameters)  {
    if(m_status != State::RUNNING) {
        return std::nullopt;
    }
    const auto queryId = std::to_string(m_server->queryId());

    const auto json = toString(parameters);

    telex_utils_assert_x(json.has_value(), "Invalid parameter");

    addRequest([this, queryId, callId, json](){
                TelexUtils::log(TelexUtils::LogLevel::Debug, "extension:", json.value());
            return m_server->send({{"type", "extension"}, {"extension_id", queryId}, {"extension_call", callId}, {"extension_parameters", json.value()}});
        });

        for(;;) {   //start waiting the response
            eventLoop();
            TelexUtils::log(TelexUtils::LogLevel::Debug, "extension - wait in eventloop done, back in mainloop", toStr(m_status));
            if(m_status != State::RUNNING) {
                m_sema->signal();
                break; //we are gone
            }

            if(m_responsemap->contains(queryId)) {
               const auto item = m_responsemap->take(queryId);
               return std::make_optional(item);
            }
        }
        return std::nullopt;
}
std::optional<std::vector<uint8_t>> Ui::resource(const std::string& url) const {
    const auto it = m_filemap.find(url);
    if(it == m_filemap.end())
        return std::nullopt;
    const auto data = Base64::decode(it->second);
    return std::make_optional(data);
}

bool Ui::addFile(const std::string& url, const std::string& file) {
    if(!TelexUtils::fileExists(file))
        return false;
    const auto it = m_filemap.find(url);
    if(it != m_filemap.end())
        return false;
    const auto data = TelexUtils::slurp<Base64::Byte>(file);
    const auto string = Base64::encode(data);
    m_filemap.insert_or_assign(url, std::move(string));
    return true;
}


