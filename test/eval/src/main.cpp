#include "telex.h"
#include "eval_resource.h"

int main(int /*argc*/, char** /*argv*/) {
    Telex::setDebug();
    Telex::Ui ui({{"/eval.html", Evalhtml}}, "eval.html");
    ui.eval(R"(var para = document.createElement("P");
            var t = document.createTextNode("This is a paragraph.");
            para.appendChild(t);
            document.getElementById("myDIV").appendChild(para);)");
    ui.run();
}
