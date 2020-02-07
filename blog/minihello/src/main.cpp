#include <telex.h>

int main(int /*args*/, char** /*argv*/) {
    Telex::Ui ui("./minihello.html");
    ui.run();
	return 0;
}
