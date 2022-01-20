
#include "ws.hpp"
#include <iostream>
#include <chrono>

#include <nlohmann/json.hpp>
using namespace websocket;
using Json = nlohmann::json;

std::string App::on_status(websocket::Status status) {
    switch(status) {
        case websocket::Status::Error:
            return Json({{"type", "error"}}).dump();
        case websocket::Status::Established:
            return Json({{"type", "uiready"}}).dump();
    }
    return std::string();
}

void websocket::debug_print(int lvl, const char* cstr) {
    std::cout << lvl << ":" << cstr << std::endl;
}

int App::received(const char* cstr)
{
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
    std::cout << "Received " << js << std::endl;
    return 0;
}
