#include "gempyre.h"
#include "gempyre_utils.h"
#include "audio_resource.h"

int main(int /*argc*/, char** /*argv*/) {
    Gempyre::set_debug(true);
    Gempyre::Ui ui(Audio_resourceh, "audio.html");
    Gempyre::Element play(ui, "play");
    Gempyre::Element audio(ui, "audio", ui.root());
    audio.set_attribute("src", "gempyre.ogg");
    const auto id = audio.id();
    const auto playIt = "document.getElementById(" + GempyreUtils::qq(id) + ").play();";
    play.subscribe(Gempyre::Event::CLICK, [&ui, playIt] (auto) {
        ui.eval(playIt);
    });
    ui.run();
}
