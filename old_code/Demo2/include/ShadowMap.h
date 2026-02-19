#pragma once

#include <cstdint>
#include <memory>
#include <glm/glm.hpp>

class ShadowMap {
public:
    ShadowMap(int width = 2048, int height = 2048);
    ~ShadowMap();
    
    void bindForWriting();
    void unbind();
    void bindForReading(unsigned int textureUnit = 0);
    
    glm::mat4 getLightSpaceMatrix(const glm::vec3& lightPos, const glm::vec3& lightDir, float size = 20.0f);
    
    unsigned int getDepthTexture() const { return m_depthTexture; }
    unsigned int getFBO() const { return m_fbo; }

private:
    unsigned int m_fbo;
    unsigned int m_depthTexture;
    int m_width;
    int m_height;
};

