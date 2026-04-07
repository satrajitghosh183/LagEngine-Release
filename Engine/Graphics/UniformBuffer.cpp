#include "UniformBuffer.hpp"
#include <glad/glad.h>

namespace GameEngine {

    UniformBuffer::~UniformBuffer() {
        Shutdown();
    }

    void UniformBuffer::Init(size_t size, uint32_t bindingSlot, bool dynamic) {
        if (m_RendererID) {
            Shutdown();
        }
        m_Size = size;
        m_BindingSlot = bindingSlot;
        glGenBuffers(1, &m_RendererID);
        glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
        glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(size), nullptr,
                     dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingSlot, m_RendererID);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void UniformBuffer::SetData(const void* data, size_t size, size_t offset) {
        if (!m_RendererID || !data) return;
        glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
        glBufferSubData(GL_UNIFORM_BUFFER, static_cast<GLintptr>(offset),
                        static_cast<GLsizeiptr>(size), data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void UniformBuffer::Bind() const {
        if (m_RendererID) {
            glBindBufferBase(GL_UNIFORM_BUFFER, m_BindingSlot, m_RendererID);
        }
    }

    void UniformBuffer::Shutdown() {
        if (m_RendererID) {
            glDeleteBuffers(1, &m_RendererID);
            m_RendererID = 0;
        }
        m_Size = 0;
    }
}
