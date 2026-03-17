#include "CodeAssistant.hpp"
#include "../Core/Logger.hpp"
#include <chrono>
#include <ctime>
#include <sstream>
#include <thread>

namespace GameEngine {

    CodeAssistant::CodeAssistant() {
        OllamaConfig config = m_Ollama.GetConfig();
        config.Model = m_Model;
        m_Ollama.SetConfig(config);
        m_SystemPrompt = CreateSystemPrompt();
    }

    bool CodeAssistant::IsAvailable() {
        return m_Ollama.IsServerRunning();
    }

    std::string CodeAssistant::CreateSystemPrompt() {
        return R"(You are an expert coding assistant integrated into a game engine editor. You help developers write, understand, fix, and optimize code.

Your capabilities:
- Code completion and generation
- Code explanation and documentation
- Bug fixing and error resolution
- Code refactoring and optimization
- Converting between programming languages
- Answering programming questions

Languages you're proficient in:
- C++ (primary engine language)
- Lua (scripting language)
- GLSL (shader language)
- JSON (configuration files)

When generating code:
- Write clean, efficient, well-structured code
- Follow modern C++17 best practices
- Include brief inline comments for complex logic
- Use consistent naming conventions (PascalCase for types, camelCase for functions/variables)

For game engine code specifically:
- Use smart pointers (Ref<T>, Scope<T>) for resource management
- Follow the entity-component-system (ECS) pattern
- Be aware of performance implications
- Use glm for math (vectors, matrices, quaternions)

When explaining code, be concise but thorough. Format responses with markdown when helpful.

IMPORTANT: When the user asks for code, provide ONLY the code without excessive explanation unless they ask for explanation.)";
    }

    std::string CodeAssistant::FormatContextForPrompt() {
        std::stringstream ss;
        
        if (!m_Context.CurrentFile.empty()) {
            ss << "\nCurrent file: " << m_Context.CurrentFile;
            ss << " (Line " << m_Context.CursorLine << ", Col " << m_Context.CursorColumn << ")";
        }
        
        if (!m_Context.CurrentLanguage.empty()) {
            ss << "\nLanguage: " << m_Context.CurrentLanguage;
        }
        
        if (!m_Context.SelectedCode.empty()) {
            ss << "\n\nSelected code:\n```\n" << m_Context.SelectedCode << "\n```";
        }
        
        if (!m_Context.SurroundingCode.empty()) {
            ss << "\n\nSurrounding context:\n```\n" << m_Context.SurroundingCode << "\n```";
        }
        
        if (!m_Context.ErrorMessage.empty()) {
            ss << "\n\nError message: " << m_Context.ErrorMessage;
        }
        
        return ss.str();
    }

    void CodeAssistant::AddToHistory(const std::string& role, const std::string& content) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char timeStr[32];
        std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", std::localtime(&time));

        CodeChatMessage msg;
        msg.Role = role;
        msg.Content = content;
        msg.Timestamp = timeStr;
        
        m_History.push_back(msg);
        
        while (m_History.size() > MAX_HISTORY) {
            m_History.pop_front();
        }
    }

    std::string CodeAssistant::CompleteCode(const std::string& code, int cursorPos) {
        std::string prompt = "Complete the following code. Only output the completion, nothing else.\n\n";
        prompt += "Code before cursor:\n```\n" + code.substr(0, cursorPos) + "\n```\n\n";
        if (cursorPos < static_cast<int>(code.length())) {
            prompt += "Code after cursor:\n```\n" + code.substr(cursorPos) + "\n```\n\n";
        }
        prompt += "Completion:";

        return m_Ollama.Generate(prompt);
    }

    std::string CodeAssistant::ExplainCode(const std::string& code) {
        std::string prompt = "Explain what this code does in a clear and concise way:\n\n```\n" + code + "\n```";
        
        AddToHistory("user", "/explain\n" + code);
        std::string response = m_Ollama.Generate(prompt);
        AddToHistory("assistant", response);
        
        return response;
    }

    std::string CodeAssistant::FixError(const std::string& code, const std::string& error) {
        std::string prompt = "Fix the following code that has this error:\n\n";
        prompt += "Error: " + error + "\n\n";
        prompt += "Code:\n```\n" + code + "\n```\n\n";
        prompt += "Provide only the corrected code:";

        AddToHistory("user", "/fix\nError: " + error + "\n" + code);
        std::string response = m_Ollama.Generate(prompt);
        AddToHistory("assistant", response);
        
        return response;
    }

    std::string CodeAssistant::RefactorCode(const std::string& code, const std::string& instruction) {
        std::string prompt = "Refactor this code according to the instruction.\n\n";
        prompt += "Instruction: " + instruction + "\n\n";
        prompt += "Code:\n```\n" + code + "\n```\n\n";
        prompt += "Refactored code:";

        AddToHistory("user", "/refactor " + instruction + "\n" + code);
        std::string response = m_Ollama.Generate(prompt);
        AddToHistory("assistant", response);
        
        return response;
    }

    std::string CodeAssistant::GenerateCode(const std::string& description) {
        std::string prompt = "Generate code for the following requirement:\n\n";
        prompt += description + "\n\n";
        prompt += FormatContextForPrompt();
        prompt += "\n\nGenerate clean, well-structured code:";

        AddToHistory("user", "/generate " + description);
        std::string response = m_Ollama.Generate(prompt);
        AddToHistory("assistant", response);
        
        return response;
    }

    std::string CodeAssistant::OptimizeCode(const std::string& code) {
        std::string prompt = "Optimize this code for better performance while maintaining functionality:\n\n";
        prompt += "```\n" + code + "\n```\n\n";
        prompt += "Optimized code:";

        AddToHistory("user", "/optimize\n" + code);
        std::string response = m_Ollama.Generate(prompt);
        AddToHistory("assistant", response);
        
        return response;
    }

    std::string CodeAssistant::AddComments(const std::string& code) {
        std::string prompt = "Add clear, helpful comments to this code:\n\n";
        prompt += "```\n" + code + "\n```\n\n";
        prompt += "Commented code:";

        AddToHistory("user", "/comment\n" + code);
        std::string response = m_Ollama.Generate(prompt);
        AddToHistory("assistant", response);
        
        return response;
    }

    std::string CodeAssistant::ConvertCode(const std::string& code, const std::string& targetLanguage) {
        std::string prompt = "Convert this code to " + targetLanguage + ":\n\n";
        prompt += "```\n" + code + "\n```\n\n";
        prompt += targetLanguage + " code:";

        AddToHistory("user", "/convert " + targetLanguage + "\n" + code);
        std::string response = m_Ollama.Generate(prompt);
        AddToHistory("assistant", response);
        
        return response;
    }

    std::string CodeAssistant::Chat(const std::string& message) {
        // Check for commands
        std::string output;
        if (ProcessCommand(message, output)) {
            return output;
        }

        // Build conversation context
        std::vector<ChatMessage> messages;
        messages.push_back({"system", m_SystemPrompt});
        
        // Add recent history
        for (const auto& msg : m_History) {
            messages.push_back({msg.Role, msg.Content});
        }
        
        // Add current context if available
        std::string contextInfo = FormatContextForPrompt();
        std::string fullMessage = message;
        if (!contextInfo.empty()) {
            fullMessage = "Context:" + contextInfo + "\n\nQuestion: " + message;
        }
        messages.push_back({"user", fullMessage});

        AddToHistory("user", message);
        
        std::string response = m_Ollama.Chat(messages);
        
        AddToHistory("assistant", response);
        
        return response;
    }

    void CodeAssistant::ChatAsync(const std::string& message, std::function<void(const std::string&)> callback) {
        std::thread([this, message, callback]() {
            std::string response = Chat(message);
            if (callback) {
                callback(response);
            }
        }).detach();
    }

    void CodeAssistant::ClearHistory() {
        m_History.clear();
    }

    bool CodeAssistant::ProcessCommand(const std::string& input, std::string& output) {
        if (input.empty() || input[0] != '/') {
            return false;
        }

        // Parse command
        size_t spacePos = input.find(' ');
        std::string command = (spacePos != std::string::npos) ? 
            input.substr(1, spacePos - 1) : input.substr(1);
        std::string args = (spacePos != std::string::npos) ? 
            input.substr(spacePos + 1) : "";

        if (command == "explain") {
            std::string code = args.empty() ? m_Context.SelectedCode : args;
            if (code.empty()) {
                output = "No code selected. Select some code first or provide code after /explain";
                return true;
            }
            output = ExplainCode(code);
            return true;
        }
        
        if (command == "fix") {
            std::string code = args.empty() ? m_Context.SelectedCode : args;
            if (code.empty()) {
                output = "No code to fix. Select code or provide code after /fix";
                return true;
            }
            output = FixError(code, m_Context.ErrorMessage);
            return true;
        }
        
        if (command == "generate") {
            if (args.empty()) {
                output = "Please provide a description: /generate <description>";
                return true;
            }
            output = GenerateCode(args);
            return true;
        }
        
        if (command == "refactor") {
            std::string code = m_Context.SelectedCode;
            if (code.empty()) {
                output = "No code selected. Select code to refactor first.";
                return true;
            }
            output = RefactorCode(code, args.empty() ? "improve readability and maintainability" : args);
            return true;
        }
        
        if (command == "optimize") {
            std::string code = args.empty() ? m_Context.SelectedCode : args;
            if (code.empty()) {
                output = "No code to optimize.";
                return true;
            }
            output = OptimizeCode(code);
            return true;
        }
        
        if (command == "comment") {
            std::string code = args.empty() ? m_Context.SelectedCode : args;
            if (code.empty()) {
                output = "No code to comment.";
                return true;
            }
            output = AddComments(code);
            return true;
        }
        
        if (command == "convert") {
            size_t langEnd = args.find(' ');
            std::string lang = (langEnd != std::string::npos) ? args.substr(0, langEnd) : args;
            std::string code = (langEnd != std::string::npos) ? args.substr(langEnd + 1) : m_Context.SelectedCode;
            
            if (lang.empty()) {
                output = "Usage: /convert <language> [code]";
                return true;
            }
            if (code.empty()) {
                output = "No code to convert. Select code or provide it after the language.";
                return true;
            }
            output = ConvertCode(code, lang);
            return true;
        }
        
        if (command == "clear") {
            ClearHistory();
            output = "Chat history cleared.";
            return true;
        }
        
        if (command == "help") {
            output = R"(Available commands:
/explain [code] - Explain the selected or provided code
/fix [code] - Fix errors in the selected or provided code
/generate <description> - Generate code from description
/refactor [instruction] - Refactor selected code
/optimize [code] - Optimize code for performance
/comment [code] - Add comments to code
/convert <language> [code] - Convert code to another language
/clear - Clear chat history
/help - Show this help message

You can also just type questions naturally without commands.)";
            return true;
        }

        // Unknown command - let it be processed as regular chat
        return false;
    }

}
