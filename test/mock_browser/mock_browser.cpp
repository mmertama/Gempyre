#include "ws.hpp"
#include <cassert>
#include <regex>

int main(int argc, char** argv) {
    assert(argc > 1); (void) argc;
    std::smatch m;
    const auto temp_strings_not_allowed = std::string(argv[1]);
    const auto ok = std::regex_search(temp_strings_not_allowed, m, std::regex(R"(http://(.*):(\d+))"));
    assert(ok); (void) ok;
    websocket::App app;
    websocket::start_ws(m[1].str(), std::stoi(m[2].str()), "gempyre", app);
    websocket::debug_print(8, "main-exit");
    return 0;
}
