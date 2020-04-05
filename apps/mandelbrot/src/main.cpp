
#include <telex_utils.h>
#include <telex.h>
#include <telex_graphics.h>

#include "mandelbrot_resource.h"

#include "mandelbrotdraw.h"

int main(int /*argc*/, char** /*argv*/) {
    Telex::setDebug();

    Telex::Ui ui({{"/mandelbrot.html", Mandelbrothtml}, {"/mandelbrot.css", Mandelbrotcss}, {"/mandelbrot.png", Mandelbrotpng}},
                 "mandelbrot.html");
    Telex::CanvasElement canvas(ui, "canvas");
    Telex::Element iterations(ui, "iterations_slider");
    Telex::Element colors(ui, "color_slider");
    Telex::Element radius(ui, "radius");
    Telex::Element zooms(ui, "zooms");
    Telex::Graphics graphics(canvas);
    Telex::Graphics backupGraphics(canvas);
    Telex::Graphics blend(canvas);
    Telex::Element busy(ui, "busy");

    bool mousedown = false;
    int mousex;
    int mousey;

    std::unique_ptr<MandelbrotDraw> mandelbrot;
    std::vector<std::array<Mandelbrot::Number, 4>> coordinateStack;

    const auto updater = [&graphics, &busy](int c, int a) {
        if(c == 0) {
            busy.setAttribute("style", "display:inline");
        }
        if(c < a) {
            busy.setHTML("Calculating..." + std::to_string(c * 100 / a) + "%");
        }
        if(c == a) {
            busy.setAttribute("style", "display:none");
            graphics.update();
        }
    };

    ui.onOpen([&]() {
        const auto type = *canvas.type();
        telex_graphics_assert(type == "canvas", "the element is expected to be a canvas")
        const auto rect = *canvas.rect();
        iterations.subscribe("change",[&mandelbrot, &updater](const Telex::Event& ev){

            const auto value = *TelexUtils::toOr<int>(ev.properties.at("value"));
            mandelbrot->setIterations(value);
            mandelbrot->update(updater);
        }, {"value"});

        colors.subscribe("change",[&mandelbrot, &updater](const Telex::Event& ev){
            const auto value = *TelexUtils::toOr<int>(ev.properties.at("value"));
            mandelbrot->setColors(value);
            mandelbrot->update(updater);
        }, {"value"});

        canvas.subscribe("mousedown", [&mousex, &mousey, &mousedown, rect, &graphics, &backupGraphics] (const Telex::Event& ev) {
            mousex = *TelexUtils::toOr<int>(ev.properties.at("clientX")) - rect.x;
            mousey = *TelexUtils::toOr<int>(ev.properties.at("clientY")) - rect.y;
            mousedown = true;
            backupGraphics = graphics.clone();
        }, {"clientX", "clientY"});

        canvas.subscribe("mouseup", [&mousex, &mousey, &mousedown, rect, &graphics, &backupGraphics, &mandelbrot, &coordinateStack, &radius, &zooms, &updater](const Telex::Event& ev) {
            const auto mx = *TelexUtils::toOr<int>(ev.properties.at("clientX")) - rect.x;
            const auto my = *TelexUtils::toOr<int>(ev.properties.at("clientY")) - rect.y;
            mousedown = false;
            graphics = std::move(backupGraphics);
            const auto delta = std::max(mx - mousex, my - mousey);
            if(delta > 5) {
                mandelbrot->setRect(mousex, mousey, delta, delta);
                coordinateStack.push_back(mandelbrot->coords());
                mandelbrot->update(updater);
            }
            graphics.update();
            radius.setHTML(Mandelbrot::toString(mandelbrot->radius()));
            zooms.setHTML(std::to_string(coordinateStack.size() - 1));
        }, {"clientX", "clientY"});

        canvas.subscribe("mousemove", [&mousex, &mousey, &mousedown, rect, &graphics, &backupGraphics, &blend](const Telex::Event& ev) {
            if(mousedown) {
                const auto mx = *TelexUtils::toOr<int>(ev.properties.at("clientX")) - rect.x;
                const auto my = *TelexUtils::toOr<int>(ev.properties.at("clientY")) - rect.y;
                blend.drawRect(Telex::Element::Rect{0, 0, rect.width, rect.height}, Telex::Graphics::pix(0x73, 0x73, 0x73, 0x83));
                blend.drawRect(Telex::Element::Rect{mousex, mousey, mx - mousex, my - mousey}, Telex::Graphics::pix(0,0,0,0));
                graphics.merge(backupGraphics);
                graphics.merge(blend);
                graphics.update();
            }
        }, {"clientX", "clientY"}, 200ms);

        canvas.subscribe("dblclick", [&coordinateStack, &mandelbrot, &radius, &zooms, &updater](const Telex::Event&){
            if(coordinateStack.size() == 1) {
              return;
            }
            coordinateStack.pop_back();
            mandelbrot->set(
                        coordinateStack.back()[0],
                        coordinateStack.back()[1],
                        coordinateStack.back()[2],
                        coordinateStack.back()[3]);
            mandelbrot->update(updater);
            radius.setHTML(Mandelbrot::toString(mandelbrot->radius()));
            zooms.setHTML(std::to_string(coordinateStack.size() - 1));
        });


        const auto ival = *TelexUtils::toOr<int>(std::any_cast<std::string>(iterations.values()->at("value")));
        const auto cval = *TelexUtils::toOr<int>(std::any_cast<std::string>(colors.values()->at("value")));


        graphics.create(rect.width, rect.height);
        coordinateStack.push_back({-2, -2, 2, 2});
        mandelbrot = std::make_unique<MandelbrotDraw>(graphics,
                coordinateStack.back()[0],
                coordinateStack.back()[1],
                coordinateStack.back()[2],
                coordinateStack.back()[3],
                ival);

        mandelbrot->setColors(cval);
        mandelbrot->update(updater);
        blend.create(rect.width, rect.height);

        radius.setHTML(Mandelbrot::toString(mandelbrot->radius()));
        zooms.setHTML(std::to_string(coordinateStack.size() - 1));

       // ui.startTimer(3000ms, true,[&canvas, &bytes](){
       //     canvas.paint(bytes);
       // });
    });

    ui.run();
    return 0;
}
