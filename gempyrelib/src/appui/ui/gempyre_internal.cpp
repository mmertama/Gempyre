#include "gempyre_internal.h"
#include "gempyre.js.h"
#include "data.h"

using namespace Gempyre;


static
inline std::string value(const std::unordered_map<std::string, std::string>& map, const std::string& key, const std::string& default_value) {
    const auto it = map.find(key);
    return it == map.end() ? default_value : it->second;
}

static
std::string osName() {
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

template <class T>
static std::optional<T> getConf(const std::string& key) {
    const auto ffind = []() {
        const std::vector<std::string> conf_names({"/gempyre.conf",  "/gempyre_default.conf"}); // How we look, at this order
        for(const auto& c_name : conf_names) {
             const auto conf = find(Gempyrejsh, c_name);
             if(conf != Gempyrejsh.end())
                return conf;
        }
        return Gempyrejsh.end();
    }; 
    const auto conf = ffind();
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


// read command line form conf
static
std::optional<std::tuple<std::string, std::string>> confCmdLine(const std::unordered_map<std::string, std::string>& replacement) {
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

[[maybe_unused]]
static std::optional<std::string> python3(const std::initializer_list<std::string>& required_libs) {
    const auto  py3 = python3();
    if(py3) {
        for(const auto& r : required_libs) {
            const auto result = GempyreUtils::read_process(*py3, {"-c", GempyreUtils::qq("import " + r)});
            if(!result)
                return std::nullopt;
            if(result->find("ModuleNotFoundError") != std::string::npos) {
                GempyreUtils::log(GempyreUtils::LogLevel::Warning,"Requested python libraries not found", *result);
                return std::nullopt;
            }    
        }
    }
    return py3;
} 


// figure out and construct gui app and command line
static
std::tuple<std::string, std::string> guiCmdLine(const std::string& indexHtml,
                                                    int port,
                                                    const std::unordered_map<std::string, std::string>& param_map,
                                                    WindowType windowType) {
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
    if(windowType == WindowType::AUTO)
#ifdef USE_PYTHON_UI
        windowType = WindowType::PYTHON;
#else    
        windowType = WindowType::PYTHON;
#endif
    if(windowType == WindowType::PYTHON) {
        const auto py3 = python3({"pywebview", "websockets"}); // OSX Sonoma + python12 made things more complex, hence I have to easier to fallback
        if(py3) {
            constexpr auto py_file = "/pyclient.py"; // let's not use definion in gempyrejsh as that may not be there
            const auto py_data = find(Gempyrejsh, py_file);
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
        }
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
static FileMapping normalizeNames(const Ui::FileMap& files) {
    FileMapping normalized;
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

const GempyreInternal& Ui::ref() const {
    return *m_ui;
}

GempyreInternal& Ui::ref() {
    return *m_ui;
}

 GempyreInternal::GempyreInternal(
        Ui* ui, 
        const Ui::FileMap& filemap,
        std::string_view indexHtml,
        unsigned short port,
        std::string_view root,
        const std::unordered_map<std::string, std::string>& parameters,
        WindowType windowType) :
        m_app_ui{ui}, 
        m_filemap{normalizeNames(filemap)},
        m_windowType{windowType},
        m_startup{[this, port, indexHtml, parameters, root]() {

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
                   std::string{root.empty() ? GempyreUtils::working_dir() : root},
                   [this](){openHandler();},
                   [this](Server::Object&& obj){messageHandler(std::move(obj));},
                   [this](CloseStatus status, int code){closeHandler(status, code);},
                   [this](std::string_view name){return getHandler(name);},
                   [indexHtml, parameters, this](int listen_port){return startListen(std::string{indexHtml}, parameters, listen_port);},
                   last_query_id + 1, // if m_server is created second time it is good that this is > as 1st as pending queries may cause confusion
                   [this]() {add_request([this](){m_app_ui->after(50ms, [this]() { // this is on send error
                            m_server->flush(); // try resend after 50ms
                        });
                        return true;
                    });}
                );

    // if server is not alive in 10s it is dead, right?
    m_app_ui->after(10s, [this]() {
        if (!m_server->isUiReady()) {
            auto errfunc = m_app_ui->on_error(nullptr);
            if (errfunc) {
                errfunc("Error", "ui not ready");
                m_app_ui->on_error(errfunc);
            }
            exit(91);
        }
    });

    }} {}

    void GempyreInternal::messageHandler(Server::Object&& params) {
        const auto it = params.find("type");
        if(it != params.end())  {
            const auto type = it->second;
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "message received:", type);
            if(type == "event") {
                const auto element = params.at("element");
                const auto event = params.at("event");
                const auto properties = params.at("properties");
                //const auto properties = convert(properties_map);
                push_event({element, event, properties});
            } else if(type == "query") {
                const std::string key = params.at("query_value");
                auto id = params.at("query_id");
                auto k = params.at(key);
                push_response(std::move(id), std::move(k));
            } else if(type == "extension_response") {
                gempyre_utils_assert_x(containsAll(keys(params), {"extension_id", "extension_call"}), "extension_response invalid parameters");
                auto id = params.at("extension_id");
                const std::string key = params.at("extension_call");
                auto k = params.at(key);
                push_response(std::move(id), std::move(k));
            } else if(type == "error") {
                GempyreUtils::log(GempyreUtils::LogLevel::Error, "JS says at:", params.at("element"),
                                "error:", params.at("error"));
                if(params.find("trace") != params.end())
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "JS trace:", params.at("trace"));
                call_error(params.at("element"), params.at("error"));
            } else if(type == "exit_request") {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "client kindly asks exit --> Status change Exit");
                set(State::EXIT);
            } else if(type == "extension_ready") {
                //do nothing
            } else if(type == "ui_ready") {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "UI ready request");

                const auto open_function = take_open();
                add_request([open_function, this]() {
                    if(open_function) {
                        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "calling onOpen");
                        open_function();
                    } else {
                        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "missing onOpen");
                    }
                    shoot_requests();
                    return true;
                });
            } else {
                  GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Message", type, "Not handled");        
            }
            signal_pending();
        }
    }

void GempyreInternal::openHandler() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Opening", state_str());
    if(*this == State::CLOSE || *this == State::PENDING) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Request reload, Status change --> Reload");
        set(State::RELOAD);
    }
    signal_pending();    // there may be some pending requests
    //  shoot_requests();
}

void GempyreInternal::closeHandler(Gempyre::CloseStatus closeStatus, int code) { //close
    if(!has_server()) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Close, Status change --> Exit");
        set(State::EXIT);
        signal_pending();
        return;
    }

    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Gempyre close",  state_str(),
                      static_cast<int>(closeStatus), is_connected(), code);

    if(*this != State::EXIT && (closeStatus != CloseStatus::EXIT  && (closeStatus == CloseStatus::CLOSE /*&& m_ui->is_connected()*/))) {
        pendingClose();
    } else if(closeStatus == CloseStatus::FAIL) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Fail, Status change --> Retry");
        set(State::RETRY);
    }

    if(*this == State::EXIT || *this == State::RETRY) {
       signal_pending();
    }
}


void GempyreInternal::pendingClose() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Pending close, Status change --> Pending");
    set(State::PENDING);
    flush_timers(false); //all timers are run here
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Start 1s wait for pending");
        if(*this == State::PENDING) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Pending close, Status change --> Exit");
            set(State::CLOSE);
            signal_pending();
        } else {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Pending cancelled", state_str());
        }
}

std::optional<std::string> GempyreInternal::getHandler(std::string_view name) { //get
    GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "HTTP get", name);
    if(name == "/gempyre.js" || name == "gempyre.js") {
        const auto encoded = Base64::decode(Gempyrejs);
        const auto page = GempyreUtils::join(encoded.begin(), encoded.end());
        return std::make_optional(page);
    }
    const auto file = this->file(name);
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

bool GempyreInternal::startListen(const std::string& indexHtml, const std::unordered_map<std::string, std::string>& parameters , int listen_port) { //listening
    
    if(*this == State::EXIT)
        return false; //we are on exit, no more listening please

    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Listening, Status change --> Running");
    
    set(State::RUNNING);

    const auto& [appui, cmd_params] = guiCmdLine(indexHtml, listen_port, parameters, m_windowType);

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
        return false;
    }
    return true;
}

// This is a bit different from json::dump - rather than json, makes a real strings of values
std::string GempyreInternal::to_string(const nlohmann::json& js) {
    if(js.is_object()) {
        using map_t = std::unordered_map<std::string, std::string>;
        map_t map;
        for(const auto& [key, value] : js.items()) {
            map.emplace(key, to_string(value));
        }
        return '{' + GempyreUtils::join(map, ",",
         [](const auto& kv){return kv.first + ':' + kv.second;}) + '}';
    } else if(js.is_array()) {
        Server::Array array;
        for(const auto& value : js) {
            array.push_back(to_string(value));
        }
        return '[' + GempyreUtils::join(array,  ",") + ']';
    } else if(js.is_boolean()) {
        return std::string(js.get<bool>() ? "true" : "false");
    } else if(js.is_number()) {
        return std::to_string(js.get<double>());
    } else if(js.is_string()) {
        return js.get<std::string>();
    } else {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "conversion dicards", js);
        return std::string("false");
    }
}


void GempyreInternal::eventLoop(bool is_main) {
    GEM_DEBUG("enter", is_main, is_running());
    const GempyreInternal::LoopWatch loop_watch (*this, is_main);
    while(is_running()) {
        wait_events();

        if(*this == State::EXIT) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Eventloop is exiting");
            break;
        }

         if(*this == State::SUSPEND) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Eventloop is suspend");
            break;
        }


        if(*this == State::RETRY) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Eventloop will retry");
            if(! retry_start()) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "retry failed --> Status change Exit");
                set(State::EXIT);
                break;
            }
            continue;
        }

        if(*this == State::CLOSE) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Eventloop is Close", is_running());
            if( ! is_connected()) {
                close_server();
            }
            continue;
        }

        if(*this == State::RELOAD) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Eventloop is Reload");
            if(has_reload())
                add_request([this]() {
                on_reload();
                return true;
            });
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Reload, Status change --> Running");
            set(State::RUNNING);
        }

        if(has_requests() && *this == State::EXIT) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "skip timerqueue", state_str());
        }

        /*if(has_open() && *this == State::RUNNING && is_connected()) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "has open running", state_str());
            //const auto open_function = take_open();
            //set_hold(true);
            //add_request([open_function, this]() {
            //GempyreUtils::log(GempyreUtils::LogLevel::Debug, "call onOpen");
            //open_function();
                //set_hold(false);
            //shoot_requests();
            //    return true;
            //}); //we try to keep logic call order
            continue;
        }*/

        if(!has_open() && is_connected()) {
            assert(!m_onOpen);
            handle_timer_requests();
        }
        

        if(*this == State::PENDING) {
            continue;
        }


        if( has_requests() &&
            *this == State::RUNNING &&
            !has_open() &&
            !is_hold()) {
                shoot_requests();
            }

        if(has_requests()) {
             GempyreUtils::log(GempyreUtils::LogLevel::Debug, "unfinished business", state_str(), is_running());
        }

        //if there are responses they must be handled
        if(has_responses()) {
            if(!is_main) // this was eventloop(true) in exit, changed to false for logic...still works...
                return; //handle query elsewhere, hopefully some one is pending
           // TODO: looks pretty busy (see calc) GempyreUtils::log(GempyreUtils::LogLevel::Warning, "There are unhandled responses on main");
        }

        if( ! has_events() && *this != State::RUNNING) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "skip eventqueue", state_str());
        }

        //events must be last as they may generate more requests or responses
        consume_events();
#if 0
        // Todo: Here all pending tasks are done, but this starts a loop again. I.e. make a busy loop. Therefore in NEXT version 
        // this thread should hold here either for a timer thread or a service thread wakeup. For timebeing just add miniscle sleep
        // that is really a poor solution, but avoids CPU 100%
        std::this_thread::sleep_for(10ms);
#endif

    }
    GEM_DEBUG("Eventloop exit");
}

void GempyreInternal::shoot_requests() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, 
    "shoot_requests",  has_requests(), "running", *this == State::RUNNING, "available", is_ui_available());
    //shoot pending requests
    while(has_requests() && *this == State::RUNNING && is_ui_available()) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "do request");
        auto topRequest = take_request();
        if(!topRequest) // since "flush" can be called in another thread this can happen :-o
            return;
        if(!topRequest()) { //yes I wanna  mutex to be unlocked
            if( ! has_requests())
                std::this_thread::sleep_for(10ms); // busyness
            put_request(std::move(topRequest));
        }
    }
}

void GempyreInternal::send(const DataPtr& data, bool droppable) {
    auto clonedBytes = data->clone();
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "send ui_bin", clonedBytes->size());
    add_request([this, clonedBytes = std::move(clonedBytes), droppable]() mutable {
        #ifdef ENSURE_SEND
            const auto sz = clonedBytes->size();
        #endif
        const auto ok = m_server->send(std::move(clonedBytes), droppable);
        #ifdef ENSURE_SEND
        if(ok && !droppable && sz >= ENSURE_SEND) {           //For some reason the DataPtr MAY not be send (propability high on my mac), but his cludge seems to fix it
            send(m_app_ui->root(), "nil", "");     //correct fix may be adjust buffers and or send Data in several smaller packets .i.e. in case of canvas as
        }                                        //multiple tiles
       #endif
    return ok;
    });
}

// timer elapses calls a function
// that calls a function that adds a another function to a queue
// that function calls a the actual timer function when on top
std::function<void(int)> GempyreInternal::makeCaller(const std::function<void (Ui::TimerId id)>& function) {
    const auto caller =  [this, function](int id) {
        auto call = [function, id]() {
            function(id);
        };
        add_timer(std::move(call));
        signal_pending();
    };
    return caller;
}

void GempyreInternal::consume_events() {
        while(has_events() && *this == State::RUNNING) {
            const auto it = m_eventqueue.take();
            if(it.element.empty()) {
                 GempyreUtils::log(GempyreUtils::LogLevel::Debug,
                  "Root got event:", it.handler,
                  "has open:", has_open(),
                  "State:", state_str(),
                  "Connected", is_connected());
                  //continue;
            }
            const auto element = m_elements.find(it.element);
            if(element != m_elements.end()) {
                const auto handlerName = it.handler;
                const auto handlers = std::get<1>(*element);
                const auto h = handlers.find(handlerName);

                if(h != handlers.end()) {
                    h->second(Event{Element(*m_app_ui, std::move(element->first)), std::move(it.data)});
                } else {
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Cannot find a handler", handlerName, "for element", it.element);
                }
            } else {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Cannot find", it.element, "from elements");
            }
        }
    }
