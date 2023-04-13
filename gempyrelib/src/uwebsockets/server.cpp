#include "uws_server.h"
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


constexpr unsigned PAYLOAD_SIZE = 4 * 1024 * 1024;
constexpr unsigned BACKPRESSURE_SIZE = 4 * 1024 * 1024;

#ifdef PULL_MODE
constexpr size_t WS_MAX_LEN = 16 * 1024;
#endif

constexpr auto SERVICE_NAME = "Gempyre";

std::unique_ptr<Server> Gempyre::create_server(unsigned int port,
           const std::string& rootFolder,
           const Server::OpenFunction& onOpen,
           const Server::MessageFunction& onMessage,
           const Server::CloseFunction& onClose,
           const Server::GetFunction& onGet,
           const Server::ListenFunction& onListen,
           int queryIdBase,
           const Server::ResendRequest& request) {
                return std::unique_ptr<Server>(new Uws_Server(
                    port, rootFolder, onOpen, onMessage, onClose, onGet, onListen, queryIdBase, request
                    ));
           }


static std::string toLower(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), [](auto c) {return std::tolower(c);});
    return s;
}



class Gempyre::Batch {
public:
    Batch() : m_arrays{} {
    }

    void push_back(Server::TargetSocket target, json&& jobj) {
        m_arrays[target].push_back(std::forward<json>(jobj));
    }

    std::string dump(Server::TargetSocket target) {
        auto data = json::object();
        data["type"] = "batch";
        data["batches"] =  std::move(m_arrays[target]);
        return data.dump();
    }
private:
    std::unordered_map<Server::TargetSocket, json::array_t> m_arrays;
};


class Gempyre::SocketHandler {
    public:
    explicit SocketHandler(Uws_Server& server) : m_s(server){}
    void openHandler(Gempyre::WSSocket* ws) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS open");
        m_s.m_broadcaster->append(ws);
        m_s.m_onOpen();
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
    Uws_Server& m_s;
};



Uws_Server::Uws_Server(
    unsigned port,
    const std::string& root,
    const Server::OpenFunction& onOpen,
    const Server::MessageFunction& onMessage,
    const Server::CloseFunction& onClose,
    const Server::GetFunction& onGet,
    const Server::ListenFunction& onListen,
    int queryIdBase,
    const Server::ResendRequest& resendRequest) : Server{port, root, onOpen, onMessage, onClose, onGet, onListen, queryIdBase},
    //mStartFunction([this]()->std::unique_ptr<std::thread> {
//   return makeServer();
//}),
    m_broadcaster(std::make_unique<Broadcaster>([this, resendRequest](WSSocket::SendStatus) {
        resendRequest();
    })),
    m_serverThread{newThread()} {
#ifdef RANDOM_PORT
    const auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> distribution(PORT_MIN, PORT_MIN + 100);
    m_port = distribution(generator);
#endif
}

 void Uws_Server::flush() {
    m_broadcaster->flush();
 }

std::unique_ptr<std::thread> Uws_Server::newThread() {
    auto thread = std::make_unique<std::thread>([this]() {
                serverThread(m_port);
            });
    m_waitStart.wait();
    return thread;
}

void Uws_Server::serverThread(unsigned int port) {
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
        (void) opCode;
        switch(messageHandler(message)) {
            case MessageReply::DoNothing:
                if(m_doExit) {
                    ws->close();
                }
                return;
            case MessageReply::AddUiSocket:
                m_uiready = true;
                m_broadcaster->setType(ws, Server::TargetSocket::Ui);
                return;
             case MessageReply::AddExtensionSocket:
                 m_broadcaster->setType(ws, Server::TargetSocket::Extension);
                return;    
        }
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

    m_broadcaster->set_loop(uWS::Loop::get());

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


Uws_Server::~Uws_Server() {
    close(true);
    assert(!m_broadcaster ||  m_broadcaster->empty());
}

int Uws_Server::addPulled(DataType type, const std::string_view& data) {
    ++m_pulledId;
    m_pulled.emplace(std::to_string(m_pulledId), std::pair<DataType, std::string> {type, std::string(data)});
    return m_pulledId;
}

void Uws_Server::closeListenSocket() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server", "closeSocket", static_cast<bool>(m_closeData));
    if(m_closeData) {
        auto socket = m_closeData.load();
        m_closeData.store(nullptr);
        us_listen_socket_close(0, socket);
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server", "listen socket closed");
    }
}



bool Uws_Server::retryStart() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Retry", m_port);
    const auto ui_port = wishAport(m_port, portAttempts());
    if(ui_port == 0) {
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

bool Uws_Server::isConnected() const {
    return !m_broadcaster->empty() && m_uiready;
}

bool Uws_Server::beginBatch() {
    m_batch = std::make_unique<Batch>();
    return true;
}

bool Uws_Server::endBatch() {
    if(m_batch) {
        const auto targets = {Server::TargetSocket::Ui, Server::TargetSocket::Extension};
        for(const auto target : targets) {
            auto str = m_batch->dump(target);
#ifdef PULL_MODE        
        if(str.size() < WS_MAX_LEN) {
#endif            
            if(!m_broadcaster->send(Server::TargetSocket::Ui, std::move(str)))
                return false;
#ifdef PULL_MODE                
        } else {
            const auto pull = addPulled(DataType::Json, str);
            const json obj = {{"type", "pull_json"}, {"id", pull}};
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "add batch pull", str.size(), pull);
            if(!m_broadcaster->send(target, obj.dump()))
                return false;
        }
#endif      
        }  
        m_batch.reset();
    }
    return true;
}

bool Uws_Server::send(Server::TargetSocket target, Server::Value&& value) {

    if(m_batch) {
        m_batch->push_back(target, std::move(value));
    } else {
            auto str = value.dump();
#ifdef PULL_MODE        
        if(str.size() < WS_MAX_LEN) {
#endif            
            if(!m_broadcaster->send(target, std::move(str)))
                return false;
#ifdef PULL_MODE               
    This is not working - but keep here as a reference if pull mode want to be re-enabled 
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

bool Uws_Server::send(Gempyre::DataPtr&& ptr) {
#ifdef PULL_MODE    
    if(len < WS_MAX_LEN) {
#endif        
        if(!m_broadcaster->send(std::move(ptr)))
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

void Uws_Server::doClose() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Do Close", static_cast<bool>(m_closeData));
    m_doExit = true;
    closeListenSocket();
}

void Uws_Server::close(bool wait) {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server - going");
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
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Server - gone");
}

 bool Uws_Server::isJoinable() const {return m_serverThread && m_serverThread->joinable();}
 bool Uws_Server::isRunning() const {return isJoinable() && m_isRunning;}

