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




int main(int /*argc*/, char** /*argv*/) {
    Gempyre::set_debug();

    auto ui =  Gempyre::Ui(Dialogs_test_resourceh,
                             "dialogs_test.html");

    Gempyre::Element content(ui, "content");
    Gempyre::Element openFile(ui, "open_file");
    Gempyre::Element openFiles(ui, "open_files");
    Gempyre::Element openDir(ui, "open_dir");
    Gempyre::Element saveFile(ui, "save_file");

    Gempyre::Element(ui, "exit").subscribe("click", [&ui](const Gempyre::Event&) {
        ui.exit();
    });


    openFile.subscribe("click", [&content](const Gempyre::Event&) {
        const auto out = Gempyre::Dialog::open_file_dialog("Open something", GempyreUtils::home_dir(), {{"Text", {"*.txt"}}});
        if(out && !out->empty()) {
            const auto stuff = GempyreUtils::slurp(*out);
            content.set_html("<h3>" + *out + "</h3>" + stuff + "</br>" + "size:" + std::to_string(GempyreUtils::file_size(*out)));
        }
    });

    openFiles.subscribe("click", [&content](const Gempyre::Event&) {
        const auto out = Gempyre::Dialog::open_files_dialog();
        if(out && !out->empty()) {
            std::string line;
            for(const auto& o : *out) {
                line += "filename:" + o + " size:" + std::to_string(GempyreUtils::file_size(o)) + "</br>";
            }
            content.set_html(line);
        }
    });

    openDir.subscribe("click", [&content](const Gempyre::Event&) {
        const auto out = Gempyre::Dialog::open_dir_dialog("dir");
        if(out && !out->empty()) {
            const auto dirlist = GempyreUtils::entries(*out);
            std::string line;
            for(const auto& d : dirlist) {
                line += d +"</br>";
            }
            content.set_html(line);
        }
    });

    saveFile.subscribe("click", [&content](const Gempyre::Event&) {
        const auto out = Gempyre::Dialog::save_file_dialog("", "", {{"Text", {"*.txt", "*.text"}}, {"Log", {"*.log"}}});
        if(out && !out->empty()) {
            if(GempyreUtils::file_exists(*out)) {
                 content.set_html("Do not pick existing file:" + *out);
                 return;
            }
            std::ofstream f;
            f.open (*out);
            f << *content.html();
            f.close();
            content.set_html("Written in file, size:" + std::to_string(GempyreUtils::file_size(*out)));
        }
    });


    ui.run();
    return 0;
}
