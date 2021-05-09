#include "gempyre_graphics.h"
#include "gempyre_utils.h"
#include "imageblit_resource.h"
#include <iostream>

using namespace std::chrono_literals;

void writeText(int x, int y, const std::string& text, Gempyre::CanvasElement& el) {
    el.ui().beginBatch();
    const auto width = 1000. / 9.;
    const auto height = 1000. / 9.;
    auto caret = x;
    for(const auto& c : text) {
        if(c >= 'a' && c <=  'z') {
            const int row = (c - 'a') % 9;
            const int col = (c - 'a') / 9;
            el.paintImage("salcat", {caret, y, 20, 20}, {
                              static_cast<int>(row * width),
                              static_cast<int>(col * height),
                              static_cast<int>(width),
                              static_cast<int>(height)});
        }
        if(c == '\n') {
            y += 20;
            caret = x;
        } else
            caret += 20;
    }
    el.ui().endBatch();
}

int main(int argc, char** argv) {
   // Gempyre::setDebug();
    const auto args = GempyreUtils::parseArgs(argc, argv, {{"resources", 'r', GempyreUtils::ArgType::REQ_ARG}});
    const auto options = std::get<GempyreUtils::Options>(std::get<GempyreUtils::Params>(args));
    const auto it = options.find("resources");
    const auto root = (it != options.end()) ? (it->second + "/") : std::string("");

    Gempyre::Ui ui({{"/imageblit.html", Imageblithtml}, {"/owl.png", Owlpng}}, "imageblit.html", Gempyre::Ui::UseDefaultPort, root);

    Gempyre::CanvasElement canvas(ui, "canvas");

    //Five ways to load image

    //1) external resource using http/https from somewhere
    ui.startTimer(2000ms, true, [&canvas]() {
        writeText(0, 40, "the beach\nis place\nto be\npalm to\nstay under\nthe sea\nand sand", canvas);
    });

    //2) via baked in resource (this image is added in above)
    canvas.addImage("/owl.png", [&canvas](const auto id){
        canvas.paintImage(id, {200, 0, 200, 200});
    });


    //3). via page and add as a resources
    ui.startTimer(2000ms, true, [&canvas]() {
        canvas.paintImage("some_sceneid", {0, 200, 200, 200});
    });
    /* This does not work
    Gempyre::Element(ui, "some_sceneid").subscribe("load", [&canvas] (const Gempyre::Element::Event&){
         canvas.paintImage("some_sceneid", {0, 200, 200, 200});
    });
    */
    const auto simage1 = root + "free-scenery-7.jpg";
    if(GempyreUtils::fileExists(simage1)) {
        if(!ui.addFile("/scene.jpg", simage1)) {
            std::cerr << "Cannot load " << simage1 << " (try: -r <PATH TO>/Gempyre-framework/test/imageblit/stuff)" << std::endl;
            return -1;
        }
    } else ui.alert(simage1 + " not found!");

    //4) add as resource and image
    const auto simage2 = root + "tom-hanssens-shot-01.jpg";
    if(GempyreUtils::fileExists(simage2)) {
        if(!ui.addFile("/scene2.jpg", simage2)) {
            std::cerr << "Cannot load " << simage2 << " (try: -r <PATH TO>/Gempyre-framework/test/imageblit/stuff)" << std::endl;
            return -1;
        }
    } else ui.alert(simage2 + " not found!");

    canvas.addImage("/scene2.jpg", [&canvas](const auto& scene){
         canvas.paintImage(scene, {200, 200, 200, 200});
    });

    //5) local file - see root parameter in constructor (assuming it is set correctly here to imageblit/stuff folder)
    ui.startTimer(3000ms, true, [&canvas]() {
        canvas.paintImage("huld", {0, 400, 200, 200});
    });

    auto frame = 0U;

    canvas.addImage("leigh-kellogg-captainamerica-poseframes-2.jpg", [&canvas, &ui, &frame](const auto& marica){
        ui.startTimer(50ms, false, [&canvas, &frame, marica]{
            const std::vector<Gempyre::Element::Rect> frames{
                {100, 300, 248, 344},
                {348, 300, 282, 344},
                {616, 300, 278, 344},
                {880, 300, 278, 344},
                {1200, 300, 300, 344},
                {1518, 300, 300, 344},

                {100, 806, 248, 344},
                {378, 806, 282, 344},
                {656, 806, 282, 344},
                {944, 806, 286, 344},

                {100, 1314, 248, 344},
                {378, 1314, 278, 344},
                {656, 1314, 278, 344},
                {945, 1314, 330, 344},
                {1300, 1314, 330, 344},

                {100, 1832, 248, 344},
                {378, 1832, 278, 344},
                {678, 1832, 284, 344},
                {964, 1832, 320, 344},
                {1295, 1832, 320, 344},
            };
       /*     for(int i = 0; i < 6; i++){
                canvas.paintImage(marica, {i * 50, 0, 49, 49}, frames.at(i));
            }
            for(int i = 0; i < 4; i++){
                canvas.paintImage(marica, {i * 50, 50, 49, 49}, frames.at(i + 6));
            }
            for(int i = 0; i < 5; i++){
                canvas.paintImage(marica, {i * 50, 100, 49, 49}, frames.at(i + 10));
            }
            for(int i = 0; i < 5; i++){
                canvas.paintImage(marica, {i * 50, 150, 49, 49}, frames.at(i + 15));
            }
*/



            if(frame >= frames.size())
                frame = 0;
            canvas.paintImage(marica, {200, 400, 200, 200}, frames.at(frame));
            ++frame;
  //          std::cout << "frame" << frame << std::endl;
        });
    });

    ui.run();
}
