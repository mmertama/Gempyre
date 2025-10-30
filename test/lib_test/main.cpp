#include <iostream>
#include <map>
#include <string_view>
#include <gempyre_utils.h>
#include <gempyre.h>
#include "test.h"

constexpr auto page = R"(<!DOCTYPE html>
<html>
<head>
    <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
    <meta http-equiv="Pragma" content="no-cache" />
    <meta http-equiv="Expires" content="0" />
    <meta charset="utf-8">
    <title>libtesttest</title>
    </head>
<body>
    <script src="/gempyre.js"></script>
</body>
</html>)";


int main(int argc, char** argv)  {
    for(int i = 1 ; i < argc; ++i) {
       if(argv[i] == std::string_view("--verbose"))
            Gempyre::set_debug();
    }

    const auto& [_p, opt] = GempyreUtils::parse_args(argc, argv, {
        {"no-ui", 'n', GempyreUtils::ArgType::NO_ARG}}); 

    const std::map<GempyreUtils::OS, std::string_view> names {
      {GempyreUtils::OS::MacOs, "Mac"},
      {GempyreUtils::OS::WinOs, "Win"},
      {GempyreUtils::OS::LinuxOs, "Linux"},
      {GempyreUtils::OS::AndroidOs, "Android"},
      {GempyreUtils::OS::RaspberryOs, "Raspberry"},
      {GempyreUtils::OS::OtherOs, "Other OS"}
    };
    std::cout 
        << names.at(GempyreUtils::current_os()) << " " 
        << GempyreUtils::home_dir() << " "
        << GempyreUtils::host_name();

    if(opt.find("no-ui") == opt.end()) {
        Gempyre::Ui ui{{{"/index.html", GempyreUtils::base64_encode(page)}}, "index.html"};
        ui.on_open([&](){ui.exit();});
        ui.on_error([&](auto, auto){ui.exit();}); // in unix if DISPLAY is not set
        ui.run();
    }
  return 0;
}
