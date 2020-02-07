#ifndef SERVER_H
#define SERVER_H


#include <thread>
#include <memory>
#include <functional>
#include <any>
#include <optional>
#include <unordered_map>

namespace Telex {

class Broadcaster;
class Batch;

class Server {
public:
    static constexpr int PAGEXIT = 1001;
    enum class Close {EXIT, FAIL, CLOSE};
    using Data = std::any;
    using Array = std::vector<Data>;
    using Object = std::unordered_map<std::string, Data>;
    using MessageFunction = std::function<void (const Object&)>;
    using CloseFunction =  std::function<void (Close, int)>;
    using OpenFunction =  std::function<void (int)>;
    using GetFunction =  std::function<std::optional<std::string> (const std::string& filename)>;
    using ListenFunction =  std::function<bool (unsigned short)>;
    Server(unsigned short port,
           const std::string& rootFolder,
           const std::string& serviceName,
           const OpenFunction& onOpen,
           const MessageFunction& onMessage,
           const CloseFunction& onClose,
           const GetFunction& onGet,
           const ListenFunction& onListen);
    bool isRunning() const {return m_server && m_server->joinable();}
    bool isConnected() const;
    bool retryStart();
    bool hasRoom(size_t sz) const;
    void close(bool wait = false);
   // void send(const std::unordered_map<std::string, std::string>& object);
    bool send(const std::unordered_map<std::string, std::string>& object, const std::any& values = std::any());
    bool send(const char* data, size_t len);
    int queryId() const {return ++m_queryId;}
    unsigned short port() const {return m_port;}
    std::function<std::string (const std::string&)> onFile(const std::function<std::string (const std::string&)>&f );
    ~Server();
    bool beginBatch();
    bool endBatch();
private:
    unsigned short getPort(unsigned short port);
    std::unique_ptr<std::thread> makeServer(unsigned short port,
                                                      const std::string& serviceName);
    void doClose();
    void closeSocket();
private:
    std::string m_rootFolder;
    std::unique_ptr<Broadcaster> m_broadcaster;
    std::unique_ptr<Broadcaster> m_extensions;
    const OpenFunction m_onOpen;
    const MessageFunction m_onMessage;
    const CloseFunction m_onClose;
    const GetFunction m_onGet;
    const ListenFunction m_onListen;
    std::function<std::unique_ptr<std::thread> ()> m_startFunction = nullptr;
    std::unique_ptr<std::thread> m_server;
    std::any m_closeData; //arbitrary
    unsigned short m_port = 0;
    bool m_uiready = false;
    mutable int m_queryId = 0;
    std::unique_ptr<Batch> m_batch;
    size_t m_payloadSize = 0;
};
}

#endif // SERVER_H
