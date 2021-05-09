#include <gempyre.h>
#include "hello.html.h"
int main(int /*argc*/, char** /*argv*/)  {
   Gempyre::Ui ui({{"/hello.html", Hellohtml}}, "hello.html");
   Gempyre::Element text(ui, "content");
   Gempyre::Element button(ui, "startbutton");
   button.setHTML("Hello?");
   button.subscribe("click", [&text](const Gempyre::Event&) {
       text.setHTML("Hello World!");
     });
   ui.run();
   return 0;
}
