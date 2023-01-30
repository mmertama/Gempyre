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

#include "core.h"

using namespace std::chrono_literals;
using namespace Gempyre;

constexpr auto SERVER_ADDRESS{"http://localhost"};

constexpr auto BROWSER_KEY{"browser"};
constexpr auto BROWSER_PARAMS_KEY{"params"};
constexpr auto WIDTH_KEY{"width"};
constexpr auto HEIGHT_KEY{"height"};
constexpr auto TITLE_KEY{"title"};
constexpr auto FLAGS_KEY{"flags"};

#ifdef ANDROID_OS
extern int androidLoadUi(const std::string&);
#endif

#define CHECK_FATAL(x) if(ec) {error(ec, merge(x, " at ", __LINE__)); return;}  std::cout << x << " - ok" << std::endl;

void Gempyre::set_debug(bool is_debug) {
    GempyreUtils::setLogLevel(is_debug ? GempyreUtils::LogLevel::Debug : GempyreUtils::LogLevel::Error);
}


#ifndef ANDROID_OS
void Gempyre::setJNIENV(void*, void*) {
    GempyreUtils::log(GempyreUtils::LogLevel::Fatal, "setJNIENV should not be called within current OS");
}
#endif

#define STR(x) #x
#define TOSTRING(x) STR(x)

template <class T>
static std::optional<T> getConf(const std::string& key) {
    const auto find = []() {
        const std::vector<std::string> conf_names({"/gempyre.conf",  "/gempyre_default.conf"}); // How we look, at this order
        for(const auto& c_name : conf_names) {
             const auto conf = Gempyrejsh.find(c_name);
             if(conf != Gempyrejsh.end())
                return conf;
        }
        return Gempyrejsh.end();
    }; 
    const auto conf = find();
    if(conf == Gempyrejsh.end())
        return std::nullopt;    
    const auto js_data = Base64::decode(conf->second);
    gempyre_utils_assert_x(!js_data.empty(), "Broken resource " + conf->first);
  
    const auto js_string = std::string(reinterpret_cast<const char*>(js_data.data()),
                                       js_data.size());
                                       
    const auto js = GempyreUtils::jsonToAny(js_string);
    
    gempyre_utils_assert_x(js, "Broken json " + js_string);
       
    const auto map = std::any_cast<std::unordered_map<std::string, std::any>>(&js.value());
    if(map && map->find(key) != map->end()) {
        const auto any_value = map->at(key);
        const auto value = std::any_cast<T>(&any_value);
        if(value) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "getConf", key, *value);
            return std::make_optional<T>(*value);
        }
    }
    
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "getConf", key, "No found");
    return std::nullopt;
}

static std::string osName() {
    switch (GempyreUtils::currentOS()) {
    case GempyreUtils::OS::WinOs: return "windows";
    case GempyreUtils::OS::LinuxOs: return "linux";
    case GempyreUtils::OS::MacOs: return "macos";
    case GempyreUtils::OS::AndroidOs: return "android";
    case GempyreUtils::OS::RaspberryOs: return "raspberry";
    case GempyreUtils::OS::OtherOs: return "other";
    default: return "undefined";
    }
}

[[maybe_unused]] static inline std::string join(const std::unordered_map<std::string, std::string>& map, const std::string& key, const std::string& prefix) {
    const auto it = map.find(key);
    return it == map.end() ? std::string() : prefix + it->second;
}

[[maybe_unused]]
static std::optional<std::string> python3() {
    const auto  py3 = GempyreUtils::which("python3");
    if(py3)
        return *py3;
    const auto  py = GempyreUtils::which("python");
    if(!py)
        return std::nullopt;
    const auto out = GempyreUtils::readProcess("python", {"--version"});
    if(!out)
        return std::nullopt;
    const auto pv = GempyreUtils::split<std::vector<std::string>>(*out, ' '); //// Python 2.7.16
    if(pv.size() < 2)
        return std::nullopt;
    const auto ver = GempyreUtils::split<std::vector<std::string>>(pv[1], '.');
    if(pv.size() < 1)
        return std::nullopt;
    const auto major = GempyreUtils::convert<int>(ver[0]);
    if(major < 3)
        return std::nullopt;
    return py;
}

// read command line form conf

static std::optional<std::tuple<std::string, std::string>> confCmdLine(const std::unordered_map<std::string, std::string>& replacement) {
    auto cmdName = getConf<std::string>(osName() + "-" + "cmd_name");
    if(!cmdName)
        cmdName = getConf<std::string>("cmd_name");
    if(cmdName) {
        auto cmd_params = getConf<std::string>(osName() + "-" + "cmd_params");
        if(!cmd_params)
            cmd_params = getConf<std::string>("cmd_params");
        if(cmd_params) {
            auto params = *cmd_params;
            for(const auto& [key, value] : replacement)
                params = GempyreUtils::substitute(params, R"(\$\{\s*)" + key + R"(\s*\})", value);
            return std::tuple<std::string, std::string>(*cmdName, params); // make_tuple uses refs, hence copy
          }
    }
    return std::nullopt;
}

static inline std::string value(const std::unordered_map<std::string, std::string>& map, const std::string& key, const std::string& default_value) {
    const auto it = map.find(key);
    return it == map.end() ? default_value : it->second;
}


// figure out and construct gui app and command line
static std::tuple<std::string, std::string> guiCmdLine(const std::string& indexHtml,
                                                    int port,
                                                    const std::unordered_map<std::string, std::string>& param_map) {
    const auto appPage = GempyreUtils::split<std::vector<std::string>>(indexHtml, '/').back();
    const auto url =  std::string(SERVER_ADDRESS) + ":"
    + std::to_string(port) + "/"
    + (appPage.empty() ? "index.html" : appPage);
    if(param_map.find(BROWSER_KEY) == param_map.end()) {
        const auto width = value(param_map, WIDTH_KEY, "320");
        const auto height = value(param_map, HEIGHT_KEY, "240");
        const auto title = value(param_map, TITLE_KEY, "Gempyre");
        const auto extra = value(param_map, BROWSER_PARAMS_KEY, "");
        const auto flags = value(param_map, FLAGS_KEY, "");
        const auto conf = confCmdLine({{"URL", url}, {"WIDTH", width}, {"HEIGHT", height}, {"TITLE", title}, {"FLAGS", flags}});
        if(conf)
            return conf.value();
#ifdef USE_PYTHON_UI
        const auto py3 = python3();
        if(py3) {
            constexpr auto py_file = "/pyclient.py"; // let's not use definion in gempyrejsh as that may not be there
            const auto py_data = Gempyrejsh.find(py_file);
            if(py_data != Gempyrejsh.end()) {
                const auto py_code = Base64::decode(py_data->second);
                const std::string py = GempyreUtils::join(py_code);
                const auto call_param = std::string("-c \"") + py + "\" ";
                return {*py3, call_param
                            + GempyreUtils::join<std::vector<std::string>>({
                                     "--gempyre-url=" + url,
                                     join(param_map, WIDTH_KEY, "--gempyre-width="),
                                     join(param_map, HEIGHT_KEY,"--gempyre-height="),
                                     join(param_map, TITLE_KEY,"--gempyre-title="),
                                     join(param_map, FLAGS_KEY,"--gempyre-flags="),
                                     join(param_map, BROWSER_PARAMS_KEY,"--gempyre-extra=")}, " ") };
            }
        }
#endif
    }

    const auto params = url + " " + value(param_map, BROWSER_PARAMS_KEY, "");
    const auto appui = value(param_map, BROWSER_KEY, GempyreUtils::htmlFileLaunchCmd());
#ifndef ANDROID_OS
    gempyre_utils_assert_x(!appui.empty(), "I have no idea what browser should be spawned, please use other constructor");
#endif
    return {appui, params};
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

Ui::Filemap Ui::to_file_map(const std::vector<std::string>& filenames) {
    Ui::Filemap map;
    for(const auto& filename : filenames) {
        const auto bytes = GempyreUtils::slurp<Base64::Byte>(filename);
        const auto encoded = Base64::encode(bytes);
        const auto name = GempyreUtils::baseName(filename);
        map.emplace('/' + name, encoded);
    }
    return map;
}

std::tuple<int, int, int> Gempyre::version() {
    static_assert(TOSTRING(GEMPYRE_PROJECT_VERSION)[0], "GEMPYRE_PROJECT_VERSION not set");
    const auto c = GempyreUtils::split<std::vector<std::string>>(TOSTRING(GEMPYRE_PROJECT_VERSION), '.');
    return {GempyreUtils::convert<int>(c[0]), GempyreUtils::convert<int>(c[1]), GempyreUtils::convert<int>(c[2])};
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


/// Create UI using default ui app or gempyre.conf
Ui::Ui(const Filemap& filemap,
       const std::string& indexHtml,
       const std::string& title,
       int width,
       int height,
       unsigned flags,
       unsigned short port,
       const std::string& root) : Ui(filemap, indexHtml, port, root,
            {
    // add only if valid
    {!title.empty() ? TITLE_KEY : "", title},
    {flags != 0 ? FLAGS_KEY : "", std::to_string(flags)},
    {width > 0 ? WIDTH_KEY : "", std::to_string(width)},
    {height > 0 ? HEIGHT_KEY : "", std::to_string(height)},
    {GempyreUtils::logLevel() >= GempyreUtils::LogLevel::Debug ? BROWSER_PARAMS_KEY : "", "debug=True" }}){}


Ui::Ui(const Filemap& filemap,
       const std::string& indexHtml,
       const std::string& browser,
       const std::string& browser_params,
       unsigned short port,
       const std::string& root) : Ui(filemap, indexHtml, port, root,
        {
    {!browser.empty() ? BROWSER_KEY : "", browser},
    {!browser_params.empty() ? BROWSER_PARAMS_KEY : "", browser_params}}){}

void Ui::openHandler() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Opening", toStr(m_status));
    if(m_status == State::CLOSE || m_status == State::PENDING) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Request reload, Status change --> Reload");
        m_status = State::RELOAD;
    }

    if(m_sema) {
        m_sema->signal();    // there may be some pending requests
    }
}

void Ui::messageHandler(const Server::Object& params) {
    const auto kit = params.find("type");
    if(kit != params.end())  {
        const auto type = std::any_cast<std::string>(kit->second);
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "message", type);
        if(type == "event") {
            const auto element = std::any_cast<std::string>(params.at("element"));
            const auto event = std::any_cast<std::string>(params.at("event"));
            const auto properties = std::any_cast<Server::Object>(params.at("properties"));
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
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "JS trace:", std::any_cast<std::string>(params.at("trace")));
            if(m_onError) {
                m_onError(std::any_cast<std::string>(params.at("element")), std::any_cast<std::string>(params.at("error")));
            }
        } else if(type == "exit_request") {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "client kindly asks exit --> Status change Exit");
            m_status = State::EXIT;
        } else if(type == "extensionready") {
             /* no more like this
              * const auto appPage = GempyreUtils::split<std::vector<std::string>>(indexHtml, '/').back();
             const auto address =
             + " " + std::string(SERVER_ADDRESS) + "/"
             + (appPage.empty() ? "index.html" : appPage);

             extensionCall("ui_info", {
                              {"url", address},
                              {"params", ""}});
            */
        }
        m_sema->signal();
    }
}

void Ui::closeHandler(Gempyre::CloseStatus closeStatus, int code) { //close
    if(!m_server) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Close, Status change --> Exit");
        m_status = State::EXIT;
        m_sema->signal();
        return;
    }
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Gempyre close",  toStr(m_status),
                      static_cast<int>(closeStatus), (m_server ? m_server->isConnected() : false), code);

    if(m_status != State::EXIT && (closeStatus != CloseStatus::EXIT  && (closeStatus == CloseStatus::CLOSE && m_server && !m_server->isConnected()))) {
        pendingClose();
    } else if(closeStatus == CloseStatus::FAIL) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Fail, Status change --> Retry");
        m_status = State::RETRY;
    }

    if(m_status == State::EXIT || m_status == State::RETRY) {
        m_sema->signal();
    }
}



bool Ui::startListen(const std::string& indexHtml, const std::unordered_map<std::string, std::string>& parameters , int listen_port) { //listening
    if(m_status == State::EXIT)
        return false; //we are on exit, no more listening please
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Listening, Status change --> Running");
    m_status = State::RUNNING;

    const auto& [appui, cmd_params] = guiCmdLine(indexHtml, listen_port, parameters);

     GempyreUtils::log(GempyreUtils::LogLevel::Debug, "gui cmd:", appui, cmd_params);


#if defined (ANDROID_OS)
    const auto result = androidLoadUi(appui + " " + cmd_params);
#else

    const auto on_path = GempyreUtils::which(appui);
    const auto is_exec = GempyreUtils::isExecutable(appui) || (on_path && GempyreUtils::isExecutable(*on_path));
    const auto result = is_exec ?
            GempyreUtils::execute(appui, cmd_params) :
            GempyreUtils::execute("", appui + " " +  cmd_params);

#endif

    if(result != 0) {
        //TODO: Change to Fatal
        GempyreUtils::log(GempyreUtils::LogLevel::Error, "gui cmd Error:", result, GempyreUtils::lastError());
    }
    return true;
}

std::optional<std::string> Ui::getHandler(const std::string_view & name) { //get
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
}

Ui::Ui(const Filemap& filemap,
       const std::string& indexHtml,
       unsigned short port,
       const std::string& root,
       const std::unordered_map<std::string, std::string>& parameters) :
    m_eventqueue(std::make_unique<EventQueue<InternalEvent>>()),
    m_responsemap(std::make_unique<EventMap<std::string, std::any>>()),
    m_sema(std::make_unique<Semaphore>()),
    m_timers(std::make_unique<TimerMgr>()),
    m_filemap(normalizeNames(filemap)),
    m_startup{[this, port, indexHtml, parameters, root]() {

    m_server = std::make_unique<Server>(
                   port,
                   root.empty() ? GempyreUtils::workingDir() : root,
                   [this](){openHandler();},
                   [this](const Server::Object& obj){messageHandler(obj);},
                   [this](CloseStatus status, int code){closeHandler(status, code);},
                   [this](const std::string_view& name){return getHandler(name);},
                   [indexHtml, parameters, this](int listen_port){return startListen(indexHtml, parameters, listen_port);}
                );
    }}{
    GempyreUtils::init();
    // automatically try to set app icon if favicon is available
    const auto icon = resource("/favicon.ico");
    if(icon)
        set_application_icon(icon->data(), icon->size(), "ico");
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
    after(1000ms, [this]() { //delay as a get may come due page chage
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
                GempyreUtils::log(GempyreUtils::LogLevel::Warning, "exit - send force", toStr(m_status));
                m_server->close(true); //at this point we can close server (it may already be close)
                return false;
            }
            return true;
        });
        //Utils::log(Utils::LogLevel::Debug, "Status change -> CLOSE");
        //m_status = State::CLOSE;
        m_timers->flush(true);
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - wait in eventloop", toStr(m_status));
        eventLoop(true);
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - wait in eventloop done, back in mainloop", toStr(m_status));
    //    m_server.reset();
      //  GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server cleaned");
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



void Ui::begin_batch() {
    addRequest([this]() {
        return m_server->beginBatch();
    });
}

void Ui::end_batch() {
    addRequest([this]() {
        return m_server->endBatch();
    });
}

void Ui::send(const Element& el, const std::string& type, const std::any& values, bool unique) {
    std::unordered_map<std::string, std::string> params {{"element", el.m_id}, {"type", type}};
    if(unique) {     // for some reason WS message get sometimes duplicated in JS and that causes issues here, msgid msgs are only handled once
        params.emplace("msgid", std::to_string(m_msgId++));
    }
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

// timer elapses calls a function
// that calls a function that adds a another function to a queue
// that function calls a the actual timer functio when on top
std::function<void(int)> Ui::makeCaller(const std::function<void (TimerId id)>& function) {
    const auto caller =  [this, function](int id) {
        auto call = [function, id]() {
            function(id);
        };
        m_timerqueue.emplace_back(std::move(call));
        m_sema->signal();
    };
    return caller;
}


Ui::TimerId Ui::start_periodic(const std::chrono::milliseconds &ms, const std::function<void (TimerId)> &timerFunc) {
    assert(timerFunc);
    auto caller = makeCaller(timerFunc);
    const int id = m_timers->append(ms, false, std::move(caller));
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Start Periodic", ms.count(), id);
    return id;
}

Ui::TimerId Ui::start_periodic(const std::chrono::milliseconds &ms, const std::function<void ()> &timerFunc) {
    return start_periodic(ms, [timerFunc](TimerId) {
        return timerFunc();
    });
}

Ui::TimerId Ui::after(const std::chrono::milliseconds &ms, const std::function<void (TimerId)> &timerFunc) {
    auto caller = makeCaller(timerFunc);
    const int id = m_timers->append(ms, true, std::move(caller));
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Start After", ms.count(), id);
    return id;
}

Ui::TimerId Ui::after(const std::chrono::milliseconds &ms, const std::function<void ()> &timerFunc) {
    return after(ms, [timerFunc](TimerId) {
        return timerFunc();
    });
}


bool Ui::cancel_timer(TimerId id) {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Stop Timer", id);
    return m_timers->remove(id);
}

Ui& Ui::on_exit(std::function<void ()> onUiExitFunction) {
    m_onUiExit = std::move(onUiExitFunction);
    return *this;
}

Ui& Ui::on_reload(std::function<void ()> onReloadFunction) {
    m_onReload = std::move(onReloadFunction);
    return *this;
}

Ui& Ui::on_open(std::function<void ()> onOpenFunction) {
    m_onOpen = std::move(onOpenFunction);
    return *this;
}

Ui& Ui::on_error(std::function<void (const std::string&, const std::string&)> onErrorFunction) {
    m_onError = std::move(onErrorFunction);
    return *this;
}

void Ui::run() {
    gempyre_utils_assert_x(!m_server, "You shall not run more than once");
    m_startup();
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "run, Status change --> RUNNING");
    m_status = State::RUNNING;
    eventLoop(true);
    if(m_onUiExit) { // what is point? Should this be here
        m_onUiExit();
    }
    GEM_DEBUG("requests:", m_requestqueue.size(), "timers:", m_timerqueue.size());
    m_requestqueue.clear(); // we have exit, rest of requests get ignored
    GEM_DEBUG("run, exit event loop");
    m_server->close(true);
    assert(!m_server->isJoinable());
    m_server.reset(); // so the run can be recalled
    m_timers->flush(false);
    
    // clear or erase calls destructor and that seems to be issue in raspberry
    while(!m_timerqueue.empty())
        m_timerqueue.pop_front();
    
    assert(m_requestqueue.empty());
    assert(!m_timers->isValid());
}


void Ui::eventLoop(bool is_main) {
    GEM_DEBUG("enter", is_main, !!m_server, (m_server && m_server->isRunning()));
    while(m_server && m_server->isRunning()) {
        if(!m_sema->empty()) {
            const auto start = std::chrono::steady_clock::now();

            m_sema->wait();

            const auto end = std::chrono::steady_clock::now();
            const auto duration = end - start;
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "Eventloop is waited", duration.count());

        }

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
            hold_timers(true);
            addRequest([fptr, this]() {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "call onOpen");
                fptr();
                hold_timers(false);
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
            if(!is_main)
                return; //handle query elsewhere, hopefully some one is pending
           // TODO: looks pretty busy (see calc) GempyreUtils::log(GempyreUtils::LogLevel::Warning, "There are unhandled responses on main");
        }

        if(!m_eventqueue->empty() && m_status != State::RUNNING) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "skip eventqueue", toStr(m_status));
        }

        //events must be last as they may generate more requests or responses
        while(!m_eventqueue->empty() && m_status == State::RUNNING) {
            const auto it = m_eventqueue->take();
            const auto element = m_elements.find(it.element);
            if(element != m_elements.end()) {
                const auto handlerName = it.handler;
                const auto handlers = std::get<1>(*element);
                const auto h = handlers.find(handlerName);

                if(h != handlers.end()) {
                    h->second(Event{Element(*this, std::move(element->first)), std::move(it.data)});
                } else {
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Cannot find a handler", handlerName, "for element", it.element);
                }
            } else {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Cannot find", it.element, "from elements");
            }
        }

        // Todo: Here all pending tasks are done, but this starts a loop again. I.e. make a busy loop. Therefore in NEXT version 
        // this thread should hold here either for a timer thread or a service thread wakeup. For timebeing just add miniscle sleep
        // that is really a poor solution, but avoids CPU 100%
        std::this_thread::sleep_for(10ms);

    }
    GEM_DEBUG("Eventloop exit");
}

void Ui::set_logging(bool logging) {
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
    const auto milliseconds_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const clock_t begin_time = ::clock();
    const auto pong = const_cast<Ui*>(this)->query<std::string>(std::string(), "ping");
    if(pong.has_value() && !pong->empty()) {
        // full loop
        const auto full = double(::clock() - begin_time) / (CLOCKS_PER_SEC / 1000000.0);
        // timestamp from the response
        const auto pong_time = pong.value();
        const auto half = GempyreUtils::convert<decltype(milliseconds_since_epoch)>(pong_time) - milliseconds_since_epoch;
        return std::make_pair(
                   std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double, std::ratio<1, 1000000>>(full)),
                   std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double, std::ratio<1, 1000000>>(half)));
    } else {
        GEM_DEBUG("Bad ping pong");
    }
    return std::nullopt;
}

Element Ui::root() const {
    return Element(*const_cast<Ui*>(this), "");
}


std::string Ui::address_of(const std::string& filepath) const {
    gempyre_utils_assert_x(m_server, "Not connected");
    return std::string(SERVER_ADDRESS) + ":" + std::to_string(m_server->port()) +
           "?file=" + GempyreUtils::hexify(GempyreUtils::absPath(filepath), R"([^a-zA-Z0-9-,.,_~])");
}

std::optional<Element::Elements> Ui::by_class(const std::string& className) const {
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

std::optional<Element::Elements> Ui::by_name(const std::string& className) const {
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

void Ui::extension_call(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters) {
    const auto json = GempyreUtils::toJsonString(parameters);
    gempyre_utils_assert_x(json.has_value(), "Invalid parameter");
    addRequest([this, callId, json]() {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "extension:", json.value());
        return m_server->send({
                                  {"type", "extension"},
                                  {"extension_call", callId},
                                  {"extension_id", ""},
                                  {"extension_parameters", json.value()}});
    });
}

/*
 * TODO: remove me
std::optional<std::any> Ui::extension(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters) {
    return extensionGet(callId, parameters);
}
*/

std::optional<std::any> Ui::extension_get(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters)  {
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
        eventLoop(false);
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

bool Ui::add_file(const std::string& url, const std::string& file) {
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

std::optional<double> Ui::device_pixel_ratio() const {
    const auto value = const_cast<Ui*>(this)->query<std::string>("", "devicePixelRatio");
    return value.has_value() && m_status == Ui::State::RUNNING ? GempyreUtils::toOr<double>(value.value()) : std::nullopt;
}

void Ui::set_application_icon(const uint8_t *data, size_t dataLen, const std::string& type) {
    extension_call("setAppIcon", {{"image_data", Base64::encode(data, dataLen)}, {"type", type}});
}

void Ui::resize(int width, int height) {
    extension_call("resize", {{"width", width}, {"height", height}});
}

void Ui::set_title(const std::string& name) {
    extension_call("setTitle", {{"title", name}});
}

/*
std::string Ui::stdParams(int width, int height, const std::string& title) {
    std::stringstream ss;
    ss << " --gempyre-width=" << width << " --gempyre-height=" << height << " --gempyre-title=\"" << title << "\""; // circle with spaces
    return ss.str();
}
*/
std::optional<std::string> Ui::add_file(Gempyre::Ui::Filemap& map, const std::string& file) {
    if(!GempyreUtils::fileExists(file)) {
        return std::nullopt;
    }
    auto url = GempyreUtils::substitute(file, R"([\/\\])", "_");
    if(map.find(url) != map.end()) {
        return std::nullopt;
    }

    url.insert(url.begin(), '/');

    const auto data = GempyreUtils::slurp<Base64::Byte>(file);
    const auto string = Base64::encode(data);
    map.insert_or_assign(url, std::move(string));
    return url;
}

