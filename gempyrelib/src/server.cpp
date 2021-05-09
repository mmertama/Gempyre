#include "server.h"
#include "gempyre_utils.h"
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <chrono>
#include <random>
#include <numeric>
#ifdef COMPILER_CLANG
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wall"
    #pragma clang diagnostic ignored "-Wextra"
#endif
#ifdef COMPILER_GCC
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wall"
    #pragma GCC diagnostic ignored "-Wextra"
    #pragma GCC diagnostic ignored "-Wunused-parameter"
    #pragma GCC diagnostic ignored "-Wsign-compare"
    #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    #pragma GCC diagnostic ignored "-Wunused-variable"
#endif
#define UWS_NO_ZLIB
#include <App.h>
#ifdef COMPILER_GCC
    #pragma GCC diagnostic pop
#endif
#ifdef COMPILER_CLANG
    #pragma clang diagnostic pop
#endif

#include <nlohmann/json.hpp>

// for convenience

using json = nlohmann::json;
using namespace std::chrono_literals;

struct SomeData {};
using WSServer = uWS::TemplatedApp<false>;
using WSBehaviour = WSServer::WebSocketBehavior<SomeData>;
using WSSocket = uWS::WebSocket<false, true, SomeData>;
using namespace Gempyre;

constexpr unsigned short DEFAULT_PORT  = 30000;
constexpr unsigned short PORT_ATTEMPTS = 50;
constexpr size_t WS_MAX_LEN = 16 * 1024;
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


std::any convert(const nlohmann::json& js) {
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

class Gempyre::Broadcaster {
    static constexpr auto DELAY = 100ms;
public:
    bool send(const std::string_view& text) {
        for(auto& s : m_sockets) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "socket txt buffer", s->getBufferedAmount());
            const auto success = s->send(text, uWS::OpCode::TEXT);
            if(!success) {
                GempyreUtils::log(GempyreUtils::LogLevel::Warning, "socket t2 buffer", s->getBufferedAmount());
                if(!m_backPressureMutex.try_lock_for(DELAY))
                    GempyreUtils::log(GempyreUtils::LogLevel::Warning, "Cannot lock backpressure mutex");
                return false;
            }
        }
        return !m_sockets.empty();
    }

    bool send(const char* data, size_t len) {
        for(auto& s : m_sockets) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "socket bin buffer", s->getBufferedAmount());
            const auto success = s->send(std::string_view(data, len), uWS::OpCode::BINARY);
            if(!success) {
                GempyreUtils::log(GempyreUtils::LogLevel::Warning, "socket b2 buffer", s->getBufferedAmount());
                if(!m_backPressureMutex.try_lock_for(DELAY))
                       GempyreUtils::log(GempyreUtils::LogLevel::Warning, "Cannot lock backpressure mutex");
                return false;
            }
        }
        return !m_sockets.empty();
    }

    void append(WSSocket* socket) {
        m_sockets.emplace(socket);
    }

    void remove(WSSocket* socket) {
        auto it = m_sockets.find(socket);
        if(it != m_sockets.end()) {
            m_sockets.erase(it);
        }
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "socket erased", m_sockets.size());
    }

    bool empty() const {
        return m_sockets.empty();
    }

    bool size() const {
        return m_sockets.size();
    }

    size_t bufferSize() const {
        auto min = 0U;
        for(const auto& s : m_sockets) {
            min = std::min(min, s->getBufferedAmount());
        }
        return static_cast<size_t>(min);
    }

    void unlock() {
        m_backPressureMutex.unlock();
    }

private:
    std::unordered_set<WSSocket*> m_sockets;
    std::timed_mutex m_backPressureMutex;
};

int wishAport(int port, int max) {
    int end = port + max;
    while(!GempyreUtils::isAvailable(port)) {
        ++port;
        if(port == end) {
            GempyreUtils::log(GempyreUtils::LogLevel::Error, "wish a port", GempyreUtils::lastError());
            return 0;
        }
    }
    return port;
}

Server::Server(
    unsigned short port,
    const std::string& root,
    const OpenFunction& onOpen,
    const MessageFunction& onMessage,
    const CloseFunction& onClose,
    const GetFunction& onGet,
    const ListenFunction& onListen) :
    m_port(port == 0 ? wishAport(DEFAULT_PORT, PORT_ATTEMPTS) : port),
    m_rootFolder(root),
    m_broadcaster(std::make_unique<Broadcaster>()),
    m_onOpen(onOpen),
    m_onMessage(onMessage),
    m_onClose(onClose),
    m_onGet(onGet),
    m_onListen(onListen),
    //mStartFunction([this]()->std::unique_ptr<std::thread> {
//   return makeServer();
//}),
    m_serverThread(std::make_unique<std::thread>([this]() {
    serverThread(m_port);
})) {
#ifdef RANDOM_PORT
    const auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> distribution(PORT_MIN, PORT_MIN + 100);
    m_port = distribution(generator);
#endif
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

void Server::serverThread(unsigned short port) {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS", "makeServe - execute, using port:", port);
    auto openHandler =
            [this](auto ws) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS open");
                m_broadcaster->append(ws);
                m_onOpen(m_broadcaster->size());
            };
    auto messageHandler =
            [this](auto ws, auto message, auto opCode) {
                (void) ws;
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS message", message, opCode);
                const auto jsObj = json::parse(message);
                const auto f = jsObj.find("type");
                if(f != jsObj.end()) {
                    if(*f == "keepalive") {
                        return;
                    }
                    if(*f == "uiready") {
                        m_uiready = true;
                    }
                    if(*f == "extensionready") {
                        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS", "exteansionready");
                        return;
                    }
                    if(*f == "log") {
                        const auto log = jsObj.find("level");
                        const auto msg = jsObj.find("msg");
                        if(*log == "log")
                            GempyreUtils::log(GempyreUtils::LogLevel::Info, "JS", *msg);
                        if(*log == "info")
                            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "JS", *msg);
                        if(*log == "warn")
                            GempyreUtils::log(GempyreUtils::LogLevel::Warning, "JS", *msg);
                        if(*log == "")
                            GempyreUtils::log(GempyreUtils::LogLevel::Error, "JS", *msg);
                        return;
                    }
                }
                const auto js = convert(jsObj);
                auto object = std::any_cast<Object>(js);
                m_onMessage(std::move(object));

            };
    auto closeHandler = [this](auto ws, auto code, auto message) {
        if(code != 1001 && code != 1006) {  //browser window closed
            if(code == 1000 || (code >= 1002 && code <= 1015)  || (code >= 3000 && code <= 3999) || (code >= 4000 && code <= 4999)) {
                GempyreUtils::log(GempyreUtils::LogLevel::Error, "WS", "closed on error", code, message);
            }   else if(code != 0) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS", "Non closing error", code, message);
                return;
            }
        }
        //exit request
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS", "close", code, message);
        m_broadcaster->remove(ws);
        m_onClose(Close::CLOSE, code);
        ws->close();
        doClose();
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Socket is closed");
    };

    WSBehaviour behavior;
    behavior.open = openHandler;
    behavior.message = messageHandler;
    behavior.close = closeHandler;
    //  bh.maxPayloadLength = 1024 * 1024;
    behavior.drain = [this](auto ws) {
        GempyreUtils::log(GempyreUtils::LogLevel::Warning, "drain", ws->getBufferedAmount());
        m_broadcaster->unlock(); //release backpressure wait
    };



    WSServer()
    .ws<SomeData>("/" + toLower(SERVICE_NAME), std::move(behavior))
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
                    const auto queries = GempyreUtils::split<std::vector<std::string>>(q, '=');
                    if(queries.size() == 2) {
                        if(queries[0] == "file") {
                            fullPath = GempyreUtils::unhexify(queries[1]);
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
            if(page.empty() && GempyreUtils::fileExists(fullPath)) {
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
    .listen(port, [this, port](auto * socket) {
        char PADDING[2];
        (void) PADDING;
        if(socket) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "listening on port:", port);
            m_closeData = socket;
            if(!m_onListen(port)) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "List callback failed, closing");
                doClose();
            } else {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Listen ok, wait for event");
            }
        } else {
            GempyreUtils::log(GempyreUtils::LogLevel::Warning, "try listen on port:", port, "failed", GempyreUtils::lastError());
            m_onClose(Close::FAIL, -1);
        }
    }).run();
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server is about go close");
}


Server::~Server() {
    close(true);
}

int Server::addPulled(DataType type, const std::string_view& data) {
    ++m_pulledId;
    m_pulled.emplace(std::to_string(m_pulledId), std::pair<DataType, std::string> {type, std::string(data)});
    return m_pulledId;
}

void Server::closeSocket() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server", "closeSocket", m_closeData.has_value());
    if(m_closeData.has_value()) {
        auto socket = std::any_cast<us_listen_socket_t*>(m_closeData);
        us_listen_socket_close(0, socket);
        m_closeData.reset();
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server", "listen socket closed");
    }
}



bool Server::retryStart() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Retry", m_port);
    const int port = wishAport(m_port, PORT_ATTEMPTS);
    if(port <= 0) {
        GempyreUtils::log(GempyreUtils::LogLevel::Error, "Listen ports:", m_port, "failed", GempyreUtils::lastError());
        return false;
    }

    closeSocket();

    if(m_serverThread) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Joining server");
        m_serverThread->join();
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server joined");
    }

    m_serverThread.reset();

    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "retry end", m_doExit);
    if(!m_doExit) {
        m_serverThread = std::make_unique<std::thread>([this]() {serverThread(m_port);});
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
        if(str.size() < WS_MAX_LEN) {
            if(!m_broadcaster->send(str))
                return false;
        } else {
            const auto pull = addPulled(DataType::Json, str);
            const json obj = {{"type", "pull_json"}, {"id", pull}};
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "add batch pull", str.size(), pull);
            if(!m_broadcaster->send(obj.dump()))
                return false;
        }
        m_batch.reset();
    }
    return true;
}

bool Server::send(const std::unordered_map<std::string, std::string>& object, const std::any& values) {
    json js;
    if(values.has_value()) {
        const auto jopt = GempyreUtils::toJsonString(values);
        if(jopt.has_value()) {
            js = json::parse(*jopt);
        }
    }
    for(const auto& [key, value] : object) {
        js[key] = value;
    }
    if(m_batch) {
        m_batch->push_back(std::move(js));
    } else {
        const auto str = js.dump();
        if(str.size() < WS_MAX_LEN) {
            if(!m_broadcaster->send(str))
                return false;
        } else {
            const auto pull = addPulled(DataType::Json, str);
            const json obj = {{"type", "pull_json"}, {"id", pull}};
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "add text pull", str.size(), pull);
            if(!m_broadcaster->send(obj.dump()))
                   return false;
        }

    }
    return true;
}

bool Server::send(const char* data, size_t len) {
    if(len < WS_MAX_LEN) {
        if(!m_broadcaster->send(data, len))
            return false;
    } else {
        const auto pull = addPulled(DataType::Bin, {data, len});
        const json obj = {{"type", "pull_binary"}, {"id", pull}};
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "add bin pull", len, pull);
        if(!m_broadcaster->send(obj.dump()))
            return false;
    }
    return true;
}

void Server::doClose() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Do Close", m_closeData.has_value());
    m_doExit = true;
    //mStartFunction = nullptr; //mutexx?
    closeSocket();
}

void Server::close(bool wait) {
    doClose();
    if(wait && m_serverThread && m_serverThread->joinable()) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Wait server to close");
        m_serverThread->join();
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server close");
    }
}
