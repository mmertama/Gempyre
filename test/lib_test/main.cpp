#include <iostream>
#include <map>
#include <string_view>
#include <gempyre_utils.h>
#include "test.h"




int main(int /*argc*/, char** /*argv*/)  {
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
  return 0;
}
