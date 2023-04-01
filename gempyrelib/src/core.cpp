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
#include "gempyre.js.h"

#include "idlist.h"
#include "timer.h"

#include "core.h"
#include "data.h"

#include "gempyre_internal.h"

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
    GempyreUtils::set_log_level(is_debug ? GempyreUtils::LogLevel::Debug : GempyreUtils::LogLevel::Error);
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
                                       
    const auto js = GempyreUtils::json_to_any(js_string);
    
    gempyre_utils_assert_x(js, "Broken json " + js_string);
       
    const auto map = std::any_cast<std::unordered_map<std::string, std::any>>(&js.value());
    if(map && map->find(key) != map->end()) {
        const auto any_value = map->at(key);
        const auto conf_value = std::any_cast<T>(&any_value);
        if(conf_value) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "getConf", key, *conf_value);
            return std::make_optional<T>(*conf_value);
        }
    }
    
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "getConf", key, "No found");
    return std::nullopt;
}

static std::string osName() {
    switch (GempyreUtils::current_os()) {
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
    const auto out = GempyreUtils::read_process("python", {"--version"});
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
        [[maybe_unused]] const auto extra = value(param_map, BROWSER_PARAMS_KEY, "");
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
    const auto appui = value(param_map, BROWSER_KEY, GempyreUtils::html_file_launch_cmd());
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
        const auto name = GempyreUtils::base_name(filename);
        map.emplace('/' + name, encoded);
    }
    return map;
}

std::tuple<int, int, int> Gempyre::version() {
    static_assert(TOSTRING(GEMPYRE_PROJECT_VERSION)[0], "GEMPYRE_PROJECT_VERSION not set");
    const auto c = GempyreUtils::split<std::vector<std::string>>(TOSTRING(GEMPYRE_PROJECT_VERSION), '.');
    return {GempyreUtils::convert<int>(c[0]), GempyreUtils::convert<int>(c[1]), GempyreUtils::convert<int>(c[2])};
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
    {!title.empty() ? TITLE_KEY : "", GempyreUtils::qq(title)},
    {flags != 0 ? FLAGS_KEY : "", std::to_string(flags)},
    {width > 0 ? WIDTH_KEY : "", std::to_string(width)},
    {height > 0 ? HEIGHT_KEY : "", std::to_string(height)},
    {GempyreUtils::log_level() >= GempyreUtils::LogLevel::Debug ? BROWSER_PARAMS_KEY : "",
    "" }}){}


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
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Opening", m_ui->state_str());
    if(*m_ui == State::CLOSE || *m_ui == State::PENDING) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Request reload, Status change --> Reload");
        m_ui->set(State::RELOAD);
    }

    m_ui->signal_pending();    // there may be some pending requests
    
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
            m_ui->push_event({element, event, properties});
        } else if(type == "query") {
            const auto key = std::any_cast<std::string>(params.at("query_value"));
            auto id = std::any_cast<std::string>(params.at("query_id"));
            auto k = params.at(key);
            m_ui->push_response(std::move(id), std::move(k));
        } else if(type == "extension_response") {
            gempyre_utils_assert_x(containsAll(keys(params), {"extension_id", "extension_call"}), "extension_response invalid parameters");
            auto id = std::any_cast<std::string>(params.at("extension_id"));
            const auto key = std::any_cast<std::string>(params.at("extension_call"));
            auto k = params.at(key);
            m_ui->push_response(std::move(id), std::move(k));
        } else if(type == "error") {
            GempyreUtils::log(GempyreUtils::LogLevel::Error, "JS says at:", std::any_cast<std::string>(params.at("element")),
                              "error:", std::any_cast<std::string>(params.at("error")));
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "JS trace:", std::any_cast<std::string>(params.at("trace")));
            m_ui->call_error(std::any_cast<std::string>(params.at("element")), std::any_cast<std::string>(params.at("error")));
        } else if(type == "exit_request") {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "client kindly asks exit --> Status change Exit");
            m_ui->set(State::EXIT);
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
        m_ui->signal_pending();
    }
}

void Ui::closeHandler(Gempyre::CloseStatus closeStatus, int code) { //close
    if(!m_ui->has_server()) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Close, Status change --> Exit");
        m_ui->set(State::EXIT);
        m_ui->signal_pending();
        return;
    }

    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Gempyre close",  m_ui->state_str(),
                      static_cast<int>(closeStatus), m_ui->is_connected(), code);

    if(*m_ui != State::EXIT && (closeStatus != CloseStatus::EXIT  && (closeStatus == CloseStatus::CLOSE /*&& m_ui->is_connected()*/))) {
        pendingClose();
    } else if(closeStatus == CloseStatus::FAIL) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Fail, Status change --> Retry");
        m_ui->set(State::RETRY);
    }

    if(*m_ui == State::EXIT || *m_ui == State::RETRY) {
       m_ui->signal_pending();
    }
}



bool Ui::startListen(const std::string& indexHtml, const std::unordered_map<std::string, std::string>& parameters , int listen_port) { //listening
    if(*m_ui == State::EXIT)
        return false; //we are on exit, no more listening please
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Listening, Status change --> Running");
    m_ui->set(State::RUNNING);

    const auto& [appui, cmd_params] = guiCmdLine(indexHtml, listen_port, parameters);

    if (GempyreUtils::log_level() >= GempyreUtils::LogLevel::Debug) {
        const auto lines = GempyreUtils::split(cmd_params, '\n');
        if (GempyreUtils::log_level() != GempyreUtils::LogLevel::Debug_Trace || lines.size() > 2) {
            // show only 1st and last
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "gui params:", appui,
             lines[0], "...", lines[lines.size() - 1]);
        } else {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "gui params:", appui, cmd_params);
        }
    }


#if defined (ANDROID_OS)
    const auto result = androidLoadUi(appui + " " + cmd_params);
#else

    const auto on_path = GempyreUtils::which(appui);
    const auto is_exec = GempyreUtils::is_executable(appui) || (on_path && GempyreUtils::is_executable(*on_path));
    const auto result = is_exec ?
            GempyreUtils::execute(appui, cmd_params) :
            GempyreUtils::execute("", appui + " " +  cmd_params);

#endif

    if(result != 0) {
        GempyreUtils::log(GempyreUtils::LogLevel::Fatal, "gui cmd Error:", result, GempyreUtils::last_error());
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
    const auto file = m_ui->file(name);
    if(file) {
        if(file->size() == 0) {
            GempyreUtils::log(GempyreUtils::LogLevel::Warning, "Empty data:", name);
        }
        const auto encoded = Base64::decode(*file);
        if(encoded.size() == 0) {
            GempyreUtils::log(GempyreUtils::LogLevel::Error, "Invalid Base64:", name);
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "This is bad:", *file);
        }
        const auto page = GempyreUtils::join(encoded.begin(), encoded.end());
        GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "HTTP get:", page.size(), file->size());
        return std::make_optional(page);
    }
    return std::nullopt;
}

Ui::Ui(const Filemap& filemap,
       const std::string& indexHtml,
       unsigned short port,
       const std::string& root,
       const std::unordered_map<std::string, std::string>& parameters) :
    m_ui{std::make_unique<GempyreInternal>(
        this,
        filemap,
        indexHtml,
        port,
        root,
        parameters)} {
    GempyreUtils::init();
    // automatically try to set app icon if favicon is available
    const auto icon = resource("/favicon.ico");
    if(icon)
        set_application_icon(icon->data(), icon->size(), "ico");
}

// destoructor is slow due server thread close is slow (join) - to be fixed (?) when uwebsocket will be ditched
Ui::~Ui() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Ui Destructor", m_ui ? m_ui->state_str() : "N/A");
    if(m_ui) {
        if(*m_ui == State::SUSPEND) {
            m_ui->do_exit();
        }
        else if(*m_ui == State::RUNNING) {
            exit();
        }
        else if (*m_ui != State::NOTSTARTED && *m_ui != State::EXIT) {
            GempyreUtils::log(GempyreUtils::LogLevel::Error, "Strange state", m_ui->state_str());
        }
    }
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Ui Destroyed"); 
}

void Ui::pendingClose() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Pending close, Status change --> Pending");
    m_ui->set(State::PENDING);
    m_ui->flush_timers(false); //all timers are run here
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Start 1s wait for pending");
  //  after(1000ms, [this]() { //delay as a get may come due page chage
        if(*m_ui == State::PENDING) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Pending close, Status change --> Exit");
            m_ui->set(State::CLOSE);
            m_ui->signal_pending();
        } else {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Pending cancelled", m_ui->state_str());
        }
   // });
}

void Ui::close() {
    m_ui->add_request([this]() {
        return m_ui->send({{"type", "close_request"}});
    });
}

void Ui::exit() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - start", m_ui->state_str());
    if (*m_ui == State::RUNNING) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - request", m_ui->state_str());
        if(! m_ui->is_running()) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - no run", m_ui->state_str());
            m_ui->set(State::EXIT);
            return;
        }
        if(! m_ui->is_connected()) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - no connect", m_ui->state_str());
            m_ui->close_server();
            m_ui->set(State::EXIT);
            return;
        }
        m_ui->add_request([this]() {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - send", m_ui->state_str());
            if(! m_ui->send({{"type", "exit_request"}})) {
                //on fail we force
                GempyreUtils::log(GempyreUtils::LogLevel::Warning, "exit - send force", m_ui->state_str());
                m_ui->close_server(); //at this point we can close server (it may already be close)
                return false;
            }
            return true;
        });
        m_ui->flush_timers(true);
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - wait in eventloop", m_ui->state_str());
        eventLoop(false);
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - wait in eventloop done, back in mainloop", m_ui->state_str());
        }

    else if(*m_ui == State::CLOSE) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Status change -> EXIT");
        m_ui->set(State::EXIT);  //there will be no one      
    }
    else {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "on exit switch", m_ui->state_str());
    }
    m_ui->signal_pending();
}

#ifndef ENSURE_SEND
#define ENSURE_SEND 65536
#endif


//DIRECT_DATA is MAYBE ok
//#define DIRECT_DATA

void Ui::send(const DataPtr& data) {
#ifndef DIRECT_DATA
    const auto clonedBytes = data->clone();
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "send ui_bin", clonedBytes->size());
    m_ui->add_request([this, clonedBytes]() {
#else
    const auto [bytes, len] = data->payload();
#endif
        const auto ok = m_ui->send(*clonedBytes);
        // not sure if this is needed any more as there are other fixes that has potentially fixed this
        if(ok && clonedBytes->size() > ENSURE_SEND) {           //For some reason the DataPtr MAY not be send (propability high on my mac), but his cludge seems to fix it
            send(root(), "nil", "");     //correct fix may be adjust buffers and or send Data in several smaller packets .i.e. in case of canvas as
        }                                        //multiple tiles
#ifndef DIRECT_DATA
    return ok;
    });
#endif
}



void Ui::begin_batch() {
    m_ui->add_request([this]() {
        return m_ui->begin_batch();
    });
}

void Ui::end_batch() {
    m_ui->add_request([this]() {
        return m_ui->end_batch();
    });
}

void Ui::send(const Element& el, const std::string& type, const std::any& values, bool unique) {
    std::unordered_map<std::string, std::string> params {{"element", el.m_id}, {"type", type}};
    if(unique) {     // for some reason WS message get sometimes duplicated in JS and that causes issues here, msgid msgs are only handled once
        params.emplace("msgid", m_ui->next_msg_id());
    }
    if(const auto s = std::any_cast<std::string>(&values)) {
        params.emplace(type, *s);
        m_ui->add_request([this, params]() {
            return m_ui->send(params);
        });
    } else if(const auto* c = std::any_cast<const char*>(&values)) {
        params.emplace(type, std::string(*c));
        m_ui->add_request([this, params]() {
            return m_ui->send(params);
        });
    } else {
        m_ui->add_request([this, params, values]() {
            return m_ui->send(params, values);
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
        m_ui->add_timer(std::move(call));
        m_ui->signal_pending();
    };
    return caller;
}


Ui::TimerId Ui::start_periodic(const std::chrono::milliseconds &ms, const std::function<void (TimerId)> &timerFunc) {
    assert(timerFunc);
    auto caller = makeCaller(timerFunc);
    const int id = m_ui->append_timer(ms, false, std::move(caller));
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
    const int id = m_ui->append_timer(ms, true, std::move(caller));
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
    return m_ui->remove_timer(id);
}

Gempyre::Ui::ExitFunction Ui::on_exit(const ExitFunction& onUiExitFunction) {
    return m_ui->set_on_exit(onUiExitFunction);
}

Gempyre::Ui::ReloadFunction Ui::on_reload(const ReloadFunction& onReloadFunction) {
    return m_ui->set_on_reload(onReloadFunction);
}

Gempyre::Ui::OpenFunction Ui::on_open(const OpenFunction& onOpenFunction) {
    return m_ui->set_on_open(onOpenFunction);
}

Gempyre::Ui::ErrorFunction Ui::on_error(const ErrorFunction& onErrorFunction) {
    return m_ui->set_on_error(onErrorFunction);
}

void Ui::run() {
    m_ui->start_server();
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "run, Status change --> RUNNING");
    m_ui->set(State::RUNNING);
    eventLoop(true);
    if(*m_ui != State::SUSPEND)
        m_ui->do_exit();
}



void Ui::eventLoop(bool is_main) {
    GEM_DEBUG("enter", is_main, m_ui->is_running());
    const GempyreInternal::LoopWatch loop_watch (*m_ui, is_main);
    while(m_ui->is_running()) {
        m_ui->wait_events();

        if(*m_ui == State::EXIT) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Eventloop is exiting");
            break;
        }

         if(*m_ui == State::SUSPEND) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Eventloop is suspend");
            break;
        }


        if(*m_ui == State::RETRY) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Eventloop will retry");
            if(! m_ui->retry_start()) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "retry failed --> Status change Exit");
                m_ui->set(State::EXIT);
                break;
            }
            continue;
        }

        if(*m_ui == State::CLOSE) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Eventloop is Close", m_ui->is_running());
            if(!m_ui->is_connected()) {
                m_ui->close_server();
            }
            continue;
        }

        if(*m_ui == State::RELOAD) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Eventloop is Reload");
            if(m_ui->has_reload())
                m_ui->add_request([this]() {
                m_ui->on_reload();
                return true;
            });
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Reload, Status change --> Running");
            m_ui->set(State::RUNNING);
        }

        if(m_ui->has_requests() && *m_ui == State::EXIT) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "skip timerqueue", m_ui->state_str());
        }


        if(m_ui->is_connected())  
            m_ui->handle_requests();
        

        if(*m_ui == State::PENDING) {
            continue;
        }

        if(m_ui->has_open() && *m_ui == State::RUNNING && m_ui->is_connected()) {
            const auto fptr = m_ui->take_open();
            set_timer_on_hold(true);
            m_ui->add_request([fptr, this]() {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "call onOpen");
                fptr();
                set_timer_on_hold(false);
                return true;
            }); //we try to keep logic call order
        }

        if(m_ui->has_requests() && *m_ui != State::RUNNING) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "skip requestqueue", m_ui->state_str());
        }

        //shoot pending requests
        while(m_ui->has_requests() && *m_ui == State::RUNNING && m_ui->is_connected()) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "do request");
            auto topRequest = m_ui->take_request();
            if(!topRequest()) { //yes I wanna  mutex to be unlocked
                if(!m_ui->has_requests())
                    std::this_thread::sleep_for(10ms); // busyness
                m_ui->put_request(std::move(topRequest));
            }
        }

        if(m_ui->has_requests()) {
             GempyreUtils::log(GempyreUtils::LogLevel::Debug, "unfinished business", m_ui->state_str(), m_ui->is_connected());
        }

        //if there are responses they must be handled
        if(m_ui->has_responses()) {
            if(!is_main) // this was eventloop(true) in exit, changed to false for logic...still works...
                return; //handle query elsewhere, hopefully some one is pending
           // TODO: looks pretty busy (see calc) GempyreUtils::log(GempyreUtils::LogLevel::Warning, "There are unhandled responses on main");
        }

        if(!m_ui->has_events() && *m_ui != State::RUNNING) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "skip eventqueue", m_ui->state_str());
        }

        //events must be last as they may generate more requests or responses
        m_ui->consume_events(*this);
#if 0
        // Todo: Here all pending tasks are done, but this starts a loop again. I.e. make a busy loop. Therefore in NEXT version 
        // this thread should hold here either for a timer thread or a service thread wakeup. For timebeing just add miniscle sleep
        // that is really a poor solution, but avoids CPU 100%
        std::this_thread::sleep_for(10ms);
#endif

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
    gempyre_utils_assert_x(m_ui->is_connected(), "Not connected");
    return std::string(SERVER_ADDRESS) + ":" + std::to_string(m_ui->port()) +
           "?file=" + GempyreUtils::hexify(GempyreUtils::abs_path(filepath), R"([^a-zA-Z0-9-,.,_~])");
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
    return *m_ui == State::RUNNING ? std::make_optional(childArray) : std::nullopt;
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
    return *m_ui == State::RUNNING ? std::make_optional(childArray) : std::nullopt;
}

void Ui::extension_call(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters) {
    const auto json = GempyreUtils::to_json_string(parameters);
    gempyre_utils_assert_x(json.has_value(), "Invalid parameter");
    m_ui->add_request([this, callId, json]() {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "extension:", json.value());
        return m_ui->send({
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
    if(*m_ui != State::RUNNING) {
        return std::nullopt;
    }
    const auto queryId = m_ui->query_id();

    const auto json = GempyreUtils::to_json_string(parameters);

    gempyre_utils_assert_x(json.has_value(), "Invalid parameter");

    m_ui->add_request([this, queryId, callId, json]() {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "extension:", json.value());
        return m_ui->send({{"type", "extension"}, {"extension_id", queryId}, {"extension_call", callId}, {"extension_parameters", json.value()}});
    });

    while(m_ui->is_running()) {   //start waiting the response
        eventLoop(false);
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "extension - wait in eventloop done, back in mainloop", m_ui->state_str());
        if(*m_ui != State::RUNNING) {
            m_ui->signal_pending();
            break; //we are gone
        }

        const auto response = m_ui->take_response(queryId);
        if(response)
            return response;
    }
    return std::nullopt;
}
std::optional<std::vector<uint8_t>> Ui::resource(const std::string& url) const {
    const auto file = m_ui->file(url);
    if(!file) {
        return std::nullopt;
    }
    const auto data = Base64::decode(*file);
    return std::make_optional(data);
}

bool Ui::add_file(const std::string& url, const std::string& file_name) {
    if(!GempyreUtils::file_exists(file_name)) {
        return false;
    }
    const auto file = m_ui->file(url);
    if(file) { // expect not found
        return false;
    }
    m_ui->add_file(url, file_name);
    return true;
}

std::optional<double> Ui::device_pixel_ratio() const {
    const auto value = const_cast<Ui*>(this)->query<std::string>("", "devicePixelRatio");
    return value.has_value() && *m_ui == State::RUNNING ? GempyreUtils::parse<double>(value.value()) : std::nullopt;
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
    if(!GempyreUtils::file_exists(file)) {
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

void Ui::set_timer_on_hold(bool on_hold) {
    m_ui->set_hold(on_hold);
    }

bool Ui::is_timer_on_hold() const {
    return m_ui->hold();
    }


void Ui::suspend() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "suspend state is " + std::string(m_ui->state_str()));
    gempyre_utils_assert(m_ui->loop() == 1);

    if(*m_ui != State::RUNNING) { // maybe for ealier error
        GempyreUtils::log(GempyreUtils::LogLevel::Warning, "suspend state is " + std::string(m_ui->state_str()));
        return;
    }

    gempyre_utils_assert(!is_timer_on_hold());
    set_timer_on_hold(true);
    m_ui->set(State::SUSPEND);
    m_ui->clear_handlers();
    m_ui->flush_timers(false);
    m_ui->clear();
    m_ui->signal_pending();
    std::this_thread::sleep_for(200ms); // uws has strange corking problem
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "suspend exit is " + std::string(m_ui->state_str()));
 
}

void Ui::resume() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "resume state is " + std::string(m_ui->state_str()));
    gempyre_utils_assert(m_ui->loop() == 0);
    if(*m_ui == State::SUSPEND) {
        m_ui->set(State::RUNNING);
        set_timer_on_hold(false);
        eventLoop(true);
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "resumed exit " + std::string(m_ui->state_str()));  
    } else {
        gempyre_utils_assert(*m_ui != State::EXIT);  
        exit();
    }
}

const GempyreInternal& Ui::ref() const {
    return *m_ui;
}

GempyreInternal& Ui::ref() {
    return *m_ui;
}


 GempyreInternal::GempyreInternal(
        Ui* ui, 
        const Ui::Filemap& filemap,
        const std::string& indexHtml,
        unsigned short port,
        const std::string& root,
        const std::unordered_map<std::string, std::string>& parameters) : 
    m_filemap(normalizeNames(filemap)),
    m_startup{[this, ui, port, indexHtml, parameters, root]() {

    auto last_query_id  = 0;
    for(const auto& id_string : m_responsemap.keys()) {
        const auto id = GempyreUtils::convert<int>(id_string);
        if(id > last_query_id)
            last_query_id = id;
    }

    m_responsemap.clear();
    
    // This is executed in m_startup 
    m_server = create_server(
                   port,
                   root.empty() ? GempyreUtils::working_dir() : root,
                   [ui](){ui->openHandler();},
                   [ui](const Server::Object& obj){ui->messageHandler(obj);},
                   [ui](CloseStatus status, int code){ui->closeHandler(status, code);},
                   [ui](const std::string_view& name){return ui->getHandler(name);},
                   [indexHtml, parameters, ui](int listen_port){return ui->startListen(indexHtml, parameters, listen_port);},
                   last_query_id + 1 // if m_server is created second time it is good that this is > as 1st as pending queries may cause confusion
                );
    }} {}
