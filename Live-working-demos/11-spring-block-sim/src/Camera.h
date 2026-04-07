#pragma once
#include <glm/glm.hpp>

// Orbit camera: right-drag to orbit, scroll to zoom, middle-drag to pan
class Camera {
public:
    float yaw      =  45.0f;   // degrees
    float pitch    =  25.0f;   // degrees
    float distance =  12.0f;   // from target
    glm::vec3 target = {0.0f, 1.0f, 0.0f};

    float fovY  = 45.0f;
    float nearZ = 0.1f;
    float farZ  = 200.0f;

    glm::mat4 viewMatrix(float aspect) const;
    glm::mat4 projMatrix(float aspect) const;
    glm::vec3 position()               const;

    // Call on mouse drag
    void orbit(float dx, float dy);       // right-drag
    void pan  (float dx, float dy);       // middle-drag
    void zoom (float delta);              // scroll wheel
};
