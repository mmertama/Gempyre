#ifndef GEMPYRECLIENT_H
#define GEMPYRECLIENT_H

#include <map>
#include <tuple>
#include <vector>
#include <string>
#include <optional>

namespace Gempyre {
    class Ui;

class Dialog {
public:
     using Filter = std::vector<std::tuple<std::string, std::vector<std::string>>>; 
    /**
     * @brief openDirDialog
     * @param caption
     * @param root
     * @param filterName
     * @param filters
     * @return
     */
    static std::optional<std::string> openFileDialog(Gempyre::Ui& ui, const std::string& caption = "",
                                  const std::string& root = "",
                                  const Filter& filters = {});

    /**
     * @brief openDirDialog
     * @param caption
     * @param root
     * @param filterName
     * @param filters
     * @return
     */
    static std::optional<std::vector<std::string>> openFilesDialog(Gempyre::Ui& ui, const std::string& caption = "",
                                  const std::string& root = "",
                                  const Filter& filters = {});
    /**
     * @brief openDirDialog
     * @param caption
     * @param root
     * @return
     */
    static std::optional<std::string> openDirDialog(Gempyre::Ui& ui, const std::string& caption = "",
                                   const std::string& root = "");
    /**
     * @brief openDirDialog
     * @param caption
     * @param root
     * @param filterName
     * @param filters
     * @return
     */
     static std::optional<std::string> saveFileDialog(Gempyre::Ui& ui, const std::string& caption = "",
                                      const std::string& root = "",
                                      const Filter& filters = {});
};
}


#endif // GEMPYRECLIENT_H
