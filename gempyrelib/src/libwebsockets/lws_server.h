#ifndef LWS_SERVER_H
#define LWS_SERVER_H

#include "server.h"
#include "semaphore.h"
#include "broadcaster.h"

#include <future>
#include <atomic>
#include <optional>
#include <string_view>

//extern "C" {
#include <libwebsockets.h>
//}

namespace Gempyre {

class SendBuffer;


struct LWS_Socket {
public:
    void close();
    enum class SendStatus {
    };
    LWS_Socket(lws* wsocket) : ws{wsocket} {}
    lws* ws;
};

using LWS_Loop = std::future<void>;

using LWS_Broadcaster = Broadcaster<LWS_Socket, LWS_Loop, LWS_Socket>;

class LWS_Server : public Server {
public:
    LWS_Server(unsigned int port,
           const std::string& rootFolder,
           const Server::OpenFunction& onOpen,
           const Server::MessageFunction& onMessage,
           const Server::CloseFunction& onClose,
           const Server::GetFunction& onGet,
           const Server::ListenFunction& onListen,
           int queryIdBase,
           const Server::ResendRequest& resendRequest);
     ~LWS_Server();
private: // let's not use Server API
    bool isJoinable() const override;
    bool isRunning() const override;
    // joinable does not mean it is running, and not running does not mean it soon wont :-)
    
    bool isConnected() const override;
    bool isUiReady() const override;
    bool retryStart() override;
    void close(bool wait = false) override;
    bool send(Server::TargetSocket target, Server::Value&& value, bool batchable) override;
    bool send(Gempyre::DataPtr&& data, bool droppable) override;
    void flush() override;
private:
    static int wsCallback(lws* wsi, lws_callback_reasons reason, void *user, void *in, size_t len);
    static int httpCallback(lws* wsi, lws_callback_reasons reason, void *user, void *in, size_t len);
    std::string parseQuery(std::string_view query) const;
    std::optional<std::string_view> match(const std::string_view prefix, const std::string_view param) const;
    bool get_http(std::string_view get_param) const;
    void appendSocket(lws* wsi);
    bool removeSocket(lws* wsi, unsigned error_code);
    bool received(lws* wsi, std::string_view msg);
private:
    std::atomic_bool m_running{false};
    std::atomic_bool m_do_close{false};
    std::atomic_bool m_uiready{false};
    LWS_Loop m_loop;
    std::unique_ptr<SendBuffer> m_send_buffer;
    std::unique_ptr<LWS_Broadcaster> m_broadcaster;
    std::unordered_map<void*, std::unique_ptr<LWS_Socket>> m_sockets;
};
} // ns Gempyre
#endif // LWS_SERVER_H