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
class LWS_Server;

using SKey = const void*;

class LWS_Socket {
public:
    void close();
    enum class SendStatus {
        DUMMY_FAIL,
        SUCCESS,
        BACKPRESSURE
    };

    static constexpr lws_write_protocol BIN = LWS_WRITE_BINARY;
    static constexpr lws_write_protocol TEXT = LWS_WRITE_TEXT;


    LWS_Socket(lws* wsocket) : m_ws{wsocket} {}

    LWS_Socket::SendStatus append(std::string_view data, lws_write_protocol type) {
        if (is_full())
            return SendStatus::BACKPRESSURE;
        std::vector<unsigned char> bytes;
        bytes.reserve(LWS_PRE + data.size());
        bytes.insert(bytes.begin() + LWS_PRE, data.begin(), data.end());
        m_buffer.emplace_back(std::make_pair(
            type,
            std::move(bytes)
        ));
        m_buffer_size += data.size();
        return SendStatus::SUCCESS;
    }

    bool empty() const {
        return m_buffer.empty();
    }

    std::tuple<lws_write_protocol, const unsigned char*, size_t> front() const {
        const auto front = m_buffer.front();
        return {front.first, front.second.data(), front.second.size() - LWS_PRE};
    }

    bool is_full() const {
        return m_buffer.size() > 20U; // I dunno what would be a valid number
    }

    size_t size() const {
        return m_buffer_size;
    }

    void shift() {
        const auto it = m_buffer.begin();
        m_buffer_size -= it->second.size();
        m_buffer.erase(it);
    }

private:
    lws* m_ws;
    std::vector<std::pair<lws_write_protocol, std::vector<unsigned char>>> m_buffer;
    size_t m_buffer_size{0};
};

class LWS_Loop {
public:
    void defer(std::function<void ()>&& f);
    bool valid() const {return m_fut.joinable();}
    LWS_Loop& operator=(std::thread&& fut) {
        m_fut = std::move(fut);
        return *this;
    }
    void execute();
private:
    std::thread m_fut;
    std::mutex m_mutex;
    std::vector<std::function<void()>> m_deferred;   
};

using LWS_Broadcaster = Broadcaster<LWS_Socket, LWS_Loop, LWS_Server>;

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

    bool isJoinable() const override;
    bool isRunning() const override;
    // joinable does not mean it is running, and not running does not mean it soon wont :-)
    
    bool isConnected() const override;
    bool isUiReady() const override;
    bool retryStart() override;
    void close(bool wait = false) override;
    BroadcasterBase& broadcaster() override;

    static bool has_backpressure(LWS_Socket* s, size_t len);
    static LWS_Socket::SendStatus send_text(LWS_Socket* s, std::string_view text);
    static LWS_Socket::SendStatus send_bin(LWS_Socket* s, std::string_view bin); 

private:
    static int wsCallback(lws* wsi, lws_callback_reasons reason, void *user, void *in, size_t len);
    static int httpCallback(lws* wsi, lws_callback_reasons reason, void *user, void *in, size_t len);
    std::string parseQuery(std::string_view query) const;
    std::optional<std::string_view> match(const std::string_view prefix, const std::string_view param) const;
    bool get_http(std::string_view get_param) const;
    void appendSocket(lws* wsi);
    bool removeSocket(lws* wsi, unsigned error_code);
    bool received(lws* wsi, std::string_view msg);
    size_t on_write(lws* wsi);
    int on_http(lws *wsi, void* in);
    int on_http_write(lws *wsi);
private:
    std::atomic_bool m_running{false};
    std::atomic_bool m_do_close{false};
    std::atomic_bool m_uiready{false};
    LWS_Loop m_loop;
    std::unique_ptr<SendBuffer> m_send_buffer;
    std::unique_ptr<LWS_Broadcaster> m_broadcaster;
    std::unordered_map<SKey, std::unique_ptr<LWS_Socket>> m_sockets;
};
} // ns Gempyre
#endif // LWS_SERVER_H