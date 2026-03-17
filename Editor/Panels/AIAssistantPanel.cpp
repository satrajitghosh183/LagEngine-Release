#include "AIAssistantPanel.hpp"
#include "../../Engine/Core/Logger.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <regex>

namespace GameEngine {

    AIAssistantPanel::AIAssistantPanel() {
        strcpy(m_ModelBuffer, m_Assistant.GetModel().c_str());
        strncpy(m_HostBuffer, m_Assistant.GetHost().c_str(), sizeof(m_HostBuffer) - 1);
        m_PortValue = m_Assistant.GetPort();
    }

    void AIAssistantPanel::OnImGuiRender() {
        if (!m_IsOpen) return;
        
        ImGui::Begin("AI Assistant", nullptr, ImGuiWindowFlags_MenuBar);

        RenderHeader();

        // Main content area
        float footerHeight = 120.0f;  // Input area + quick actions
        
        // Chat history
        ImGui::BeginChild("ChatHistory", ImVec2(0, -footerHeight), true);
        RenderChatHistory();
        ImGui::EndChild();

        // Quick actions bar
        RenderQuickActions();

        // Input area
        RenderInputArea();

        // Settings popup
        if (m_ShowSettings) {
            ImGui::OpenPopup("AI Settings");
            m_ShowSettings = false;
        }

        if (ImGui::BeginPopupModal("AI Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Ollama Configuration");
            ImGui::Separator();

            ImGui::InputText("Host", m_HostBuffer, sizeof(m_HostBuffer));
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Ollama server host (default: localhost)");
            }

            ImGui::InputInt("Port", &m_PortValue);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Ollama server port (default: 11434)");
            }

            ImGui::Separator();

            ImGui::InputText("Model", m_ModelBuffer, sizeof(m_ModelBuffer));
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Recommended: codellama:13b, codellama:7b, deepseek-coder:6.7b");
            }

            ImGui::Separator();

            if (ImGui::Button("Apply", ImVec2(100, 0))) {
                m_Assistant.SetModel(m_ModelBuffer);
                m_Assistant.SetHost(m_HostBuffer, m_PortValue);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0))) {
                strcpy(m_ModelBuffer, m_Assistant.GetModel().c_str());
                strncpy(m_HostBuffer, m_Assistant.GetHost().c_str(), sizeof(m_HostBuffer) - 1);
                m_PortValue = m_Assistant.GetPort();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::End();
    }

    void AIAssistantPanel::RenderHeader() {
        if (ImGui::BeginMenuBar()) {
            // Status indicator
            if (m_Assistant.IsAvailable()) {
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "Online");
            } else {
                ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "Offline");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Start Ollama with: ollama serve");
                }
            }

            ImGui::Separator();

            // Model info
            ImGui::Text("Model: %s", m_Assistant.GetModel().c_str());

            ImGui::Separator();

            // Settings button
            if (ImGui::Button("Settings")) {
                m_ShowSettings = true;
            }

            ImGui::Separator();

            // Clear history
            if (ImGui::Button("Clear")) {
                m_Assistant.ClearHistory();
            }

            ImGui::EndMenuBar();
        }
    }

    void AIAssistantPanel::RenderChatHistory() {
        const auto& history = m_Assistant.GetHistory();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 12));

        for (const auto& msg : history) {
            bool isUser = (msg.Role == "user");
            
            // Message bubble styling
            ImVec4 bgColor = isUser ? 
                ImVec4(0.2f, 0.3f, 0.5f, 0.8f) :   // User: blue
                ImVec4(0.25f, 0.25f, 0.25f, 0.8f); // Assistant: gray

            // Alignment
            float bubbleWidth = ImGui::GetContentRegionAvail().x * 0.85f;
            float indent = isUser ? (ImGui::GetContentRegionAvail().x - bubbleWidth) : 0.0f;

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);

            // Draw bubble background
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 textSize = ImGui::CalcTextSize(msg.Content.c_str(), nullptr, false, bubbleWidth - 16);
            
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 bubbleMin = pos;
            ImVec2 bubbleMax = ImVec2(pos.x + bubbleWidth, pos.y + textSize.y + 24);
            
            drawList->AddRectFilled(bubbleMin, bubbleMax, ImGui::ColorConvertFloat4ToU32(bgColor), 8.0f);

            // Role label
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent + 8);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::Text("%s  %s", isUser ? "You" : "AI", msg.Timestamp.c_str());
            ImGui::PopStyleColor();

            // Message content
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent + 8);
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + bubbleWidth - 16);
            
            // Render code blocks differently
            std::string content = msg.Content;
            size_t codeStart = content.find("```");
            
            if (codeStart != std::string::npos && !isUser) {
                // Text before code
                if (codeStart > 0) {
                    ImGui::TextWrapped("%s", content.substr(0, codeStart).c_str());
                }
                
                // Find code block
                size_t codeEnd = content.find("```", codeStart + 3);
                if (codeEnd != std::string::npos) {
                    // Extract code (skip language identifier on first line)
                    std::string codeBlock = content.substr(codeStart + 3, codeEnd - codeStart - 3);
                    size_t newline = codeBlock.find('\n');
                    if (newline != std::string::npos) {
                        codeBlock = codeBlock.substr(newline + 1);
                    }
                    
                    // Code background
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
                    ImGui::BeginChild(("code_" + std::to_string(reinterpret_cast<uintptr_t>(&msg))).c_str(), 
                                      ImVec2(bubbleWidth - 32, 0), true);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 0.7f, 1.0f));
                    ImGui::TextUnformatted(codeBlock.c_str());
                    ImGui::PopStyleColor();
                    ImGui::EndChild();
                    ImGui::PopStyleColor();

                    // Copy button for code
                    if (ImGui::Button(("Copy##" + std::to_string(reinterpret_cast<uintptr_t>(&msg))).c_str())) {
                        CopyToClipboard(codeBlock);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(("Apply##" + std::to_string(reinterpret_cast<uintptr_t>(&msg))).c_str())) {
                        if (m_OnApplyCodeCallback) {
                            m_OnApplyCodeCallback(codeBlock);
                        }
                    }

                    // Text after code
                    if (codeEnd + 3 < content.length()) {
                        ImGui::TextWrapped("%s", content.substr(codeEnd + 3).c_str());
                    }
                } else {
                    ImGui::TextWrapped("%s", content.c_str());
                }
            } else {
                ImGui::TextWrapped("%s", content.c_str());
            }
            
            ImGui::PopTextWrapPos();

            ImGui::Dummy(ImVec2(0, 8));
        }

        ImGui::PopStyleVar();

        // Auto-scroll
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20) {
            ImGui::SetScrollHereY(1.0f);
        }

        // Processing indicator
        if (m_IsProcessing) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Thinking...");
        }
    }

    void AIAssistantPanel::RenderQuickActions() {
        ImGui::Separator();
        
        ImGui::Text("Quick Actions:");
        ImGui::SameLine();

        bool hasSelection = !m_Context.SelectedCode.empty();

        ImGui::BeginDisabled(!hasSelection || m_IsProcessing);
        
        if (ImGui::Button("Explain")) {
            strcpy(m_InputBuffer, "/explain");
            SendMessage();
        }
        ImGui::SameLine();
        
        if (ImGui::Button("Fix")) {
            strcpy(m_InputBuffer, "/fix");
            SendMessage();
        }
        ImGui::SameLine();
        
        if (ImGui::Button("Optimize")) {
            strcpy(m_InputBuffer, "/optimize");
            SendMessage();
        }
        ImGui::SameLine();
        
        if (ImGui::Button("Comment")) {
            strcpy(m_InputBuffer, "/comment");
            SendMessage();
        }
        ImGui::SameLine();
        
        if (ImGui::Button("Refactor")) {
            strcpy(m_InputBuffer, "/refactor");
            SendMessage();
        }
        
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Help")) {
            strcpy(m_InputBuffer, "/help");
            SendMessage();
        }

        // Show selection status
        if (hasSelection) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), 
                "(%zu chars selected)", m_Context.SelectedCode.length());
        }
    }

    void AIAssistantPanel::RenderInputArea() {
        ImGui::Separator();

        float buttonWidth = 80.0f;
        float inputWidth = ImGui::GetContentRegionAvail().x - buttonWidth - 10.0f;

        // Multi-line input
        ImGui::PushItemWidth(inputWidth);
        
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | 
                                    ImGuiInputTextFlags_CtrlEnterForNewLine;
        
        bool submitted = ImGui::InputTextMultiline("##ChatInput", m_InputBuffer, sizeof(m_InputBuffer),
                                                    ImVec2(inputWidth, 60), flags);
        
        ImGui::PopItemWidth();

        ImGui::SameLine();

        // Send button
        ImGui::BeginDisabled(m_IsProcessing || strlen(m_InputBuffer) == 0);
        if (ImGui::Button("Send", ImVec2(buttonWidth, 60)) || submitted) {
            SendMessage();
        }
        ImGui::EndDisabled();

        // Hint
        ImGui::TextDisabled("Enter to send, Ctrl+Enter for new line. Type /help for commands.");
    }

    void AIAssistantPanel::SendMessage() {
        if (strlen(m_InputBuffer) == 0 || m_IsProcessing) return;

        std::string message = m_InputBuffer;
        m_InputBuffer[0] = '\0';

        // Update context
        m_Assistant.SetContext(m_Context);

        m_IsProcessing = true;

        // Async call
        m_Assistant.ChatAsync(message, [this](const std::string& response) {
            if (response.empty()) {
                // Replace the empty assistant entry that Chat() already added
                // with a useful error message so the user sees what went wrong
                m_Assistant.ReplaceLastAssistantMessage(
                    "[Error] No response from Ollama. Check that:\n"
                    "  1. Ollama is running (ollama serve)\n"
                    "  2. The model is installed (ollama pull codellama:13b)\n"
                    "  3. The host/port in Settings matches your Ollama setup\n"
                    "  Current host: " + m_Assistant.GetHost() +
                    ":" + std::to_string(m_Assistant.GetPort()));
            }
            m_LastResponse = response;
            m_LastExtractedCode = ExtractCodeFromResponse(response);
            m_IsProcessing = false;
        });
    }

    void AIAssistantPanel::ApplyLastCode() {
        if (!m_LastExtractedCode.empty() && m_OnApplyCodeCallback) {
            m_OnApplyCodeCallback(m_LastExtractedCode);
        }
    }

    void AIAssistantPanel::CopyToClipboard(const std::string& text) {
        ImGui::SetClipboardText(text.c_str());
    }

    std::string AIAssistantPanel::ExtractCodeFromResponse(const std::string& response) {
        // Find code blocks
        std::regex codeBlockRegex(R"(```(?:\w*\n)?([\s\S]*?)```)");
        std::smatch match;
        
        if (std::regex_search(response, match, codeBlockRegex)) {
            return match[1].str();
        }
        
        return "";
    }

}
