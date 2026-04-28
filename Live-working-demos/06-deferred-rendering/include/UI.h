#pragma once

#include "DAGScheduler.h"
#include "PipelineController.h"
#include "RenderGraph.h"
#include "AIBackend.h"
#include <vulkan/vulkan.h>
#include <memory>

struct GLFWwindow;
namespace vkdemo { class VulkanBase; }

class UI {
public:
    UI();
    ~UI();

    bool initialize(vkdemo::VulkanBase* vkBase);
    void shutdown();

    void beginFrame();
    void endFrame(VkCommandBuffer cmd);

    void render(DAGScheduler* scheduler, PipelineController* controller,
                RenderGraph* graph, float frameTime, float fps);

private:
    vkdemo::VulkanBase* m_vkBase = nullptr;
    bool m_initialized = false;

    void renderMainWindow(DAGScheduler* scheduler, PipelineController* controller,
                         RenderGraph* graph, float frameTime, float fps);
    void renderTimeline(DAGScheduler* scheduler);
    void renderPerPassChart(DAGScheduler* scheduler);
    void renderUtilizationBars(DAGScheduler* scheduler, PipelineController* controller);
};
