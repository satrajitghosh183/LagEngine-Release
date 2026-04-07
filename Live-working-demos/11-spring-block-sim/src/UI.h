#pragma once
#include "Physics.h"
#include "Renderer.h"
#include <functional>

// Pending actions returned by UI (avoids mutating world during ImGui draw)
struct UIActions {
    bool addBlock    = false;
    bool addSpring   = false;
    bool deleteBlock = false;   // delete selectedBlock
    bool deleteSpring= false;   // delete selectedSpring
    bool resetSim    = false;
    bool stepOnce    = false;
    // Spring-add wizard
    bool springWizardActive = false;
    int  springWizardA      = -1;  // first picked block
};

class UI {
public:
    UIActions actions;

    // Draw all ImGui panels
    void draw(PhysicsWorld& world, RenderConfig& rendCfg,
              int& selectedBlock, int& selectedSpring);

private:
    void drawSimControls(PhysicsWorld& world);
    void drawHierarchy  (const PhysicsWorld& world,
                         int& selectedBlock, int& selectedSpring);
    void drawProperties (PhysicsWorld& world,
                         int  selectedBlock, int  selectedSpring);
    void drawRenderConfig(RenderConfig& cfg);
};
