#include "gempyre.h"
#include "gempyre_graphics.h"
#include "fire_resources.h"

void draw_frame(Gempyre::CanvasElement& canvas) {

}

int main(int /*argc*/, char** /*argv*/) {
    Gempyre::Ui ui{Fire_resourcesh, "fire.html"};
    Gempyre::CanvasElement canvas{ui, "canvas"};
    ui.run();
}