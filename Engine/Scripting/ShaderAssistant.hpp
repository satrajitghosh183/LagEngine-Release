#pragma once

#include "OllamaClient.hpp"
#include <string>
#include <vector>
#include <functional>
#include <future>

namespace GameEngine {

    enum class ShaderOperation {
        Explain,
        Modify,
        Generate,
        FixError
    };

    struct ShaderContext {
        std::string CurrentVertexShader;
        std::string CurrentFragmentShader;
        std::string ShaderName;
        std::string CompileError;
        int LightCount = 0;
        bool HasShadows = false;
        bool DeferredMode = false;
        std::vector<std::string> MaterialNames;
    };

    class ShaderAssistant {
    public:
        ShaderAssistant();

        void SetOllamaConfig(const OllamaConfig& config);
        bool IsAvailable();

        // Get available models
        std::vector<std::string> GetAvailableModels();

        // Synchronous operations
        std::string ExplainShader(const std::string& shaderSource);
        std::string ModifyShader(const std::string& shaderSource, const std::string& instruction);
        std::string GenerateShader(const std::string& description);
        std::string FixShaderError(const std::string& shaderSource, const std::string& errorLog);

        // Chat-based interaction with full context
        std::string Chat(const std::string& userMessage, const ShaderContext& context);

        // Async versions
        std::future<std::string> ChatAsync(const std::string& userMessage, const ShaderContext& context);

        // Conversation management
        void ClearHistory();
        const std::vector<ChatMessage>& GetHistory() const { return m_History; }

        OllamaClient& GetClient() { return m_Client; }

    private:
        std::string BuildSystemPrompt(const ShaderContext& context) const;
        std::string BuildOperationPrompt(ShaderOperation op, const std::string& source,
                                          const std::string& extra) const;

        OllamaClient m_Client;
        std::vector<ChatMessage> m_History;
    };

}
