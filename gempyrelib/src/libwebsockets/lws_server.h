#ifndef LWS_SERVER_H
#define LWS_SERVER_H

#include "server.h"
#include "semaphore.h"
#include "broadcaster.h"

#include <chrono>
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

using SKey = const lws*;

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
        bytes.resize(LWS_PRE + data.size());
        std::copy(data.begin(), data.end(), bytes.begin() + LWS_PRE);
        m_buffer.emplace_back(std::make_pair(
            type,
            std::move(bytes)
        ));
        m_buffer_size += data.size();

        return 0 != lws_callback_on_writable(m_ws) ?
            SendStatus::SUCCESS : SendStatus::BACKPRESSURE;
    }

    bool empty() const {
        return m_buffer.empty();
    }

    std::tuple<lws_write_protocol, const unsigned char*, size_t> front() const {
        const auto& front = m_buffer.front();
        const auto* ptr = front.second.data() + LWS_PRE;
        const auto sz = front.second.size() - LWS_PRE;
        return {front.first, ptr, sz};
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
    void execute();
    bool valid() const;
    void join();
    LWS_Loop& operator=(std::thread&& fut);
    void set_context(lws_context* context);
    void wakeup();
private:
    std::thread m_fut;
    std::mutex m_mutex;
    std::vector<std::function<void()>> m_deferred;
    std::atomic <lws_context*> m_context;   
};

using LWS_Broadcaster = Broadcaster<LWS_Socket, LWS_Loop, LWS_Server>;

class LWS_Server : public Server {
public:
    LWS_Server(unsigned int port,
           const std::string& rootFolder,
           Server::OpenFunction&& onOpen,
           Server::MessageFunction&& onMessage,
           Server::CloseFunction&& onClose,
           Server::GetFunction&& onGet,
           Server::ListenFunction&& onListen,
           int queryIdBase,
           Server::ResendRequest&& resendRequest);
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
    static int ws_callback(lws* wsi, lws_callback_reasons reason, void *user, void *in, size_t len);
    static int http_callback(lws* wsi, lws_callback_reasons reason, void *user, void *in, size_t len);
    std::string parse_query(std::string_view query) const;
    std::optional<std::string_view> match(std::string_view prefix, std::string_view param) const;
    bool get_http(lws* wsi, std::string_view get_param);
    void append_socket(lws* wsi);
    bool remove_socket(lws* wsi, unsigned error_code);
    bool received(lws* wsi, std::string_view msg);
    size_t on_write(lws* wsi);
    int on_http(lws *wsi, void* in);
    int on_http_write(lws *wsi);
    bool write_http_header(lws* wsi, std::string_view mime_type, size_t size);
private:
    std::atomic_bool m_running{false};
    std::atomic_bool m_do_close{false};
    std::atomic_bool m_uiready{false};
    LWS_Loop m_loop;
    std::unique_ptr<LWS_Broadcaster> m_broadcaster;
    std::unordered_map<SKey, std::unique_ptr<LWS_Socket>> m_sockets;
    std::unordered_map<SKey, std::unique_ptr<SendBuffer>> m_send_buffers;
};
} // ns Gempyre
#endif // LWS_SERVER_H