#include <gempyre.h>
#include <string_view>
#include <format>
#include "hello.html.h"


using namespace std::literals;


int main(int /*argc*/, char** /*argv*/)  {
  Gempyre::Ui ui({{"/hello.html", Hellohtml}}, "hello.html"sv, "\"Welcome!\""sv, 250, 150);
  Gempyre::Element text(ui, "content"sv);
  Gempyre::Element button(ui, "startbutton"sv);
  button.subscribe(Gempyre::Event::CLICK, [&text](const Gempyre::Event&) {
    text.set_html("Hello World!"sv);
    });
  int count = 5; 
  ui.on_open([&]() {
    const auto [a, b, c] = Gempyre::version();
    Gempyre::Element(ui, "version"sv).set_html(std::format("{}.{}.{}", a, b, c));
    ui.start_periodic(1s, [&]() {
      --count;
      button.set_html(std::format("Exit in {}s", count));
      if(count == 0)
        ui.exit();
      });
  });    
  ui.run();
  return 0;
}
