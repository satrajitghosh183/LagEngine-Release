#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods.
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera3D {
public:
    // Camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // Euler Angles
    float Yaw;
    float Pitch;
    // Camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // Constructor with vectors.
    Camera3D(glm::vec3 position, glm::vec3 up, float yaw, float pitch);

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix.
    glm::mat4 GetViewMatrix() const;
    // Returns the projection matrix.
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;

    // Processes input received from any keyboard-like input system.
    void ProcessKeyboard(Camera_Movement direction, float deltaTime);
    // Processes input received from a mouse input system.
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    // Processes input received from a mouse scroll-wheel event.
    void ProcessMouseScroll(float yoffset);

private:
    // Calculates the front vector from the Camera's updated Euler Angles.
    void updateCameraVectors();
};
