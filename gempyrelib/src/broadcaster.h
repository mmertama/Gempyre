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
public:
    enum class Type {
        Undefined,
        Ui,
        Extension
    };
    bool send(const std::string_view& text, bool is_ext = false) {
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        for(auto& [s, type] : m_sockets) {
            if((type == Type::Ui && is_ext) || (type == Type::Extension && !is_ext))
                continue;
            const auto success = s->send(text, uWS::OpCode::TEXT);
            if(success != WSSocket::SendStatus::SUCCESS) {
                GempyreUtils::log(GempyreUtils::LogLevel::Warning, "socket t2 fail", s->getBufferedAmount());
                if(success == WSSocket::SendStatus::BACKPRESSURE && !m_backPressureMutex.try_lock_for(DELAY))
                    GempyreUtils::log(GempyreUtils::LogLevel::Warning, "Cannot lock backpressure mutex");
                return false;
            }
        }
        return !m_sockets.empty();
    }

    bool send(const Data& ptr) {
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        for(auto& [s, type] : m_sockets) {
            if(type == Type::Ui) { // extension is not expected to handle binary messages
                const auto& [data, len] = ptr.payload();
                const auto success = s->send(std::string_view(data, len), uWS::OpCode::BINARY);
                if(success != WSSocket::SendStatus::SUCCESS) {
                    GempyreUtils::log(GempyreUtils::LogLevel::Warning, "socket b2 fail", s->getBufferedAmount());
                    if(success == WSSocket::SendStatus::BACKPRESSURE && !m_backPressureMutex.try_lock_for(DELAY))
                           GempyreUtils::log(GempyreUtils::LogLevel::Warning, "Cannot lock backpressure mutex");
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
            ws->close();
            remove(ws);
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
