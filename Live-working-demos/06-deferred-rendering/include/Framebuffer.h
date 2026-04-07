#pragma once

#include <cstdint>
#include <memory>

class Framebuffer {
public:
    Framebuffer(int width, int height);
    ~Framebuffer();
    
    void bind();
    void unbind();
    
    unsigned int getFBO() const { return m_fbo; }
    unsigned int getColorTexture() const { return m_colorTexture; }
    unsigned int getDepthTexture() const { return m_depthTexture; }
    
    void resize(int width, int height);

private:
    unsigned int m_fbo;
    unsigned int m_colorTexture;
    unsigned int m_depthTexture;
    int m_width;
    int m_height;
    
    void create();
    void destroy();
};

