#pragma once

#include "../Core/Base.hpp"
#include "OllamaClient.hpp"
#include <string>
#include <vector>
#include <functional>
#include <deque>

namespace GameEngine {

    struct CodeContext {
        std::string CurrentFile;
        std::string CurrentLanguage;
        std::string SelectedCode;
        std::string SurroundingCode;
        int CursorLine = 0;
        int CursorColumn = 0;
        std::vector<std::string> OpenFiles;
        std::string ProjectName;
        std::string ErrorMessage;
    };

    struct CodeChatMessage {
        std::string Role;  // "user", "assistant", "system"
        std::string Content;
        std::string Timestamp;
    };

    class CodeAssistant {
    public:
        CodeAssistant();
        ~CodeAssistant() = default;

        // Configuration
        void SetModel(const std::string& model) {
            m_Model = model;
            OllamaConfig config = m_Ollama.GetConfig();
            config.Model = model;
            m_Ollama.SetConfig(config);
        }
        std::string GetModel() const { return m_Model; }
        bool IsAvailable();

        // Host / port convenience (wraps OllamaConfig)
        std::string GetHost() const { return m_Ollama.GetConfig().Host; }
        int         GetPort() const { return m_Ollama.GetConfig().Port; }
        void SetHost(const std::string& host, int port) {
            OllamaConfig cfg = m_Ollama.GetConfig();
            cfg.Host = host; cfg.Port = port;
            m_Ollama.SetConfig(cfg);
        }

        // Replace the last assistant message in history (used for streaming updates)
        void ReplaceLastAssistantMessage(const std::string& msg) {
            for (auto it = m_History.rbegin(); it != m_History.rend(); ++it) {
                if (it->Role == "assistant") { it->Content = msg; return; }
            }
            AddToHistory("assistant", msg);
        }

        // Context
        void SetContext(const CodeContext& context) { m_Context = context; }
        const CodeContext& GetContext() const { return m_Context; }

        // Synchronous operations
        std::string CompleteCode(const std::string& code, int cursorPos);
        std::string ExplainCode(const std::string& code);
        std::string FixError(const std::string& code, const std::string& error);
        std::string RefactorCode(const std::string& code, const std::string& instruction);
        std::string GenerateCode(const std::string& description);
        std::string OptimizeCode(const std::string& code);
        std::string AddComments(const std::string& code);
        std::string ConvertCode(const std::string& code, const std::string& targetLanguage);

        // Chat interface
        std::string Chat(const std::string& message);
        void ChatAsync(const std::string& message, std::function<void(const std::string&)> callback);
        void ClearHistory();
        const std::deque<CodeChatMessage>& GetHistory() const { return m_History; }

        // Commands (parsed from chat)
        bool ProcessCommand(const std::string& input, std::string& output);

    private:
        std::string CreateSystemPrompt();
        std::string FormatContextForPrompt();
        void AddToHistory(const std::string& role, const std::string& content);

    private:
        OllamaClient m_Ollama;
        std::string m_Model = "codellama:13b";
        CodeContext m_Context;
        
        std::deque<CodeChatMessage> m_History;
        static constexpr size_t MAX_HISTORY = 50;
        
        std::string m_SystemPrompt;
    };

}
