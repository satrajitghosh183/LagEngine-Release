// main3D.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/graphics/Shader.hpp"
#include "engine/scene/SceneManager3D.hpp"
#include "engine/objects/Ball3D.hpp"
#include "engine/physics/ClothSolver3D.hpp"
#include "engine/core/Logger.hpp"
#include "engine/core/Time.hpp"

#include <memory>

using namespace engine;
using namespace engine::core::log;

const unsigned int SCR_WIDTH  = 1280;
const unsigned int SCR_HEIGHT = 720;

// Globals for timing
float deltaTime = 0.0f, lastFrame = 0.0f;

// Scene manager
std::shared_ptr<scene::SceneManager3D> sceneManager;

// GLFW callbacks (resize & simple WASD + ESC)
void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
    glViewport(0, 0, w, h);
}
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main() {
    Logger::log("Starting 3D Engine...", LogLevel::Info);

    // ——— Init GLFW + GLAD —————————————————————————
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
                                           "3D Cloth + Balls", nullptr, nullptr);
    if (!window) {
        Logger::log("GLFW window creation failed", LogLevel::Error);
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        Logger::log("GLAD init failed", LogLevel::Error);
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // ——— Load a simple “pass‐through” shader (we only use it to render cloth+balls) —————————————————————
    // WRONG on Linux/WSL (and in any C++ string literal):
    graphics::Shader shader("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");
    
    Logger::log("Shaders compiled.", LogLevel::Info);

    // ——— Create the 3D scene (this already spawns one cloth + one ball for you) ————————————————
    sceneManager = std::make_shared<scene::SceneManager3D>(
        float(SCR_WIDTH) / float(SCR_HEIGHT)
    );
    Logger::log("SceneManager3D created.", LogLevel::Info);

    // Grab a shortcut to the physics world
    auto phys = sceneManager->getPhysicsWorld();

    {
        // ClothSolver3D takes (VertletSystem3D&, width, height, particleSpacing)
        auto extraCloth = std::make_shared<physics::ClothSolver3D>(
            *phys,        // physics world
            20, 15,       // 20×15 grid
            0.2f          // spacing = 0.2
        );
        // position it a bit to the right and above
        extraCloth->createCloth(
            glm::vec3(1.0f, 2.5f, 0.0f),    // origin
            glm::vec3(1,0,0),               // “right” direction
            glm::vec3(0,-1,0)               // “down” direction
        );
        // register it:
        phys->addCloth(extraCloth);
    }

    // ——— Add two more balls ——————————————————————————————————————————————————
    {
        auto b1 = std::make_shared<objects::Ball3D>(
            glm::vec3(-1.0f, 4.0f, 0.0f),  // start position
            0.3f,                          // radius
            1.0f                           // mass
        );
        // give it a little push:
        b1->setVelocity(glm::vec3(2.0f, 0.0f, -1.0f));
        phys->addBall(b1);

        auto b2 = std::make_shared<objects::Ball3D>(
            glm::vec3( 1.0f, 4.0f, 0.0f),  // start position
            0.4f,                          // radius
            2.0f                           // mass
        );
        // let gravity do its work (we’ll also pin it if you like)
        b2->applyForce(glm::vec3(0.0f, -9.81f * b2->getMass(), 0.0f));
        phys->addBall(b2);
    }

    // ——— Main loop ——————————————————————————————————————————————————————————————
    while (!glfwWindowShouldClose(window)) {
        // timing
        float now = (float)glfwGetTime();
        deltaTime = now - lastFrame;
        lastFrame = now;

        processInput(window);

        // 1) step simulation (cloth + balls + any other physics)
        sceneManager->update(deltaTime);

        // 2) draw
        glClearColor(0.1f,0.1f,0.15f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.bind();
        sceneManager->render(shader);
        shader.unbind();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
