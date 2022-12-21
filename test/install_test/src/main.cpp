#include <gempyre.h>
#include "hello.html.h"

int main(int /*argc*/, char** /*argv*/)  {
  Gempyre::Ui ui({{"/hello.html", Hellohtml}}, "hello.html", "\"Welcome!\"", 250, 150);
  Gempyre::Element text(ui, "content");
  Gempyre::Element button(ui, "startbutton");
  button.subscribe("click", [&text](const Gempyre::Event&) {
    text.set_html("Hello World!");
    });
  int count = 5; 
  ui.on_open([&]() {
    ui.start_periodic(1s, [&]() {
      --count;
      button.set_html("Exit in " +  std::to_string(count) + "s");
      if(count == 0)
        ui.exit();
      });
  });    
  ui.run();
  return 0;
}
