#include "OllamaClient.hpp"
#include "Core/HttpClient.hpp"
#include "Core/Logger.hpp"
#include <sstream>

namespace GameEngine {

    OllamaClient::OllamaClient() {}

    OllamaClient::OllamaClient(const OllamaConfig& config) : m_Config(config) {}

    std::string OllamaClient::GetBaseURL() const {
        return "http://" + m_Config.Host + ":" + std::to_string(m_Config.Port);
    }

    bool OllamaClient::IsServerRunning() {
        return HttpClient::IsServerAvailable(m_Config.Host, m_Config.Port);
    }

    std::vector<std::string> OllamaClient::ListModels() {
        std::vector<std::string> models;
        auto resp = HttpClient::Get(GetBaseURL() + "/api/tags", 5000);
        if (!resp.Success) {
            m_LastError = resp.Error;
            return models;
        }

        // Simple JSON parsing for model names - look for "name" fields
        // Format: {"models":[{"name":"codellama:13b",...},...]
        std::string body = resp.Body;
        size_t pos = 0;
        while ((pos = body.find("\"name\"", pos)) != std::string::npos) {
            pos = body.find(':', pos);
            if (pos == std::string::npos) break;
            pos = body.find('"', pos + 1);
            if (pos == std::string::npos) break;
            pos++;
            size_t end = body.find('"', pos);
            if (end == std::string::npos) break;
            models.push_back(body.substr(pos, end - pos));
            pos = end + 1;
        }

        return models;
    }

    std::string OllamaClient::BuildChatPayload(const std::vector<ChatMessage>& messages) const {
        std::ostringstream ss;
        ss << "{\"model\":\"" << m_Config.Model << "\",\"stream\":false,";
        ss << "\"options\":{\"temperature\":" << m_Config.Temperature
           << ",\"num_predict\":" << m_Config.MaxTokens << "},";
        ss << "\"messages\":[";
        for (size_t i = 0; i < messages.size(); i++) {
            if (i > 0) ss << ",";
            ss << "{\"role\":\"" << messages[i].Role << "\",\"content\":\"";
            // Escape special characters in content
            for (char c : messages[i].Content) {
                switch (c) {
                    case '"': ss << "\\\""; break;
                    case '\\': ss << "\\\\"; break;
                    case '\n': ss << "\\n"; break;
                    case '\r': ss << "\\r"; break;
                    case '\t': ss << "\\t"; break;
                    default: ss << c;
                }
            }
            ss << "\"}";
        }
        ss << "]}";
        return ss.str();
    }

    std::string OllamaClient::BuildGeneratePayload(const std::string& prompt) const {
        std::ostringstream ss;
        ss << "{\"model\":\"" << m_Config.Model << "\",\"stream\":false,";
        ss << "\"options\":{\"temperature\":" << m_Config.Temperature
           << ",\"num_predict\":" << m_Config.MaxTokens << "},";
        ss << "\"prompt\":\"";
        for (char c : prompt) {
            switch (c) {
                case '"': ss << "\\\""; break;
                case '\\': ss << "\\\\"; break;
                case '\n': ss << "\\n"; break;
                case '\r': ss << "\\r"; break;
                case '\t': ss << "\\t"; break;
                default: ss << c;
            }
        }
        ss << "\"}";
        return ss.str();
    }

    std::string OllamaClient::ExtractResponse(const std::string& jsonResponse, const std::string& key) const {
        // Find "key":"value" or "key":{"content":"value"}
        // For /api/chat, response is in message.content
        // For /api/generate, response is in "response"
        std::string searchKey = "\"" + key + "\"";
        size_t pos = jsonResponse.find(searchKey);
        if (pos == std::string::npos) return "";

        pos = jsonResponse.find(':', pos + searchKey.size());
        if (pos == std::string::npos) return "";
        pos++;

        // Skip whitespace
        while (pos < jsonResponse.size() && (jsonResponse[pos] == ' ' || jsonResponse[pos] == '\t'))
            pos++;

        if (pos >= jsonResponse.size()) return "";

        if (jsonResponse[pos] == '"') {
            // Direct string value
            pos++;
            std::string result;
            while (pos < jsonResponse.size() && jsonResponse[pos] != '"') {
                if (jsonResponse[pos] == '\\' && pos + 1 < jsonResponse.size()) {
                    pos++;
                    switch (jsonResponse[pos]) {
                        case 'n': result += '\n'; break;
                        case 'r': result += '\r'; break;
                        case 't': result += '\t'; break;
                        case '"': result += '"'; break;
                        case '\\': result += '\\'; break;
                        default: result += jsonResponse[pos]; break;
                    }
                } else {
                    result += jsonResponse[pos];
                }
                pos++;
            }
            return result;
        } else if (jsonResponse[pos] == '{') {
            // Object - extract "content" from it
            return ExtractResponse(jsonResponse.substr(pos), "content");
        }

        return "";
    }

    std::string OllamaClient::Chat(const std::vector<ChatMessage>& messages) {
        std::string payload = BuildChatPayload(messages);
        auto resp = HttpClient::Post(GetBaseURL() + "/api/chat", payload);

        if (!resp.Success) {
            m_LastError = resp.Error.empty() ? ("HTTP " + std::to_string(resp.StatusCode)) : resp.Error;
            GE_CORE_ERROR("OllamaClient::Chat failed: {0}", m_LastError);
            return "";
        }

        std::string content = ExtractResponse(resp.Body, "content");
        if (content.empty()) {
            // Try extracting from message object
            content = ExtractResponse(resp.Body, "message");
        }
        return content;
    }

    std::string OllamaClient::Generate(const std::string& prompt) {
        std::string payload = BuildGeneratePayload(prompt);
        auto resp = HttpClient::Post(GetBaseURL() + "/api/generate", payload);

        if (!resp.Success) {
            m_LastError = resp.Error.empty() ? ("HTTP " + std::to_string(resp.StatusCode)) : resp.Error;
            GE_CORE_ERROR("OllamaClient::Generate failed: {0}", m_LastError);
            return "";
        }

        return ExtractResponse(resp.Body, "response");
    }

    std::future<std::string> OllamaClient::ChatAsync(const std::vector<ChatMessage>& messages) {
        return std::async(std::launch::async, [this, messages]() {
            return Chat(messages);
        });
    }

    std::future<std::string> OllamaClient::GenerateAsync(const std::string& prompt) {
        return std::async(std::launch::async, [this, prompt]() {
            return Generate(prompt);
        });
    }

}
