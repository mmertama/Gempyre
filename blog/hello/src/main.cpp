#include <telex.h>
#include "hello.html.h"
int main(int /*argc*/, char** /*argv*/)  {
   Telex::Ui ui({{"/hello.html", Hellohtml}}, "hello.html");
   Telex::Element text(ui, "content");
   Telex::Element button(ui, "startbutton");
   button.setHTML("Hello?");
   button.subscribe("click", [&text](const Telex::Element::Event&) {
       text.setHTML("Hello World!");
     });
   ui.run();
   return 0;
}
