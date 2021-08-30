#include <gempyre.h>
#include <gempyre_utils.h>
#include <gempyre_client.h>
#include "dialogs_test_resource.h"
#include <iostream>


/**
 Command line depends on system, on my Mac following works:
 for python "/usr/local/bin/python3 ../py_client/pyclient.py"
 for Qt "../qt_client/gempyreqtclient"
 */




int main(int argc, char* argv[]) {
    Gempyre::setDebug();
    const auto plist = GempyreUtils::parseArgs(argc, argv, {{"gempyre-app", 'a', GempyreUtils::ArgType::OPT_ARG}});

    const auto opt = std::get<GempyreUtils::Options>(plist);
    const auto gempyre_app = GempyreUtils::getValue(opt, std::string("gempyre-app"));

    auto ui = [&](){
        if(!gempyre_app) {
            if(!std::get<GempyreUtils::ParamList>(plist).empty()) { // python
                const std::string py = std::get<GempyreUtils::ParamList>(plist)[0];
                return Gempyre::Ui(Dialogs_test_resourceh,
                             "dialogs_test.html", py, Gempyre::Ui::stdParams(500, 640, "Test Dialogs"));
            } else { // plain
                return Gempyre::Ui(Dialogs_test_resourceh,
                             "dialogs_test.html");
            }
        } else { //Hiillos
            return Gempyre::Ui(Dialogs_test_resourceh,
                         "dialogs_test.html", argc, argv);
        }
    }();

    Gempyre::Element content(ui, "content");
    Gempyre::Element openFile(ui, "open_file");
    Gempyre::Element openFiles(ui, "open_files");
    Gempyre::Element openDir(ui, "open_dir");
    Gempyre::Element saveFile(ui, "save_file");

    Gempyre::Element(ui, "exit").subscribe("click", [&ui](const Gempyre::Event&) {
        ui.exit();
    });

    openFile.subscribe("click", [&ui, &content](const Gempyre::Event&) {
        const auto out = Gempyre::Dialog::openFileDialog(ui, "foo", "bar", {{"Text", {"*.txt"}}});
        if(out && !out->empty()) {
            const auto stuff = GempyreUtils::slurp(*out);
            content.setHTML("<h3>" + *out + "</h3>" + stuff + "</br>" + "size:" + std::to_string(GempyreUtils::fileSize(*out)));
        }
    });

    openFiles.subscribe("click", [&ui, &content](const Gempyre::Event&) {
        const auto out = Gempyre::Dialog::openFilesDialog(ui);
        if(out && !out->empty()) {
            std::string line;
            for(const auto& o : *out) {
                line += "filename:" + o + " size:" + std::to_string(GempyreUtils::fileSize(o)) + "</br>";
            }
            content.setHTML(line);
        }
    });

    openDir.subscribe("click", [&ui, &content](const Gempyre::Event&) {
        const auto out = Gempyre::Dialog::openDirDialog(ui, "dir");
        if(out && !out->empty()) {
            const auto dirlist = GempyreUtils::directory(*out);
            std::string line;
            for(const auto& d : dirlist) {
                line += d +"</br>";
            }
            content.setHTML(line);
        }
    });

    saveFile.subscribe("click", [&ui, &content](const Gempyre::Event&) {
        const auto out = Gempyre::Dialog::saveFileDialog(ui, "", "", {{"Text", {"*.txt", "*.text"}}, {"Log", {"*.log"}}});
        if(out && !out->empty()) {
            if(GempyreUtils::fileExists(*out)) {
                 content.setHTML("Do not pick existing file:" + *out);
                 return;
            }
            std::ofstream f;
            f.open (*out);
            f << *content.html();
            f.close();
            content.setHTML("Written in file, size:" + std::to_string(GempyreUtils::fileSize(*out)));
        }
    });


    ui.run();
    return 0;
}