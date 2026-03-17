#pragma once

#include "DAGScheduler.h"
#include "PipelineController.h"
#include "RenderGraph.h"
#include "AIBackend.h"
#include <memory>

struct GLFWwindow;

class UI {
public:
    UI();
    ~UI();
    
    bool initialize(GLFWwindow* window);
    void shutdown();
    
    void beginFrame();
    void endFrame();
    
    void render(DAGScheduler* scheduler, PipelineController* controller, 
                RenderGraph* graph, float frameTime, float fps);

private:
    bool m_initialized;
    
    void renderMainWindow(DAGScheduler* scheduler, PipelineController* controller, 
                         RenderGraph* graph, float frameTime, float fps);
    void renderTimeline(DAGScheduler* scheduler);
    void renderPerPassChart(DAGScheduler* scheduler);
    void renderUtilizationBars(DAGScheduler* scheduler, PipelineController* controller);
};

