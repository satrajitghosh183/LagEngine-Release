#pragma once

#include "../Core/Base.hpp"
#include <string>
#include <vector>
#include <filesystem>

namespace GameEngine {

    /**
     * @brief File system utilities
     * 
     * Cross-platform file operations
     */
    class FileSystem {
    public:
        /**
         * @brief Check if file exists
         */
        static bool Exists(const std::string& path);
        
        /**
         * @brief Check if path is directory
         */
        static bool IsDirectory(const std::string& path);
        
        /**
         * @brief Check if path is file
         */
        static bool IsFile(const std::string& path);
        
        /**
         * @brief Get file size in bytes
         */
        static size_t GetFileSize(const std::string& path);
        
        /**
         * @brief Read entire file as string
         */
        static std::string ReadFileAsString(const std::string& path);
        
        /**
         * @brief Read entire file as binary data
         */
        static std::vector<uint8_t> ReadFileAsBinary(const std::string& path);
        
        /**
         * @brief Write string to file
         */
        static bool WriteStringToFile(const std::string& path, const std::string& content);
        
        /**
         * @brief Write binary data to file
         */
        static bool WriteBinaryToFile(const std::string& path, const std::vector<uint8_t>& data);
        
        /**
         * @brief Create directory (and parents if needed)
         */
        static bool CreateDirectory(const std::string& path);
        
        /**
         * @brief Delete file
         */
        static bool DeleteFile(const std::string& path);
        
        /**
         * @brief Delete directory (recursive)
         */
        static bool DeleteDirectory(const std::string& path);
        
        /**
         * @brief Copy file
         */
        static bool CopyFile(const std::string& source, const std::string& destination);
        
        /**
         * @brief Move/rename file
         */
        static bool MoveFile(const std::string& source, const std::string& destination);
        
        /**
         * @brief Get file extension
         */
        static std::string GetExtension(const std::string& path);
        
        /**
         * @brief Get filename without extension
         */
        static std::string GetFilenameWithoutExtension(const std::string& path);
        
        /**
         * @brief Get filename with extension
         */
        static std::string GetFilename(const std::string& path);
        
        /**
         * @brief Get directory from path
         */
        static std::string GetDirectory(const std::string& path);
        
        /**
         * @brief Join paths
         */
        static std::string Join(const std::string& path1, const std::string& path2);
        
        /**
         * @brief Get absolute path
         */
        static std::string GetAbsolutePath(const std::string& path);
        
        /**
         * @brief Get relative path
         */
        static std::string GetRelativePath(const std::string& path, const std::string& base);
        
        /**
         * @brief Normalize path (resolve .., ., etc.)
         */
        static std::string NormalizePath(const std::string& path);
        
        /**
         * @brief Get all files in directory
         */
        static std::vector<std::string> GetFilesInDirectory(const std::string& directory, 
                                                            bool recursive = false);
        
        /**
         * @brief Get all files with extension in directory
         */
        static std::vector<std::string> GetFilesWithExtension(const std::string& directory, 
                                                              const std::string& extension,
                                                              bool recursive = false);
        
        /**
         * @brief Get current working directory
         */
        static std::string GetCurrentDirectory();
        
        /**
         * @brief Set current working directory
         */
        static bool SetCurrentDirectory(const std::string& path);
        
        /**
         * @brief Get executable directory
         */
        static std::string GetExecutableDirectory();
        
        /**
         * @brief Open file dialog (platform-specific)
         */
        static std::string OpenFileDialog(const std::string& filter = "All Files (*.*)\0*.*\0");
        
        /**
         * @brief Save file dialog (platform-specific)
         */
        static std::string SaveFileDialog(const std::string& filter = "All Files (*.*)\0*.*\0");
    };
}