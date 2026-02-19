#pragma once

#include "../../Engine/Scripting/ShaderAssistant.hpp"
#include <imgui.h>
#include <string>
#include <vector>
#include <future>
#include <mutex>

namespace GameEngine {

    class ShaderAssistantPanel {
    public:
        ShaderAssistantPanel();
        ~ShaderAssistantPanel();

        void OnImGuiRender();

        void SetShaderContext(const ShaderContext& ctx) { m_Context = ctx; }

    private:
        struct ChatEntry {
            std::string Role;    // "user" or "assistant"
            std::string Content;
        };

        void SendMessage();
        void CheckAsyncResponse();
        void RenderChatEntry(const ChatEntry& entry, int index);
        void RenderConnectionStatus();
        void RenderModelSelector();
        std::string ExtractCodeBlock(const std::string& text) const;

        ShaderAssistant m_Assistant;
        ShaderContext m_Context;

        // Chat state
        std::vector<ChatEntry> m_ChatHistory;
        char m_InputBuffer[2048] = "";
        bool m_ScrollToBottom = false;

        // Async state
        std::future<std::string> m_PendingResponse;
        bool m_WaitingForResponse = false;
        std::string m_PendingUserMessage;

        // Connection
        bool m_Connected = false;
        float m_ConnectionCheckTimer = 0.0f;
        std::vector<std::string> m_AvailableModels;
        int m_SelectedModel = 0;

        // Settings
        OllamaConfig m_Config;
        char m_HostBuffer[256] = "localhost";
        int m_Port = 11434;
    };

}
