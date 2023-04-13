#ifndef LWS_SERVER_H
#define LWS_SERVER_H

#include <future>
#include <atomic>
#include <optional>
#include <string_view>

extern "C" {
    #include <libwebsockets.h>
}

namespace Gempyre {

class SendBuffer;    

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
    bool retryStart() override;
    void close(bool wait = false) override;
    bool send(Server::TargetSocket target, Server::Value&& value) override;
    bool send(const Gempyre::Data& data) override;
    bool beginBatch() override;
    bool endBatch() override;
private:
    static int wsCallback(lws *wsi, lws_callback_reasons reason, void *user, void *in, size_t len);
    static int httpCallback(lws *wsi, lws_callback_reasons reason, void *user, void *in, size_t len);
    std::string parseQuery(const std::string_view query) const;
    bool get(const std::string_view get_param) const;
private:
    std::atomic<bool> m_running;
    std::atomic<bool> m_connected;
    std::atomic<bool> m_do_close;
    std::future<void> m_loop;
    std::unique_ptr<SendBuffer> m_send_buffer;
};
} // ns Gempyre
#endif // LWS_SERVER_H