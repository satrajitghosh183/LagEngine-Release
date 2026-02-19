#include "FileDialog.hpp"
#include "../Core/Logger.hpp"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>
    #include <commdlg.h>
    #include <shlobj.h>
    #include <codecvt>
    #include <locale>
#elif __linux__
    #include <cstdlib>
    #include <cstdio>
    #include <memory>
    #include <array>
#endif

namespace GameEngine {

#ifdef _WIN32
    // Helper to convert string to wide string
    static std::wstring ToWideString(const std::string& str) {
        if (str.empty()) return std::wstring();
        int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        std::wstring result(size - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size);
        return result;
    }
    
    // Helper to convert wide string to string
    static std::string FromWideString(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string result(size - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], size, nullptr, nullptr);
        return result;
    }
    
    // Build filter string for Windows file dialogs
    static std::wstring BuildFilterString(const std::vector<FileFilter>& filters) {
        std::wstring result;
        for (const auto& filter : filters) {
            result += ToWideString(filter.Description);
            result += L'\0';
            result += ToWideString(filter.Extensions);
            result += L'\0';
        }
        result += L"All Files (*.*)\0*.*\0";
        result += L'\0';
        return result;
    }
#endif

#ifdef __linux__
    // Escape string for safe use in shell command (prevents command injection)
    static std::string EscapeForShell(const std::string& str) {
        std::string result;
        result.reserve(str.size() + 8);
        for (char c : str) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '`':  result += "\\`"; break;
                case '$':  result += "\\$"; break;
                case '\n': result += " "; break;
                case '\r': result += " "; break;
                default:   result += c; break;
            }
        }
        return result;
    }
    
    // Execute command and get output
    static std::string ExecuteCommand(const std::string& cmd) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (pipe) {
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                result += buffer.data();
            }
        }
        // Remove trailing newline
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
        return result;
    }
    
    // Check if zenity is available
    static bool HasZenity() {
        return system("which zenity > /dev/null 2>&1") == 0;
    }
    
    // Check if kdialog is available
    static bool HasKdialog() {
        return system("which kdialog > /dev/null 2>&1") == 0;
    }
#endif

    std::optional<std::string> FileDialog::OpenFile(
        const std::string& title,
        const std::vector<FileFilter>& filters,
        const std::string& defaultPath) {
        
#ifdef _WIN32
        OPENFILENAMEW ofn = {};
        wchar_t filename[MAX_PATH] = {};
        
        std::wstring filterStr = BuildFilterString(filters);
        std::wstring initialDir = ToWideString(defaultPath);
        std::wstring titleStr = ToWideString(title);
        
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFilter = filterStr.c_str();
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = titleStr.c_str();
        ofn.lpstrInitialDir = initialDir.empty() ? NULL : initialDir.c_str();
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        
        if (GetOpenFileNameW(&ofn)) {
            return FromWideString(filename);
        }
        
#elif __linux__
        std::string cmd;
        if (HasZenity()) {
            cmd = "zenity --file-selection";
            if (!title.empty()) {
                cmd += " --title=\"" + EscapeForShell(title) + "\"";
            }
            if (!filters.empty()) {
                for (const auto& f : filters) {
                    cmd += " --file-filter=\"" + EscapeForShell(f.Description) + " | " + EscapeForShell(f.Extensions) + "\"";
                }
            }
        } else if (HasKdialog()) {
            cmd = "kdialog --getopenfilename";
            if (!defaultPath.empty()) {
                cmd += " \"" + EscapeForShell(defaultPath) + "\"";
            }
        }
        
        if (!cmd.empty()) {
            std::string result = ExecuteCommand(cmd);
            if (!result.empty()) {
                return result;
            }
        }
#endif
        
        return std::nullopt;
    }

    std::optional<std::string> FileDialog::SaveFile(
        const std::string& title,
        const std::vector<FileFilter>& filters,
        const std::string& defaultPath,
        const std::string& defaultFilename) {
        
#ifdef _WIN32
        OPENFILENAMEW ofn = {};
        wchar_t filename[MAX_PATH] = {};
        
        // Set default filename
        if (!defaultFilename.empty()) {
            std::wstring wFilename = ToWideString(defaultFilename);
            wcscpy_s(filename, wFilename.c_str());
        }
        
        std::wstring filterStr = BuildFilterString(filters);
        std::wstring initialDir = ToWideString(defaultPath);
        std::wstring titleStr = ToWideString(title);
        
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFilter = filterStr.c_str();
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = titleStr.c_str();
        ofn.lpstrInitialDir = initialDir.empty() ? NULL : initialDir.c_str();
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        
        // Add default extension
        std::wstring defExt;
        if (!filters.empty()) {
            std::string ext = filters[0].Extensions;
            size_t pos = ext.find('*');
            if (pos != std::string::npos && pos + 2 < ext.size()) {
                defExt = ToWideString(ext.substr(pos + 2));
            }
        }
        ofn.lpstrDefExt = defExt.empty() ? NULL : defExt.c_str();
        
        if (GetSaveFileNameW(&ofn)) {
            return FromWideString(filename);
        }
        
#elif __linux__
        std::string cmd;
        if (HasZenity()) {
            cmd = "zenity --file-selection --save";
            if (!title.empty()) {
                cmd += " --title=\"" + title + "\"";
            }
            if (!defaultFilename.empty()) {
                cmd += " --filename=\"" + defaultFilename + "\"";
            }
            cmd += " --confirm-overwrite";
        } else if (HasKdialog()) {
            cmd = "kdialog --getsavefilename";
            if (!defaultPath.empty()) {
                cmd += " \"" + defaultPath + "/" + defaultFilename + "\"";
            }
        }
        
        if (!cmd.empty()) {
            std::string result = ExecuteCommand(cmd);
            if (!result.empty()) {
                return result;
            }
        }
#endif
        
        return std::nullopt;
    }

    std::optional<std::string> FileDialog::SelectFolder(
        const std::string& title,
        const std::string& defaultPath) {
        
#ifdef _WIN32
        BROWSEINFOW bi = {};
        wchar_t path[MAX_PATH] = {};
        
        std::wstring titleStr = ToWideString(title);
        
        bi.lpszTitle = titleStr.c_str();
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        
        LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
        if (pidl) {
            if (SHGetPathFromIDListW(pidl, path)) {
                CoTaskMemFree(pidl);
                return FromWideString(path);
            }
            CoTaskMemFree(pidl);
        }
        
#elif __linux__
        std::string cmd;
        if (HasZenity()) {
            cmd = "zenity --file-selection --directory";
            if (!title.empty()) {
                cmd += " --title=\"" + EscapeForShell(title) + "\"";
            }
        } else if (HasKdialog()) {
            cmd = "kdialog --getexistingdirectory";
            if (!defaultPath.empty()) {
                cmd += " \"" + EscapeForShell(defaultPath) + "\"";
            }
        }
        
        if (!cmd.empty()) {
            std::string result = ExecuteCommand(cmd);
            if (!result.empty()) {
                return result;
            }
        }
#endif
        
        return std::nullopt;
    }

    std::vector<std::string> FileDialog::OpenMultiple(
        const std::string& title,
        const std::vector<FileFilter>& filters,
        const std::string& defaultPath) {
        
        std::vector<std::string> result;
        
#ifdef _WIN32
        OPENFILENAMEW ofn = {};
        wchar_t filename[4096] = {};  // Larger buffer for multiple files
        
        std::wstring filterStr = BuildFilterString(filters);
        std::wstring initialDir = ToWideString(defaultPath);
        std::wstring titleStr = ToWideString(title);
        
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFilter = filterStr.c_str();
        ofn.lpstrFile = filename;
        ofn.nMaxFile = 4096;
        ofn.lpstrTitle = titleStr.c_str();
        ofn.lpstrInitialDir = initialDir.empty() ? NULL : initialDir.c_str();
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
        
        if (GetOpenFileNameW(&ofn)) {
            wchar_t* ptr = filename;
            std::wstring directory = ptr;
            ptr += directory.length() + 1;
            
            if (*ptr == L'\0') {
                // Single file selected
                result.push_back(FromWideString(directory));
            } else {
                // Multiple files selected
                while (*ptr != L'\0') {
                    std::wstring file = ptr;
                    result.push_back(FromWideString(directory + L"\\" + file));
                    ptr += file.length() + 1;
                }
            }
        }
        
#elif __linux__
        std::string cmd;
        if (HasZenity()) {
            cmd = "zenity --file-selection --multiple --separator=\\n";
            if (!title.empty()) {
                cmd += " --title=\"" + EscapeForShell(title) + "\"";
            }
        }
        
        if (!cmd.empty()) {
            std::string output = ExecuteCommand(cmd);
            if (!output.empty()) {
                size_t pos = 0;
                while ((pos = output.find('\n')) != std::string::npos) {
                    result.push_back(output.substr(0, pos));
                    output.erase(0, pos + 1);
                }
                if (!output.empty()) {
                    result.push_back(output);
                }
            }
        }
#endif
        
        return result;
    }

}
