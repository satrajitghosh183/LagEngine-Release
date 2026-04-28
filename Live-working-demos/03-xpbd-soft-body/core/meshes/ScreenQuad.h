//
// Created by barth on 18/10/23.
//
// Vulkan port: ScreenQuad is a stub. Not used in the Vulkan render path.
//

#ifndef FEATHERGL_SCREENQUAD_H
#define FEATHERGL_SCREENQUAD_H

#include <vector>

class ScreenQuad {
public:
    ScreenQuad() {
        // No GPU resources created in Vulkan stub
    }

    void render() {
        // No-op
    }

private:
    std::vector<int> indices = {
            0, 1, 2,
            1, 3, 2
    };
};

#endif //FEATHERGL_SCREENQUAD_H
