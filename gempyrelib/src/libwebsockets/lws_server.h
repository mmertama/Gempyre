#ifndef LWS_SERVER_H
#define LWS_SERVER_H

#include <future>
#include <atomic>

struct lws_context;

namespace Gempyre {

class LWS_Server : public Server {
public:
    LWS_Server(unsigned int port,
           const std::string& rootFolder,
           const Server::OpenFunction& onOpen,
           const Server::MessageFunction& onMessage,
           const Server::CloseFunction& onClose,
           const Server::GetFunction& onGet,
           const Server::ListenFunction& onListen,
           int queryIdBase);
     ~LWS_Server();
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
    std::atomic<bool> m_running;
    std::future<void> m_loop;
    lws_context m_context;
};
} // ns Gempyre
#endif // LWS_SERVER_H