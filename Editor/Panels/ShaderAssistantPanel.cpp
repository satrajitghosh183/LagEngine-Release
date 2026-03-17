#include "ShaderAssistantPanel.hpp"
#include <algorithm>

namespace GameEngine {

    ShaderAssistantPanel::ShaderAssistantPanel() {
        m_Config.Host = "localhost";
        m_Config.Port = 11434;
        m_Config.Model = "codellama:13b";
    }

    ShaderAssistantPanel::~ShaderAssistantPanel() {
        // Wait for any pending async response
        if (m_PendingResponse.valid()) {
            m_PendingResponse.wait();
        }
    }

    void ShaderAssistantPanel::OnImGuiRender() {
        if (!m_IsOpen) return;
        
        ImGui::Begin("Shader Assistant", &m_IsOpen);

        // Connection settings bar
        RenderConnectionStatus();
        ImGui::Separator();

        // Chat area
        float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing() * 2;
        ImGui::BeginChild("ChatScroll", ImVec2(0, -footerHeight), true);

        if (m_ChatHistory.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::TextWrapped("Welcome to the Shader Assistant! Ask me to:");
            ImGui::BulletText("Generate a new shader (e.g., \"Create a toon shader\")");
            ImGui::BulletText("Explain existing shader code");
            ImGui::BulletText("Fix shader compilation errors");
            ImGui::BulletText("Modify a shader (e.g., \"Add fog to this shader\")");
            ImGui::PopStyleColor();
        }

        for (int i = 0; i < static_cast<int>(m_ChatHistory.size()); i++) {
            RenderChatEntry(m_ChatHistory[i], i);
        }

        if (m_WaitingForResponse) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
            ImGui::TextWrapped("Generating response...");
            ImGui::PopStyleColor();
        }

        if (m_ScrollToBottom) {
            ImGui::SetScrollHereY(1.0f);
            m_ScrollToBottom = false;
        }

        ImGui::EndChild();

        // Input area
        ImGui::Separator();

        // Quick action buttons
        if (ImGui::SmallButton("Explain")) {
            snprintf(m_InputBuffer, sizeof(m_InputBuffer), "Explain the current shader in detail");
            SendMessage();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Generate")) {
            snprintf(m_InputBuffer, sizeof(m_InputBuffer), "Generate a basic PBR shader");
            SendMessage();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Fix Error")) {
            if (!m_Context.CompileError.empty()) {
                snprintf(m_InputBuffer, sizeof(m_InputBuffer), "Fix the shader compilation error");
                SendMessage();
            }
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear Chat")) {
            m_ChatHistory.clear();
            m_Assistant.ClearHistory();
        }

        // Text input
        bool enterPressed = ImGui::InputTextMultiline("##ShaderInput", m_InputBuffer, sizeof(m_InputBuffer),
            ImVec2(-60, ImGui::GetFrameHeightWithSpacing()),
            ImGuiInputTextFlags_CtrlEnterForNewLine | ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::SameLine();
        bool sendClicked = ImGui::Button("Send", ImVec2(50, ImGui::GetFrameHeightWithSpacing()));

        if ((enterPressed || sendClicked) && !m_WaitingForResponse && strlen(m_InputBuffer) > 0) {
            SendMessage();
        }

        // Check for async response
        CheckAsyncResponse();

        ImGui::End();
    }

    void ShaderAssistantPanel::RenderConnectionStatus() {
        // Check connection periodically
        m_ConnectionCheckTimer += ImGui::GetIO().DeltaTime;
        if (m_ConnectionCheckTimer >= 5.0f) {
            m_ConnectionCheckTimer = 0.0f;
            m_Connected = m_Assistant.IsAvailable();
            if (m_Connected && m_AvailableModels.empty()) {
                m_AvailableModels = m_Assistant.GetAvailableModels();
            }
        }

        // Status indicator
        ImVec4 statusColor = m_Connected ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, statusColor);
        ImGui::Text(m_Connected ? "Connected" : "Disconnected");
        ImGui::PopStyleColor();

        ImGui::SameLine();

        // Model selector
        RenderModelSelector();

        ImGui::SameLine();

        // Settings popup
        if (ImGui::SmallButton("Settings")) {
            ImGui::OpenPopup("ShaderAssistantSettings");
        }

        if (ImGui::BeginPopup("ShaderAssistantSettings")) {
            ImGui::Text("Ollama Connection");
            ImGui::Separator();
            ImGui::InputText("Host", m_HostBuffer, sizeof(m_HostBuffer));
            ImGui::InputInt("Port", &m_Port);

            float temp = m_Config.Temperature;
            if (ImGui::SliderFloat("Temperature", &temp, 0.0f, 2.0f)) {
                m_Config.Temperature = temp;
            }

            int maxTokens = m_Config.MaxTokens;
            if (ImGui::InputInt("Max Tokens", &maxTokens)) {
                m_Config.MaxTokens = std::max(128, maxTokens);
            }

            if (ImGui::Button("Apply")) {
                m_Config.Host = m_HostBuffer;
                m_Config.Port = m_Port;
                m_Assistant.SetOllamaConfig(m_Config);
                m_ConnectionCheckTimer = 5.0f; // Force reconnect check
                m_AvailableModels.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void ShaderAssistantPanel::RenderModelSelector() {
        if (m_AvailableModels.empty()) {
            ImGui::Text("No models");
            return;
        }

        ImGui::SetNextItemWidth(200);
        if (ImGui::BeginCombo("##Model", m_AvailableModels[m_SelectedModel].c_str())) {
            for (int i = 0; i < static_cast<int>(m_AvailableModels.size()); i++) {
                bool selected = (i == m_SelectedModel);
                if (ImGui::Selectable(m_AvailableModels[i].c_str(), selected)) {
                    m_SelectedModel = i;
                    m_Config.Model = m_AvailableModels[i];
                    m_Assistant.SetOllamaConfig(m_Config);
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    void ShaderAssistantPanel::RenderChatEntry(const ChatEntry& entry, int index) {
        ImGui::PushID(index);

        bool isUser = (entry.Role == "user");
        ImVec4 color = isUser ? ImVec4(0.4f, 0.7f, 1.0f, 1.0f) : ImVec4(0.5f, 1.0f, 0.5f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text(isUser ? "You:" : "Assistant:");
        ImGui::PopStyleColor();

        ImGui::SameLine();

        // Check if this message contains a code block
        std::string code = ExtractCodeBlock(entry.Content);
        if (!code.empty() && !isUser) {
            // Render text before code block
            size_t codeStart = entry.Content.find("```");
            if (codeStart > 0) {
                std::string beforeCode = entry.Content.substr(0, codeStart);
                ImGui::TextWrapped("%s", beforeCode.c_str());
            }

            // Render code block with monospace font and background
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.15f, 1.0f));
            ImGui::BeginChild(("code_" + std::to_string(index)).c_str(),
                ImVec2(0, std::min(300.0f, ImGui::GetTextLineHeightWithSpacing() * 20)), true);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.8f, 1.0f));
            ImGui::TextUnformatted(code.c_str());
            ImGui::PopStyleColor();
            ImGui::EndChild();
            ImGui::PopStyleColor();

            // Apply button for shader code
            if (ImGui::SmallButton(("Apply Shader##" + std::to_string(index)).c_str())) {
                // Copy to clipboard for now - in a full implementation this would
                // write to Assets/Shaders/ and trigger hot-reload
                ImGui::SetClipboardText(code.c_str());
            }
            ImGui::SameLine();
            if (ImGui::SmallButton(("Copy##" + std::to_string(index)).c_str())) {
                ImGui::SetClipboardText(code.c_str());
            }

            // Render text after code block
            size_t codeEnd = entry.Content.rfind("```");
            if (codeEnd != std::string::npos && codeEnd + 3 < entry.Content.size()) {
                std::string afterCode = entry.Content.substr(codeEnd + 3);
                ImGui::TextWrapped("%s", afterCode.c_str());
            }
        } else {
            ImGui::TextWrapped("%s", entry.Content.c_str());
        }

        ImGui::Separator();
        ImGui::PopID();
    }

    void ShaderAssistantPanel::SendMessage() {
        if (m_WaitingForResponse) return;

        std::string message(m_InputBuffer);
        if (message.empty()) return;

        // Add user message to chat
        m_ChatHistory.push_back({"user", message});
        m_PendingUserMessage = message;

        // Clear input
        m_InputBuffer[0] = '\0';
        m_ScrollToBottom = true;

        // Start async generation
        m_PendingResponse = m_Assistant.ChatAsync(message, m_Context);
        m_WaitingForResponse = true;
    }

    void ShaderAssistantPanel::CheckAsyncResponse() {
        if (!m_WaitingForResponse || !m_PendingResponse.valid()) return;

        auto status = m_PendingResponse.wait_for(std::chrono::milliseconds(0));
        if (status == std::future_status::ready) {
            std::string response = m_PendingResponse.get();
            if (response.empty()) {
                response = "Error: " + m_Assistant.GetClient().GetLastError();
            }
            m_ChatHistory.push_back({"assistant", response});
            m_WaitingForResponse = false;
            m_ScrollToBottom = true;
        }
    }

    std::string ShaderAssistantPanel::ExtractCodeBlock(const std::string& text) const {
        size_t start = text.find("```");
        if (start == std::string::npos) return "";

        start = text.find('\n', start);
        if (start == std::string::npos) return "";
        start++;

        size_t end = text.find("```", start);
        if (end == std::string::npos) return "";

        return text.substr(start, end - start);
    }

}
