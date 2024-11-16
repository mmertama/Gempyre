#ifndef BROADCASTER_H
#define BROADCASTER_H

#include "gempyre.h"
#include "gempyre_utils.h"
#include "data.h"
#include "server.h"



#include <unordered_map>
#include <cassert>

using namespace std::chrono_literals;

namespace Gempyre {

struct ExtraSocketData {};

template<typename WSSocket, typename Loop, typename WSServer>
class Broadcaster : public BroadcasterBase {
    static constexpr auto DELAY = 100ms;
    static constexpr auto BACKPRESSURE_DELAY = 100ms;
    static constexpr unsigned SEND_SUCCESS = 0xFFFFFFFF;
    enum class SType {Bin, Txt};

    Broadcaster(const Gempyre::Broadcaster<WSSocket, Loop, WSServer>&) = delete;
    Broadcaster& operator=(const Gempyre::Broadcaster<WSSocket, Loop, WSServer>&) = delete;
public:
    
    Broadcaster(const std::function<void(WSSocket*, typename WSSocket::SendStatus)>& resendRequest) : m_resendRequest{resendRequest} {}

    bool send_text(TargetSocket send_to, std::string&& text) override {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "send txt", text.size());
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        const auto sz = text.size();
        if(send_to == TargetSocket::All) {
            for(auto& [s, type] : m_sockets) {
                auto copy_of_text = text;
                add_queue(s, std::move(copy_of_text));
                socket_send(s, sz);
            }
        } else {
            for(auto& [s, type] : m_sockets) {
                if(type != send_to)
                    continue;
                auto copy_of_text = text;
                add_queue(s, std::move(copy_of_text));
                socket_send(s, sz);
            }
        }
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "sent txt", !m_sockets.empty());
        return !m_sockets.empty();
    }

    bool send_bin(DataPtr&& ptr, bool droppable) override {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "send bin", ptr->size());
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        for(auto& [s, type] : m_sockets) {
            if(type == TargetSocket::Ui) { // extension is not expected to handle binary messages
                const auto sz = ptr->size();
                add_queue(s, std::move(ptr), droppable);
                socket_send(s, sz);
            }
        }
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "sent bin", !m_sockets.empty());
        return !m_sockets.empty();
    }

    void append(WSSocket* socket) {
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        m_sockets.emplace(socket, TargetSocket::Undefined);
    }

    void remove(WSSocket* socket) {
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        auto it = m_sockets.find(socket);
        if(it != m_sockets.end()) {
            m_sockets.erase(it);
        }
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "socket erased", m_sockets.size());
    }

    void close() {
        int attempts = 20;
        while (!empty() && --attempts > 0) {
            std::this_thread::sleep_for(100ms);
        }
        forceClose();
    }

    bool empty() const {
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        return m_sockets.empty();
    }

    bool size() const {
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        return m_sockets.size();
    }

    void setType(WSSocket* ws, TargetSocket type) {
        const std::lock_guard<std::mutex> lock(m_socketMutex);
        assert(m_sockets[ws] == TargetSocket::Undefined);
        m_sockets[ws] = type;
    }

    void drain(WSSocket* ws) {
        send_all(ws);
    }

    void set_loop(Loop* loop) {
        m_loop = loop;
    }

// check if there is data in queues and request their send
    void flush() override {
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
            socket_send(nullptr, 0);
        }
    }

private:
    // see socket_send
    void add_queue(WSSocket* s, std::string&& text) {
        std::unique_lock<std::mutex> lock(m_sendTxtMutex);
        m_textQueue.push_back(std::make_tuple(s, std::move(text)));
    }

    // see socket_send
    void add_queue(WSSocket* s, DataPtr&& ptr, bool droppable) {
        std::unique_lock<std::mutex> lock(m_sendBinMutex);
        m_dataQueue.push_back(std::make_tuple(s, std::move(ptr), droppable));
    }

    bool forceReduceData() {
        std::unique_lock<std::mutex> lock(m_sendBinMutex);
        return forceReduceData_unsafe();
    }

     bool forceReduceData_unsafe() {
        const auto sz = m_dataQueue.size();
        for(auto it = m_dataQueue.begin(); it != m_dataQueue.end();) {
            auto& [s, ptr, droppable] = *it;
            if(droppable) {
                m_dataQueue.erase(it);
            } else {
                ++it;
            }   
        }
        return sz != m_dataQueue.size();
    }

    void removeDuplicates_unsafe() {
        m_textQueue.erase(std::unique(m_textQueue.begin(), m_textQueue.end()), m_textQueue.end());

    }

    void removeDuplicates() {
        std::unique_lock<std::mutex> lock(m_sendBinMutex);   
        removeDuplicates_unsafe();
    }

    void send_text(WSSocket* target_socket) {
        std::unique_lock<std::mutex> lock(m_sendTxtMutex);
        for(auto it = m_textQueue.begin(); it != m_textQueue.end();) {
            auto& [s, txt] = *it;
            if(target_socket && target_socket != s)
                continue;
            if(WSServer::has_backpressure(s, txt.size())) {
                // remove all extra and wait for drain
                if(!forceReduceData())
                    removeDuplicates_unsafe();
                return;
            }
            const auto status = WSServer::send_text(s, txt);
            if(status == WSSocket::SendStatus::SUCCESS) {
                m_textQueue.erase(it);
            } else {
                if(status == WSSocket::SendStatus::BACKPRESSURE) {
                    // This should not happen, but who knows uws
                     if(!forceReduceData())
                        removeDuplicates_unsafe();
                }
                if(status != WSSocket::SendStatus::BACKPRESSURE)
                    m_resendRequest(s, status);
                return;
            }
        }
    }

    void send_bin(WSSocket* target_socket) {
        std::unique_lock<std::mutex> lock(m_sendBinMutex);
        for(auto it = m_dataQueue.begin(); it != m_dataQueue.end();) {
            auto& [s, ptr, droppable] = *it;
            if(target_socket && target_socket != s)
                continue;
            const auto& [data, len] = ptr->payload();
            if(WSServer::has_backpressure(s, len)) {
                if(droppable)
                    m_dataQueue.erase(it);
                return;
            }

            const auto status = WSServer::send_bin(s, std::string_view{data, len});
                    
            if(status == WSSocket::SendStatus::SUCCESS || !droppable) {
                m_dataQueue.erase(it);
            } else {
                if(status != WSSocket::SendStatus::BACKPRESSURE) {
                    m_resendRequest(s, status); // on drops we keep non-droppables and request resend
                    return;
                } else {
                    ++it; // on backpressure  we keep non-droppables
                }
            }
        }
    }        

    void send_all(WSSocket* target_socket) {
        send_text(target_socket);
        send_bin(target_socket);
    }

    // uws requires send happen in its thread, therefore we queue them and then send them using m_loop->defer
    void socket_send(WSSocket* ws, size_t sz) {
         if(ws && sz > 0 && WSServer::has_backpressure(ws, sz)) {
            std::this_thread::sleep_for(BACKPRESSURE_DELAY);
         }
         m_loop->defer([this] () { // this happens in server thread 
            send_all(nullptr);
         });
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

private:
    std::function<void (WSSocket*, typename WSSocket::SendStatus)> m_resendRequest;
    std::unordered_map<WSSocket*, TargetSocket> m_sockets{};
    std::mutex m_sendTxtMutex{};
    std::mutex m_sendBinMutex{};
    std::vector<std::tuple<WSSocket*, std::string>> m_textQueue{};
    std::vector<std::tuple<WSSocket*, DataPtr, bool>> m_dataQueue{};
    mutable std::mutex m_socketMutex{};
    Loop* m_loop{nullptr};
    };
}


#endif // BROADCASTER_H
