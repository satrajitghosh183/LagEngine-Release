#include "DemoScene.h"
#include "Shader.h"
#include "Mesh.h"
#include "Framebuffer.h"
#include "ShadowMap.h"
#include "GBuffer.h"
#include "SSAO.h"
#include "ParticleSystem.h"
#include "Camera.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>

// Forward declaration
extern void renderFullscreenQuad();

DemoScene::DemoScene(int width, int height)
    : m_width(width)
    , m_height(height)
    , m_rotationAngle(0.0f)
    , m_lightRotation(0.0f)
{
    m_camera.setAspect((float)width / (float)height);
    initialize();
}

DemoScene::~DemoScene() {
}

void DemoScene::initialize() {
    createRenderGraph();
    setupMeshes();
    setupShaders();
    
    m_shadowMap = std::make_shared<ShadowMap>(2048, 2048);
    m_gbuffer = std::make_shared<GBuffer>(m_width, m_height);
    m_ssao = std::make_shared<SSAO>(m_width, m_height);
    m_ssao->initialize();
    // Increase particle count significantly for more GPU load
    m_particles = std::make_shared<ParticleSystem>(50000);  // 50k particles - challenging but still visible!
    m_particles->initialize();
    
    m_outputFBO = std::make_shared<Framebuffer>(m_width, m_height);
}

void DemoScene::createRenderGraph() {
    m_renderGraph = std::make_shared<RenderGraph>();
    
    // Create tasks with executor types for three-tier parallelization
    // OpenGL tasks: Main thread only (ShadowPass, GBufferPass, LightingPass, PostFXPass)
    // CUDA tasks: Parallel streams (CudaSSAO, CudaParticles)
    // CPU tasks: Worker threads (FrustumCulling, MeshUpdate, PhysicsUpdate) - enables work stealing!
    auto shadowTask = m_renderGraph->addTask(TaskType::ShadowPass, "ShadowPass", 2.0f, TaskExecutor::OpenGL_MainThread);
    auto gbufferTask = m_renderGraph->addTask(TaskType::GBufferPass, "GBufferPass", 5.0f, TaskExecutor::OpenGL_MainThread);
    auto ssaoTask = m_renderGraph->addTask(TaskType::CudaSSAO, "CudaSSAO", 1.5f, TaskExecutor::CUDA_Stream);
    auto particleTask = m_renderGraph->addTask(TaskType::CudaParticles, "CudaParticles", 8.0f, TaskExecutor::CUDA_Stream);
    auto lightingTask = m_renderGraph->addTask(TaskType::LightingPass, "LightingPass", 3.0f, TaskExecutor::OpenGL_MainThread);
    auto postfxTask = m_renderGraph->addTask(TaskType::PostFXPass, "PostFXPass", 1.0f, TaskExecutor::OpenGL_MainThread);
    
    // CPU tasks for work stealing - these run in parallel on worker threads
    auto cullingTask = m_renderGraph->addTask(TaskType::FrustumCulling, "FrustumCulling", 3.0f, TaskExecutor::CPU_Worker);
    auto meshUpdateTask = m_renderGraph->addTask(TaskType::MeshUpdate, "MeshUpdate", 2.5f, TaskExecutor::CPU_Worker);
    auto physicsTask = m_renderGraph->addTask(TaskType::PhysicsUpdate, "PhysicsUpdate", 4.0f, TaskExecutor::CPU_Worker);
    
    // Set execute functions
    shadowTask->setExecuteFunc([this]() { this->renderShadowPass(); });
    gbufferTask->setExecuteFunc([this]() { this->renderGBufferPass(); });
    ssaoTask->setExecuteFunc([this]() { 
        // CUDA SSAO processing - placeholder for now
        // Could add actual SSAO processing here if needed
    });
    particleTask->setExecuteFunc([this, particleTask]() { 
        // Update particles - make sure deltaTime is valid
        // Get the CUDA stream assigned to this task
        if (m_particles && m_deltaTime > 0.0f && m_deltaTime < 1.0f) {
            // Get the CUDA stream pointer from the task
            void* streamPtr = particleTask->getCudaStreamPtr();
            m_particles->update(m_deltaTime, m_camera.getViewMatrix(), streamPtr);
        }
    });
    lightingTask->setExecuteFunc([this]() { this->renderLightingPass(); });
    postfxTask->setExecuteFunc([this]() { this->renderPostFXPass(); });
    
    // CPU task execute functions
    cullingTask->setExecuteFunc([this]() { this->performFrustumCulling(); });
    meshUpdateTask->setExecuteFunc([this]() { this->updateMeshes(); });
    physicsTask->setExecuteFunc([this]() { this->updatePhysics(); });
    
    // Set GL state for work stealing
    GLState shadowState;
    shadowState.shaderID = m_shadowShader ? m_shadowShader->getID() : 0;
    shadowState.fboID = m_shadowMap ? m_shadowMap->getFBO() : 0;
    shadowTask->setGLState(shadowState);
    
    GLState gbufferState;
    gbufferState.shaderID = m_gbufferShader ? m_gbufferShader->getID() : 0;
    gbufferState.fboID = m_gbuffer ? m_gbuffer->getFBO() : 0;
    gbufferTask->setGLState(gbufferState);
    
    // Build dependencies
    // CPU tasks can run in parallel with early passes
    m_renderGraph->addDependency(cullingTask, gbufferTask);  // Culling before rendering
    m_renderGraph->addDependency(meshUpdateTask, gbufferTask);  // Mesh updates before rendering
    m_renderGraph->addDependency(physicsTask, gbufferTask);  // Physics before rendering
    
    // Rendering dependencies
    m_renderGraph->addDependency(shadowTask, lightingTask);
    m_renderGraph->addDependency(gbufferTask, lightingTask);
    m_renderGraph->addDependency(gbufferTask, ssaoTask);
    m_renderGraph->addDependency(lightingTask, postfxTask);
    m_renderGraph->addDependency(ssaoTask, postfxTask);
    m_renderGraph->addDependency(particleTask, postfxTask);
    
    // Build graph
    if (!m_renderGraph->build()) {
        std::cerr << "Failed to build render graph" << std::endl;
    }
}

void DemoScene::setupMeshes() {
    m_cubeMesh = createCubeMesh();
    m_sphereMesh = createSphereMesh(32);
}

void DemoScene::setupShaders() {
    // Shadow shader
    m_shadowShader = std::make_shared<Shader>();
    if (!m_shadowShader->loadFromFiles("shaders/shadow.vert", "shaders/shadow.frag")) {
        std::cerr << "ERROR: Failed to load shadow shader" << std::endl;
    } else {
        std::cout << "✓ Shadow shader loaded successfully" << std::endl;
    }
    
    // G-buffer shader
    m_gbufferShader = std::make_shared<Shader>();
    if (!m_gbufferShader->loadFromFiles("shaders/gbuffer.vert", "shaders/gbuffer.frag")) {
        std::cerr << "ERROR: Failed to load G-buffer shader" << std::endl;
    } else {
        std::cout << "✓ G-buffer shader loaded successfully" << std::endl;
    }
    
    // Lighting shader
    m_lightingShader = std::make_shared<Shader>();
    if (!m_lightingShader->loadFromFiles("shaders/lighting.vert", "shaders/lighting.frag")) {
        std::cerr << "ERROR: Failed to load lighting shader" << std::endl;
    } else {
        std::cout << "✓ Lighting shader loaded successfully" << std::endl;
    }
    
    // PostFX shader
    m_postFXShader = std::make_shared<Shader>();
    if (!m_postFXShader->loadFromFiles("shaders/postfx.vert", "shaders/postfx.frag")) {
        std::cerr << "ERROR: Failed to load post-FX shader" << std::endl;
    } else {
        std::cout << "✓ PostFX shader loaded successfully" << std::endl;
    }
}

void DemoScene::resize(int width, int height) {
    m_width = width;
    m_height = height;
    m_camera.setAspect((float)width / (float)height);
    
    if (m_gbuffer) {
        m_gbuffer->resize(width, height);
    }
    if (m_ssao) {
        m_ssao->resize(width, height);
    }
    if (m_outputFBO) {
        m_outputFBO->resize(width, height);
    }
}

void DemoScene::update(float deltaTime) {
    // Clamp deltaTime to prevent issues
    deltaTime = std::max(0.0f, std::min(deltaTime, 0.1f));
    m_deltaTime = deltaTime;
    m_rotationAngle += deltaTime * 30.0f;
    m_lightRotation += deltaTime * 20.0f;
    m_camera.update(deltaTime);
}

void DemoScene::renderShadowPass() {
    m_shadowMap->bindForWriting();
    glClear(GL_DEPTH_BUFFER_BIT);
    
    if (!m_shadowShader) return;
    
    m_shadowShader->use();
    
    glm::vec3 lightPos = glm::vec3(
        cos(glm::radians(m_lightRotation)) * 10.0f,
        10.0f,
        sin(glm::radians(m_lightRotation)) * 10.0f
    );
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.0f) - lightPos);
    glm::mat4 lightSpaceMatrix = m_shadowMap->getLightSpaceMatrix(lightPos, lightDir, 20.0f);
    
    m_shadowShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(10.0f, 0.5f, 10.0f));
    m_shadowShader->setMat4("model", model);
    if (m_cubeMesh) {
        m_cubeMesh->render();
    }
    
    // Render rotating cubes - match GBuffer count
    for (int i = 0; i < 10; ++i) {
        float angle = m_rotationAngle + i * 36.0f;
        float radius = 3.0f + (i % 3) * 2.0f;
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(cos(glm::radians(angle)) * radius, 2.0f + (i % 2) * 2.0f, sin(glm::radians(angle)) * radius));
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.8f + (i % 3) * 0.2f));
        m_shadowShader->setMat4("model", model);
        if (m_cubeMesh) {
            m_cubeMesh->render();
        }
    }
    
    m_shadowMap->unbind();
}

void DemoScene::renderGBufferPass() {
    m_gbuffer->bindForWriting();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (!m_gbufferShader) return;
    
    m_gbufferShader->use();
    m_gbufferShader->setMat4("view", m_camera.getViewMatrix());
    m_gbufferShader->setMat4("projection", m_camera.getProjectionMatrix());
    
    // Floor
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(10.0f, 0.5f, 10.0f));
    m_gbufferShader->setMat4("model", model);
    if (m_cubeMesh) {
        m_cubeMesh->render();
    }
    
    // Rotating cubes - increase count for more geometry
    for (int i = 0; i < 10; ++i) {  // 10 cubes instead of 3
        float angle = m_rotationAngle + i * 36.0f;
        float radius = 3.0f + (i % 3) * 2.0f;
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(cos(glm::radians(angle)) * radius, 2.0f + (i % 2) * 2.0f, sin(glm::radians(angle)) * radius));
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.8f + (i % 3) * 0.2f));
        m_gbufferShader->setMat4("model", model);
        if (m_cubeMesh) {
            m_cubeMesh->render();
        }
    }
    
    // Central sphere
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f));
    m_gbufferShader->setMat4("model", model);
    if (m_sphereMesh) {
        m_sphereMesh->render();
    }
    
    m_gbuffer->unbind();
}

void DemoScene::renderLightingPass() {
    m_outputFBO->bind();
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f); // Dark blue background instead of black
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (!m_lightingShader) {
        std::cerr << "Warning: Lighting shader not loaded!" << std::endl;
        m_outputFBO->unbind();
        return;
    }
    
    m_lightingShader->use();
    
    // Bind G-buffer textures
    m_gbuffer->bindForReading();
    
    // Bind shadow map
    m_shadowMap->bindForReading(3);
    
    m_lightingShader->setInt("gPosition", 0);
    m_lightingShader->setInt("gNormal", 1);
    m_lightingShader->setInt("gAlbedo", 2);
    m_lightingShader->setInt("shadowMap", 3);
    
    glm::vec3 lightPos = glm::vec3(
        cos(glm::radians(m_lightRotation)) * 10.0f,
        10.0f,
        sin(glm::radians(m_lightRotation)) * 10.0f
    );
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.0f) - lightPos);
    glm::mat4 lightSpaceMatrix = m_shadowMap->getLightSpaceMatrix(lightPos, lightDir, 20.0f);
    
    m_lightingShader->setVec3("lightPos", lightPos);
    m_lightingShader->setVec3("viewPos", m_camera.getPosition());
    m_lightingShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    
    // Set viewport
    glViewport(0, 0, m_width, m_height);
    
    // Check for OpenGL errors before rendering
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error before lighting quad: " << err << std::endl;
    }
    
    // Render fullscreen quad
    renderFullscreenQuad();
    
    // Check for errors after rendering
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error after lighting quad: " << err << std::endl;
    }
    
    m_outputFBO->unbind();
}

void DemoScene::performFrustumCulling() {
    // Simulate frustum culling work - CPU intensive
    // In a real system, this would cull objects outside the camera frustum
    volatile int dummy = 0;
    for (int i = 0; i < 50000; ++i) {
        dummy += i * 2;
        dummy %= 1000;
    }
    (void)dummy;  // Suppress unused variable warning
}

void DemoScene::updateMeshes() {
    // Simulate mesh update work - CPU intensive
    // In a real system, this would update mesh transforms, animations, etc.
    volatile int dummy = 0;
    for (int i = 0; i < 40000; ++i) {
        dummy += i * 3;
        dummy %= 1000;
    }
    (void)dummy;
}

void DemoScene::updatePhysics() {
    // Simulate physics update work - CPU intensive
    // In a real system, this would update physics simulation
    volatile int dummy = 0;
    for (int i = 0; i < 60000; ++i) {
        dummy += i * 4;
        dummy %= 1000;
    }
    (void)dummy;
}

void DemoScene::renderPostFXPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    
    // Clear with dark blue background
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Precompute camera matrices so both the scene quad and particles can share them
    glm::mat4 view = m_camera.getViewMatrix();
    glm::vec3 cameraPos = m_camera.getPosition();
    glm::vec3 cameraRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
    glm::vec3 cameraUp = glm::vec3(view[0][1], view[1][1], view[2][1]);
    glm::mat4 vp = m_camera.getViewProjectionMatrix();

    // Try to render scene if available (but don't let it block particles)
    if (m_postFXShader && m_outputFBO) {
        unsigned int sceneTex = m_outputFBO->getColorTexture();
        if (sceneTex != 0) {
            m_postFXShader->use();
            
            // Bind input textures
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sceneTex);
            
            if (m_ssao && m_ssao->getResultTexture() != 0) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, m_ssao->getResultTexture());
            } else {
                // Use a white texture if SSAO is not available
                glActiveTexture(GL_TEXTURE1);
                static unsigned int whiteTexture = 0;
                if (whiteTexture == 0) {
                    glGenTextures(1, &whiteTexture);
                    glBindTexture(GL_TEXTURE_2D, whiteTexture);
                    unsigned char white[] = {255, 255, 255, 255};
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
                } else {
                    glBindTexture(GL_TEXTURE_2D, whiteTexture);
                }
            }
            
            m_postFXShader->setInt("scene", 0);
            m_postFXShader->setInt("ssao", 1);
            
            renderFullscreenQuad();
        }
    }

    // Always render particles on top of the final composite
    if (m_particles) {
        try {
            // Always render particles - they should be visible
            m_particles->render(vp, cameraPos, cameraRight, cameraUp);
            
            // Debug output
            static int frameCount = 0;
            if (frameCount++ < 3) {
                std::cout << "Rendering " << m_particles->getMaxParticles() << " particles" << std::endl;
                std::cout << "Camera pos: (" << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ")" << std::endl;
            }
        } catch (const std::exception& e) {
            static bool warned = false;
            if (!warned) {
                std::cerr << "Exception in particle rendering: " << e.what() << std::endl;
                warned = true;
            }
        } catch (...) {
            static bool warned = false;
            if (!warned) {
                std::cerr << "Unknown exception in particle rendering" << std::endl;
                warned = true;
            }
        }
    }
}

