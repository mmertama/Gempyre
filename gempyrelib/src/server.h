#ifndef SERVER_H
#define SERVER_H


#include <thread>
#include <memory>
#include <functional>
#include <any>
#include <optional>
#include <unordered_map>
#include <string_view>
#include <string>
#include <atomic>
#include <cassert>

namespace Gempyre {

class Batch;
class Data;

enum class CloseStatus {EXIT, FAIL, CLOSE};

class Server {
public:
    static constexpr int PAGEXIT = 1001;
    using JSData = std::any;
    using Array = std::vector<JSData>;
    using Object = std::unordered_map<std::string, JSData>;
    using MessageFunction = std::function<void (const Object&)>;
    using CloseFunction =  std::function<void (CloseStatus, int)>;
    using OpenFunction =  std::function<void ()>;
    using GetFunction =  std::function<std::optional<std::string> (const std::string_view& filename)>;
    using ListenFunction =  std::function<bool (unsigned)>;
    Server(unsigned int port,
           const std::string& rootFolder,
           const OpenFunction& onOpen,
           const MessageFunction& onMessage,
           const CloseFunction& onClose,
           const GetFunction& onGet,
           const ListenFunction& onListen,
           int querIdBase);
    
    virtual ~Server() = default;       
   
    // joinable does not mean it is running, and not running does not mean it soon wont :-)
    
    virtual bool retryStart() = 0;
    virtual void close(bool wait = false) = 0;

    virtual bool send(const std::unordered_map<std::string, std::string>& object, const std::any& values = std::any()) = 0;
    virtual bool send(const Gempyre::Data& data) = 0;

    virtual bool isJoinable() const = 0;
    virtual bool isRunning() const = 0;
    virtual bool isConnected() const = 0;
    
    int queryId() const {return ++m_queryId;}
    unsigned int port() const {return m_port;}
    
    virtual bool beginBatch() = 0;
    virtual bool endBatch() = 0;

    static unsigned wishAport(unsigned port, unsigned max);
    static unsigned portAttempts();

protected:
    unsigned int m_port;
    std::string m_rootFolder;
    mutable int m_queryId;
    const OpenFunction m_onOpen;
    const MessageFunction m_onMessage;
    const CloseFunction m_onClose;
    const GetFunction m_onGet;
    const ListenFunction m_onListen;    
};

std::unique_ptr<Server> create_server(unsigned int port,
           const std::string& rootFolder,
           const Server::OpenFunction& onOpen,
           const Server::MessageFunction& onMessage,
           const Server::CloseFunction& onClose,
           const Server::GetFunction& onGet,
           const Server::ListenFunction& onListen,
           int querIdBase);

}

#endif  //SERVER_H
