#ifndef GEMPYRECLIENT_H
#define GEMPYRECLIENT_H

#include <map>
#include <any>
#include <tuple>
#include <vector>

namespace GempyreClient {

template <class T>
class Dialog {
public:
     using Filter = std::vector<std::tuple<std::string, std::vector<std::string>>>;
private:
    T& m_this;
    using AnyMap = std::map<std::string, std::any>;
    using AnyVec = std::vector<std::any>;

    static std::any makeFilters(const Filter& f) {
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
public:
    /**
     * @brief Dialog
     * @param gempyreUi
     */
    Dialog(T& gempyreUi) : m_this(gempyreUi) {}

    /**
     * @brief openDirDialog
     * @param caption
     * @param root
     * @param filterName
     * @param filters
     * @return
     */
    std::optional<std::string> openFileDialog(const std::string& caption = "",
                                  const std::string& root = "",
                                  const Filter& filters = {}) {
        const auto f = makeFilters(filters);
        const auto out = m_this.extension("openFile", {{"caption", caption}, {"dir", root}, {"filter", f}});
        if(out.has_value()) {
            const auto filename = std::any_cast<std::string>(*out);
            return filename;
        }
        return std::nullopt;
    }

    /**
     * @brief openDirDialog
     * @param caption
     * @param root
     * @param filterName
     * @param filters
     * @return
     */
    std::optional<std::vector<std::string>> openFilesDialog(const std::string& caption = "",
                                  const std::string& root = "",
                                  const Filter& filters = {}) {
        const auto f = makeFilters(filters);
        const auto out = m_this.extension("openFiles", {{"caption", caption}, {"dir", root}, {"filter", f}});
        if(out.has_value()) {
            const auto vec = std::any_cast<std::vector<std::any>>(*out);
            std::vector<std::string> files;
            std::transform(vec.begin(), vec.end(), std::back_inserter(files), [](const auto& a){return std::any_cast<std::string>(a);});
            return files;
        }
        return std::nullopt;
    }

    /**
     * @brief openDirDialog
     * @param caption
     * @param root
     * @return
     */
    std::optional<std::string> openDirDialog(const std::string& caption = "",
                                   const std::string& root = "") {
      const auto out = m_this.extension("openDir", {{"caption", caption}, {"dir", root}});
      if(out.has_value()) {
          const auto filename = std::any_cast<std::string>(*out);
          return filename;
      }
      return std::nullopt;
    }

    /**
     * @brief openDirDialog
     * @param caption
     * @param root
     * @param filterName
     * @param filters
     * @return
     */
     std::optional<std::string> saveFileDialog(const std::string& caption = "",
                                      const std::string& root = "",
                                      const Filter& filters = {}) {
        const auto f = makeFilters(filters);
        const auto out = m_this.extension("saveFile", {{"caption", caption}, {"dir", root}, {"filter", f}});
        if(out.has_value()) {
            const auto filename = std::any_cast<std::string>(*out);
            return filename;
        }
        return std::nullopt;
    }
};
}



#endif // GEMPYRECLIENT_H
