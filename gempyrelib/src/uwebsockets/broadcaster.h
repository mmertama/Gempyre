#ifndef BROADCASTER_H
#define BROADCASTER_H

#include "gempyre.h"
#include "gempyre_utils.h"
#include "data.h"

#ifndef UWS_NO_ZLIB
#define UWS_NO_ZLIB
#endif

#include <App.h>

#include <unordered_map>
#include <cassert>

using namespace std::chrono_literals;;

namespace Gempyre {

struct ExtraSocketData {};
using WSSocket = uWS::WebSocket<false, true, ExtraSocketData>;

class Broadcaster {
    static constexpr auto DELAY = 100ms;
    static constexpr auto BACKPRESSURE_DELAY = 100ms;
    static constexpr unsigned SEND_SUCCESS = 0xFFFFFFFF;
    enum class SType {Bin, Txt};
    template<SType T, class S>
    void socket_error(Gempyre::WSSocket::SendStatus status, S& s) {
        constexpr auto type = T == SType::Bin ? "Bin" : "Txt";
        if(status == WSSocket::SendStatus::BACKPRESSURE) {
            std::this_thread::sleep_for(BACKPRESSURE_DELAY);
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "socket", type, "backpressure", s.getBufferedAmount());
            if(!m_backPressureMutex.try_lock_for(DELAY)) {
                GempyreUtils::log(GempyreUtils::LogLevel::Warning, "Cannot lock backpressure mutex");
            }
        } else {
            GempyreUtils::log(GempyreUtils::LogLevel::Warning, "socket", type, "dropped", s.getBufferedAmount());
            std::this_thread::sleep_for(300ms);
        }
    }
    template<typename L, class S>
    bool has_backpressure(const L len, S& s) {
        auto s_ptr = &s; 
        const auto webSocketContextData = static_cast<uWS::WebSocketContextData<false, ExtraSocketData>*>
        (us_socket_context_ext(false, static_cast<us_socket_context_t *> (us_socket_context(false, reinterpret_cast<us_socket_t *> (s_ptr)))));
        const auto free_space =  webSocketContextData->maxBackpressure -s.getBufferedAmount(); 
        if(len > webSocketContextData->maxBackpressure) {
            GempyreUtils::log(GempyreUtils::LogLevel::Fatal,
            "Too much data: max:", webSocketContextData->maxBackpressure,
            "Data size:", len,
            "Free:", free_space);
        }
        if(len > free_space) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "buf full", free_space, len);
            std::this_thread::sleep_for(BACKPRESSURE_DELAY);
            if(!m_backPressureMutex.try_lock_for(DELAY)) {
                GempyreUtils::log(GempyreUtils::LogLevel::Warning, "Cannot lock backpressure mutex");
            }
            return true;
            }
        return false;    
    }

public:
    
    Broadcaster(const std::function<void(WSSocket::SendStatus)>& onErr) : m_onErr{onErr} {}

    bool send(Server::TargetSocket send_to, std::string&& text) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "send txt", text.size());
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        for(auto& [s, type] : m_sockets) {
            if(send_to != Server::TargetSocket::All && type != send_to)
                continue;
            add_queue(s, std::move(text));
            socket_send();    
           /*if(check_backpressure(text.size(), *s))
                return false;
            const auto success = socket_send(s, text, true);
            if(success != WSSocket::SendStatus::SUCCESS) {
                socket_error<SType::Txt>(success, *s);
                return false;
            }*/
        }
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "sent txt", !m_sockets.empty());
        return !m_sockets.empty();
    }

    bool send(DataPtr&& ptr) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "send bin", ptr->size());
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        //if(ptr.index() > m_lastResend )
        //    return false; // re-queue to keep order
        for(auto& [s, type] : m_sockets) {
            if(type == Server::TargetSocket::Ui) { // extension is not expected to handle binary messages
                add_queue(s, std::move(ptr));
                socket_send();
                /*const auto& [data, len] = ptr.payload();
                if(check_backpressure(len, *s)) {
                    m_lastResend = ptr.index();
                    return false;
                }
                const auto success = socket_send(s, std::string_view(data, len), false);
                if(success != WSSocket::SendStatus::SUCCESS) {
                    socket_error<SType::Bin>(success, *s);
                    m_lastResend = ptr.index();
                    return false;
                }*/
            }
        }
        //m_lastResend = SEND_SUCCESS;
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "sent bin", !m_sockets.empty());
        return !m_sockets.empty();
    }

    void append(WSSocket* socket) {
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        m_sockets.emplace(socket, Server::TargetSocket::Undefined);
    }

    void remove(WSSocket* socket) {
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        auto it = m_sockets.find(socket);
        if(it != m_sockets.end()) {
            m_sockets.erase(it);
        }
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "socket erased", m_sockets.size());
    }

    void forceClose() {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Force Close", m_sockets.size());
        while(true) {
            auto it = m_sockets.begin();
            if(it == m_sockets.end())
                return;
            auto ws = it->first;
            remove(ws);
            ws->close();
        }
    }

    bool empty() const {
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        return m_sockets.empty();
    }

    bool size() const {
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        return m_sockets.size();
    }

    size_t bufferSize() const {
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        auto min = 0U;
        for(const auto& [s, type] : m_sockets) {
            min = std::min(min, s->getBufferedAmount());
        }
        return static_cast<size_t>(min);
    }

    void setType(WSSocket* ws, Server::TargetSocket type) {
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        assert(m_sockets[ws] == Server::TargetSocket::Undefined);
        m_sockets[ws] = type;
    }

    void unlock() {
        m_backPressureMutex.unlock();
    }

    void set_loop( uWS::Loop* loop) {
        m_loop = loop;
    }

// check if there is data in queues and request their send
    void flush() {
        bool has_data = false;
        if(!has_data) {
        std::unique_lock<std::mutex> lock(m_sendTxtMutex);
            has_data |=  !m_textQueue.empty();   
        }
        if(!has_data) {
        std::unique_lock<std::mutex> lock(m_sendTxtMutex);
            has_data |=  !m_textQueue.empty();   
        }
        if(has_data) {
            socket_send();
        }
    }

private:
    // see socket_send
    void add_queue(WSSocket* s, std::string&& text) {
        std::unique_lock<std::mutex> lock(m_sendTxtMutex);
        m_textQueue.push_back(std::make_tuple(s, std::move(text)));
    }

    // see socket_send
    void add_queue(WSSocket* s, DataPtr&& text) {
        std::unique_lock<std::mutex> lock(m_sendBinMutex);
        m_dataQueue.push_back(std::make_tuple(s, std::move(text)));
    }

    // uws requires send happen in its thread, therefore we queue them and then send them using m_loop->defer
    void socket_send() {
         m_loop->defer([this] () {{ // this happens in server thread 
            std::unique_lock<std::mutex> lock(m_sendTxtMutex);
            for(auto it = m_textQueue.begin(); it != m_textQueue.end();) {
                auto& [s, txt] = *it;
                if(has_backpressure(txt.size(), *s)) {
                    m_onErr(WSSocket::SendStatus::BACKPRESSURE);
                    return;
                }
                const WSSocket::SendStatus status = s->send(txt, uWS::OpCode::TEXT);
                if(status == WSSocket::SendStatus::SUCCESS) {
                    m_textQueue.erase(it);
                } else {
                    m_onErr(status);
                    return;
                }
            }}

            {
            std::unique_lock<std::mutex> lock(m_sendBinMutex);
            for(auto it = m_dataQueue.begin(); it != m_dataQueue.end();) {
                 auto& [s, ptr] = *it;
                 const auto& [data, len] = ptr->payload();
                 if(has_backpressure(len, *s)) {
                    m_onErr(WSSocket::SendStatus::BACKPRESSURE);
                    return;
                }
                const WSSocket::SendStatus status = s->send(std::string_view(data, len), uWS::OpCode::BINARY);
                if(status == WSSocket::SendStatus::SUCCESS) {
                    m_dataQueue.erase(it);
                } else {
                    m_onErr(status);
                    return;
                }
            }}
         });
    }
/*
    WSSocket::SendStatus socket_send(WSSocket* s, std::string_view data, bool is_text) {
        assert(m_loop);
        WSSocket::SendStatus status = WSSocket::SendStatus::DROPPED;
        std::unique_lock<std::mutex> lock(is_text ? m_sendTxtMutex : m_sendBinMutex);
        std::condition_variable cv;
        m_loop->defer([&] () {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "socket sending", is_text);
            status = s->send(data, is_text ? uWS::OpCode::TEXT : uWS::OpCode::BINARY);
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "socket sent", status);
            cv.notify_all();
        });
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "waiting socket send", is_text);
        if(std::cv_status::timeout == cv.wait_for(lock, 1s)) {
            GempyreUtils::log(GempyreUtils::LogLevel::Warning, "socket send expired", is_text);
        }
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "socket done", status);
        return status;
    }*/    
private:
    std::function<void (WSSocket::SendStatus)> m_onErr;
    std::unordered_map<WSSocket*, Server::TargetSocket> m_sockets;
    std::timed_mutex m_backPressureMutex;
    std::mutex m_sendTxtMutex;
    std::mutex m_sendBinMutex;
    std::vector<std::tuple<WSSocket*, std::string>> m_textQueue;
    std::vector<std::tuple<WSSocket*, DataPtr>> m_dataQueue;
    mutable std::mutex m_socketMutex;
    //unsigned m_lastResend = SEND_SUCCESS; 
    uWS::Loop* m_loop = nullptr;
    };
}


#endif // BROADCASTER_H
