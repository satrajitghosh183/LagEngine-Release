#pragma once

/**
 * @file OllamaClient.hpp
 * @brief HTTP client for Ollama AI server
 * 
 * ## Setup Instructions
 * 
 * 1. Install Ollama: https://ollama.ai
 *    - Windows: Download installer and run
 *    - For D: drive storage, set OLLAMA_MODELS=D:\Ollama\models
 * 
 * 2. Start the Ollama server: ollama serve
 * 
 * 3. Pull a model: ollama pull codellama:13b
 * 
 * ## Environment Variables
 * - OLLAMA_HOST: Server host (default: localhost)
 * - OLLAMA_PORT: Server port (default: 11434)
 * - OLLAMA_MODELS: Path to store downloaded models
 */

#include <string>
#include <vector>
#include <functional>
#include <future>

namespace GameEngine {

    struct ChatMessage {
        std::string Role;   // "system", "user", "assistant"
        std::string Content;
    };

    struct OllamaConfig {
        std::string Host = "127.0.0.1";
        int Port = 11434;
        std::string Model = "codellama:13b";
        float Temperature = 0.7f;
        int MaxTokens = 2048;
    };

    class OllamaClient {
    public:
        OllamaClient();
        explicit OllamaClient(const OllamaConfig& config);

        void SetConfig(const OllamaConfig& config) { m_Config = config; }
        const OllamaConfig& GetConfig() const { return m_Config; }

        // Check if Ollama server is reachable
        bool IsServerRunning();

        // List available models
        std::vector<std::string> ListModels();

        // Synchronous chat completion
        std::string Chat(const std::vector<ChatMessage>& messages);

        // Synchronous text generation
        std::string Generate(const std::string& prompt);

        // Async versions
        std::future<std::string> ChatAsync(const std::vector<ChatMessage>& messages);
        std::future<std::string> GenerateAsync(const std::string& prompt);

        const std::string& GetLastError() const { return m_LastError; }

    private:
        std::string GetBaseURL() const;
        std::string BuildChatPayload(const std::vector<ChatMessage>& messages) const;
        std::string BuildGeneratePayload(const std::string& prompt) const;
        std::string ExtractResponse(const std::string& jsonResponse, const std::string& key) const;

        OllamaConfig m_Config;
        std::string m_LastError;
    };

}
