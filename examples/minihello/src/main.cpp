#include <gempyre.h>
#include <gempyre_utils.h>

int main(int /*args*/, char** /*argv*/) {
    constexpr auto APP_FILE = "./minihello.html";
    const auto map = Gempyre::Ui::to_file_map({APP_FILE});
    gempyre_utils_assert_x(!map.empty(), std::string("Cannot load ") + APP_FILE);
    Gempyre::Ui ui(map, map.begin()->first);
    ui.run();
	return 0;
}
