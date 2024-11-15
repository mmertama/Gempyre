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
#include <nlohmann/json.hpp>

namespace Gempyre {


class Batch;
class Data;
using DataPtr = std::shared_ptr<Data>;

enum class CloseStatus {EXIT, FAIL, CLOSE};

using json = nlohmann::json;

class Server {
public:
    static constexpr int PAGEXIT = 1001;
    
    using Array = json::array_t;
    using Object = json::object_t;
    using Value = json;
    using MessageFunction = std::function<void (Object&&)>;
    using CloseFunction =  std::function<void (CloseStatus, int)>;
    using OpenFunction =  std::function<void ()>;
    using GetFunction =  std::function<std::optional<std::string> (const std::string_view& filename)>;
    using ListenFunction =  std::function<bool (unsigned)>;
    using ResendRequest = std::function<void()>;

    enum class TargetSocket{Undefined, Ui, Extension, All};

    Server(unsigned int port,
           const std::string& rootFolder,
           const OpenFunction& onOpen,
           const MessageFunction& onMessage,
           const CloseFunction& onClose,
           const GetFunction& onGet,
           const ListenFunction& onListen,
           int queryIdBase);
    
    virtual ~Server() = default;       
   
    // joinable does not mean it is running, and not running does not mean it soon wont :-)
    
    virtual bool retryStart() = 0;
    virtual void close(bool wait = false) = 0;

    virtual bool send(TargetSocket target, Server::Value&& value, bool batchable = true) = 0;
    virtual bool send(Gempyre::DataPtr&& data, bool droppable) = 0;

    virtual bool isJoinable() const = 0;
    virtual bool isRunning() const = 0;
    virtual bool isConnected() const = 0;
    virtual bool isUiReady() const = 0;
    
    int queryId() const {return ++m_queryId;}
    unsigned int port() const {return m_port;}
    
    virtual void flush() = 0;

    static unsigned wishAport(unsigned port, unsigned max);
    static unsigned portAttempts();

    bool beginBatch();
    bool endBatch();

    static std::string fileToMime(const std::string_view& filename);
    static std::string notFoundPage(const std::string_view& url, const std::string_view& info = "");

protected:
    enum class MessageReply {DoNothing, AddUiSocket, AddExtensionSocket};
    MessageReply messageHandler(std::string_view message);
#ifdef PULL_MODE      
    enum class DataType{Json, Bin};
    int addPulled(DataType, const std::string_view& data);
#endif

protected:
    unsigned int m_port;
    std::string m_rootFolder;
    mutable int m_queryId;
    const OpenFunction m_onOpen;
    const MessageFunction m_onMessage;
    const CloseFunction m_onClose;
    const GetFunction m_onGet;
    const ListenFunction m_onListen;
    std::unique_ptr<Batch> m_batch{};
#ifdef PULL_MODE   
    int m_pulledId{0};
    std::unordered_map<std::string, std::pair<DataType, std::string>> m_pulled{};
#endif    
};

class Batch {
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


std::unique_ptr<Server> create_server(unsigned int port,
           const std::string& rootFolder,
           const Server::OpenFunction& onOpen,
           const Server::MessageFunction& onMessage,
           const Server::CloseFunction& onClose,
           const Server::GetFunction& onGet,
           const Server::ListenFunction& onListen,
           int queryIdBase,
           const Server::ResendRequest& resendRequest);

}

#endif  //SERVER_H
