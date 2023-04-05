#ifndef WSPP_SERVER_H
#define WSPP_SERVER_H

namespace Gempyre {

class WSPP_Server : public Server {
public:
    WSPP_Server(unsigned int port,
           const std::string& rootFolder,
           const Server::OpenFunction& onOpen,
           const Server::MessageFunction& onMessage,
           const Server::CloseFunction& onClose,
           const Server::GetFunction& onGet,
           const Server::ListenFunction& onListen,
           int querIdBase);
     ~WSPP_Server();
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
};
} // ns Gempyre
#endif // WSPP_SERVER_H