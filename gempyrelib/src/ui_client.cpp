
#include "gempyre_client.h"
#include "gempyre.h"

#include <map>
#include <any>
#include <tuple>
#include <vector>

using namespace Gempyre;
using AnyMap = std::map<std::string, std::any>;
using AnyVec = std::vector<std::any>;

static std::any makeFilters(const Gempyre::Dialog::Filter& f) {
    AnyMap map;
    for(const auto& e : f) {
        const auto name = std::get<std::string>(e);
        const auto files = std::get<std::vector<std::string>>(e);
        AnyVec vec;
        std::transform(files.begin(), files.end(), std::back_inserter(vec), [](const auto& n){return std::make_any<std::string>(n);});
        map.emplace(name, std::make_any<AnyVec>(vec));
    }
    return std::make_any<AnyMap>(map);
}

std::optional<std::string> Dialog::openFileDialog(Gempyre::Ui& ui, const std::string& caption,
                              const std::string& root,
                              const Filter& filters) {
    const auto f = makeFilters(filters);
    const auto out = ui.extensionGet("openFile", {{"caption", caption}, {"dir", root}, {"filter", f}});
    if(out.has_value()) {
        const auto filename = std::any_cast<std::string>(*out);
        return filename;
    }
    return std::nullopt;
}


std::optional<std::vector<std::string>> Dialog::openFilesDialog(Gempyre::Ui& ui, const std::string& caption,
                              const std::string& root,
                              const Filter& filters) {
    const auto f = makeFilters(filters);
    const auto out = ui.extensionGet("openFiles", {{"caption", caption}, {"dir", root}, {"filter", f}});
    if(out.has_value()) {
        const auto vec = std::any_cast<std::vector<std::any>>(*out);
        std::vector<std::string> files;
        std::transform(vec.begin(), vec.end(), std::back_inserter(files), [](const auto& a){return std::any_cast<std::string>(a);});
        return files;
    }
    return std::nullopt;
}


std::optional<std::string> Dialog::openDirDialog(Ui& ui, const std::string& caption,
                               const std::string& root) {
  const auto out = ui.extensionGet("openDir", {{"caption", caption}, {"dir", root}});
  if(out.has_value()) {
      const auto filename = std::any_cast<std::string>(*out);
      return filename;
  }
  return std::nullopt;
}


 std::optional<std::string> Dialog::saveFileDialog(Gempyre::Ui& ui, const std::string& caption,
                                  const std::string& root,
                                  const Filter& filters) {
    const auto f = makeFilters(filters);
    const auto out = ui.extensionGet("saveFile", {{"caption", caption}, {"dir", root}, {"filter", f}});
    if(out.has_value()) {
        const auto filename = std::any_cast<std::string>(*out);
        return filename;
    }
    return std::nullopt;
}
