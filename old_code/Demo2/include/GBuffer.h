#pragma once

#include <cstdint>
#include <memory>

class GBuffer {
public:
    GBuffer(int width, int height);
    ~GBuffer();
    
    void bindForWriting();
    void unbind();
    void bindForReading();
    
    void setReadBuffer(int attachment);
    
    unsigned int getFBO() const { return m_fbo; }
    unsigned int getPositionTexture() const { return m_positionTexture; }
    unsigned int getNormalTexture() const { return m_normalTexture; }
    unsigned int getAlbedoTexture() const { return m_albedoTexture; }
    unsigned int getDepthTexture() const { return m_depthTexture; }
    
    void resize(int width, int height);

private:
    unsigned int m_fbo;
    unsigned int m_positionTexture;
    unsigned int m_normalTexture;
    unsigned int m_albedoTexture;
    unsigned int m_depthTexture;
    int m_width;
    int m_height;
    
    void create();
    void destroy();
};

