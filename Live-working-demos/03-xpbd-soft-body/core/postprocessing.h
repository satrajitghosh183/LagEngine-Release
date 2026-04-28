#ifndef FEATHERGL_POSTPROCESSING_H
#define FEATHERGL_POSTPROCESSING_H

#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>
#include "Shader.h"
#include "Engine.h"
#include "Observable.h"

// Vulkan port: PostProcessing is a stub. Full-screen post-processing
// requires additional Vulkan render passes and framebuffer attachments.
// The API is retained so the rest of the code compiles.

class PostProcessing {
public:
    explicit PostProcessing(const char *shaderFolder, Engine *engine) {
        _shader = std::make_shared<Shader>(shaderFolder);
    }

    void RenderTo(unsigned int targetFrameBuffer) {
        // No-op in Vulkan port
        (void)targetFrameBuffer;
    }

    void RenderToScreen() {
        // No-op
    }

    void setFBO(unsigned int FBO) {
        // No-op
        (void)FBO;
    }

    unsigned int getFBO() const {
        return 0;
    }

    void resize(int width, int height) {
        // No-op
        (void)width;
        (void)height;
    }

    std::shared_ptr<Shader> shader() {
        return _shader;
    }

    Observable<> onBeforeRenderObservable;

    ~PostProcessing() = default;

private:
    std::shared_ptr<Shader> _shader;
};

#endif //FEATHERGL_POSTPROCESSING_H
