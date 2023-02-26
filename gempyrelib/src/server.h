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


#include "semaphore.h"
#include "data.h"

struct us_listen_socket_t;
using ListenSocket = std::atomic<us_listen_socket_t*>;

namespace Gempyre {

class Broadcaster;
class Batch;
class SocketHandler;

enum class CloseStatus {EXIT, FAIL, CLOSE};

class Server {
public:
    static constexpr int PAGEXIT = 1001;
    using Data = std::any;
    using Array = std::vector<Data>;
    using Object = std::unordered_map<std::string, Data>;
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
           const ListenFunction& onListen);
    bool isJoinable() const {return m_serverThread && m_serverThread->joinable();}
    // joinable does not mean it is running, and not running does not mean it soon wont :-)
    bool isRunning() const {return isJoinable() && m_isRunning;}
    bool isConnected() const;
    bool retryStart();
    void close(bool wait = false);
   // void send(const std::unordered_map<std::string, std::string>& object);
    bool send(const std::unordered_map<std::string, std::string>& object, const std::any& values = std::any());
    bool send(const Gempyre::Data& data);
    int queryId() const {return ++m_queryId;}
    unsigned int port() const {return m_port;}
    std::function<std::string (const std::string&)> onFile(const std::function<std::string (const std::string&)>&f );
    ~Server();
    bool beginBatch();
    bool endBatch();
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
    unsigned int m_port;
    std::string m_rootFolder;
    std::unique_ptr<Broadcaster> m_broadcaster;
    std::unique_ptr<Broadcaster> m_extensions;
    const OpenFunction m_onOpen;
    const MessageFunction m_onMessage;
    const CloseFunction m_onClose;
    const GetFunction m_onGet;
    const ListenFunction m_onListen;
    ListenSocket m_closeData = nullptr; //arbitrary
    bool m_uiready = false;
    mutable int m_queryId = 0;
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

#endif // SERVER_H
