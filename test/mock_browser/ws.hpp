#ifndef WS_HPP
#define WS_HPP

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>


namespace websocket {
    enum class Status{Error, Established};
    class App {
    public:
        int received(const char* cstr);
        std::string on_status(Status status);
        std::function<void ()> exit = nullptr;
        std::function<bool (const char* cstr)> send_message = nullptr;
    };
    int start_ws(const std::string& address, int port, const std::string& protocol, App& ext);
    void debug_print(int, const char* cstr);
}

#endif // WS_HPP
