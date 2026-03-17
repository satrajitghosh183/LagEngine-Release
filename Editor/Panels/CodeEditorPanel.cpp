#include "CodeEditorPanel.hpp"
#include "../../Engine/Core/Logger.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <algorithm>

namespace GameEngine {

    // Keyword lists for syntax highlighting
    static const std::vector<std::string> LuaKeywords = {
        "and", "break", "do", "else", "elseif", "end", "false", "for", "function",
        "goto", "if", "in", "local", "nil", "not", "or", "repeat", "return",
        "then", "true", "until", "while"
    };

    static const std::vector<std::string> GLSLKeywords = {
        "attribute", "const", "uniform", "varying", "buffer", "shared", "coherent",
        "volatile", "restrict", "readonly", "writeonly", "layout", "centroid", "flat",
        "smooth", "noperspective", "patch", "sample", "break", "continue", "do",
        "for", "while", "switch", "case", "default", "if", "else", "subroutine",
        "in", "out", "inout", "true", "false", "discard", "return", "struct"
    };

    static const std::vector<std::string> GLSLTypes = {
        "void", "bool", "int", "uint", "float", "double", "vec2", "vec3", "vec4",
        "dvec2", "dvec3", "dvec4", "bvec2", "bvec3", "bvec4", "ivec2", "ivec3", "ivec4",
        "uvec2", "uvec3", "uvec4", "mat2", "mat3", "mat4", "mat2x2", "mat2x3", "mat2x4",
        "mat3x2", "mat3x3", "mat3x4", "mat4x2", "mat4x3", "mat4x4", "sampler1D",
        "sampler2D", "sampler3D", "samplerCube", "sampler2DShadow"
    };

    static const std::vector<std::string> CppKeywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
        "bool", "break", "case", "catch", "char", "char16_t", "char32_t", "class",
        "compl", "const", "constexpr", "const_cast", "continue", "decltype", "default",
        "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export",
        "extern", "false", "float", "for", "friend", "goto", "if", "inline", "int",
        "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr",
        "operator", "or", "or_eq", "private", "protected", "public", "register",
        "reinterpret_cast", "return", "short", "signed", "sizeof", "static",
        "static_assert", "static_cast", "struct", "switch", "template", "this",
        "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
        "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t",
        "while", "xor", "xor_eq", "override", "final"
    };

    CodeEditorPanel::CodeEditorPanel() {
    }

    void CodeEditorPanel::OnImGuiRender() {
        if (!m_IsOpen) return;
        
        ImGui::Begin("Code Editor", nullptr, ImGuiWindowFlags_MenuBar);

        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New", "Ctrl+N")) {
                    CodeEditorTab tab;
                    tab.FileName = "Untitled";
                    tab.Language = SyntaxLanguage::PlainText;
                    m_Tabs.push_back(tab);
                    m_CurrentTab = static_cast<int>(m_Tabs.size()) - 1;
                }
                if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                    // TODO: File dialog
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save", "Ctrl+S", false, m_CurrentTab >= 0)) {
                    SaveFile();
                }
                if (ImGui::MenuItem("Save All", "Ctrl+Shift+S")) {
                    SaveAllFiles();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Close Tab", "Ctrl+W", false, m_CurrentTab >= 0)) {
                    CloseTab(m_CurrentTab);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, !m_UndoStack.empty())) {
                    Undo();
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, !m_RedoStack.empty())) {
                    Redo();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Find...", "Ctrl+F")) {
                    m_ShowFindReplace = true;
                }
                if (ImGui::MenuItem("Replace...", "Ctrl+H")) {
                    m_ShowFindReplace = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Line Numbers", nullptr, &m_ShowLineNumbers);
                ImGui::MenuItem("Word Wrap", nullptr, &m_WordWrap);
                ImGui::MenuItem("File Tree", nullptr, &m_ShowFileTree);
                ImGui::Separator();
                if (ImGui::BeginMenu("Font Size")) {
                    if (ImGui::MenuItem("Small (12)")) m_FontSize = 12.0f;
                    if (ImGui::MenuItem("Normal (14)")) m_FontSize = 14.0f;
                    if (ImGui::MenuItem("Large (16)")) m_FontSize = 16.0f;
                    if (ImGui::MenuItem("Extra Large (18)")) m_FontSize = 18.0f;
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Handle keyboard shortcuts
        HandleKeyboardShortcuts();

        // Find/Replace popup
        if (m_ShowFindReplace) {
            RenderFindReplace();
        }

        // Main content
        float fileTreeWidth = m_ShowFileTree ? 200.0f : 0.0f;
        
        if (m_ShowFileTree) {
            ImGui::BeginChild("FileTree", ImVec2(fileTreeWidth, 0), true);
            RenderFileTree();
            ImGui::EndChild();
            ImGui::SameLine();
        }

        ImGui::BeginChild("EditorArea", ImVec2(0, 0), false);
        
        RenderTabBar();
        
        if (m_CurrentTab >= 0 && m_CurrentTab < static_cast<int>(m_Tabs.size())) {
            RenderEditor();
        } else {
            // Empty state
            ImVec2 center = ImGui::GetWindowSize();
            center.x /= 2;
            center.y /= 2;
            ImGui::SetCursorPos(center);
            ImGui::TextDisabled("No file open");
            ImGui::SetCursorPosX(center.x - 50);
            ImGui::TextDisabled("Ctrl+O to open");
        }

        ImGui::EndChild();

        ImGui::End();
    }

    void CodeEditorPanel::RenderTabBar() {
        if (ImGui::BeginTabBar("EditorTabs", ImGuiTabBarFlags_Reorderable | 
                                             ImGuiTabBarFlags_AutoSelectNewTabs |
                                             ImGuiTabBarFlags_TabListPopupButton)) {
            for (int i = 0; i < static_cast<int>(m_Tabs.size()); i++) {
                auto& tab = m_Tabs[i];
                
                ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
                if (tab.Modified) {
                    flags |= ImGuiTabItemFlags_UnsavedDocument;
                }

                bool open = true;
                std::string tabName = tab.Modified ? (tab.FileName + "*") : tab.FileName;
                
                if (ImGui::BeginTabItem(tabName.c_str(), &open, flags)) {
                    m_CurrentTab = i;
                    ImGui::EndTabItem();
                }

                if (!open) {
                    CloseTab(i);
                    if (i <= m_CurrentTab && m_CurrentTab > 0) {
                        m_CurrentTab--;
                    }
                    i--;
                }
            }
            ImGui::EndTabBar();
        }
    }

    void CodeEditorPanel::RenderEditor() {
        if (m_CurrentTab < 0 || m_CurrentTab >= static_cast<int>(m_Tabs.size())) return;

        auto& tab = m_Tabs[m_CurrentTab];

        // Editor area with line numbers
        ImGui::BeginChild("EditorContent", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), 
                          true, ImGuiWindowFlags_HorizontalScrollbar);

        float lineNumberWidth = m_ShowLineNumbers ? 50.0f : 0.0f;

        if (m_ShowLineNumbers) {
            ImGui::BeginChild("LineNumbers", ImVec2(lineNumberWidth, 0), false);
            RenderLineNumbers(tab);
            ImGui::EndChild();
            ImGui::SameLine();
        }

        ImGui::BeginChild("TextArea", ImVec2(0, 0), false);
        RenderTextWithSyntax(tab);
        ImGui::EndChild();

        ImGui::EndChild();

        // Status bar
        RenderStatusBar();
    }

    void CodeEditorPanel::RenderLineNumbers(const CodeEditorTab& tab) {
        std::istringstream stream(tab.Content);
        std::string line;
        int lineNum = 1;

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        
        while (std::getline(stream, line)) {
            // Check if line has error/warning
            bool hasError = std::find(tab.ErrorLines.begin(), tab.ErrorLines.end(), lineNum) 
                           != tab.ErrorLines.end();
            bool hasWarning = std::find(tab.WarningLines.begin(), tab.WarningLines.end(), lineNum)
                             != tab.WarningLines.end();

            if (hasError) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            } else if (hasWarning) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
            }

            ImGui::Text("%4d", lineNum);

            if (hasError || hasWarning) {
                ImGui::PopStyleColor();
            }

            lineNum++;
        }

        ImGui::PopStyleColor();
    }

    void CodeEditorPanel::RenderTextWithSyntax(CodeEditorTab& tab) {
        // For now, use a simple text input
        // A full implementation would render each line with colored spans

        ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
        if (tab.ReadOnly) {
            flags |= ImGuiInputTextFlags_ReadOnly;
        }

        // Calculate size needed
        ImVec2 size(-FLT_MIN, -FLT_MIN);

        // Use multiline input with callback for syntax highlighting
        static std::string tempBuffer;
        tempBuffer = tab.Content;
        tempBuffer.resize(1024 * 1024);  // 1MB max

        if (ImGui::InputTextMultiline("##CodeEditor", tempBuffer.data(), tempBuffer.capacity(),
                                       size, flags)) {
            std::string newContent(tempBuffer.c_str());
            if (newContent != tab.Content) {
                // Save undo state
                UndoEntry entry;
                entry.Content = tab.Content;
                entry.CursorPos = 0;
                m_UndoStack.push_back(entry);
                if (m_UndoStack.size() > MAX_UNDO) {
                    m_UndoStack.erase(m_UndoStack.begin());
                }
                m_RedoStack.clear();

                tab.Content = newContent;
                tab.Modified = (tab.Content != tab.OriginalContent);
                
                // Re-tokenize
                TokenizeContent(tab);

                if (m_OnContentChangedCallback) {
                    m_OnContentChangedCallback();
                }
            }
        }
    }

    void CodeEditorPanel::RenderStatusBar() {
        if (m_CurrentTab < 0 || m_CurrentTab >= static_cast<int>(m_Tabs.size())) return;

        auto& tab = m_Tabs[m_CurrentTab];

        // Count lines
        int lineCount = 1;
        for (char c : tab.Content) {
            if (c == '\n') lineCount++;
        }

        // Language name
        const char* langName = "Plain Text";
        switch (tab.Language) {
            case SyntaxLanguage::Lua: langName = "Lua"; break;
            case SyntaxLanguage::GLSL: langName = "GLSL"; break;
            case SyntaxLanguage::Cpp: langName = "C++"; break;
            case SyntaxLanguage::JSON: langName = "JSON"; break;
            default: break;
        }

        ImGui::Separator();
        ImGui::Text("Ln %d, Col %d | %d lines | %s | %s",
            tab.CursorLine + 1, tab.CursorColumn + 1, lineCount, langName,
            tab.Modified ? "Modified" : "Saved");
    }

    void CodeEditorPanel::RenderFindReplace() {
        ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Find & Replace", &m_ShowFindReplace)) {
            ImGui::InputText("Find", m_FindBuffer, sizeof(m_FindBuffer));
            ImGui::InputText("Replace", m_ReplaceBuffer, sizeof(m_ReplaceBuffer));

            ImGui::Checkbox("Case Sensitive", &m_CaseSensitive);
            ImGui::SameLine();
            ImGui::Checkbox("Whole Word", &m_WholeWord);
            ImGui::SameLine();
            ImGui::Checkbox("Regex", &m_UseRegex);

            if (ImGui::Button("Find Next")) {
                Find();
            }
            ImGui::SameLine();
            if (ImGui::Button("Replace")) {
                Replace();
            }
            ImGui::SameLine();
            if (ImGui::Button("Replace All")) {
                // TODO: Replace all
            }
        }
        ImGui::End();
    }

    void CodeEditorPanel::RenderFileTree() {
        ImGui::Text("Files");
        ImGui::Separator();

        if (m_RootDirectory.empty()) {
            ImGui::TextDisabled("No project open");
            return;
        }

        // Simple file tree
        try {
            for (const auto& entry : std::filesystem::directory_iterator(m_RootDirectory)) {
                std::string filename = entry.path().filename().string();
                
                if (entry.is_directory()) {
                    if (ImGui::TreeNode(filename.c_str())) {
                        // Recursively show contents (simplified)
                        ImGui::TreePop();
                    }
                } else {
                    if (ImGui::Selectable(filename.c_str())) {
                        OpenFile(entry.path().string());
                    }
                }
            }
        } catch (...) {
            ImGui::TextDisabled("Error reading directory");
        }
    }

    bool CodeEditorPanel::OpenFile(const std::string& path) {
        // Check if already open
        for (int i = 0; i < static_cast<int>(m_Tabs.size()); i++) {
            if (m_Tabs[i].FilePath == path) {
                m_CurrentTab = i;
                return true;
            }
        }

        // Read file
        std::ifstream file(path);
        if (!file.is_open()) {
            GE_CORE_ERROR("Failed to open file: {}", path);
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        CodeEditorTab tab;
        tab.FilePath = path;
        tab.FileName = std::filesystem::path(path).filename().string();
        tab.Content = buffer.str();
        tab.OriginalContent = tab.Content;
        tab.Language = DetectLanguage(tab.FileName);
        
        TokenizeContent(tab);

        m_Tabs.push_back(tab);
        m_CurrentTab = static_cast<int>(m_Tabs.size()) - 1;

        GE_CORE_INFO("Opened file: {}", path);
        return true;
    }

    bool CodeEditorPanel::SaveFile(int tabIndex) {
        if (tabIndex < 0) tabIndex = m_CurrentTab;
        if (tabIndex < 0 || tabIndex >= static_cast<int>(m_Tabs.size())) return false;

        auto& tab = m_Tabs[tabIndex];

        if (tab.FilePath.empty()) {
            // TODO: Save As dialog
            GE_CORE_WARN("Cannot save untitled file");
            return false;
        }

        std::ofstream file(tab.FilePath);
        if (!file.is_open()) {
            GE_CORE_ERROR("Failed to save file: {}", tab.FilePath);
            return false;
        }

        file << tab.Content;
        file.close();

        tab.OriginalContent = tab.Content;
        tab.Modified = false;

        GE_CORE_INFO("Saved file: {}", tab.FilePath);

        if (m_OnSaveCallback) {
            m_OnSaveCallback(tab.FilePath);
        }

        return true;
    }

    bool CodeEditorPanel::SaveAllFiles() {
        bool allSaved = true;
        for (int i = 0; i < static_cast<int>(m_Tabs.size()); i++) {
            if (m_Tabs[i].Modified) {
                if (!SaveFile(i)) {
                    allSaved = false;
                }
            }
        }
        return allSaved;
    }

    void CodeEditorPanel::CloseTab(int tabIndex) {
        if (tabIndex < 0 || tabIndex >= static_cast<int>(m_Tabs.size())) return;

        // TODO: Prompt to save if modified

        m_Tabs.erase(m_Tabs.begin() + tabIndex);
        
        if (m_CurrentTab >= static_cast<int>(m_Tabs.size())) {
            m_CurrentTab = static_cast<int>(m_Tabs.size()) - 1;
        }
    }

    void CodeEditorPanel::CloseAllTabs() {
        m_Tabs.clear();
        m_CurrentTab = -1;
    }

    SyntaxLanguage CodeEditorPanel::DetectLanguage(const std::string& filename) {
        std::string ext = std::filesystem::path(filename).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".lua") return SyntaxLanguage::Lua;
        if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".geom" ||
            ext == ".comp" || ext == ".shader") return SyntaxLanguage::GLSL;
        if (ext == ".cpp" || ext == ".hpp" || ext == ".c" || ext == ".h" ||
            ext == ".cxx" || ext == ".cc") return SyntaxLanguage::Cpp;
        if (ext == ".json") return SyntaxLanguage::JSON;

        return SyntaxLanguage::PlainText;
    }

    void CodeEditorPanel::TokenizeContent(CodeEditorTab& tab) {
        tab.Tokens.clear();

        switch (tab.Language) {
            case SyntaxLanguage::Lua: TokenizeLua(tab); break;
            case SyntaxLanguage::GLSL: TokenizeGLSL(tab); break;
            case SyntaxLanguage::Cpp: TokenizeCpp(tab); break;
            case SyntaxLanguage::JSON: TokenizeJSON(tab); break;
            default: break;
        }
    }

    void CodeEditorPanel::TokenizeLua(CodeEditorTab& tab) {
        const std::string& text = tab.Content;
        size_t i = 0;

        while (i < text.size()) {
            // Skip whitespace
            if (std::isspace(text[i])) {
                i++;
                continue;
            }

            // Comments
            if (i + 1 < text.size() && text[i] == '-' && text[i + 1] == '-') {
                size_t start = i;
                // Check for long comment
                if (i + 3 < text.size() && text[i + 2] == '[' && text[i + 3] == '[') {
                    i += 4;
                    while (i + 1 < text.size() && !(text[i] == ']' && text[i + 1] == ']')) {
                        i++;
                    }
                    i += 2;
                } else {
                    // Single line comment
                    while (i < text.size() && text[i] != '\n') i++;
                }
                tab.Tokens.push_back({start, i - start, GetCommentColor()});
                continue;
            }

            // Strings
            if (text[i] == '"' || text[i] == '\'') {
                char quote = text[i];
                size_t start = i++;
                while (i < text.size() && text[i] != quote) {
                    if (text[i] == '\\' && i + 1 < text.size()) i++;
                    i++;
                }
                if (i < text.size()) i++;
                tab.Tokens.push_back({start, i - start, GetStringColor()});
                continue;
            }

            // Numbers
            if (std::isdigit(text[i]) || (text[i] == '.' && i + 1 < text.size() && std::isdigit(text[i + 1]))) {
                size_t start = i;
                while (i < text.size() && (std::isdigit(text[i]) || text[i] == '.' || 
                       text[i] == 'x' || text[i] == 'X' || 
                       (text[i] >= 'a' && text[i] <= 'f') ||
                       (text[i] >= 'A' && text[i] <= 'F'))) {
                    i++;
                }
                tab.Tokens.push_back({start, i - start, GetNumberColor()});
                continue;
            }

            // Identifiers and keywords
            if (std::isalpha(text[i]) || text[i] == '_') {
                size_t start = i;
                while (i < text.size() && (std::isalnum(text[i]) || text[i] == '_')) {
                    i++;
                }
                std::string word = text.substr(start, i - start);
                
                // Check if keyword
                bool isKeyword = std::find(LuaKeywords.begin(), LuaKeywords.end(), word) 
                                != LuaKeywords.end();
                if (isKeyword) {
                    tab.Tokens.push_back({start, i - start, GetKeywordColor()});
                }
                continue;
            }

            i++;
        }
    }

    void CodeEditorPanel::TokenizeGLSL(CodeEditorTab& tab) {
        const std::string& text = tab.Content;
        size_t i = 0;

        while (i < text.size()) {
            if (std::isspace(text[i])) {
                i++;
                continue;
            }

            // Comments
            if (i + 1 < text.size() && text[i] == '/' && text[i + 1] == '/') {
                size_t start = i;
                while (i < text.size() && text[i] != '\n') i++;
                tab.Tokens.push_back({start, i - start, GetCommentColor()});
                continue;
            }
            if (i + 1 < text.size() && text[i] == '/' && text[i + 1] == '*') {
                size_t start = i;
                i += 2;
                while (i + 1 < text.size() && !(text[i] == '*' && text[i + 1] == '/')) i++;
                i += 2;
                tab.Tokens.push_back({start, i - start, GetCommentColor()});
                continue;
            }

            // Preprocessor
            if (text[i] == '#') {
                size_t start = i;
                while (i < text.size() && text[i] != '\n') i++;
                tab.Tokens.push_back({start, i - start, 0xFFCC77CC});  // Purple
                continue;
            }

            // Identifiers
            if (std::isalpha(text[i]) || text[i] == '_') {
                size_t start = i;
                while (i < text.size() && (std::isalnum(text[i]) || text[i] == '_')) i++;
                std::string word = text.substr(start, i - start);
                
                if (std::find(GLSLKeywords.begin(), GLSLKeywords.end(), word) != GLSLKeywords.end()) {
                    tab.Tokens.push_back({start, i - start, GetKeywordColor()});
                } else if (std::find(GLSLTypes.begin(), GLSLTypes.end(), word) != GLSLTypes.end()) {
                    tab.Tokens.push_back({start, i - start, GetTypeColor()});
                }
                continue;
            }

            // Numbers
            if (std::isdigit(text[i])) {
                size_t start = i;
                while (i < text.size() && (std::isdigit(text[i]) || text[i] == '.' || 
                       text[i] == 'f' || text[i] == 'F')) i++;
                tab.Tokens.push_back({start, i - start, GetNumberColor()});
                continue;
            }

            i++;
        }
    }

    void CodeEditorPanel::TokenizeCpp(CodeEditorTab& tab) {
        // Similar to GLSL but with C++ keywords
        const std::string& text = tab.Content;
        size_t i = 0;

        while (i < text.size()) {
            if (std::isspace(text[i])) {
                i++;
                continue;
            }

            // Comments
            if (i + 1 < text.size() && text[i] == '/' && text[i + 1] == '/') {
                size_t start = i;
                while (i < text.size() && text[i] != '\n') i++;
                tab.Tokens.push_back({start, i - start, GetCommentColor()});
                continue;
            }
            if (i + 1 < text.size() && text[i] == '/' && text[i + 1] == '*') {
                size_t start = i;
                i += 2;
                while (i + 1 < text.size() && !(text[i] == '*' && text[i + 1] == '/')) i++;
                i += 2;
                tab.Tokens.push_back({start, i - start, GetCommentColor()});
                continue;
            }

            // Preprocessor
            if (text[i] == '#') {
                size_t start = i;
                while (i < text.size() && text[i] != '\n') {
                    if (text[i] == '\\' && i + 1 < text.size()) i++;
                    i++;
                }
                tab.Tokens.push_back({start, i - start, 0xFFCC77CC});
                continue;
            }

            // Strings
            if (text[i] == '"') {
                size_t start = i++;
                while (i < text.size() && text[i] != '"') {
                    if (text[i] == '\\' && i + 1 < text.size()) i++;
                    i++;
                }
                if (i < text.size()) i++;
                tab.Tokens.push_back({start, i - start, GetStringColor()});
                continue;
            }

            // Identifiers
            if (std::isalpha(text[i]) || text[i] == '_') {
                size_t start = i;
                while (i < text.size() && (std::isalnum(text[i]) || text[i] == '_')) i++;
                std::string word = text.substr(start, i - start);
                
                if (std::find(CppKeywords.begin(), CppKeywords.end(), word) != CppKeywords.end()) {
                    tab.Tokens.push_back({start, i - start, GetKeywordColor()});
                }
                continue;
            }

            // Numbers
            if (std::isdigit(text[i])) {
                size_t start = i;
                while (i < text.size() && (std::isalnum(text[i]) || text[i] == '.' || 
                       text[i] == 'x' || text[i] == 'X')) i++;
                tab.Tokens.push_back({start, i - start, GetNumberColor()});
                continue;
            }

            i++;
        }
    }

    void CodeEditorPanel::TokenizeJSON(CodeEditorTab& tab) {
        const std::string& text = tab.Content;
        size_t i = 0;

        while (i < text.size()) {
            if (std::isspace(text[i])) {
                i++;
                continue;
            }

            // Strings (keys and values)
            if (text[i] == '"') {
                size_t start = i++;
                while (i < text.size() && text[i] != '"') {
                    if (text[i] == '\\' && i + 1 < text.size()) i++;
                    i++;
                }
                if (i < text.size()) i++;
                
                // Check if it's a key (followed by :)
                size_t j = i;
                while (j < text.size() && std::isspace(text[j])) j++;
                uint32_t color = (j < text.size() && text[j] == ':') ? 
                    GetKeywordColor() : GetStringColor();
                tab.Tokens.push_back({start, i - start, color});
                continue;
            }

            // Numbers
            if (std::isdigit(text[i]) || text[i] == '-') {
                size_t start = i;
                if (text[i] == '-') i++;
                while (i < text.size() && (std::isdigit(text[i]) || text[i] == '.' ||
                       text[i] == 'e' || text[i] == 'E' || text[i] == '+' || text[i] == '-')) i++;
                tab.Tokens.push_back({start, i - start, GetNumberColor()});
                continue;
            }

            // Keywords (true, false, null)
            if (std::isalpha(text[i])) {
                size_t start = i;
                while (i < text.size() && std::isalpha(text[i])) i++;
                std::string word = text.substr(start, i - start);
                if (word == "true" || word == "false" || word == "null") {
                    tab.Tokens.push_back({start, i - start, GetKeywordColor()});
                }
                continue;
            }

            i++;
        }
    }

    void CodeEditorPanel::HandleKeyboardShortcuts() {
        ImGuiIO& io = ImGui::GetIO();

        if (io.KeyCtrl) {
            if (ImGui::IsKeyPressed(ImGuiKey_S)) {
                if (io.KeyShift) {
                    SaveAllFiles();
                } else {
                    SaveFile();
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
                Undo();
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
                Redo();
            }
            if (ImGui::IsKeyPressed(ImGuiKey_F)) {
                m_ShowFindReplace = true;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_W)) {
                CloseTab(m_CurrentTab);
            }
        }
    }

    void CodeEditorPanel::Undo() {
        if (m_UndoStack.empty() || m_CurrentTab < 0) return;

        auto& tab = m_Tabs[m_CurrentTab];
        
        // Save current state to redo
        UndoEntry redoEntry;
        redoEntry.Content = tab.Content;
        m_RedoStack.push_back(redoEntry);

        // Restore from undo
        UndoEntry& entry = m_UndoStack.back();
        tab.Content = entry.Content;
        tab.Modified = (tab.Content != tab.OriginalContent);
        TokenizeContent(tab);
        
        m_UndoStack.pop_back();
    }

    void CodeEditorPanel::Redo() {
        if (m_RedoStack.empty() || m_CurrentTab < 0) return;

        auto& tab = m_Tabs[m_CurrentTab];
        
        // Save current state to undo
        UndoEntry undoEntry;
        undoEntry.Content = tab.Content;
        m_UndoStack.push_back(undoEntry);

        // Restore from redo
        UndoEntry& entry = m_RedoStack.back();
        tab.Content = entry.Content;
        tab.Modified = (tab.Content != tab.OriginalContent);
        TokenizeContent(tab);
        
        m_RedoStack.pop_back();
    }

    void CodeEditorPanel::Find() {
        if (m_CurrentTab < 0 || strlen(m_FindBuffer) == 0) return;

        auto& tab = m_Tabs[m_CurrentTab];
        std::string searchStr = m_FindBuffer;
        
        // Simple find (no regex for now)
        size_t pos = tab.Content.find(searchStr);
        if (pos != std::string::npos) {
            // TODO: Select the found text
            GE_CORE_INFO("Found '{}' at position {}", searchStr, pos);
        }
    }

    void CodeEditorPanel::Replace() {
        if (m_CurrentTab < 0 || strlen(m_FindBuffer) == 0) return;

        auto& tab = m_Tabs[m_CurrentTab];
        std::string searchStr = m_FindBuffer;
        std::string replaceStr = m_ReplaceBuffer;

        size_t pos = tab.Content.find(searchStr);
        if (pos != std::string::npos) {
            // Save undo state
            UndoEntry entry;
            entry.Content = tab.Content;
            m_UndoStack.push_back(entry);
            m_RedoStack.clear();

            tab.Content.replace(pos, searchStr.length(), replaceStr);
            tab.Modified = true;
            TokenizeContent(tab);
        }
    }

    bool CodeEditorPanel::HasUnsavedChanges() const {
        for (const auto& tab : m_Tabs) {
            if (tab.Modified) return true;
        }
        return false;
    }

    std::string CodeEditorPanel::GetCurrentFilePath() const {
        if (m_CurrentTab >= 0 && m_CurrentTab < static_cast<int>(m_Tabs.size())) {
            return m_Tabs[m_CurrentTab].FilePath;
        }
        return "";
    }

    void CodeEditorPanel::SetContent(const std::string& content, SyntaxLanguage lang) {
        if (m_CurrentTab >= 0 && m_CurrentTab < static_cast<int>(m_Tabs.size())) {
            auto& tab = m_Tabs[m_CurrentTab];
            tab.Content = content;
            tab.Language = lang;
            tab.Modified = true;
            TokenizeContent(tab);
        }
    }

    std::string CodeEditorPanel::GetContent() const {
        if (m_CurrentTab >= 0 && m_CurrentTab < static_cast<int>(m_Tabs.size())) {
            return m_Tabs[m_CurrentTab].Content;
        }
        return "";
    }

    void CodeEditorPanel::GoToLine(int line) {
        if (m_CurrentTab >= 0 && m_CurrentTab < static_cast<int>(m_Tabs.size())) {
            m_Tabs[m_CurrentTab].CursorLine = line - 1;
        }
    }

    void CodeEditorPanel::SetErrorMarkers(const std::vector<std::pair<int, std::string>>& errors) {
        if (m_CurrentTab < 0) return;
        
        auto& tab = m_Tabs[m_CurrentTab];
        tab.ErrorLines.clear();
        for (const auto& [line, msg] : errors) {
            tab.ErrorLines.push_back(line);
        }
    }

    void CodeEditorPanel::ClearErrorMarkers() {
        if (m_CurrentTab >= 0 && m_CurrentTab < static_cast<int>(m_Tabs.size())) {
            m_Tabs[m_CurrentTab].ErrorLines.clear();
            m_Tabs[m_CurrentTab].WarningLines.clear();
        }
    }

}
