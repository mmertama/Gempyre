#include <gempyre.h>
#include "hello2_resource.h"

int main(int /*args*/, char** /*argv*/) {
    Gempyre::setDebug();
    Gempyre::Ui ui({{"/main.html", Mainhtml}}, "main.html");
    Gempyre::Element(ui, "h2").setHTML("Hello Gempyre");
    ui.run();
    return 0;
}
