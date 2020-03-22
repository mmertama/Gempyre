#include "telex.h"
#include "telex_utils.h"
#include <iostream>

const unsigned short DefaultPort = 8080;


int main(int argc, char** argv) {
    Telex::setDebug();
    const auto p = TelexUtils::parseArgs(argc, argv, {{"port", 'p', TelexUtils::ArgType::REQ_ARG}});
    if(!std::get_if<int>(&p) || std::get<TelexUtils::ParamList>(std::get<TelexUtils::Params>(p)).size() < 1) {
        std::cerr << "[path to INDEX.HTML] <-p value>" << std::endl;
        return -1;
    }
    const auto indexPath = std::get<TelexUtils::ParamList>(std::get<TelexUtils::Params>(p))[0];
    Telex::Ui ui(indexPath,
                 TelexUtils::to<unsigned short>(TelexUtils::atOr(std::get<TelexUtils::Options>(std::get<TelexUtils::Params>(p)), "port", std::to_string(DefaultPort))),
                 TelexUtils::pathPop(indexPath)); //root directory;
    Telex::Element text(ui, "content");
    Telex::Element button(ui, "startbutton");
    button.setHTML("Hello?");
    button.subscribe("click", [&ui, &text](const Telex::Event&) {
        text.setHTML("Hello World!");
        const auto p = ui.ping();
        if(p.has_value()) {
            auto [ping, halfping] = p.value();
            text.setHTML("Ping roundtrip:" + std::to_string(static_cast<double>(ping.count()) / 1000.)
                    + " to UI:" + std::to_string(static_cast<double>(halfping.count()) / 1000.));
       }

    });
    ui.run();
    return 0;
}

