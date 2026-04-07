#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Scripting/CodeAssistant.hpp"
#include <string>
#include <functional>

namespace GameEngine {

    class AIAssistantPanel {
    public:
        AIAssistantPanel();
        ~AIAssistantPanel() = default;

        void OnImGuiRender();

        // Visibility control
        void Toggle() { m_IsOpen = !m_IsOpen; }
        void SetOpen(bool open) { m_IsOpen = open; }
        bool IsOpen() const { return m_IsOpen; }

        // Context from code editor
        void SetSelectedCode(const std::string& code) { m_Context.SelectedCode = code; }
        void SetCurrentFile(const std::string& file) { m_Context.CurrentFile = file; }
        void SetCurrentLanguage(const std::string& lang) { m_Context.CurrentLanguage = lang; }
        void SetErrorMessage(const std::string& error) { m_Context.ErrorMessage = error; }

        // Callbacks
        void SetOnApplyCodeCallback(std::function<void(const std::string&)> callback) {
            m_OnApplyCodeCallback = callback;
        }

    private:
        void RenderHeader();
        void RenderChatHistory();
        void RenderInputArea();
        void RenderQuickActions();

        void SendMessage();
        void ApplyLastCode();
        void CopyToClipboard(const std::string& text);
        
        std::string ExtractCodeFromResponse(const std::string& response);

    private:
        CodeAssistant m_Assistant;
        CodeContext m_Context;

        // Input
        char m_InputBuffer[4096] = "";
        bool m_InputFocused = false;

        // State
        bool m_IsProcessing = false;
        std::string m_LastResponse;
        std::string m_LastExtractedCode;

        // Settings
        char m_ModelBuffer[128] = "codellama:13b";
        char m_HostBuffer[256] = "localhost";
        int  m_PortValue = 11434;
        bool m_ShowSettings = false;

        // Callbacks
        std::function<void(const std::string&)> m_OnApplyCodeCallback;

        // Visibility
        bool m_IsOpen = true;
    };

}
