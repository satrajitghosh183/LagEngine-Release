#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Shader.hpp"
#include "Camera3D.hpp"
#include "Cloth3D.hpp"
#include "Ball3D.hpp"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool cameraEnabled = false; // Toggle for camera movement

Camera3D camera(glm::vec3(0.0f, 50.0f, 150.0f),
                glm::vec3(0.0f, 1.0f, 0.0f),
                -90.0f, -20.0f);

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!cameraEnabled) return;

    static bool firstMouse = true;
    static float lastX = SCR_WIDTH / 2.0f;
    static float lastY = SCR_HEIGHT / 2.0f;
    
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }
    
    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);
    
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (!cameraEnabled) return;
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Toggle camera control with C key
    static bool cPressedLastFrame = false;
    bool cPressedThisFrame = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
    if (cPressedThisFrame && !cPressedLastFrame) {
        cameraEnabled = !cameraEnabled;
        std::cout << "Camera " << (cameraEnabled ? "enabled" : "disabled") << std::endl;
    }
    cPressedLastFrame = cPressedThisFrame;

    if (cameraEnabled) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.ProcessKeyboard(UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            camera.ProcessKeyboard(DOWN, deltaTime);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_CORE_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Cloth & Balls Simulation", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW!" << std::endl;
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);

    Shader shader("../shaders/vertex_shader.glsl", "../shaders/fragment_shader.glsl");

    int clothWidth = 30, clothHeight = 20;
    float spacing = 2.0f;
    Cloth3D cloth(clothWidth, clothHeight, spacing, glm::vec3(-clothWidth * spacing * 0.5f, 50.0f, 0.0f));

    std::vector<Ball3D> balls;
    int numBalls = 50;
    for (int i = 0; i < numBalls; ++i) {
        float x = -50.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 100.0f));
        float y = 10.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 40.0f));
        float z = -50.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 100.0f));
        glm::vec3 pos(x, y, z);
        float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.1415926f;
        float speed = 10.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 30.0f));
        glm::vec3 vel(std::cos(angle) * speed, std::sin(angle) * speed, std::cos(angle) * speed);
        float radius = 1.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 2.0f));
        balls.emplace_back(pos, vel, radius);
    }

    glm::vec3 gravity(0.0f, -9.81f, 0.0f);
    glm::vec3 wind(5.0f, 0.0f, 0.0f);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        glfwPollEvents();

        cloth.update(deltaTime, gravity + wind, 20);
        for (auto &ball : balls)
            ball.update(deltaTime, gravity, glm::vec3(-100.0f, 0.0f, -100.0f), glm::vec3(100.0f, 100.0f, 100.0f));

        for (auto &ball : balls) {
            for (auto &p : cloth.particles) {
                glm::vec3 diff = p.pos - ball.particle.pos;
                float dist = glm::length(diff);
                float minDist = ball.radius + 0.5f;
                if (dist < minDist && dist > 0.001f) {
                    float penetration = minDist - dist;
                    glm::vec3 correction = glm::normalize(diff) * penetration;
                    if (!p.locked)
                        p.pos += correction;
                }
            }
        }

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        glm::mat4 projection = camera.GetProjectionMatrix(static_cast<float>(SCR_WIDTH) / SCR_HEIGHT);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);

        std::vector<glm::vec3> clothVertices;
        for (const auto &p : cloth.particles)
            clothVertices.push_back(p.pos);
        unsigned int clothVAO, clothVBO;
        glGenVertexArrays(1, &clothVAO);
        glGenBuffers(1, &clothVBO);
        glBindVertexArray(clothVAO);
        glBindBuffer(GL_ARRAY_BUFFER, clothVBO);
        glBufferData(GL_ARRAY_BUFFER, clothVertices.size() * sizeof(glm::vec3), clothVertices.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        glPointSize(4.0f);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(clothVertices.size()));
        glDeleteBuffers(1, &clothVBO);
        glDeleteVertexArrays(1, &clothVAO);

        std::vector<glm::vec3> ballVertices;
        for (const auto &ball : balls)
            ballVertices.push_back(ball.particle.pos);
        unsigned int ballVAO, ballVBO;
        glGenVertexArrays(1, &ballVAO);
        glGenBuffers(1, &ballVBO);
        glBindVertexArray(ballVAO);
        glBindBuffer(GL_ARRAY_BUFFER, ballVBO);
        glBufferData(GL_ARRAY_BUFFER, ballVertices.size() * sizeof(glm::vec3), ballVertices.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        glPointSize(6.0f);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(ballVertices.size()));
        glDeleteBuffers(1, &ballVBO);
        glDeleteVertexArrays(1, &ballVAO);

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
