#include "RenderCommand.hpp"
#include "../Core/Logger.hpp"
#include <glad/glad.h>

namespace GameEngine {

    void RenderCommand::Init() {
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        
        // Enable blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Enable MSAA
        glEnable(GL_MULTISAMPLE);
        
        // Enable face culling
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        
        GE_CORE_INFO("RenderCommand initialized");
    }

    void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        glViewport(x, y, width, height);
    }

    void RenderCommand::SetClearColor(const glm::vec4& color) {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void RenderCommand::Clear() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void RenderCommand::SetDepthTest(bool enabled) {
        if (enabled)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
    }

    void RenderCommand::SetBlending(bool enabled) {
        if (enabled)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
    }

    void RenderCommand::SetCulling(bool enabled) {
        if (enabled)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);
    }

    void RenderCommand::SetPolygonMode(PolygonMode mode) {
        switch (mode) {
            case PolygonMode::Fill:
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                break;
            case PolygonMode::Line:
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                break;
            case PolygonMode::Point:
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                break;
        }
    }

    void RenderCommand::DrawIndexed(uint32_t count, bool wireframe) {
        if (wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
        
        if (wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    void RenderCommand::DrawArrays(uint32_t count) {
        glDrawArrays(GL_TRIANGLES, 0, count);
    }
}