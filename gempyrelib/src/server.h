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


namespace Gempyre {

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
    using GetFunction =  std::function<std::optional<std::string> (const std::string_view& filename)>;
    using ListenFunction =  std::function<bool (unsigned short)>;
    Server(unsigned short port,
           const std::string& rootFolder,
           const OpenFunction& onOpen,
           const MessageFunction& onMessage,
           const CloseFunction& onClose,
           const GetFunction& onGet,
           const ListenFunction& onListen);
    bool isRunning() const {return m_serverThread && m_serverThread->joinable();}
    bool isConnected() const;
    bool retryStart();
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
    std::unique_ptr<std::thread> makeServer(unsigned short port);
    void doClose();
    void closeSocket();
    enum class DataType{Json, Bin};
    int addPulled(DataType, const std::string_view& data);
    void serverThread(unsigned short port);
    bool checkPort();
private:
    unsigned short m_port;
    std::string m_rootFolder;
    std::unique_ptr<Broadcaster> m_broadcaster;
    std::unique_ptr<Broadcaster> m_extensions;
    const OpenFunction m_onOpen;
    const MessageFunction m_onMessage;
    const CloseFunction m_onClose;
    const GetFunction m_onGet;
    const ListenFunction m_onListen;
    std::unique_ptr<std::thread> m_serverThread;
    std::any m_closeData; //arbitrary
    bool m_uiready = false;
    mutable int m_queryId = 0;
    std::unique_ptr<Batch> m_batch;
    std::unordered_map<std::string, std::pair<DataType, std::string>> m_pulled;
    int m_pulledId = 0;
    bool m_doExit = false;
};
}

#endif // SERVER_H
