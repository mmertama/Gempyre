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


int main(int /*argc*/, char** /*argv*/)  {
  Gempyre::set_debug(true);
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

  Gempyre::Ui ui{{{"/index.html", GempyreUtils::base64_encode(page)}}, "index.html"};
  ui.on_open([&](){ui.exit();});
  ui.on_error([&](auto, auto){ui.exit();}); // in unix if DISPLAY is not set
  ui.run();
  return 0;
}
