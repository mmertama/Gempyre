#include "gempyre.h"
#include "gempyre_utils.h"
#include "audio_resource.h"

int main(int /*argc*/, char** /*argv*/) {
    Gempyre::setDebug(Gempyre::DebugLevel::Debug);
    Gempyre::Ui ui(Audio_resourceh, "audio.html");
    Gempyre::Element play(ui, "play");
    Gempyre::Element audio(ui, "audio", ui.root());
    audio.setAttribute("src", "gempyre.ogg");
    const auto id = audio.id();
    const auto playIt = "document.getElementById(" + GempyreUtils::qq(id) + ").play();";
    play.subscribe("click", [&ui, playIt] (auto) {
        ui.eval(playIt);
    });
    ui.run();
}
