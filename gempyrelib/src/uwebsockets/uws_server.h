#ifndef UWS_SERVER_H
#define UWS_SERVER_H

#include "server.h"
#include "semaphore.h"


struct us_listen_socket_t;
using ListenSocket = std::atomic<us_listen_socket_t*>;

namespace Gempyre {

class Broadcaster;
class SocketHandler;  



class Uws_Server : public Server {
public:
    Uws_Server(unsigned int port,
           const std::string& rootFolder,
           const Server::OpenFunction& onOpen,
           const Server::MessageFunction& onMessage,
           const Server::CloseFunction& onClose,
           const Server::GetFunction& onGet,
           const Server::ListenFunction& onListen,
           int queryIdBase);
     
     ~Uws_Server();
private: // let's not use Server API
    bool isJoinable() const override;
    bool isRunning() const override;
    // joinable does not mean it is running, and not running does not mean it soon wont :-)
    
    bool isConnected() const override;
    bool retryStart() override;
    void close(bool wait = false) override;
    bool send(const std::unordered_map<std::string, std::string>& object, const std::any& values = std::any()) override;
    bool send(const Gempyre::Data& data) override;
    bool beginBatch() override;
    bool endBatch() override;
private:
    std::unique_ptr<std::thread> makeServer(unsigned short port);
    void doClose();
    void closeListenSocket();
    enum class DataType{Json, Bin};
    int addPulled(DataType, const std::string_view& data);
    void serverThread(unsigned port);
    bool checkPort();
    std::unique_ptr<std::thread> newThread();
    void messageHandler(const std::string& message, int opCode);
private:
    
    std::unique_ptr<Broadcaster> m_broadcaster;
    std::unique_ptr<Broadcaster> m_extensions;
   
    ListenSocket m_closeData = nullptr; //arbitrary
    bool m_uiready = false;

    std::unique_ptr<Batch> m_batch;
    std::unordered_map<std::string, std::pair<DataType, std::string>> m_pulled;
    int m_pulledId = 0;
    std::atomic_bool m_doExit = false;
    std::atomic_bool m_isRunning = false;
    Semaphore   m_waitStart; // must be before thread
    std::unique_ptr<std::thread> m_serverThread; // must be last
    friend class SocketHandler;
    };

}

#endif // UWS_SERVER_H
