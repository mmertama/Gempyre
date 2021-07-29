#include "gempyre.h"
#include "gempyre_utils.h"
#include "gempyre_client.h"
#include <iostream>
#include <cassert>

const unsigned short DefaultPort = 8080;
#include "html_resource.h"

int main(int argc, char** argv) {
    Gempyre::setDebug();
    const auto p = GempyreUtils::parseArgs(argc, argv, {{"port", 'p', GempyreUtils::ArgType::REQ_ARG}});
    if(std::get<GempyreUtils::ParamList>(p).size() < 1) {
        std::cerr << "[path to INDEX.HTML] <-p value>" << std::endl;
        return -1;
    }
    const auto indexPath = std::get<GempyreUtils::ParamList>(p)[0];
    Gempyre::Ui::Filemap map;
    const auto url = Gempyre::Ui::addFile(map, indexPath);
    gempyre_utils_assert_x(url, "Not Found:" + indexPath);
    Gempyre::Ui ui(map, *url,
                 GempyreUtils::to<unsigned short>(GempyreUtils::atOr(std::get<GempyreUtils::Options>(p), "port", std::to_string(DefaultPort))),
                 GempyreUtils::pathPop(indexPath)); //root directory;
    Gempyre::Element text(ui, "content");
    Gempyre::Element button(ui, "startbutton");
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

    Gempyre::Element(*ui, "open_icon").subscribe("click", [&ui](const auto&)  {
        const auto icon_name = Gempyre::Dialog::openFileDialog(*ui, "Open Image");
        if(!icon_name)
            return;
        Gempyre::Element(*ui, "icon_label").setHTML(*icon_name);
        const auto icon_data = GempyreUtils::slurp<uint8_t>(*icon_name);
        if(icon_data.empty())
            return;
        const auto& [_, ext] = GempyreUtils::splitName(*icon_name);
        ui->setApplicationIcon(icon_data.data(), icon_data.size(), ext);
    });

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

