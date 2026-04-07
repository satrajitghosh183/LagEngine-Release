#pragma once

#include "../Core/Base.hpp"
#include "../Graphics/Shader.hpp"
#include "../Graphics/Texture2D.hpp"
#include "../Graphics/Camera3D.hpp"
#include "ParticleEmitter.hpp"

namespace GameEngine {

    /**
     * @brief Particle renderer
     * 
     * GPU-instanced billboard rendering
     */
    class ParticleRenderer {
    public:
        ParticleRenderer();
        ~ParticleRenderer();
        
        /**
         * @brief Render particles
         */
        void Render(const ParticleEmitter& emitter, const Camera3D& camera, 
                   const Ref<Texture2D>& texture = nullptr);
        
    private:
        void InitBuffers();
        
    private:
        Ref<Shader> m_Shader;
        uint32_t m_VAO;
        uint32_t m_VBO;
        uint32_t m_InstanceVBO;
    };
}