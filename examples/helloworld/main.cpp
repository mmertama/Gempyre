#include "gempyre.h"
#include "gempyre_utils.h"
#include "gempyre_client.h"
#include <iostream>
#include <cassert>

#include "html_resource.h"

int main(int /*argc*/, char** /*argv*/) {
    Gempyre::setDebug();

    auto ui = std::make_unique<Gempyre::Ui>(Html_resourceh, "index.html");

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

    Gempyre::Element(*ui, "open_icon").subscribe("click", [&ui](const auto&)  {
        const auto icon_name = Gempyre::Dialog::openFileDialog("Open Image");
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
        const auto w = GempyreUtils::convert<int>(event.properties.at("value"));
        const auto r = ui->root().rect();
        assert(r);
        ui->resize(w, r->height);
    }, {"value"});


    Gempyre::Element(*ui, "height").subscribe("change", [&ui](const auto& event)  {
        const auto h = GempyreUtils::convert<int>(event.properties.at("value"));
        const auto r = ui->root().rect();
        assert(r);
        ui->resize(r->width, h);
    }, {"value"});


    ui->run();
    return 0;
}

