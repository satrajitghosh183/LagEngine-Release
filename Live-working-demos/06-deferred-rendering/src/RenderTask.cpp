#include "RenderTask.h"
#include <algorithm>
#include <iostream>

bool GLState::operator==(const GLState& other) const {
    if (shaderID != other.shaderID || fboID != other.fboID) {
        return false;
    }
    if (boundTextures.size() != other.boundTextures.size()) {
        return false;
    }
    for (size_t i = 0; i < boundTextures.size(); ++i) {
        if (boundTextures[i] != other.boundTextures[i]) {
            return false;
        }
    }
    return true;
}

float GLState::distance(const GLState& other) const {
    float distance = 0.0f;
    
    // Shader change cost
    if (shaderID != other.shaderID) {
        distance += 10.0f;
    }
    
    // FBO change cost
    if (fboID != other.fboID) {
        distance += 5.0f;
    }
    
    // Texture binding changes
    size_t maxTex = std::max(boundTextures.size(), other.boundTextures.size());
    size_t matching = 0;
    for (size_t i = 0; i < std::min(boundTextures.size(), other.boundTextures.size()); ++i) {
        if (boundTextures[i] == other.boundTextures[i]) {
            matching++;
        }
    }
    float textureChangeRatio = 1.0f - (float)matching / (float)maxTex;
    distance += textureChangeRatio * 2.0f * maxTex;
    
    return distance;
}

RenderTask::RenderTask(TaskType type, const std::string& name, float costEstimate, TaskExecutor executor)
    : m_type(type)
    , m_executor(executor)
    , m_name(name)
    , m_costEstimate(costEstimate)
    , m_complete(false)
    , m_cudaStreamIndex(0)
    , m_cudaStreamPtr(nullptr)
{
}

void RenderTask::addDependency(std::shared_ptr<RenderTask> dep) {
    m_dependencies.push_back(dep);
}

void RenderTask::setExecuteFunc(std::function<void()> func) {
    m_executeFunc = func;
}

void RenderTask::execute() {
    try {
        if (m_executeFunc) {
            m_executeFunc();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error executing task " << m_name << ": " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error executing task " << m_name << std::endl;
    }
    m_complete = true;
}

