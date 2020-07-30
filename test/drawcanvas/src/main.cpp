#include "gempyre_graphics.h"
#include "gempyre_utils.h"
#include "drawcanvas_resource.h"
#include <iostream>
#include <cmath>

using namespace std::chrono_literals;


int main(int /*argc*/, char** /*argv*/) {
    Gempyre::setDebug();
    Gempyre::Ui ui({{"/drawcanvas.html", Drawcanvashtml}}, "drawcanvas.html");

    Gempyre::CanvasElement canvas(ui, "canvas");

    canvas.draw(Gempyre::FrameComposer()
                    .lineWidth(10)
                    .strokeRect({75, 140, 150, 110})
                    .fillRect({130, 190, 40, 60})
                    .beginPath()
                    .moveTo(50, 140)
                    .lineTo(150, 60)
                    .lineTo(250, 140)
                    .closePath()
                    .stroke());


    canvas.draw({
                    "beginPath",
                    "strokeStyle", "red",
                    "fillStyle", "yellow",
                    "ellipse" ,400, 100, 50, 75, 3.1456 / 4., 2. * 3.1456,
                    "closePath",
                    "stroke",
                    "fill",
                });

    canvas.draw({
                    "beginPath",
                    "strokeStyle", "green",
                    "moveTo", 500, 500,
                    "bezierCurveTo", 100, 520, 550, 550, 70, 580,
                    "stroke"
                });

    canvas.draw({
                    "lineWidth", 12,
                    "fillStyle", "#0000EE00",
                    "strokeStyle", "purple",
                    "beginPath",
                    "moveTo", 50, 20,
                    "quadraticCurveTo", 230, 30, 50, 100,
                    "closePath",
                    "stroke",
                    "fill"
                });


    const auto x = 300.;
    const auto y = 300.;
    Gempyre::CanvasElement::CommandList commands({
                                                   "beginPath",
                                                   "lineWidth", 1,
                                                   "strokeStyle", "blue",
                                                   "fillStyle", "#BB000055",
                                                   "arc", x, y, 50, 0, 2 * M_PI,
                                                   "fill"});

    const double pof = M_PI / 10.;
    for(double i = 0; i < 2 * M_PI; i += M_PI / 8.) {
        commands.push_back("beginPath");
        commands.push_back("moveTo");
        commands.push_back(x);
        commands.push_back(y);
        commands.push_back("fillStyle");
        commands.push_back("#" + GempyreUtils::toHex<int>(::rand()));
        commands.push_back("lineTo");
        commands.push_back(x + ::sin(i + pof) * 100.);
        commands.push_back(y + ::cos(i + pof) * 100.);
        commands.push_back("arcTo");
        commands.push_back(x + ::sin(i) * 150.);
        commands.push_back(y + ::cos(i) * 150.);
        commands.push_back(x + ::sin(i - pof) * 100.);
        commands.push_back(y + ::cos(i - pof) * 100.);
        commands.push_back(pof * 100);
        commands.push_back("closePath");
        commands.push_back("stroke");
        commands.push_back("fill");
    }
    canvas.draw(commands);


    canvas.draw({
                    "fillStyle", "#00CC00F3",
                    "beginPath",
                    "arc", 300, 475, 50, 0, 1.5 * M_PI,
                    "closePath",
                    "fill"
                });

    canvas.draw({
        "fillStyle", "lime",
        "save",
        "strokeStyle", "maroon",
        "fillStyle", "grey",
        "textAlign", "middle",
        "font", "50px serif",
        "strokeText", "Solenoidal serenity", 50, 510,
        "fillText", "Solenoidal serenity", 52, 512,
        "restore",
        "font", "20px monospace",
        "fillText", "Gempyre", 400, 51,
                });

    bool erase = false;
    //canvas is no KB focus, use root instead!
    ui.root().subscribe("keydown", [&canvas, &erase](const auto& e) {
        if(GempyreUtils::to<int>(e.properties.at("keyCode")) != 'T')
            return;
        if(erase) {
            canvas.draw(Gempyre::FrameComposer().clearRect({450, 350, 100 , 150}));
            erase = false;
        } else {
            auto f = Gempyre::FrameComposer();
            f.beginPath();
            f.fillStyle("red");
            f.ellipse(500, 400, 50, 75, M_PI / 4., 0, 2. * M_PI);
            f.fill();
            canvas.draw(f);
            erase = true;
        }
    }, {"keyCode"});


    ui.run();
}
