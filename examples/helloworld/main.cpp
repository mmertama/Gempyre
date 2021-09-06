#include "gempyre.h"
#include "gempyre_utils.h"
#include <iostream>
#include <cassert>

const unsigned short DefaultPort = 8080;
#include "html_resource.h"

int main(int argc, char** argv) {
    Gempyre::setDebug();
    const auto p = GempyreUtils::parseArgs(argc, argv, {{"port", 'p', GempyreUtils::ArgType::REQ_ARG}});

    std::function<std::unique_ptr<Gempyre::Ui> ()> makeUi = nullptr;

    if(std::get<GempyreUtils::ParamList>(p).size() < 1) {
        makeUi = [&]() {
            return std::make_unique<Gempyre::Ui>(Html_resourceh, "index.html", argc, argv);
        };

    } else {
        makeUi = [&]() {
            const auto indexPath = std::get<GempyreUtils::ParamList>(p)[0];
            Gempyre::Ui::Filemap map;
            const auto url = Gempyre::Ui::addFile(map, indexPath);
            gempyre_utils_assert_x(url, "Not Found:" + indexPath);
            return std::make_unique<Gempyre::Ui>(map, *url, argc, argv, "",
                         GempyreUtils::to<unsigned short>(GempyreUtils::atOr(std::get<GempyreUtils::Options>(p), "port", std::to_string(DefaultPort))),
                         GempyreUtils::pathPop(indexPath)); //root directory;
        };
    }

    auto ui = makeUi();

    Gempyre::Element text(*ui, "content");
    Gempyre::Element button(*ui, "startbutton");
    button.setHTML("Hello?");
    button.subscribe("click", [&ui, &text](auto) {
        text.setHTML("Hello World!");
        const auto p = ui->ping();
        if(p.has_value()) {
            auto [ping, halfping] = p.value();
            text.setHTML("Ping roundtrip:" + std::to_string(static_cast<double>(ping.count()) / 1000.)
                    + " to UI:" + std::to_string(static_cast<double>(halfping.count()) / 1000.));
       }
    });

    Gempyre::Element(*ui, "title").subscribe("change", [&ui](const auto& event)  {
        const auto title_name = event.properties.at("value");
        ui->setTitle(title_name);
    }, {"value"});

            ui->onOpen([&ui] {
               const auto r = ui->root().rect();
               if(r) {
                   Gempyre::Element(*ui, "width").setAttribute("value", std::to_string(r->width));
                   Gempyre::Element(*ui, "height").setAttribute("value", std::to_string(r->height));
               }
            });

    Gempyre::Element(*ui, "width").subscribe("change", [&ui](const auto& event)  {
        const auto w = GempyreUtils::to<int>(event.properties.at("value"));
        const auto r = ui->root().rect();
        assert(r);
        ui->resize(w, r->height);
    }, {"value"});


    Gempyre::Element(*ui, "height").subscribe("change", [&ui](const auto& event)  {
        const auto h = GempyreUtils::to<int>(event.properties.at("value"));
        const auto r = ui->root().rect();
        assert(r);
        ui->resize(r->width, h);
    }, {"value"});


    ui->run();
    return 0;
}

