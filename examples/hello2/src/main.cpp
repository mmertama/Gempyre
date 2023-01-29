#include <gempyre.h>
#include "hello2_resource.h"

int main(int /*args*/, char** /*argv*/) {
    Gempyre::set_debug();
    Gempyre::Ui ui({{"/main.html", Mainhtml}}, "main.html");
    Gempyre::Element(ui, "h2").set_html("Hello Gempyre");
    ui.run();
    return 0;
}
