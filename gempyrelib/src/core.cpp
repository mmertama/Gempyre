#include <iostream>
#include <condition_variable>
#include <thread>
#include <sstream>
#include <future>
#include <cassert>


#include "semaphore.h"

#include "gempyre.h"
#include "server.h"
#include "gempyre_utils.h"
#include "eventqueue.h"

#include "base64.h"
#include "generated/gempyre.js.h"

#include "idlist.h"
#include "timer.h"

#include "internal.h"

using namespace std::chrono_literals;
using namespace Gempyre;

const std::string SERVER_ADDRESS = "http://localhost";



#ifdef ANDROID_OS
extern int androidLoadUi(const std::string&);
#endif

#define CHECK_FATAL(x) if(ec) {error(ec, merge(x, " at ", __LINE__)); return;}  std::cout << x << " - ok" << std::endl;

void Gempyre::setDebug(Gempyre::DebugLevel level, bool useLog) {
    const std::unordered_map<Gempyre::DebugLevel, GempyreUtils::LogLevel> lvl =  {
        {Gempyre::DebugLevel::Quiet, GempyreUtils::LogLevel::None},
        {Gempyre::DebugLevel::Fatal, GempyreUtils::LogLevel::Fatal},
        {Gempyre::DebugLevel::Error, GempyreUtils::LogLevel::Error},
        {Gempyre::DebugLevel::Warning, GempyreUtils::LogLevel::Warning},
        {Gempyre::DebugLevel::Info, GempyreUtils::LogLevel::Info},
        {Gempyre::DebugLevel::Debug, GempyreUtils::LogLevel::Debug},
        {Gempyre::DebugLevel::Debug_Trace, GempyreUtils::LogLevel::Debug_Trace}
    };
    GempyreUtils::setLogLevel(lvl.at(level), useLog);
}


#ifndef ANDROID_OS
void Gempyre::setJNIENV(void*, void*) {
    GempyreUtils::log(GempyreUtils::LogLevel::Fatal, "setJNIENV should not be called within current OS");
}
#endif

#define STR(x) #x
#define TOSTRING(x) STR(x)

std::tuple<int, int, int> Gempyre::version() {
    static_assert(TOSTRING(GEMPYRE_PROJECT_VERSION)[0], "GEMPYRE_PROJECT_VERSION not set");
    const auto c = GempyreUtils::split<std::vector<std::string>>(TOSTRING(GEMPYRE_PROJECT_VERSION), '.');
    return {GempyreUtils::to<int>(c[0]), GempyreUtils::to<int>(c[1]), GempyreUtils::to<int>(c[2])};
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

template <class C>
static bool containsAll(const C& container, const std::initializer_list<typename C::value_type>& lst) {
    for(const auto& i : lst) {
        if(std::find(container.begin(), container.end(), i) == container.end()) {
            return false;
        }
    }
    return true;
}

template <class C>
static std::vector<typename C::key_type> keys(const C& map) {
    std::vector<typename C::key_type> k;
    std::transform(map.begin(), map.end(), std::back_inserter(k), [](const auto & p) {return p.first;});
    return k;
}


static Ui::Filemap toFileMap(const std::string& filename) {
    const auto bytes = GempyreUtils::slurp<Base64::Byte>(filename);
    const auto encoded = Base64::encode(bytes);
    const auto name = GempyreUtils::baseName(filename);
    return {{'/' + name, encoded}};
}

std::string Ui::toStr(const std::atomic<Gempyre::Ui::State>& s) {
    const std::unordered_map<Gempyre::Ui::State, std::string> m{
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
            ""
#endif
            , "", port, root) {}

Ui::Ui(const Filemap& filemap, const std::string& indexHtml, unsigned short port, const std::string& root)
    : Ui(filemap, indexHtml, GempyreUtils::osBrowser(), "", port, root) {}

Ui::Ui(const std::string& indexHtml, const std::string& browser, const std::string& extraParams, unsigned short port, const std::string& root) :
    Ui(toFileMap(indexHtml), '/' + GempyreUtils::baseName(indexHtml), browser, extraParams, port, root) {}

Ui::Ui(const Filemap& filemap, const std::string& indexHtml, const std::string& browser, const std::string& extraParams, unsigned short port, const std::string& root) :
    m_eventqueue(std::make_unique<EventQueue<std::tuple<std::string, std::string, std::unordered_map<std::string, std::any>>>>()),
    m_responsemap(std::make_unique<EventMap<std::string, std::any>>()),
    m_sema(std::make_unique<Semaphore>()),
    m_timers(std::make_unique<TimerMgr>()),
    m_onUiExit([this]() {exit();}),
m_filemap(normalizeNames(filemap)) {
    GempyreUtils::init();

    m_startup = [this, port, indexHtml, browser, extraParams, root]() {
        auto openHandler = [this](int) { //open
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Opening", toStr(m_status));
            if(m_status == State::CLOSE || m_status == State::PENDING) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Request reload, Status change --> Reload");
                m_status = State::RELOAD;
            }
            //   setLogging(Utils::logLevel() == Utils::LogLevel::Debug);
            if(m_sema) {
                m_sema->signal();    // there may be some pending requests
            }
        };

        auto messageHandler = [this](const std::unordered_map<std::string, std::any>& params) { //message
            const auto kit = params.find("type");
            if(kit != params.end())  {
                const auto type = std::any_cast<std::string>(kit->second);
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "message", type);
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
                    gempyre_utils_assert_x(containsAll(keys(params), {"extension_id", "extension_call"}), "extension_response invalid parameters");
                    const auto id = std::any_cast<std::string>(params.at("extension_id"));
                    const auto key = std::any_cast<std::string>(params.at("extension_call"));
                    auto k = params.at(key);
                    m_responsemap->push(id, std::move(k));
                } else if(type == "error") {
                    GempyreUtils::log(GempyreUtils::LogLevel::Error, "JS says at:", std::any_cast<std::string>(params.at("element")),
                                      "error:", std::any_cast<std::string>(params.at("error")));
                    if(m_onError) {
                        m_onError(std::any_cast<std::string>(params.at("element")), std::any_cast<std::string>(params.at("error")));
                    }
                } else if(type == "exit_request") {
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "client kindly asks exit --> Status change Exit");
                    m_status = State::EXIT;
                }
                m_sema->signal();
            }
        };

        auto closeHandler = [this](Server::Close closeStatus, int code) { //close
            if(!m_server) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Close, Status change --> Exit");
                m_status = State::EXIT;
                m_sema->signal();
                return;
            }
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Gempyre close",  toStr(m_status),
                              static_cast<int>(closeStatus), (m_server ? m_server->isConnected() : false), code);

            if(m_status != State::EXIT && (closeStatus != Server::Close::EXIT  && (closeStatus == Server::Close::CLOSE && m_server && !m_server->isConnected()))) {
                pendingClose();
            } else if(closeStatus == Server::Close::FAIL) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Fail, Status change --> Retry");
                m_status = State::RETRY;
            }

            if(m_status == State::EXIT || m_status == State::RETRY) {
                m_sema->signal();
            }
        };

        auto getHandler = [this](const std::string_view & name)->std::optional<std::string> { //get
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "HTTP get", name);
            if(name == "/gempyre.js") {
                const auto encoded = Base64::decode(Gempyrejs);
                const auto page = GempyreUtils::join(encoded.begin(), encoded.end());
                return std::make_optional(page);
            }
            const auto it = m_filemap.find(std::string(name));
            if(it != m_filemap.end()) {
                if(it->second.size() == 0) {
                    GempyreUtils::log(GempyreUtils::LogLevel::Warning, "Empty data:", it->first);
                }
                const auto encoded = Base64::decode(it->second);
                if(encoded.size() == 0) {
                    GempyreUtils::log(GempyreUtils::LogLevel::Error, "Invalid Base64:", it->first);
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "This is bad:", it->second);
                }
                const auto page = GempyreUtils::join(encoded.begin(), encoded.end());
                GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "HTTP get:", page.size(), it->second.size());
                return std::make_optional(page);
            }
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "HTTP get - not found from:", GempyreUtils::join(GempyreUtils::keys(m_filemap), ","));
            return std::nullopt;
        };

        auto listener = [this, indexHtml, browser, extraParams](auto port)->bool { //listening
            if(m_status == State::EXIT)
                return false; //we are on exit, no more listening please
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Listening, Status change --> Running");
            m_status = State::RUNNING;
            const auto appPage = GempyreUtils::split<std::vector<std::string>>(indexHtml, '/').back();
#ifndef ANDROID_OS
            gempyre_utils_assert_x(!browser.empty() || !GempyreUtils::osBrowser().empty(), "I have no idea what browser should be spawned, please use other constructor");
#endif
            const auto cmdLine = (browser.empty() ? GempyreUtils::osBrowser() : browser)
            + " " + SERVER_ADDRESS + ":"
            + std::to_string(port) + "/"
            + (appPage.empty() ? "index.html" : appPage)
            + " " + extraParams;
            const auto result =
#if defined(WINDOWS_OS)
            std::system(cmdLine.c_str());
#elif defined (ANDROID_OS)
            androidLoadUi(cmdLine);

#else
            std::system((cmdLine + "&").c_str());
#endif
            if(result != 0) {
                GempyreUtils::log(GempyreUtils::LogLevel::Fatal, "Cannot open:", cmdLine);
            } else {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Opening:", cmdLine);
            }
            return true;
        };

        m_server = std::make_unique<Server>(
                       port,
                       root.empty() ? GempyreUtils::workingDir() : root,
                       openHandler,
                       messageHandler,
                       closeHandler,
                       getHandler,
                       listener
                   );
    };
}

Ui::~Ui() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Ui Destructor");
    exit();
}

void Ui::pendingClose() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Pending close, Status change --> Pending");
    m_status = State::PENDING;
    m_timers->flush(false); //all timers are run here
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Start 1s wait for pending");
    startTimer(1000ms, true, [this]() { //delay as a get may come due page chage
        if(m_status == State::PENDING) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Pending close, Status change --> Exit");
            m_status = State::CLOSE;
            m_sema->signal();
        } else {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Pending cancelled", toStr(m_status));
        }
    });
}

void Ui::close() {
    addRequest([this]() {
        return m_server->send({{"type", "close_request"}});
    });
}

void Ui::exit() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - start", toStr(m_status));
    switch(m_status) {
    case State::RUNNING: {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - request", toStr(m_status));
        if(!(m_server && m_server->isRunning())) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - no run", toStr(m_status));
            m_status = State::EXIT;
            return;
        }
        if(!m_server->isConnected()) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - no connect", toStr(m_status));
            m_server->close(true);
            m_status = State::EXIT;
            return;
        }

        addRequest([this]() {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - send", toStr(m_status));
            if(!m_server->send({{"type", "exit_request"}})) {
                //on fail we force
                m_server->close(true); //at this point we can close server (it may already be close)
                return false;
            }
            return true;
        });
        //Utils::log(Utils::LogLevel::Debug, "Status change -> CLOSE");
        //m_status = State::CLOSE;
        m_timers->flush(true);
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - wait in eventloop", toStr(m_status));
        eventLoop();
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - wait in eventloop done, back in mainloop", toStr(m_status));
    }
        break;
    case State::CLOSE:
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Status change -> EXIT");
        m_status = State::EXIT;  //there will be no one      
        break;
    default:
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "on exit switch", toStr(m_status));
    }
    m_sema->signal();
}

#ifndef ENSURE_SEND
#define ENSURE_SEND 65536
#endif


//DIRECT_DATA is MAYBE ok
//#define DIRECT_DATA

void Ui::send(const DataPtr& data) {
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
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "send data", type);
    }
}

void Ui::send(const Element& el, const std::string& type, const std::vector<std::pair<std::string, std::string>>& values) {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "send values", GempyreUtils::joinPairs(values.begin(), values.end()));
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
    addRequest([this]() {
        return m_server->beginBatch();
    });
}

void Ui::endBatch() {
    addRequest([this]() {
        return m_server->endBatch();
    });
}

void Ui::send(const Element& el, const std::string& type, const std::any& values) {
    std::unordered_map<std::string, std::string> params {{"element", el.m_id}, {"type", type}};
    if(const auto s = std::any_cast<std::string>(&values)) {
        params.emplace(type, *s);
        addRequest([this, params]() {
            return m_server->send(params);
        });
    } else if(const auto* c = std::any_cast<const char*>(&values)) {
        params.emplace(type, std::string(*c));
        addRequest([this, params]() {
            return m_server->send(params);
        });
    } else {
        addRequest([this, params, values]() {
            return m_server->send(params, values);
        });
    }
}


Ui::TimerId Ui::startTimer(const std::chrono::milliseconds& ms, bool singleShot, const std::function<void ()>& timerFunc) {
    return startTimer(ms, singleShot, [timerFunc](TimerId) {
        return timerFunc();
    });
}

Ui::TimerId Ui::startTimer(const std::chrono::milliseconds& ms, bool singleShot, const std::function<void (int)>& timerFunc) {
    const int id = m_timers->append(ms, singleShot, timerFunc, [this](const std::function<void()>& f) {
        m_timerqueue.emplace_back(f);
        m_sema->signal();
    });
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Start Timer", ms.count(), id);
    return id;
}

bool Ui::stopTimer(TimerId id) {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Stop Timer", id);
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

Ui& Ui::onOpen(std::function<void ()> onOpenFunction) {
    m_onOpen = std::move(onOpenFunction);
    return *this;
}

Ui& Ui::onError(std::function<void (const std::string&, const std::string&)> onErrorFunction) {
    m_onError = std::move(onErrorFunction);
    return *this;
}

void Ui::run() {
    gempyre_utils_assert_x(!m_server, "You shall not run more than once");
    m_startup();
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "run, Status change --> RUNNING");
    m_status = State::RUNNING;
    eventLoop();
    //assert(m_status == State::EXIT);
}

void Ui::eventLoop() {
    while(m_server && m_server->isRunning()) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "Eventloop is waiting");
        m_sema->wait();
        if(m_status == State::EXIT) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Eventloop is exiting");
            break;
        }

        if(m_status == State::RETRY) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Eventloop will retry");
            if(!m_server->retryStart()) {
                m_status = State::EXIT;
                break;
            }
            continue;
        }

        if(m_status == State::CLOSE) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Eventloop is Close", m_server && m_server->isRunning());
            if(m_onUiExit) {
                m_onUiExit();
            }
            if(!m_server->isConnected()) {
                m_server->close(true);
            }
            continue;
        }

        if(m_status == State::RELOAD) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Eventloop is Reload");
            if(m_onReload)
                addRequest([this]() {
                m_onReload();
                return true;
            });
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Reload, Status change --> Running");
            m_status = State::RUNNING;
        }

        if(!m_requestqueue.empty() && m_status == State::EXIT) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "skip timerqueue", toStr(m_status));
        }


        //shoot pending requests
        while(!m_timerqueue.empty() && m_status != State::EXIT && !m_onOpen && !m_hold) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Do timer request", m_timerqueue.size());
            const auto timerfunction = std::move(m_timerqueue.front());
            m_timerqueue.pop_front();
            if(!timerfunction) {
                //TODO: Maybe timer.reduce takes function off puts back ,and that cause sometimes an error - should be fixed
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer queue miss",
                                  toStr(m_status), !m_timerqueue.empty() && m_status != State::EXIT);
                continue;
            }
            timerfunction();
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Dod timer request", m_timerqueue.size(),
                              toStr(m_status), !m_timerqueue.empty() && m_status != State::EXIT);
        }

        if(m_status == State::PENDING) {
            continue;
        }

        if(m_onOpen && m_status == State::RUNNING && m_server->isConnected()) {
            const auto fptr = m_onOpen;
            holdTimers(true);
            addRequest([fptr, this]() {
                fptr();
                holdTimers(false);
                return true;
            }); //we try to keep logic call order
            m_onOpen = nullptr; //as the function may reset the function, we do let that happen
        }

        if(!m_requestqueue.empty() && m_status != State::RUNNING) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "skip requestqueue", toStr(m_status));
        }

        //shoot pending requests
        while(!m_requestqueue.empty() && m_status == State::RUNNING && m_server->isConnected()) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "do request");
            m_mutex.lock();
            const std::function<bool ()> topRequest = m_requestqueue.front();
            gempyre_utils_assert_x(topRequest, "Request is null");
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

        if(!m_eventqueue->empty() && m_status != State::RUNNING) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "skip eventqueue", toStr(m_status));
        }

        //events must be last as they may generate more requests or responses
        while(!m_eventqueue->empty() && m_status == State::RUNNING) {
            const auto it = m_eventqueue->take();
            const auto element = m_elements.find(std::get<0>(it));
            if(element != m_elements.end()) {
                const auto handlerName = std::get<1>(it);
                const auto handlers = std::get<1>(*element);
                const auto h = handlers.find(handlerName);
                if(h != handlers.end()) {
                    h->second(Event{Element(*this, std::move(element->first)), std::move(std::get<2>(it))});
                } else {
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Cannot find a handler", handlerName, "for element", std::get<0>(it));
                }
            } else {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Cannot find", std::get<0>(it), "from elements");
            }
        }
    }
}

void Ui::setLogging(bool logging) {
    send(root(), "logging", logging ? "true" : "false");
}

void Ui::eval(const std::string& eval) {
    send(root(), "eval", eval);
}

void Ui::debug(const std::string& msg) {
    send(root(), "debug", msg);
}

void Ui::alert(const std::string& msg) {
    send(root(), "alert", msg);
}



void Ui::open(const std::string& url, const std::string& name) {
    send(root(), "open", std::unordered_map<std::string, std::string> {{"url", url}, {"view", name}});
}

std::optional<std::pair<std::chrono::microseconds, std::chrono::microseconds>> Ui::ping() const {
    const auto ms = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    const clock_t begin_time = ::clock();
    const auto pong = const_cast<Ui*>(this)->query<std::string>(std::string(), "ping");
    if(pong.has_value() && !pong->empty()) {
        auto full = double(::clock() - begin_time) / (CLOCKS_PER_SEC / 1000000.0);
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
    gempyre_utils_assert_x(m_server, "Not connected");
    return std::string(SERVER_ADDRESS) + ":" + std::to_string(m_server->port()) +
           "?file=" + GempyreUtils::hexify(GempyreUtils::absPath(filepath), R"([^a-zA-Z0-9-,.,_~])");
}

std::optional<Element::Elements> Ui::byClass(const std::string& className) const {
    Element::Elements childArray;
    const auto childIds = const_cast<Ui*>(this)->query<std::vector<std::string>>(className, "classes");
    if(!childIds.has_value()) {
        return std::nullopt;
    }
    for(const auto& cid : *childIds) {
        childArray.push_back(Element(*const_cast<Ui*>(this), cid));
    }
    return m_status == Ui::State::RUNNING ? std::make_optional(childArray) : std::nullopt;
}

std::optional<Element::Elements> Ui::byName(const std::string& className) const {
    Element::Elements childArray;
    const auto childIds = const_cast<Ui*>(this)->query<std::vector<std::string>>(className, "names");
    if(!childIds.has_value()) {
        return std::nullopt;
    }
    for(const auto& cid : *childIds) {
        childArray.push_back(Element(*const_cast<Ui*>(this), cid));
    }
    return m_status == Ui::State::RUNNING ? std::make_optional(childArray) : std::nullopt;
}

std::optional<std::any> Ui::extension(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters)  {
    if(m_status != State::RUNNING) {
        return std::nullopt;
    }
    const auto queryId = std::to_string(m_server->queryId());

    const auto json = GempyreUtils::toJsonString(parameters);

    gempyre_utils_assert_x(json.has_value(), "Invalid parameter");

    addRequest([this, queryId, callId, json]() {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "extension:", json.value());
        return m_server->send({{"type", "extension"}, {"extension_id", queryId}, {"extension_call", callId}, {"extension_parameters", json.value()}});
    });

    for(;;) {   //start waiting the response
        eventLoop();
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "extension - wait in eventloop done, back in mainloop", toStr(m_status));
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
    if(it == m_filemap.end()) {
        return std::nullopt;
    }
    const auto data = Base64::decode(it->second);
    return std::make_optional(data);
}

bool Ui::addFile(const std::string& url, const std::string& file) {
    if(!GempyreUtils::fileExists(file)) {
        return false;
    }
    const auto it = m_filemap.find(url);
    if(it != m_filemap.end()) {
        return false;
    }
    const auto data = GempyreUtils::slurp<Base64::Byte>(file);
    const auto string = Base64::encode(data);
    m_filemap.insert_or_assign(url, std::move(string));
    return true;
}

std::optional<double> Ui::devicePixelRatio() const {
    const auto value = const_cast<Ui*>(this)->query<std::string>("", "devicePixelRatio");
    return value.has_value() && m_status == Ui::State::RUNNING ? GempyreUtils::toOr<double>(value.value()) : std::nullopt;
}


