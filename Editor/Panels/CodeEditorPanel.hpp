#pragma once

#include "../../Engine/Core/Base.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace GameEngine {

    enum class SyntaxLanguage {
        PlainText,
        Lua,
        GLSL,
        Cpp,
        JSON
    };

    struct SyntaxToken {
        size_t Start;
        size_t Length;
        uint32_t Color;  // RGBA
    };

    struct CodeEditorTab {
        std::string FilePath;
        std::string FileName;
        std::string Content;
        std::string OriginalContent;
        SyntaxLanguage Language = SyntaxLanguage::PlainText;
        std::vector<SyntaxToken> Tokens;
        bool Modified = false;
        bool ReadOnly = false;
        int CursorLine = 0;
        int CursorColumn = 0;
        int SelectionStart = -1;
        int SelectionEnd = -1;
        std::vector<int> ErrorLines;
        std::vector<int> WarningLines;
    };

    class CodeEditorPanel {
    public:
        CodeEditorPanel();
        ~CodeEditorPanel() = default;

        void OnImGuiRender();

        // Visibility control
        void Toggle() { m_IsOpen = !m_IsOpen; }
        void SetOpen(bool open) { m_IsOpen = open; }
        bool IsOpen() const { return m_IsOpen; }

        // File operations
        bool OpenFile(const std::string& path);
        bool SaveFile(int tabIndex = -1);  // -1 = current tab
        bool SaveAllFiles();
        void CloseTab(int tabIndex);
        void CloseAllTabs();

        // Editor operations
        void SetContent(const std::string& content, SyntaxLanguage lang = SyntaxLanguage::PlainText);
        std::string GetContent() const;
        std::string GetSelectedText() const;
        void InsertText(const std::string& text);
        void ReplaceSelection(const std::string& text);

        // Navigation
        void GoToLine(int line);
        void GoToPosition(int line, int column);

        // Error markers
        void SetErrorMarkers(const std::vector<std::pair<int, std::string>>& errors);
        void ClearErrorMarkers();

        // Callbacks
        void SetOnSaveCallback(std::function<void(const std::string&)> callback) {
            m_OnSaveCallback = callback;
        }
        void SetOnContentChangedCallback(std::function<void()> callback) {
            m_OnContentChangedCallback = callback;
        }

        // Getters
        bool HasUnsavedChanges() const;
        int GetCurrentTabIndex() const { return m_CurrentTab; }
        std::string GetCurrentFilePath() const;

    private:
        void RenderTabBar();
        void RenderEditor();
        void RenderStatusBar();
        void RenderFindReplace();
        void RenderFileTree();

        void TokenizeContent(CodeEditorTab& tab);
        void TokenizeLua(CodeEditorTab& tab);
        void TokenizeGLSL(CodeEditorTab& tab);
        void TokenizeCpp(CodeEditorTab& tab);
        void TokenizeJSON(CodeEditorTab& tab);

        void RenderLineNumbers(const CodeEditorTab& tab);
        void RenderTextWithSyntax(CodeEditorTab& tab);
        
        SyntaxLanguage DetectLanguage(const std::string& filename);
        uint32_t GetKeywordColor() const { return 0xFF7799FF; }  // Light blue
        uint32_t GetStringColor() const { return 0xFF99FF99; }   // Light green
        uint32_t GetCommentColor() const { return 0xFF888888; }  // Gray
        uint32_t GetNumberColor() const { return 0xFFFFBB77; }   // Orange
        uint32_t GetFunctionColor() const { return 0xFFFFFF77; } // Yellow
        uint32_t GetTypeColor() const { return 0xFF77DDFF; }     // Cyan

        void HandleKeyboardShortcuts();
        void Undo();
        void Redo();
        void Find();
        void Replace();

    private:
        std::vector<CodeEditorTab> m_Tabs;
        int m_CurrentTab = -1;

        // Find/Replace
        bool m_ShowFindReplace = false;
        char m_FindBuffer[256] = "";
        char m_ReplaceBuffer[256] = "";
        bool m_CaseSensitive = false;
        bool m_WholeWord = false;
        bool m_UseRegex = false;

        // File tree
        bool m_ShowFileTree = true;
        std::string m_RootDirectory;
        std::vector<std::string> m_FileTreeEntries;

        // Settings
        bool m_ShowLineNumbers = true;
        bool m_WordWrap = false;
        int m_TabSize = 4;
        bool m_InsertSpaces = true;
        float m_FontSize = 14.0f;

        // Undo/Redo
        struct UndoEntry {
            std::string Content;
            int CursorPos;
        };
        std::vector<UndoEntry> m_UndoStack;
        std::vector<UndoEntry> m_RedoStack;
        static constexpr size_t MAX_UNDO = 100;

        // Callbacks
        std::function<void(const std::string&)> m_OnSaveCallback;
        std::function<void()> m_OnContentChangedCallback;

        // Visibility
        bool m_IsOpen = true;
    };

}
