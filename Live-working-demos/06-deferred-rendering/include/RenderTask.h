#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <functional>
#include <memory>

enum class TaskType {
    ShadowPass,
    GBufferPass,
    LightingPass,
    CudaSSAO,
    CudaParticles,
    PostFXPass,
    FrustumCulling,  // CPU task
    MeshUpdate,      // CPU task
    PhysicsUpdate    // CPU task
};

enum class TaskExecutor {
    OpenGL_MainThread,  // Rendering tasks (main thread only)
    CUDA_Stream,        // CUDA compute (parallel streams)
    CPU_Worker          // CPU-side work (parallel threads, no GL)
};

struct GLState {
    uint32_t shaderID = 0;
    uint32_t fboID = 0;
    std::vector<uint32_t> boundTextures;
    bool operator==(const GLState& other) const;
    float distance(const GLState& other) const; // State affinity score
};

class RenderTask {
public:
    RenderTask(TaskType type, const std::string& name, float costEstimate = 1.0f, TaskExecutor executor = TaskExecutor::OpenGL_MainThread);
    
    void addDependency(std::shared_ptr<RenderTask> dep);
    void execute();
    void setExecuteFunc(std::function<void()> func);
    
    TaskType getType() const { return m_type; }
    TaskExecutor getExecutor() const { return m_executor; }
    const std::string& getName() const { return m_name; }
    float getCostEstimate() const { return m_costEstimate; }
    const std::vector<std::weak_ptr<RenderTask>>& getDependencies() const { return m_dependencies; }
    const GLState& getGLState() const { return m_glState; }
    void setGLState(const GLState& state) { m_glState = state; }
    
    void markComplete() { m_complete = true; }
    bool isComplete() const { return m_complete; }
    void reset() { m_complete = false; }
    
    // CUDA stream assignment (for CUDA_Stream tasks)
    void setCudaStream(int streamIndex) { m_cudaStreamIndex = streamIndex; }
    int getCudaStreamIndex() const { return m_cudaStreamIndex; }
    void setCudaStreamPtr(void* streamPtr) { m_cudaStreamPtr = streamPtr; }
    void* getCudaStreamPtr() const { return m_cudaStreamPtr; }

private:
    TaskType m_type;
    TaskExecutor m_executor;
    std::string m_name;
    float m_costEstimate;
    std::vector<std::weak_ptr<RenderTask>> m_dependencies;
    GLState m_glState;
    std::function<void()> m_executeFunc;
    bool m_complete;
    int m_cudaStreamIndex;  // Which CUDA stream to use (for CUDA_Stream tasks)
    void* m_cudaStreamPtr;  // Actual CUDA stream pointer (for CUDA_Stream tasks)
};

