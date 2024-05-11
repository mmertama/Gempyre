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

std::optional<std::string> Dialog::open_file_dialog(const std::string& caption,
                              const std::string& proot,
                              const Filter& filters) {

    const auto root = !proot.empty() ? proot : GempyreUtils::root_dir();
    if(!GempyreUtils::is_dir(root))
        return std::nullopt;
    const auto selection = pfd::open_file(caption, root, makeFilters(filters), pfd::opt::none).result();
    if(!selection.empty()) {
        const auto filename = selection.at(0);
        return filename;
    }
    return std::nullopt;
}


std::optional<std::vector<std::string>> Dialog::open_files_dialog(const std::string& caption,
                              const std::string& proot,
                              const Filter& filters) {
    const auto root = !proot.empty() ? proot : GempyreUtils::root_dir();
    if(!GempyreUtils::is_dir(root))
        return std::nullopt;
    const auto selection = pfd::open_file(caption, root, makeFilters(filters), pfd::opt::multiselect).result();
        if(!selection.empty()) {
            return selection;
        }
        return std::nullopt;
}


std::optional<std::string> Dialog::open_dir_dialog(const std::string& caption,
                               const std::string& proot) {
    const auto root = !proot.empty() ? proot : GempyreUtils::root_dir();
    if(!GempyreUtils::is_dir(root))
        return std::nullopt;
    const auto selection = pfd::select_folder(caption, root, pfd::opt::none).result();
        if(!selection.empty()) {
            return selection;
        }
        return std::nullopt;
}


 std::optional<std::string> Dialog::save_file_dialog(const std::string& caption,
                                  const std::string& proot,
                                  const Filter& filters) {
     const auto root = !proot.empty() ? proot : GempyreUtils::root_dir();
     if(!GempyreUtils::is_dir(root))
         return std::nullopt;
     const auto selection = pfd::save_file(caption, root, makeFilters(filters), pfd::opt::none).result();
     if(!selection.empty()) {
         return selection;
     }
     return std::nullopt;
}
