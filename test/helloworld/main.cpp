#include "gempyre.h"
#include "gempyre_utils.h"
#include <iostream>

const unsigned short DefaultPort = 8080;


int main(int argc, char** argv) {
    Gempyre::setDebug();
    const auto p = GempyreUtils::parseArgs(argc, argv, {{"port", 'p', GempyreUtils::ArgType::REQ_ARG}});
    if(!std::get_if<int>(&p) || std::get<GempyreUtils::ParamList>(std::get<GempyreUtils::Params>(p)).size() < 1) {
        std::cerr << "[path to INDEX.HTML] <-p value>" << std::endl;
        return -1;
    }
    const auto indexPath = std::get<GempyreUtils::ParamList>(std::get<GempyreUtils::Params>(p))[0];
    Gempyre::Ui ui(indexPath,
                 GempyreUtils::to<unsigned short>(GempyreUtils::atOr(std::get<GempyreUtils::Options>(std::get<GempyreUtils::Params>(p)), "port", std::to_string(DefaultPort))),
                 GempyreUtils::pathPop(indexPath)); //root directory;
    Gempyre::Element text(ui, "content");
    Gempyre::Element button(ui, "startbutton");
    button.setHTML("Hello?");
    button.subscribe("click", [&ui, &text](const Gempyre::Event&) {
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

