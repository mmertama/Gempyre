#include "server.h"
#include "gempyre_utils.h"
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <chrono>
#include <random>
#include <numeric>
#define UWS_NO_ZLIB
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall"
#pragma clang diagnostic ignored "-Wextra"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <App.h>
#pragma gcc diagnostic pop
#pragma clang diagnostic pop
#pragma gcc diagnostic pop

#include "json.h"

#include <nlohmann/json.hpp>
// for convenience
using json = nlohmann::json;
using namespace std::chrono_literals;

using  WSServer = uWS::TemplatedApp<false>;
using namespace Gempyre;

constexpr unsigned short PORT_MIN = 30000;
constexpr unsigned short PORT_MAX = 30500;

static std::string fileToMime(const std::string& filename) {
    const auto index = filename.find_last_of('.');
    if(index == std::string::npos)
        return "";
    const std::string_view ext(&filename[index], filename.length() - index);
    static const std::unordered_map<std::string_view, std::string_view> mimes = {
        {".html", "text/html;charset=utf-8"},
        {".css", "text/css;charset=utf-8"},
        {".js", "text/javascript;charset=utf-8"},
        {".txt", "text/txt;charset=utf-8"},
        {"*.ico", "image/x-icon"},
        {"*.png", "image/png"},
        {"*.jpg", "image/jpeg"},
        {"*.gif", "image/gif"}
    };
    const auto it = mimes.find(ext);
    return it == mimes.end() ? " application/octet-stream" : std::string(it->second);
}


std::any convert(const nlohmann::json& js) {
    if(js.is_object()) {
        Server::Object params;
        for(const auto& [key, value] : js.items()) {
            params.emplace(key, convert(value));
        }
#if 0
        for(const auto& [key, value] : params)
            std::cerr << key << "->" << value.type().name() << std::endl;
#endif
        return std::move(params);
    }
    else if(js.is_array()) {
        Server::Array params;
        for(const auto& value : js) {
            params.push_back(convert(value));
        }
        return std::move(params);
    }
    else if(js.is_boolean()) {
        return std::move(std::string(js.get<bool>() ? "true" : "false"));
    }
    else if(js.is_number()) {
         return std::move(std::to_string(js.get<double>()));
    }
    else if(js.is_string()) {
        return std::move(js.get<std::string>());
    }
    else {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "conversion dicards", js);
        return std::move(std::string("false"));
    }
}


static std::string toLower(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), [](auto c){return std::tolower(c);});
    return s;
}

WSServer::WebSocketBehavior options(decltype (WSServer::WebSocketBehavior::open)&& open,  decltype (WSServer::WebSocketBehavior::message)&& message, decltype (WSServer::WebSocketBehavior::close)&& close) {
    WSServer::WebSocketBehavior bh;
    bh.open = std::move(open);
    bh.message = std::move(message);
    bh.close = std::move(close);
  //  bh.maxPayloadLength = 1024 * 1024;
   /* bh.drain = [](auto ws){
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "drain", ws->getBufferedAmount());
    };*/
    return bh;
}

struct SomeData{};

using WSSocket = uWS::WebSocket<false, true>;

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
public:
    bool send(const std::string& text) {
        for(auto& s : m_sockets) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "socket txt buffer", s->getBufferedAmount());
            s->send(text, uWS::OpCode::TEXT);
            // GempyreUtils::log(GempyreUtils::LogLevel::Debug, "socket t2 buffer", s->getBufferedAmount());
        }
        return !m_sockets.empty();
    }
    bool send(const char* data, size_t len) {
        for(auto& s : m_sockets) {
             GempyreUtils::log(GempyreUtils::LogLevel::Debug, "socket bin buffer", s->getBufferedAmount());
             s->send(std::string_view(data, len), uWS::OpCode::BINARY);
          //  GempyreUtils::log(GempyreUtils::LogLevel::Debug, "socket b2 buffer", s->getBufferedAmount());
        }

        return !m_sockets.empty();
    }
    void append(WSSocket* socket) {
        m_sockets.emplace(socket);
    }
    void remove(WSSocket* socket) {
        auto it = m_sockets.find(socket);
        if(it != m_sockets.end())
            m_sockets.erase(it);
    }
    bool empty() const {
        return m_sockets.empty();
    }
    bool size() const {
        return m_sockets.size();
    }
    size_t bufferSize() const {
        auto max = 0;
        std::for_each(m_sockets.begin(), m_sockets.end(),[&max](const auto& a){max = std::max(max, a->getBufferedAmount());});
        return static_cast<size_t>(max);
    }
private:
    std::unordered_set<WSSocket*> m_sockets;
};



Server::Server(
        unsigned short port,
        const std::string& root,
        const std::string& serviceName,
        const OpenFunction& onOpen,
        const MessageFunction& onMessage,
        const CloseFunction& onClose,
        const GetFunction& onGet,
        const ListenFunction& onListen) :
    m_rootFolder(root),
    m_broadcaster(std::make_unique<Broadcaster>()),
    m_onOpen(onOpen),
    m_onMessage(onMessage),
    m_onClose(onClose),
    m_onGet(onGet),
    m_onListen(onListen),
    m_startFunction([=]()->std::unique_ptr<std::thread> {
        return makeServer(port, serviceName);
    }),
    m_server(m_startFunction()) {
#ifdef RANDOM_PORT
    const auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> distribution(PORT_MIN, PORT_MIN + 100);
    m_port = distribution(generator);
#endif
}


std::unique_ptr<std::thread> Server::makeServer(unsigned short port,
                                                  const std::string& serviceName) {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS", "makeServe - create");

    return std::make_unique<std::thread>([this, port, serviceName]() {

        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS", "makeServe - execute");
        auto behavior = options(
                                             [this](auto ws, auto) {
                                                                      GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS open");
                                                                      m_broadcaster->append(ws);
                                                                      m_onOpen(m_broadcaster->size());
                                                                   },
                                             [this](auto ws, auto message, auto opCode) {
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
                                                                       }
                                                                     const auto js = convert(jsObj);
                                                                     auto object = std::any_cast<Object>(js);
                                                                     m_onMessage(std::move(object));

                                             },
                                             [this](auto ws, auto code, auto message) {
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
                                                 GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Socket is closed");
                                             }
                                             );
        m_payloadSize = static_cast<size_t>(behavior.maxPayloadLength);
        WSServer()
        .ws<SomeData>("/" + toLower(serviceName),  std::move(behavior))
        .get("/*", [this, serviceName](auto* res, auto* req) {
            const auto url = std::string(req->getUrl());
            const auto serverData = m_onGet(url);
            std::string page;
            if(serverData.has_value()) {
                page = serverData.value();
            } else {
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
                if(fullPath.empty())
                    fullPath = m_rootFolder + url;
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "GET",
                       "Uri:", url,
                       "query:", req->getQuery(),
                       "path:", fullPath,
                       "header:", GempyreUtils::joinPairs(req->begin(), req->end()));
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
                res->end("<html><body><h1>Ooops</h1><h3>404 Not Found </h3><h5>" + std::string(req->getUrl()) + "</h5><i>" + serviceName + "</i></body></html>");
            }
            })
        .listen(getPort(port), [this, port](auto* socket) {
            char PADDING[2];
            if (socket) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "listening on port:", m_port);
                m_closeData = socket;
                if(!m_onListen(m_port)){
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "List callback failed, closing");
                    doClose();
                } else {
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Listen ok, wait for event");
                }
            } else if(port > 0) {
                GempyreUtils::log(GempyreUtils::LogLevel::Error, "listening on port:", port, "failed", GempyreUtils::lastError());
                m_onClose(Close::EXIT, -1);
            } else {
                GempyreUtils::log(GempyreUtils::LogLevel::Warning, "try listen on port:", m_port, "failed", GempyreUtils::lastError());
                m_onClose(Close::FAIL, -1);
            }
            }).run();
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server is about go close");
    });
}


Server::~Server() {
    close(true);
}

void Server::closeSocket() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server", "closeSocket", m_closeData.has_value());
    if(m_closeData.has_value()) {
        auto socket = std::any_cast<us_listen_socket_t*>(m_closeData);
        us_listen_socket_close(0, socket);
        m_closeData.reset();
    }
}

bool Server::retryStart() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Retry", m_port);
    if(m_port >= PORT_MAX)
        return false;

    closeSocket();

    if(m_server) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Joining server");
        m_server->join();
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server joined");
    }
    if(m_startFunction)
        m_server = m_startFunction();
    return true;
}

unsigned short Server::getPort(unsigned short port) {
    if(port == 0) {
        if(m_port == 0) {
            m_port = PORT_MIN;
        } else {
            ++m_port;
        }
    } else {
        m_port = port;
    }
    return m_port;
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
        if(hasRoom(str.size())) {
            m_broadcaster->send(str);
            m_batch.reset();
            return true;
        }
        return false;
    }
    return true;
}

bool Server::send(const std::unordered_map<std::string, std::string>& object, const std::any& values) {
    json js;
    if(values.has_value()) {
        const auto jopt = toString(values);
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
        if(!hasRoom(str.length())) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Not enough room Text", str.length());
            return false;
        }
        m_broadcaster->send(str);
    }
    return true;
}

bool Server::send(const char *data, size_t len) {
    if(!hasRoom(len)) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Not enough room Data", len);
        return false;
    }
    return m_broadcaster->send(data, len);
    return true;
}

bool Server::hasRoom(size_t sz) const {
    return sz <= (m_payloadSize - m_broadcaster->bufferSize()) ;
}

void Server::doClose() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Do Close", m_closeData.has_value());
    m_startFunction = nullptr; //mutexx?
    closeSocket();
}

void Server::close(bool wait) {
    doClose();
    if(wait && m_server && m_server->joinable()) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Wait server to close");
        m_server->join();
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server close");
    }
}
