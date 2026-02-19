#include "ShaderAssistant.hpp"
#include "Core/Logger.hpp"

namespace GameEngine {

    ShaderAssistant::ShaderAssistant() {}

    void ShaderAssistant::SetOllamaConfig(const OllamaConfig& config) {
        m_Client.SetConfig(config);
    }

    bool ShaderAssistant::IsAvailable() {
        return m_Client.IsServerRunning();
    }

    std::vector<std::string> ShaderAssistant::GetAvailableModels() {
        return m_Client.ListModels();
    }

    std::string ShaderAssistant::BuildSystemPrompt(const ShaderContext& context) const {
        std::string prompt =
            "You are a GLSL shader expert assistant integrated into a game engine editor. "
            "The engine uses OpenGL 4.5 with GLSL version 450 core.\n\n"
            "Guidelines:\n"
            "- Write complete, compilable GLSL shaders\n"
            "- Use proper GLSL 450 core syntax\n"
            "- Include all necessary uniform declarations\n"
            "- Wrap shader code in ```glsl code blocks\n"
            "- Explain your changes clearly\n"
            "- Consider performance implications\n\n"
            "Standard engine uniforms available:\n"
            "  uniform mat4 u_Model, u_View, u_Projection;\n"
            "  uniform mat4 u_MVP;\n"
            "  uniform vec3 u_CameraPos;\n"
            "  uniform float u_Time;\n\n"
            "Standard vertex attributes:\n"
            "  layout(location=0) in vec3 a_Position;\n"
            "  layout(location=1) in vec3 a_Normal;\n"
            "  layout(location=2) in vec2 a_TexCoord;\n"
            "  layout(location=3) in vec3 a_Tangent;\n";

        if (context.LightCount > 0) {
            prompt += "\nScene has " + std::to_string(context.LightCount) + " light(s).\n";
        }
        if (context.HasShadows) {
            prompt += "Shadow mapping is enabled.\n";
        }
        if (context.DeferredMode) {
            prompt += "Engine is in DEFERRED rendering mode. G-Buffer outputs: position, normal, albedo, metallic-roughness.\n";
        }
        if (!context.ShaderName.empty()) {
            prompt += "\nCurrently editing shader: " + context.ShaderName + "\n";
        }
        if (!context.CurrentVertexShader.empty()) {
            prompt += "\nCurrent vertex shader:\n```glsl\n" + context.CurrentVertexShader + "\n```\n";
        }
        if (!context.CurrentFragmentShader.empty()) {
            prompt += "\nCurrent fragment shader:\n```glsl\n" + context.CurrentFragmentShader + "\n```\n";
        }
        if (!context.CompileError.empty()) {
            prompt += "\nCompile error:\n" + context.CompileError + "\n";
        }

        return prompt;
    }

    std::string ShaderAssistant::BuildOperationPrompt(ShaderOperation op, const std::string& source,
                                                       const std::string& extra) const {
        switch (op) {
            case ShaderOperation::Explain:
                return "Explain the following GLSL shader in detail. Describe what each section does, "
                       "the rendering technique used, and any notable patterns:\n\n```glsl\n" + source + "\n```";

            case ShaderOperation::Modify:
                return "Modify the following shader according to these instructions: " + extra +
                       "\n\nShader:\n```glsl\n" + source + "\n```\n\n"
                       "Provide the complete modified shader.";

            case ShaderOperation::Generate:
                return "Generate a complete GLSL shader (both vertex and fragment) for: " + extra +
                       "\n\nProvide both vertex and fragment shaders as complete, compilable code.";

            case ShaderOperation::FixError:
                return "Fix the compilation error in this shader.\n\nError log:\n" + extra +
                       "\n\nShader:\n```glsl\n" + source + "\n```\n\n"
                       "Provide the corrected shader.";
        }
        return "";
    }

    std::string ShaderAssistant::ExplainShader(const std::string& shaderSource) {
        ShaderContext ctx;
        std::string prompt = BuildOperationPrompt(ShaderOperation::Explain, shaderSource, "");
        return Chat(prompt, ctx);
    }

    std::string ShaderAssistant::ModifyShader(const std::string& shaderSource, const std::string& instruction) {
        ShaderContext ctx;
        ctx.CurrentFragmentShader = shaderSource;
        std::string prompt = BuildOperationPrompt(ShaderOperation::Modify, shaderSource, instruction);
        return Chat(prompt, ctx);
    }

    std::string ShaderAssistant::GenerateShader(const std::string& description) {
        ShaderContext ctx;
        std::string prompt = BuildOperationPrompt(ShaderOperation::Generate, "", description);
        return Chat(prompt, ctx);
    }

    std::string ShaderAssistant::FixShaderError(const std::string& shaderSource, const std::string& errorLog) {
        ShaderContext ctx;
        ctx.CompileError = errorLog;
        std::string prompt = BuildOperationPrompt(ShaderOperation::FixError, shaderSource, errorLog);
        return Chat(prompt, ctx);
    }

    std::string ShaderAssistant::Chat(const std::string& userMessage, const ShaderContext& context) {
        // Build system prompt with context
        std::string systemPrompt = BuildSystemPrompt(context);

        // Build full message list
        std::vector<ChatMessage> messages;
        messages.push_back({"system", systemPrompt});

        // Add history
        for (auto& msg : m_History) {
            messages.push_back(msg);
        }

        // Add current message
        messages.push_back({"user", userMessage});

        // Send to Ollama
        std::string response = m_Client.Chat(messages);

        if (!response.empty()) {
            // Store in history
            m_History.push_back({"user", userMessage});
            m_History.push_back({"assistant", response});

            // Keep history manageable (last 20 messages)
            while (m_History.size() > 20) {
                m_History.erase(m_History.begin());
            }
        }

        return response;
    }

    std::future<std::string> ShaderAssistant::ChatAsync(const std::string& userMessage, const ShaderContext& context) {
        return std::async(std::launch::async, [this, userMessage, context]() {
            return Chat(userMessage, context);
        });
    }

    void ShaderAssistant::ClearHistory() {
        m_History.clear();
    }

}
