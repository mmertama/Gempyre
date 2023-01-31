#include <gempyre.h>
#include <gempyre_utils.h>

int main(int /*args*/, char** /*argv*/) {
    Gempyre::Ui::Filemap map;
    constexpr auto APP_FILE = "./minihello.html";
    const auto url = Gempyre::Ui::add_file(map, APP_FILE);
    gempyre_utils_assert_x(url, std::string("Cannot load ") + APP_FILE);
    Gempyre::Ui ui(map, *url);
    ui.run();
	return 0;
}
