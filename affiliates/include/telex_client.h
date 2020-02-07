#ifndef TELEXCLIENT_H
#define TELEXCLIENT_H


namespace TelexClient {

template <class T>
class Dialog {
    T& m_this;
public:
    /**
     * @brief Dialog
     * @param telexUi
     */
    Dialog(T& telexUi) : m_this(telexUi) {}
    using AnyMap = std::unordered_map<std::string, std::any>;

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
                                  const std::string& filterName = "",
                                  const std::vector<std::string> filters = {}) {
        std::vector<std::any> filterList;
        std::transform(filters.begin(), filters.end(), std::back_inserter(filterList), [](const auto& s) { return std::make_any<std::string>(s);});
        const auto out = m_this.extension("openFile", {{"caption", caption}, {"dir", root}, {"filter", std::make_any<AnyMap>(AnyMap{
                                                       {"title", filterName} , {"filters", filterList}})}});
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
                                  const std::string& filterName = "",
                                  const std::vector<std::string> filters = {}) {
        std::vector<std::any> filterList;
        std::transform(filters.begin(), filters.end(), std::back_inserter(filterList), [](const auto& s) {return std::make_any(s);});
        const auto out = m_this.extension("openFiles", {{"caption", caption}, {"dir", root}, {"filter", std::make_any<AnyMap>(AnyMap{
                                                                                                  {"title", filterName} , {"filters", filterList}})}});
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
      const auto out = m_this.extension("saveFile", {{"caption", caption}, {"dir", root}});
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
                                      const std::string& filterName = "",
                                      const std::vector<std::string> filters = {}) {
        std::vector<std::any> filterList;
        std::transform(filters.begin(), filters.end(), std::back_inserter(filterList), [](const auto& s) { return std::make_any(s);});
        const auto out = m_this.extension("saveFile", {{"caption", caption}, {"dir", root}, {"filter", std::make_any<AnyMap>(AnyMap{
                                                                                                 {"title", filterName} , {"filters", filterList}})}});
        if(out.has_value()) {
            const auto filename = std::any_cast<std::string>(*out);
            return filename;
        }
        return std::nullopt;
    }
};
}



#endif // TELEXCLIENT_H
