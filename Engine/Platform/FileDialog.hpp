#pragma once

#include <string>
#include <vector>
#include <optional>

namespace GameEngine {

    /**
     * @brief File filter for file dialogs
     */
    struct FileFilter {
        std::string Description;
        std::string Extensions; // e.g., "*.scene;*.json"
        
        FileFilter(const std::string& desc, const std::string& ext)
            : Description(desc), Extensions(ext) {}
    };

    /**
     * @brief Platform file dialog utility
     * 
     * Provides native file dialogs for open/save operations
     */
    class FileDialog {
    public:
        /**
         * @brief Show open file dialog
         * @param title Dialog title
         * @param filters File filters
         * @param defaultPath Default directory path
         * @return Selected file path, or empty if cancelled
         */
        static std::optional<std::string> OpenFile(
            const std::string& title = "Open File",
            const std::vector<FileFilter>& filters = {},
            const std::string& defaultPath = ""
        );
        
        /**
         * @brief Show save file dialog
         * @param title Dialog title
         * @param filters File filters
         * @param defaultPath Default directory path
         * @param defaultFilename Default filename
         * @return Selected file path, or empty if cancelled
         */
        static std::optional<std::string> SaveFile(
            const std::string& title = "Save File",
            const std::vector<FileFilter>& filters = {},
            const std::string& defaultPath = "",
            const std::string& defaultFilename = ""
        );
        
        /**
         * @brief Show select folder dialog
         * @param title Dialog title
         * @param defaultPath Default directory path
         * @return Selected folder path, or empty if cancelled
         */
        static std::optional<std::string> SelectFolder(
            const std::string& title = "Select Folder",
            const std::string& defaultPath = ""
        );
        
        /**
         * @brief Show open multiple files dialog
         * @param title Dialog title
         * @param filters File filters
         * @param defaultPath Default directory path
         * @return Vector of selected file paths
         */
        static std::vector<std::string> OpenMultiple(
            const std::string& title = "Open Files",
            const std::vector<FileFilter>& filters = {},
            const std::string& defaultPath = ""
        );
    };

}
