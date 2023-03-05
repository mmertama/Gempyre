#include "server.h"
#include "gempyre_utils.h"
#include "broadcaster.h"
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <chrono>
#include <random>
#include <numeric>
#include <nlohmann/json.hpp>

// for convenience

using json = nlohmann::json;
using namespace std::chrono_literals;

using WSServer = uWS::TemplatedApp<false>;
using WSBehaviour = WSServer::WebSocketBehavior<Gempyre::ExtraSocketData>;
using namespace Gempyre;

constexpr unsigned short DEFAULT_PORT  = 30000;
constexpr unsigned short PORT_ATTEMPTS = 50;
constexpr unsigned PAYLOAD_SIZE = 4 * 1024 * 1024;
constexpr unsigned BACKPRESSURE_SIZE = 4 * 1024 * 1024;

#ifdef PULL_MODE
constexpr size_t WS_MAX_LEN = 16 * 1024;
#endif

constexpr auto SERVICE_NAME = "Gempyre";

static std::string fileToMime(const std::string_view& filename) {
    const auto index = filename.find_last_of('.');
    if(index == std::string::npos) {
        return "";
    }
    const std::string_view ext(&filename[index], filename.length() - index);
    static const std::unordered_map<std::string_view, std::string_view> mimes = {
        {".html", "text/html;charset=utf-8"},
        {".css", "text/css;charset=utf-8"},
        {".js", "text/javascript;charset=utf-8"},
        {".txt", "text/txt;charset=utf-8"},
        {".ico", "image/x-icon"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"}
    };
    const auto it = mimes.find(ext);
    const auto mimeType = it == mimes.end() ? " application/octet-stream" : std::string(it->second);
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Mime type:", filename, mimeType);
    return mimeType;
}


static std::any convert(const nlohmann::json& js) {
    if(js.is_object()) {
        Server::Object params;
        for(const auto& [key, value] : js.items()) {
            params.emplace(key, convert(value));
        }
#if 0
        for(const auto& [key, value] : params) {
            std::cerr << key << "->" << value.type().name() << std::endl;
        }
#endif
        return std::make_any<Server::Object>(params);
    } else if(js.is_array()) {
        Server::Array params;
        for(const auto& value : js) {
            params.push_back(convert(value));
        }
        return std::make_any<Server::Array>(params);
    } else if(js.is_boolean()) {
        return std::make_any<std::string>(std::string(js.get<bool>() ? "true" : "false"));
    } else if(js.is_number()) {
        return std::make_any<std::string>(std::to_string(js.get<double>()));
    } else if(js.is_string()) {
        return std::make_any<std::string>(js.get<std::string>());
    } else {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "conversion dicards", js);
        return std::make_any<std::string>(std::string("false"));
    }
}


static std::string toLower(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), [](auto c) {return std::tolower(c);});
    return s;
}


static unsigned wishAport(unsigned port, unsigned max) {
    auto end = port + max;
    while(!GempyreUtils::is_available(static_cast<unsigned short>(port))) {
        ++port;
        if(port == end) {
            GempyreUtils::log(GempyreUtils::LogLevel::Error, "wish a port", GempyreUtils::last_error());
            return 0;
        }
    }
    return port;
}

static std::string notFoundPage(const std::string_view& url, const std::string_view& info = "") {
    return R"(<html>
           <header>
               <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate, max-age=0 "/>
               <meta http-equiv="Pragma" content="no-cache" />
               <meta http-equiv="Expires" content="0" />
               <meta charset="UTF-8">
               <style>
               #styled {
                   color:red;
                   font-size:32px
               }
               </style>
           </header>
      <body><h1>Ooops</h1><h3 class="styled">404 Data Not Found </h3><h5>)" + std::string(url) + "</h5><i>" + std::string(info) + "</i></body></html>";
}


class Gempyre::Batch {
public:
    Batch() {
        m_data["type"] = "batch";
        m_array = json::array();
    }
    void push_back(json&& jobj) {
        m_array.push_back(std::forward<json>(jobj));
    }
    std::string dump() {
        m_data["batches"] =  std::move(m_array);
        return m_data.dump();
    }
private:
    json m_data;
    json m_array;
};


class Gempyre::SocketHandler {
    public:
    explicit SocketHandler(Server& server) : m_s(server){}
    void openHandler(Gempyre::WSSocket* ws) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS open");
        m_s.m_broadcaster->append(ws);
        m_s.m_onOpen();
    }
    void messageHandler(Gempyre::WSSocket* ws, std::string_view message, int opCode) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS message", message, opCode);
        const auto jsObj = json::parse(message);
        const auto f = jsObj.find("type");
        if(f != jsObj.end()) {
            if(*f == "keepalive") {
                if(m_s.m_doExit) {
                    ws->close();
                }
                return;
            }
            if(*f == "uiready") {
                m_s.m_uiready = true;
                m_s.m_broadcaster->setType(ws, Broadcaster::Type::Ui);
            }
            if(*f == "extensionready") {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Ext", "extensionready");
                m_s.m_broadcaster->setType(ws, Broadcaster::Type::Extension);

            }
            if(*f == "extension") {
                const auto log = jsObj.find("level");
                const auto msg = jsObj.find("msg");
                if(*log == "log")
                    GempyreUtils::log(GempyreUtils::LogLevel::Info, "Ext", *msg);
                else if(*log == "info")
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Ext", *msg);
                else if(*log == "warn")
                    GempyreUtils::log(GempyreUtils::LogLevel::Warning, "Ext", *msg);
                else if(*log == "error" || log->empty())
                    GempyreUtils::log(GempyreUtils::LogLevel::Error, "Ext", *msg);
                return;
            }
            if(*f == "log") {
                const auto log = jsObj.find("level");
                const auto msg = jsObj.find("msg");
                if(*log == "log")
                    GempyreUtils::log(GempyreUtils::LogLevel::Info, "JS", *msg);
                else if(*log == "info")
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "JS", *msg);
                else if(*log == "warn")
                    GempyreUtils::log(GempyreUtils::LogLevel::Warning, "JS", *msg);
                else if(*log == "" || *log == "error") {
                    GempyreUtils::log(GempyreUtils::LogLevel::Error, "JS", *msg);
                    const auto trace = jsObj.find("trace");
                    if(trace != jsObj.end()) {
                        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "JS-TRACE", *trace);
                    }
                }
                return;
            }
        }
        const auto js = convert(jsObj);
        auto object = std::any_cast<Server::Object>(js);
        m_s.m_onMessage(std::move(object));

    }
    void closeHandler(WSSocket* ws, int code, std::string_view message) {
        if(code != 1001 && code != 1006) {  //browser window closed
            if(code == 1000 || (code != 1005 && code >= 1002 && code <= 1015)  || (code >= 3000 && code <= 3999) || (code >= 4000 && code <= 4999)) {
                GempyreUtils::log(GempyreUtils::LogLevel::Error, "WS", "closed on error", code, message);
            }   else if(code != 0) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS", "Non closing error", code, message);
                m_s.m_broadcaster->remove(ws);
                return;
            }
        }
        //exit request
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS", "close", code, message);
        m_s.m_broadcaster->remove(ws);
        m_s.m_onClose(CloseStatus::CLOSE, code);
        const auto close_ok = ws->close();
        m_s.doClose();
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Socket is closed", close_ok);
    }
    private:
    Server& m_s;
};



Server::Server(
    unsigned port,
    const std::string& root,
    const OpenFunction& onOpen,
    const MessageFunction& onMessage,
    const CloseFunction& onClose,
    const GetFunction& onGet,
    const ListenFunction& onListen,
    int queryIdBase) :
    m_port(port == 0 ? wishAport(DEFAULT_PORT, PORT_ATTEMPTS) : port),
    m_rootFolder(root),
    m_broadcaster(std::make_unique<Broadcaster>()),
    m_onOpen(onOpen),
    m_onMessage(onMessage),
    m_onClose(onClose),
    m_onGet(onGet),
    m_onListen(onListen),
    m_queryId{queryIdBase},
    //mStartFunction([this]()->std::unique_ptr<std::thread> {
//   return makeServer();
//}),
    m_serverThread{newThread()} {
#ifdef RANDOM_PORT
    const auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> distribution(PORT_MIN, PORT_MIN + 100);
    m_port = distribution(generator);
#endif
}

std::unique_ptr<std::thread> Server::newThread() {
    auto thread = std::make_unique<std::thread>([this]() {
                serverThread(m_port);
            });
    m_waitStart.wait();
    return thread;
}

void Server::serverThread(unsigned int port) {
    assert(!m_isRunning);
    assert(!m_uiready);

    if(m_doExit) {
        m_waitStart.signal();  // on exit
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS", "early exit request!");
        return;
    }

    m_isRunning = true;
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS", "makeServe - execute, using port:", port);
    m_waitStart.signal(); // on success

    WSBehaviour behavior;
    behavior.open =  [this](WSSocket* ws) {
        Gempyre::SocketHandler(*this).openHandler(ws);
    };;
    behavior.message =  [this](auto ws, auto message, auto opCode) {
        Gempyre::SocketHandler(*this).messageHandler(ws, message, opCode);
    };;
    behavior.close = [this](auto ws, auto code, auto message) {
        Gempyre::SocketHandler(*this).closeHandler(ws, code, message);
    };
    behavior.maxPayloadLength =  PAYLOAD_SIZE;
    behavior.maxBackpressure = BACKPRESSURE_SIZE;
    behavior.drain = [this](auto ws) {
        GempyreUtils::log(GempyreUtils::LogLevel::Warning, "drain", ws->getBufferedAmount());
        m_broadcaster->unlock(); //release backpressure wait
    };

    assert(!m_uiready);

    auto app = WSServer()
    .ws<ExtraSocketData>("/" + toLower(SERVICE_NAME), std::move(behavior))
    .get("/data/:id", [this](auto * res, auto * req) {
        const auto id = std::string(req->getParameter(0)); //till c++20 ?
        const auto it = m_pulled.find(id);
        if(it == m_pulled.end()) {
            res->writeStatus("404 Not Found");
            res->writeHeader("Content-Type", "text/html; charset=utf-8");
            res->end(notFoundPage(req->getUrl()));
            GempyreUtils::log(GempyreUtils::LogLevel::Error, "pull not found", id);

        } else {
            const auto mime = std::get<DataType>(it->second) == DataType::Json ? "application/json" : "application/octet-stream";
            res->writeHeader("Content-Type", mime);
            res->writeStatus(uWS::HTTP_200_OK);
            res->end(std::get<std::string>(it->second));
            m_pulled.erase(it);
        }
    })
    .get("/*", [this](auto * res, auto * req) {
        const auto url = req->getUrl();
        const auto serverData = m_onGet(url);
        std::string page;
        if(serverData.has_value()) {
            page = serverData.value();
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "server get:", page.size());
        } else {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "server get does an file query");
            std::string fullPath;
            const auto query = req->getQuery();
            if(!query.empty()) {
                const auto queries = GempyreUtils::split<std::vector<std::string>>(std::string(query), '&');
                for(const auto& q : queries) {
                    const auto query_list = GempyreUtils::split<std::vector<std::string>>(q, '=');
                    if(query_list.size() == 2) {
                        if(query_list[0] == "file") {
                            fullPath = GempyreUtils::unhexify(query_list[1]);
                        }
                    }
                }
            }
            if(fullPath.empty()) {
                fullPath = m_rootFolder;
                fullPath.append(url);
            }
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "GET",
                              "Uri:", url,
                              "query:", req->getQuery(),
                              "path:", fullPath,
                              "header:", GempyreUtils::join<uWS::HttpRequest::HeaderIterator, std::pair<std::string_view, std::string_view>, std::string>(
                                  req->begin(),
                                  req->end(),
                                  ",",
            [](const auto & p) {return std::string(p.first) + " " + std::string(p.second);}));
            if(page.empty() && GempyreUtils::file_exists(fullPath)) {
                page = GempyreUtils::slurp(fullPath);
            } else {
                GempyreUtils::log(GempyreUtils::LogLevel::Warning, "path:", fullPath, "Not found");
            }
        }
        if(!page.empty()) {
            const auto mime = fileToMime(url);
            res->writeHeader("Content-Type", mime);
            res->writeStatus(uWS::HTTP_200_OK);
            res->end(page);
        } else {
            res->writeStatus("404 Not Found");
            res->writeHeader("Content-Type", "text/html; charset=utf-8");
            res->end(notFoundPage(req->getUrl(), SERVICE_NAME));
            GempyreUtils::log(GempyreUtils::LogLevel::Error, "404, not found", url);
        }
    })
    .listen(static_cast<int>(port), [this, port](auto socket) {
        char PADDING[2];
        (void) PADDING;
        assert(!m_uiready);
        if(socket) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "listening on port:", port);
            assert(!m_closeData);
            m_closeData.store(socket);
            if(m_doExit) { // cancel, exit has bee requested!
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Listen cancelled, closing");
                doClose();
            }
            else if(!m_onListen(port)) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Listen callback failed, closing");
                doClose();
            } else {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Listen ok, wait for event");
            }
        } else {
            GempyreUtils::log(GempyreUtils::LogLevel::Warning, "try listen on port:", port, "failed", GempyreUtils::last_error());
            m_onClose(CloseStatus::FAIL, -1);
        }
    }).run();

    //m_doExit can false here or very soon, but we cannot avoid starting to server
    //just hope that closing sockets will desctruct just created app about right away
    // protect with mutex if not ok
    //if(!m_doExit) {
    //    app.run(); // start app
    //}
    m_isRunning = false;
    m_doExit = false;
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server is about go close");
}


Server::~Server() {
    close(true);
    assert(!m_broadcaster ||  m_broadcaster->empty());
}

int Server::addPulled(DataType type, const std::string_view& data) {
    ++m_pulledId;
    m_pulled.emplace(std::to_string(m_pulledId), std::pair<DataType, std::string> {type, std::string(data)});
    return m_pulledId;
}

void Server::closeListenSocket() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server", "closeSocket", static_cast<bool>(m_closeData));
    if(m_closeData) {
        auto socket = m_closeData.load();
        m_closeData.store(nullptr);
        us_listen_socket_close(0, socket);
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server", "listen socket closed");
    }
}



bool Server::retryStart() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Retry", m_port);
    const auto port = wishAport(m_port, PORT_ATTEMPTS);
    if(port <= 0) {
        GempyreUtils::log(GempyreUtils::LogLevel::Error, "Listen ports:", m_port, "failed", GempyreUtils::last_error());
        return false;
    }

    closeListenSocket();

    if(m_serverThread) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Joining server");
        m_serverThread->join();
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server joined");
    }

    m_serverThread.reset();

    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "retry end", m_doExit.load());
    if(!m_doExit) {
        m_serverThread = newThread();
        return true;
    } else {
        return false;
    }
    /*if(mStartFunction) {
        m_serverThread = mStartFunction();
    }*/
    return true;
}

bool Server::isConnected() const {
    return !m_broadcaster->empty() && m_uiready;
}

bool Server::beginBatch() {
    m_batch = std::make_unique<Batch>();
    return true;
}

bool Server::endBatch() {
    if(m_batch) {
        const auto str = m_batch->dump();
#ifdef PULL_MODE        
        if(str.size() < WS_MAX_LEN) {
#endif            
            if(!m_broadcaster->send(str))
                return false;
#ifdef PULL_MODE                
        } else {
            const auto pull = addPulled(DataType::Json, str);
            const json obj = {{"type", "pull_json"}, {"id", pull}};
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "add batch pull", str.size(), pull);
            if(!m_broadcaster->send(obj.dump()))
                return false;
        }
#endif        
        m_batch.reset();
    }
    return true;
}

bool Server::send(const std::unordered_map<std::string, std::string>& object, const std::any& values) {
    json js;
    if(values.has_value()) {
        const auto jopt = GempyreUtils::to_json_string(values);
        if(jopt.has_value()) {
            js = json::parse(*jopt);
        }
    }
    [[maybe_unused]] bool is_ext = false;
    for(const auto& [key, value] : object) {
        js[key] = value;
        if(key == "type" && value == "extension") {
            is_ext = true;
        }
    }
    if(m_batch) {
        m_batch->push_back(std::move(js));
    } else {
        const auto str = js.dump();
#ifdef PULL_MODE        
        if(str.size() < WS_MAX_LEN) {
#endif            
            if(!m_broadcaster->send(str))
                return false;
#ifdef PULL_MODE                
        } else {
            const auto pull = addPulled(DataType::Json, str);
            const json obj = {{"type", "pull_json"}, {"id", pull}};
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "add text pull", str.size(), pull);
            if(!m_broadcaster->send(obj.dump(), is_ext))
                   return false;
        }
#endif
    }
    return true;
}

bool Server::send(const Gempyre::Data& ptr) {
#ifdef PULL_MODE    
    if(len < WS_MAX_LEN) {
#endif        
        if(!m_broadcaster->send(ptr))
            return false;
#ifdef PULL_MODE            
    } else {
        const auto pull = addPulled(DataType::Bin, {data, len});
        const json obj = {{"type", "pull_binary"}, {"id", pull}};
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "add bin pull", len, pull);
        if(!m_broadcaster->send(obj.dump()))
            return false;
    }
#endif    
    return true;
}

void Server::doClose() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Do Close", static_cast<bool>(m_closeData));
    m_doExit = true;
    closeListenSocket();
}

void Server::close(bool wait) {
    int attempts = 20;
    while(!m_broadcaster->empty() && --attempts > 0) {
        std::this_thread::sleep_for(100ms);
    }
    m_broadcaster->forceClose();
    doClose();
    if(wait && m_serverThread && m_serverThread->joinable()) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Wait server to close", !!m_closeData);
        m_serverThread->join();
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server close");
    }
}
