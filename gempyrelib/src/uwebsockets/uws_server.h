#ifndef UWS_SERVER_H
#define UWS_SERVER_H

#include "server.h"
#include "semaphore.h"
#include "broadcaster.h"

#ifndef UWS_NO_ZLIB
    #define UWS_NO_ZLIB
#endif
#include <App.h>

struct us_listen_socket_t;
using ListenSocket = std::atomic<us_listen_socket_t*>;

namespace Gempyre {

class SocketHandler;

using WSSocket = uWS::WebSocket<false, true, ExtraSocketData>;

class Uws_Server;

using Uws_Broadcaster = Broadcaster<WSSocket, uWS::Loop, Uws_Server>;

class Uws_Server : public Server {
public:
    Uws_Server(unsigned int port,
           const std::string& rootFolder,
           Server::OpenFunction&& onOpen,
           Server::MessageFunction&& onMessage,
           Server::CloseFunction&& onClose,
           Server::GetFunction&& onGet,
           Server::ListenFunction&& onListen,
           int queryIdBase,
           Server::ResendRequest&& request);
     
    ~Uws_Server();

    static bool has_backpressure(WSSocket* s, size_t len);
    static WSSocket::SendStatus send_text(WSSocket* s, std::string_view text);
    static WSSocket::SendStatus send_bin(WSSocket* s, std::string_view bin); 

private: // let's not use Server API
    bool isJoinable() const override;
    bool isRunning() const override;
    // joinable does not mean it is running, and not running does not mean it soon wont :-)
    
    bool isConnected() const override;
    bool isUiReady() const override;
    bool retryStart() override;
    void close(bool wait = false) override;
    BroadcasterBase& broadcaster() override;
private:
    std::unique_ptr<std::thread> makeServer(unsigned short port);
    void doClose();
    void closeListenSocket();
  
    void serverThread(unsigned port);
    bool checkPort();
    std::unique_ptr<std::thread> newThread();
private:
    
    std::unique_ptr<Uws_Broadcaster> m_broadcaster;
    std::unique_ptr<Uws_Broadcaster> m_extensions{};
   
    ListenSocket m_closeData = nullptr; //arbitrary
    std::atomic_bool m_uiready{false};
  
    std::atomic_bool m_doExit{false};
    std::atomic_bool m_isRunning{false};
    Semaphore m_waitStart{}; // must be before thread
    std::unique_ptr<std::thread> m_serverThread; // must be last
    friend class SocketHandler;
    };

}

#endif // UWS_SERVER_H
