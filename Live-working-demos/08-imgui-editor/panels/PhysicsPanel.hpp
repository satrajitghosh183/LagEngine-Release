#pragma once

#include "EditorPanel.hpp"
#include <memory>

// Forward declarations
namespace engine::physics { class PhysicsWorld; }

namespace editor {

/**
 * @brief Physics Settings Panel - configure physics simulation parameters
 * Controls gravity, collision detection, solver iterations, etc.
 */
class PhysicsPanel : public EditorPanel {
public:
    PhysicsPanel();
    
    void render() override;
    void update(float dt) override;
    
    void setPhysicsWorld(std::shared_ptr<engine::physics::PhysicsWorld> world);

private:
    void renderWorldSettings();
    void renderSolverSettings();
    void renderCollisionSettings();
    void renderClothSettings();
    void renderDebugSettings();
    void renderLayerMatrix();
    
    std::shared_ptr<engine::physics::PhysicsWorld> m_physicsWorld;
    
    // World settings (demo values)
    float m_gravity[3] = {0.0f, -9.81f, 0.0f};
    float m_fixedTimeStep = 1.0f / 60.0f;
    int m_maxSubSteps = 10;
    bool m_simulatePhysics = true;
    
    // Solver settings
    int m_velocityIterations = 8;
    int m_positionIterations = 3;
    float m_contactBias = 0.2f;
    float m_contactSlop = 0.01f;
    
    // Collision settings
    bool m_enableCCD = true;  // Continuous Collision Detection
    float m_ccdThreshold = 0.01f;
    int m_broadPhaseType = 0;  // 0=AABB Tree, 1=Spatial Hash, 2=Sweep and Prune
    float m_spatialHashCellSize = 1.0f;
    
    // Cloth settings
    float m_clothStiffness = 0.8f;
    float m_clothDamping = 0.02f;
    int m_clothSubsteps = 4;
    bool m_clothSelfCollision = false;
    
    // Debug settings
    bool m_showColliders = false;
    bool m_showContactPoints = false;
    bool m_showAABBs = false;
    bool m_showVelocities = false;
    bool m_showConstraints = false;
    
    // Layer collision matrix (16x16)
    static constexpr int MAX_LAYERS = 16;
    bool m_layerMatrix[MAX_LAYERS][MAX_LAYERS] = {};
    const char* m_layerNames[MAX_LAYERS] = {
        "Default", "Static", "Dynamic", "Kinematic",
        "Cloth", "SoftBody", "Trigger", "Character",
        "Projectile", "Debris", "Water", "Layer11",
        "Layer12", "Layer13", "Layer14", "Layer15"
    };
    
    void initLayerMatrix();
};

} // namespace editor

