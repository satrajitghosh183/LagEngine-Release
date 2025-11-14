#include "FileSystem.hpp"
#include "../Core/Logger.hpp"
#include <fstream>
#include <sstream>
#ifndef _WIN32
#include <unistd.h>
#endif

namespace GameEngine {

    bool FileSystem::Exists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    bool FileSystem::IsDirectory(const std::string& path) {
        return std::filesystem::is_directory(path);
    }

    bool FileSystem::IsFile(const std::string& path) {
        return std::filesystem::is_regular_file(path);
    }

    size_t FileSystem::GetFileSize(const std::string& path) {
        if (!IsFile(path)) return 0;
        return std::filesystem::file_size(path);
    }

    std::string FileSystem::ReadFileAsString(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            GE_CORE_ERROR("Failed to open file: {0}", path);
            return "";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        return buffer.str();
    }

    std::vector<uint8_t> FileSystem::ReadFileAsBinary(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            GE_CORE_ERROR("Failed to open file: {0}", path);
            return {};
        }
        
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            GE_CORE_ERROR("Failed to read file: {0}", path);
            return {};
        }
        
        file.close();
        return buffer;
    }

    bool FileSystem::WriteStringToFile(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        if (!file.is_open()) {
            GE_CORE_ERROR("Failed to open file for writing: {0}", path);
            return false;
        }
        
        file << content;
        file.close();
        return true;
    }

    bool FileSystem::WriteBinaryToFile(const std::string& path, const std::vector<uint8_t>& data) {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            GE_CORE_ERROR("Failed to open file for writing: {0}", path);
            return false;
        }
        
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();
        return true;
    }

    bool FileSystem::CreateDirectory(const std::string& path) {
        try {
            return std::filesystem::create_directories(path);
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to create directory: {0} - {1}", path, e.what());
            return false;
        }
    }

    bool FileSystem::DeleteFile(const std::string& path) {
        try {
            return std::filesystem::remove(path);
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to delete file: {0} - {1}", path, e.what());
            return false;
        }
    }

    bool FileSystem::DeleteDirectory(const std::string& path) {
        try {
            return std::filesystem::remove_all(path) > 0;
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to delete directory: {0} - {1}", path, e.what());
            return false;
        }
    }

    bool FileSystem::CopyFile(const std::string& source, const std::string& destination) {
        try {
            std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing);
            return true;
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to copy file: {0} -> {1} - {2}", source, destination, e.what());
            return false;
        }
    }

    bool FileSystem::MoveFile(const std::string& source, const std::string& destination) {
        try {
            std::filesystem::rename(source, destination);
            return true;
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to move file: {0} -> {1} - {2}", source, destination, e.what());
            return false;
        }
    }

    std::string FileSystem::GetExtension(const std::string& path) {
        std::filesystem::path p(path);
        return p.extension().string();
    }

    std::string FileSystem::GetFilenameWithoutExtension(const std::string& path) {
        std::filesystem::path p(path);
        return p.stem().string();
    }

    std::string FileSystem::GetFilename(const std::string& path) {
        std::filesystem::path p(path);
        return p.filename().string();
    }

    std::string FileSystem::GetDirectory(const std::string& path) {
        std::filesystem::path p(path);
        return p.parent_path().string();
    }

    std::string FileSystem::Join(const std::string& path1, const std::string& path2) {
        std::filesystem::path p1(path1);
        std::filesystem::path p2(path2);
        return (p1 / p2).string();
    }

    std::string FileSystem::GetAbsolutePath(const std::string& path) {
        return std::filesystem::absolute(path).string();
    }

    std::string FileSystem::GetRelativePath(const std::string& path, const std::string& base) {
        return std::filesystem::relative(path, base).string();
    }

    std::string FileSystem::NormalizePath(const std::string& path) {
        return std::filesystem::path(path).lexically_normal().string();
    }

    std::vector<std::string> FileSystem::GetFilesInDirectory(const std::string& directory, bool recursive) {
        std::vector<std::string> files;
        
        try {
            if (recursive) {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                    if (entry.is_regular_file()) {
                        files.push_back(entry.path().string());
                    }
                }
            } else {
                for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                    if (entry.is_regular_file()) {
                        files.push_back(entry.path().string());
                    }
                }
            }
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to read directory: {0} - {1}", directory, e.what());
        }
        
        return files;
    }

    std::vector<std::string> FileSystem::GetFilesWithExtension(const std::string& directory, 
                                                               const std::string& extension,
                                                               bool recursive) {
        auto allFiles = GetFilesInDirectory(directory, recursive);
        std::vector<std::string> filtered;
        
        for (const auto& file : allFiles) {
            if (GetExtension(file) == extension) {
                filtered.push_back(file);
            }
        }
        
        return filtered;
    }

    std::string FileSystem::GetCurrentDirectory() {
        return std::filesystem::current_path().string();
    }

    bool FileSystem::SetCurrentDirectory(const std::string& path) {
        try {
            std::filesystem::current_path(path);
            return true;
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to set current directory: {0} - {1}", path, e.what());
            return false;
        }
    }

    std::string FileSystem::GetExecutableDirectory() {
#ifdef _WIN32
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        return GetDirectory(buffer);
#else
        char buffer[1024];
        ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (len != -1) {
            buffer[len] = '\0';
            return GetDirectory(buffer);
        }
        return "";
#endif
    }

    std::string FileSystem::OpenFileDialog(const std::string& filter) {
        // Platform-specific implementation
#ifdef _WIN32
        // TODO: Implement Windows file dialog
        GE_CORE_WARN("File dialogs not yet implemented on Windows");
        return "";
#else
        GE_CORE_WARN("File dialogs not yet implemented on this platform");
        return "";
#endif
    }

    std::string FileSystem::SaveFileDialog(const std::string& filter) {
        // Platform-specific implementation
#ifdef _WIN32
        // TODO: Implement Windows file dialog
        GE_CORE_WARN("File dialogs not yet implemented on Windows");
        return "";
#else
        GE_CORE_WARN("File dialogs not yet implemented on this platform");
        return "";
#endif
    }
}