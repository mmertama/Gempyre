#include <gempyre.h>
#include "hello.html.h"
int main(int /*argc*/, char** /*argv*/)  {
   Gempyre::Ui ui({{"/hello.html", Hellohtml}}, "hello.html", "\"Hello world!\"", 250, 150);
   Gempyre::Element text(ui, "content");
   Gempyre::Element button(ui, "startbutton");
   button.set_html("Hello?");
   button.subscribe("click", [&text](const Gempyre::Event&) {
       text.set_html("Hello World!");
     });
   ui.run();
   return 0;
}
