#ifndef BROADCASTER_H
#define BROADCASTER_H

#include "gempyre.h"
#include "gempyre_utils.h"
#include "data.h"

#define UWS_NO_ZLIB
#include <App.h>

#include <unordered_map>
#include <cassert>

namespace Gempyre {

struct ExtraSocketData {};
using WSSocket = uWS::WebSocket<false, true, ExtraSocketData>;

class Broadcaster {
    static constexpr auto DELAY = 100ms;
    enum class SType {Bin, Txt};
    template<SType T, class S>
    void socket_error(Gempyre::WSSocket::SendStatus status, S& s) {
        constexpr auto type = T == SType::Bin ? "Bin" : "Txt";
        if(status == WSSocket::SendStatus::BACKPRESSURE) {
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
    bool check_backpressure(const L len, S& s) {
        auto s_ptr = &s; 
        const auto webSocketContextData = static_cast<uWS::WebSocketContextData<false, ExtraSocketData>*>
        (us_socket_context_ext(false, static_cast<us_socket_context_t *> (us_socket_context(false, (us_socket_t *) s_ptr))));
        const auto free_space =  webSocketContextData->maxBackpressure -s.getBufferedAmount(); 
        if(len > webSocketContextData->maxBackpressure) {
            GempyreUtils::log(GempyreUtils::LogLevel::Fatal,
            "Too much data: max:", webSocketContextData->maxBackpressure,
            "Data size:", len,
            "Free:", free_space);
        }
        if(len > free_space) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "buf full", free_space, len);
            std::this_thread::sleep_for(100ms);
            return true;
            }
        return false;    
    }

public:
    enum class Type {
        Undefined,
        Ui,
        Extension
    };
    bool send(const std::string_view& text, bool is_ext = false) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "send txt", text.size());
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        for(auto& [s, type] : m_sockets) {
            if((type == Type::Ui && is_ext) || (type == Type::Extension && !is_ext))
                continue;
            if(check_backpressure(text.size(), *s))
                return false;
            const auto success = s->send(text, uWS::OpCode::TEXT);
            if(success != WSSocket::SendStatus::SUCCESS) {
                socket_error<SType::Txt>(success, *s);
                return false;
            }
        }
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "sent txt", !m_sockets.empty());
        return !m_sockets.empty();
    }

    bool send(const Data& ptr) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "send bin", ptr.size());
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        for(auto& [s, type] : m_sockets) {
            if(type == Type::Ui) { // extension is not expected to handle binary messages
                const auto& [data, len] = ptr.payload();
                if(check_backpressure(len, *s))
                    return false;
                const auto success = s->send(std::string_view(data, len), uWS::OpCode::BINARY);
                if(success != WSSocket::SendStatus::SUCCESS) {
                    socket_error<SType::Bin>(success, *s);
                    return false;
                }
            }
        }
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "sent bin", !m_sockets.empty());
        return !m_sockets.empty();
    }

    void append(WSSocket* socket) {
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        m_sockets.emplace(socket, Type::Undefined);
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

    void setType(WSSocket* ws, Type type) {
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        assert(m_sockets[ws] == Type::Undefined);
        m_sockets[ws] = type;
    }

    void unlock() {
        m_backPressureMutex.unlock();
    }
private:
    std::unordered_map<WSSocket*, Type> m_sockets;
    std::timed_mutex m_backPressureMutex;
    mutable std::mutex m_socketMutex;
    };
}


#endif // BROADCASTER_H
