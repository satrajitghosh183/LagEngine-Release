#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Core/Logger.hpp"
#include "../../Engine/Graphics/Shader.hpp"
#include "../../Engine/Graphics/VertexArray.hpp"
#include "../../Engine/Graphics/RenderCommand.hpp"
#include "../../Engine/Core/Time.hpp"
#include <imgui.h>
#include <glad/glad.h>

using namespace GameEngine;

class HelloTriangleApp : public Application {
public:
    HelloTriangleApp() : Application("Hello Triangle") {}
    
protected:
    void OnInit() override {
        
        // Create shader
        const char* vertexSrc = R"(
            #version 420 core
            layout(location = 0) in vec3 a_Position;
            layout(location = 1) in vec3 a_Color;
            
            out vec3 v_Color;
            
            void main() {
                v_Color = a_Color;
                gl_Position = vec4(a_Position, 1.0);
            }
        )";
        
        const char* fragmentSrc = R"(
            #version 420 core
            layout(location = 0) out vec4 FragColor;
            
            in vec3 v_Color;
            
            void main() {
                FragColor = vec4(v_Color, 1.0);
            }
        )";
        
        GE_INFO("Creating shader...");
        m_Shader = CreateRef<Shader>("Triangle", vertexSrc, fragmentSrc);
        GE_INFO("Shader created, creating vertex array...");

        // Create triangle
        float vertices[] = {
            // Position         // Color
            -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
             0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
             0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f
        };
        
        m_VertexArray = CreateRef<VertexArray>();
        auto vb = CreateRef<VertexBuffer>(vertices, sizeof(vertices));
        vb->SetLayout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float3, "a_Color" }
        });
        m_VertexArray->AddVertexBuffer(vb);
        
        GE_INFO("Hello Triangle initialized!");
    }
    
    void OnUpdate(float deltaTime) override {
        // Rotate triangle
        m_Rotation += 90.0f * deltaTime;
    }
    
    void OnRender() override {
        RenderCommand::SetClearColor({0.1f, 0.1f, 0.1f, 1.0f});
        RenderCommand::Clear();
        
        m_Shader->Bind();
        m_VertexArray->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        // Simple ImGui rendering (Note: UI needs proper initialization)
        // For now, just render the triangle without UI
    }
    
    void OnShutdown() override {
    }
    
private:
    Ref<Shader> m_Shader;
    Ref<VertexArray> m_VertexArray;
    float m_Rotation = 0.0f;
    float m_Speed = 90.0f;
};

GameEngine::Application* GameEngine::CreateApplication() {
    return new HelloTriangleApp();
}

int main() {
    auto app = GameEngine::CreateApplication();
    app->Run();
    delete app;
    return 0;
}