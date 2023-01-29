#include <gempyre.h>
#include <gempyre_utils.h>
#include "tick_resource.h"

using namespace std::chrono_literals;

int main(int /*argc*/, char** /*argv*/) {
    Gempyre::set_debug();
    Gempyre::Ui ui({{"/index.html", Tickhtml}}, "index.html");
    Gempyre::Element counter0(ui, "counter0");
    Gempyre::Element counter1(ui, "counter1");
    Gempyre::Element header(ui, "header");
    int count0 = 0;
    int count1 = 0;
    ui.after(10000ms, [header]() mutable {
        header.set_html("time force");
    });
    ui.start_periodic(1000ms, [&count0, &counter0]() mutable {
        counter0.set_html(std::to_string(++count0));
    });
    ui.start_periodic(100ms, [&count1, &counter1, &ui](Gempyre::Ui::TimerId id) mutable {
        counter1.set_html(std::to_string(++count1));
        if(count1 == 100)
            ui.cancel_timer(id);
    });
    const auto [ver, min, maj] = Gempyre::version();
    Gempyre::Element(ui, "ver").set_html(GempyreUtils::join(std::vector<std::string>{std::to_string(ver), std::to_string(min), std::to_string(maj)}, "."));
    ui.run();
    return 0;
}
