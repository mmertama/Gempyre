#include <telex.h>
#include <telex_utils.h>
#include <telex_client.h>

#include "hexview_resource.h"

#include <iostream>
#include <iomanip>

using Bytes = std::vector<unsigned char>;

constexpr unsigned MaxBytes = 0xFFFF + 1;
constexpr unsigned BytesWidth(const unsigned value) {
    return value / 0xF == 0 ? 0 : BytesWidth(value / 0xF) + 1;
}

std::string toAscii(const Bytes& bytes) {
    auto pos = 0;
    std::string out;
    for(const auto b : bytes) {
        if(pos == 16) {
            out += "<br>";
            pos = 0;
        }
        ++pos;
        switch (b) {
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '&': out += "&amp;"; break;
        case '"': out += "&quot;"; break;
        default:
            if(b >= 0x20 && b < 0x7F)
                out +=  static_cast<char>(b);
           else
                out += '.';
        }
    }
    return  out;
}

std::string toBytes(const Bytes& bytes) {
    auto pos = 0;
    std::string out;
    const std::string hex{"0123456789ABCDEF"};
    for(const auto b : bytes) {
        if(pos == 16) {
            out += "<br>";
            pos = 0;
        }
        ++pos;
        out += hex[b >> 4];
        out += hex[b & 0xF];
        out += ' ';
    }
    return  out;
}

std::string toOffset(const Bytes& bytes) {
    auto offset = 0U;
    std::stringstream stream;
    for(; offset < bytes.size(); offset += 16) {
        stream << std::uppercase << std::setfill('0') << std::setw (BytesWidth(MaxBytes)) << std::hex << offset << "<br>";
    }
    return stream.str();
}

int main(int /*argc*/, char** /*argv*/)  {
    std::string filename;
    const std::string miniview = TelexUtils::systemEnv("TELEX-EXTENSION") ;
    telex_utils_assert_x(!miniview.empty(), "TELEX-EXTENSION not set");
    telex_utils_assert_x(TelexUtils::isExecutable(miniview), "TELEX-EXTENSION does not point to file");
    Telex::Ui ui({{"/hexview.html", Hexviewhtml}, {"/hexview.css", Hexviewcss}, {"/text_x_hex.png", Text_x_hexpng}},
                 "hexview.html", miniview, "500 640 Hexview");
    Telex::Element fileDialog(ui, "openfile");
    fileDialog.subscribe("click", [&ui, &filename](const Telex::Event&) {
        const auto out = TelexClient::Dialog<Telex::Ui>(ui).openFileDialog();

        if(out.has_value()) {
                filename = std::any_cast<std::string>(*out);
                Telex::Element(ui, "filename").setHTML(filename);
                const auto content = TelexUtils::slurp<unsigned char>(filename, MaxBytes);
                Telex::Element ascii(ui, "ascii-field");
                ascii.setHTML(toAscii(content));
                Telex::Element bytes(ui, "bytes-field");
                bytes.setHTML(toBytes(content));
                Telex::Element offset(ui, "offset-field");
                offset.setHTML(toOffset(content));
                if(MaxBytes < TelexUtils::fileSize(filename)) {
                    Telex::Element(ui, "file-cut").setAttribute("style", "display:block");
                } else {
                    Telex::Element(ui, "file-cut").setAttribute("style", "display:none");
                }
         } else {
            Telex::Element(ui, "filename").setHTML("");
        }
      });
   Telex::Element(ui, "file-cut").setAttribute("style", "display:none");
   ui.run();
   return 0;
}
