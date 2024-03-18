
#include "ws.hpp"
#include <iostream>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

#include <nlohmann/json.hpp>
using namespace websocket;
using Json = nlohmann::json;

bool App::on_status(websocket::Status status) {
    switch(status) {
        case websocket::Status::Error: {
            const auto err = Json({{"type", "error"}}).dump();
            send_message(err.c_str());
            return true;
        }
        case websocket::Status::Established: {
            const auto ready = Json({{"type", "ui_ready"}}).dump();
            send_message(ready.c_str());
            std::this_thread::sleep_for(10ms);
            const auto event = Json({{"type", "event"}, {"element", ""}, {"event", "ui_ready"}, {"properties", Json::object()}}).dump();
            send_message(event.c_str());
            return true;
        }
             // one guess is that in API tests there no events coming
        default:
            return false;
    }
}

void websocket::debug_print(int lvl, const char* cstr) {
    if(lvl <= WS_LOG_LEVEL)
        std::cout << lvl << ":" << cstr << std::endl;
}

int App::received(const char* cstr) {
    const auto js = Json::parse(cstr);
    if(js["type"] == "exit_request") {
        exit();
    }
    if(js["type"] == "close_request") {
        exit();
    }
    if(js["type"] == "query") {
        if(js["query"] == "ping") {
            const auto epoch = std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::system_clock::now().time_since_epoch()).count();
            const auto msg = Json({{"type", "query"},
                                         {"query_id", js["query_id"]},
                                         {"query_value", "pong"},
                                         {"pong", epoch}
                                      }).dump();
            send_message(msg.c_str());
        }
    }
   if((1 << 3) <= WS_LOG_LEVEL)
        std::cout << (1 << 3) << ": app-received: " << cstr << std::endl;
    return 0;
}
