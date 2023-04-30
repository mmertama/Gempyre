#ifndef GEMPYRECLIENT_H
#define GEMPYRECLIENT_H

#include <map>
#include <tuple>
#include <vector>
#include <string>
#include <optional>

namespace Gempyre {
    class Ui;

/// @brief Dialogs for file access.
class Dialog {
public:
    /// @brief Filter is for Dialog File name filters.
    /// @details List of tuples having filter name + list of extensions.
    /// e.g. like [("text", [".txt", ".text"])] 
    using Filter = std::vector<std::tuple<std::string, std::vector<std::string>>>;

    
    /// @brief Pick a file.
    /// @param caption optional dialog name. 
    /// @param root optional root folder.
    /// @param filters optional filters what to show.
    /// @return file name selected.
    static std::optional<std::string> open_file_dialog(const std::string& caption = "",
                                  const std::string& root = "",
                                  const Filter& filters = {});

    /// @brief Pick files
    /// @param caption optional dialog name. 
    /// @param root optional root folder.
    /// @param filters optional filters what to show.
    /// @return vector of file names.
    static std::optional<std::vector<std::string>> open_files_dialog(const std::string& caption = "",
                                  const std::string& root = "",
                                  const Filter& filters = {});

    /// @brief Pick a dir.
    /// @param caption optional dialog name.
    /// @param root optional root folder.
    /// @return directory name selected.
    static std::optional<std::string> open_dir_dialog(const std::string& caption = "",
                                   const std::string& root = "");

    /// @brief Pick a a file or crate a new one.
    /// @param caption optional dialog name.
    /// @param root optional rool folder.
    /// @param filters optional filters what to show.
    /// @return file name selected.
    static std::optional<std::string> save_file_dialog(const std::string& caption = "",
    const std::string& root = "", const Filter& filters = {});                                    
  
};
}


#endif // GEMPYRECLIENT_H
