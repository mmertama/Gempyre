#include "gempyre_client.h"
#include "gempyre.h"
#include "portable-file-dialogs.h"
#include "gempyre_utils.h"

using namespace Gempyre;

static std::vector<std::string> makeFilters(const Gempyre::Dialog::Filter& f) {
    std::vector<std::string> filters;
    for(const auto& e : f) {
        const auto name = std::get<std::string>(e);
        const auto files = std::get<std::vector<std::string>>(e);
        filters.push_back(name);
        filters.push_back(GempyreUtils::join(files, " "));
    }
    return filters;
}

std::optional<std::string> Dialog::openFileDialog(Gempyre::Ui& ui, const std::string& caption,
                              const std::string& root,
                              const Filter& filters) {
    (void) ui;
    const auto selection = pfd::open_file(caption, root, makeFilters(filters), pfd::opt::none).result();
    if(!selection.empty()) {
        const auto filename = selection.at(0);
        return filename;
    }
    return std::nullopt;
}


std::optional<std::vector<std::string>> Dialog::openFilesDialog(Gempyre::Ui& ui, const std::string& caption,
                              const std::string& root,
                              const Filter& filters) {
    (void) ui;
    const auto selection = pfd::open_file(caption, root, makeFilters(filters), pfd::opt::multiselect).result();
        if(!selection.empty()) {
            return selection;
        }
        return std::nullopt;
}


std::optional<std::string> Dialog::openDirDialog(Ui& ui, const std::string& caption,
                               const std::string& root) {
    (void) ui;
    const auto selection = pfd::select_folder(caption, root, pfd::opt::none).result();
        if(!selection.empty()) {
            return selection;
        }
        return std::nullopt;
}


 std::optional<std::string> Dialog::saveFileDialog(Gempyre::Ui& ui, const std::string& caption,
                                  const std::string& root,
                                  const Filter& filters) {
     (void) ui;
     const auto selection = pfd::save_file(caption, root, makeFilters(filters), pfd::opt::none).result();
     if(!selection.empty()) {
         return selection;
     }
     return std::nullopt;
}
