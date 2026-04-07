#include "PhysicsPanel.hpp"
#include "../core/EditorTheme.hpp"
#include <imgui.h>

namespace editor {

PhysicsPanel::PhysicsPanel() 
    : EditorPanel("Physics Settings", true) {
    initLayerMatrix();
}

void PhysicsPanel::initLayerMatrix() {
    // Initialize all layers to collide by default
    for (int i = 0; i < MAX_LAYERS; i++) {
        for (int j = 0; j < MAX_LAYERS; j++) {
            m_layerMatrix[i][j] = true;
        }
    }
    
    // Disable some default combinations
    m_layerMatrix[6][6] = false;  // Triggers don't collide with triggers
}

void PhysicsPanel::render() {
    if (!beginPanel()) {
        endPanel();
        return;
    }
    
    // Simulation toggle
    ImGui::PushStyleColor(ImGuiCol_Button, m_simulatePhysics ? 
        ImVec4(0.2f, 0.5f, 0.3f, 1.0f) : ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
    
    if (ImGui::Button(m_simulatePhysics ? "\xef\x81\x8c Physics ON" : "\xef\x81\x8d Physics OFF", ImVec2(-1, 30))) {
        m_simulatePhysics = !m_simulatePhysics;
    }
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    
    // Tabs for different settings
    if (ImGui::BeginTabBar("PhysicsSettingsTabs")) {
        if (ImGui::BeginTabItem("World")) {
            renderWorldSettings();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Solver")) {
            renderSolverSettings();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Collision")) {
            renderCollisionSettings();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Cloth")) {
            renderClothSettings();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Debug")) {
            renderDebugSettings();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Layers")) {
            renderLayerMatrix();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    
    endPanel();
}

void PhysicsPanel::renderWorldSettings() {
    ImGui::Spacing();
    
    if (ImGui::CollapsingHeader("Gravity", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        
        // Quick presets
        ImGui::Text("Presets:");
        ImGui::SameLine();
        if (ImGui::SmallButton("Earth")) {
            m_gravity[0] = 0.0f; m_gravity[1] = -9.81f; m_gravity[2] = 0.0f;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Moon")) {
            m_gravity[0] = 0.0f; m_gravity[1] = -1.62f; m_gravity[2] = 0.0f;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Mars")) {
            m_gravity[0] = 0.0f; m_gravity[1] = -3.71f; m_gravity[2] = 0.0f;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Zero")) {
            m_gravity[0] = 0.0f; m_gravity[1] = 0.0f; m_gravity[2] = 0.0f;
        }
        
        ImGui::Spacing();
        
        ImGui::DragFloat3("Gravity Vector", m_gravity, 0.1f, -100.0f, 100.0f, "%.2f m/s²");
        
        ImGui::Unindent();
    }
    
    if (ImGui::CollapsingHeader("Time Step", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        
        float fps = 1.0f / m_fixedTimeStep;
        if (ImGui::SliderFloat("Physics FPS", &fps, 30.0f, 240.0f, "%.0f Hz")) {
            m_fixedTimeStep = 1.0f / fps;
        }
        
        ImGui::DragFloat("Fixed Time Step", &m_fixedTimeStep, 0.001f, 0.001f, 0.1f, "%.4f s");
        ImGui::DragInt("Max Sub Steps", &m_maxSubSteps, 1, 1, 20);
        
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Maximum physics steps per frame.\nHigher values improve stability at low framerates.");
        }
        
        ImGui::Unindent();
    }
    
    if (ImGui::CollapsingHeader("Sleeping", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        
        static bool enableSleeping = true;
        ImGui::Checkbox("Enable Sleeping", &enableSleeping);
        
        if (enableSleeping) {
            static float sleepThreshold = 0.01f;
            static float sleepTime = 2.0f;
            
            ImGui::DragFloat("Sleep Velocity Threshold", &sleepThreshold, 0.001f, 0.0f, 1.0f, "%.4f");
            ImGui::DragFloat("Time Before Sleep", &sleepTime, 0.1f, 0.1f, 10.0f, "%.1f s");
        }
        
        ImGui::Unindent();
    }
}

void PhysicsPanel::renderSolverSettings() {
    ImGui::Spacing();
    
    if (ImGui::CollapsingHeader("Iteration Counts", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        
        ImGui::SliderInt("Velocity Iterations", &m_velocityIterations, 1, 20);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Number of velocity solver iterations.\nHigher = more accurate, but slower.");
        }
        
        ImGui::SliderInt("Position Iterations", &m_positionIterations, 1, 10);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Number of position correction iterations.\nHigher = less penetration, but more jitter.");
        }
        
        // Presets
        ImGui::Spacing();
        ImGui::Text("Quality Presets:");
        if (ImGui::Button("Low", ImVec2(60, 0))) {
            m_velocityIterations = 4;
            m_positionIterations = 1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Medium", ImVec2(60, 0))) {
            m_velocityIterations = 8;
            m_positionIterations = 3;
        }
        ImGui::SameLine();
        if (ImGui::Button("High", ImVec2(60, 0))) {
            m_velocityIterations = 12;
            m_positionIterations = 5;
        }
        ImGui::SameLine();
        if (ImGui::Button("Ultra", ImVec2(60, 0))) {
            m_velocityIterations = 16;
            m_positionIterations = 8;
        }
        
        ImGui::Unindent();
    }
    
    if (ImGui::CollapsingHeader("Contact Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        
        ImGui::DragFloat("Baumgarte Bias", &m_contactBias, 0.01f, 0.0f, 1.0f, "%.3f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Bias factor for position correction.\n0 = no correction, 1 = full correction.");
        }
        
        ImGui::DragFloat("Allowed Penetration", &m_contactSlop, 0.001f, 0.0f, 0.1f, "%.4f m");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Small penetration allowed before correction.\nReduces jitter on resting contacts.");
        }
        
        static float restitutionThreshold = 1.0f;
        ImGui::DragFloat("Bounce Threshold", &restitutionThreshold, 0.1f, 0.0f, 10.0f, "%.1f m/s");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Minimum velocity for bounce effects.\nBelow this, objects come to rest.");
        }
        
        ImGui::Unindent();
    }
    
    if (ImGui::CollapsingHeader("Warm Starting")) {
        ImGui::Indent();
        
        static bool enableWarmStarting = true;
        ImGui::Checkbox("Enable Warm Starting", &enableWarmStarting);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Reuse constraint solutions from previous frame.\nImproves stability and convergence.");
        }
        
        if (enableWarmStarting) {
            static float warmStartFactor = 0.85f;
            ImGui::SliderFloat("Warm Start Factor", &warmStartFactor, 0.0f, 1.0f, "%.2f");
        }
        
        ImGui::Unindent();
    }
}

void PhysicsPanel::renderCollisionSettings() {
    ImGui::Spacing();
    
    if (ImGui::CollapsingHeader("Broad Phase", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        
        const char* broadPhaseTypes[] = { "AABB Tree", "Spatial Hash", "Sweep and Prune" };
        ImGui::Combo("Algorithm", &m_broadPhaseType, broadPhaseTypes, IM_ARRAYSIZE(broadPhaseTypes));
        
        if (m_broadPhaseType == 1) {  // Spatial Hash
            ImGui::DragFloat("Cell Size", &m_spatialHashCellSize, 0.1f, 0.1f, 10.0f, "%.2f");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Size of hash grid cells.\nShould be ~2x the average object size.");
            }
        }
        
        ImGui::Unindent();
    }
    
    if (ImGui::CollapsingHeader("Continuous Collision", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        
        ImGui::Checkbox("Enable CCD", &m_enableCCD);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Continuous Collision Detection.\nPrevents fast-moving objects from tunneling.");
        }
        
        if (m_enableCCD) {
            ImGui::DragFloat("CCD Threshold", &m_ccdThreshold, 0.001f, 0.0f, 1.0f, "%.4f");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Motion threshold for CCD activation.\nLower = more accurate, but slower.");
            }
        }
        
        ImGui::Unindent();
    }
    
    if (ImGui::CollapsingHeader("Contact Generation")) {
        ImGui::Indent();
        
        static int maxContactPoints = 4;
        ImGui::SliderInt("Max Contact Points", &maxContactPoints, 1, 8);
        
        static float contactTolerance = 0.001f;
        ImGui::DragFloat("Contact Tolerance", &contactTolerance, 0.0001f, 0.0f, 0.01f, "%.5f");
        
        ImGui::Unindent();
    }
}

void PhysicsPanel::renderClothSettings() {
    ImGui::Spacing();
    
    if (ImGui::CollapsingHeader("Simulation Quality", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        
        ImGui::SliderInt("Substeps", &m_clothSubsteps, 1, 16);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("More substeps = more stable cloth.\nRecommended: 4-8 for real-time.");
        }
        
        ImGui::Unindent();
    }
    
    if (ImGui::CollapsingHeader("Spring Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        
        ImGui::SliderFloat("Stiffness", &m_clothStiffness, 0.0f, 1.0f, "%.3f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("How rigid the cloth is.\n1.0 = very stiff, 0.0 = very stretchy.");
        }
        
        ImGui::SliderFloat("Damping", &m_clothDamping, 0.0f, 0.1f, "%.4f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Energy dissipation.\nHigher = cloth settles faster.");
        }
        
        // Presets
        ImGui::Spacing();
        ImGui::Text("Material Presets:");
        if (ImGui::Button("Silk", ImVec2(50, 0))) {
            m_clothStiffness = 0.95f;
            m_clothDamping = 0.005f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cotton", ImVec2(50, 0))) {
            m_clothStiffness = 0.85f;
            m_clothDamping = 0.02f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Leather", ImVec2(50, 0))) {
            m_clothStiffness = 0.99f;
            m_clothDamping = 0.05f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Rubber", ImVec2(50, 0))) {
            m_clothStiffness = 0.5f;
            m_clothDamping = 0.08f;
        }
        
        ImGui::Unindent();
    }
    
    if (ImGui::CollapsingHeader("Collision", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        
        ImGui::Checkbox("Self Collision", &m_clothSelfCollision);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enable cloth-to-cloth collision.\nExpensive but prevents clipping.");
        }
        
        static float collisionDistance = 0.01f;
        ImGui::DragFloat("Collision Distance", &collisionDistance, 0.001f, 0.001f, 0.1f, "%.4f");
        
        static float friction = 0.5f;
        ImGui::SliderFloat("Friction", &friction, 0.0f, 1.0f, "%.2f");
        
        ImGui::Unindent();
    }
    
    if (ImGui::CollapsingHeader("Wind")) {
        ImGui::Indent();
        
        static bool enableWind = false;
        ImGui::Checkbox("Enable Wind", &enableWind);
        
        if (enableWind) {
            static float windDir[3] = {1.0f, 0.0f, 0.0f};
            static float windStrength = 5.0f;
            static float windTurbulence = 0.3f;
            
            ImGui::DragFloat3("Direction", windDir, 0.1f);
            ImGui::DragFloat("Strength", &windStrength, 0.1f, 0.0f, 50.0f, "%.1f");
            ImGui::SliderFloat("Turbulence", &windTurbulence, 0.0f, 1.0f, "%.2f");
        }
        
        ImGui::Unindent();
    }
}

void PhysicsPanel::renderDebugSettings() {
    ImGui::Spacing();
    
    if (ImGui::CollapsingHeader("Visualization", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        
        ImGui::Checkbox("Show Colliders", &m_showColliders);
        ImGui::Checkbox("Show Contact Points", &m_showContactPoints);
        ImGui::Checkbox("Show AABBs", &m_showAABBs);
        ImGui::Checkbox("Show Velocities", &m_showVelocities);
        ImGui::Checkbox("Show Constraints", &m_showConstraints);
        
        ImGui::Spacing();
        
        if (ImGui::Button("Enable All", ImVec2(100, 0))) {
            m_showColliders = m_showContactPoints = m_showAABBs = 
                m_showVelocities = m_showConstraints = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Disable All", ImVec2(100, 0))) {
            m_showColliders = m_showContactPoints = m_showAABBs = 
                m_showVelocities = m_showConstraints = false;
        }
        
        ImGui::Unindent();
    }
    
    if (ImGui::CollapsingHeader("Colors")) {
        ImGui::Indent();
        
        static float colliderColor[4] = {0.0f, 1.0f, 0.0f, 0.5f};
        static float contactColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};
        static float aabbColor[4] = {1.0f, 1.0f, 0.0f, 0.3f};
        static float velocityColor[4] = {0.0f, 0.5f, 1.0f, 1.0f};
        
        ImGui::ColorEdit4("Collider Color", colliderColor);
        ImGui::ColorEdit4("Contact Color", contactColor);
        ImGui::ColorEdit4("AABB Color", aabbColor);
        ImGui::ColorEdit4("Velocity Color", velocityColor);
        
        ImGui::Unindent();
    }
    
    if (ImGui::CollapsingHeader("Statistics")) {
        ImGui::Indent();
        
        // Demo statistics
        ImGui::Text("Active Bodies: %d", 42);
        ImGui::Text("Sleeping Bodies: %d", 15);
        ImGui::Text("Contact Pairs: %d", 128);
        ImGui::Text("Contact Points: %d", 356);
        ImGui::Text("Constraints: %d", 24);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text("Broad Phase Time: %.3f ms", 0.12f);
        ImGui::Text("Narrow Phase Time: %.3f ms", 0.45f);
        ImGui::Text("Solver Time: %.3f ms", 0.82f);
        ImGui::Text("Integration Time: %.3f ms", 0.15f);
        ImGui::Text("Total Physics Time: %.3f ms", 1.54f);
        
        ImGui::Unindent();
    }
}

void PhysicsPanel::renderLayerMatrix() {
    ImGui::Spacing();
    ImGui::Text("Layer Collision Matrix");
    ImGui::Separator();
    ImGui::Spacing();
    
    // Only show first 8 layers for visibility
    int displayLayers = 8;
    
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2, 2));
    
    if (ImGui::BeginTable("LayerMatrix", displayLayers + 1, ImGuiTableFlags_Borders)) {
        // Header row
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80);
        for (int i = 0; i < displayLayers; i++) {
            ImGui::TableSetupColumn(m_layerNames[i], ImGuiTableColumnFlags_WidthFixed, 20);
        }
        
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("");
        
        for (int i = 0; i < displayLayers; i++) {
            ImGui::TableSetColumnIndex(i + 1);
            ImGui::PushID(i);
            // Rotated text would be ideal here, but we'll use abbreviated
            ImGui::Text("%d", i);
            ImGui::PopID();
        }
        
        // Matrix rows
        for (int i = 0; i < displayLayers; i++) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", m_layerNames[i]);
            
            for (int j = 0; j < displayLayers; j++) {
                ImGui::TableSetColumnIndex(j + 1);
                
                if (j > i) {
                    // Above diagonal - disabled (symmetric)
                    ImGui::BeginDisabled();
                    ImGui::Checkbox(("##m" + std::to_string(i) + "_" + std::to_string(j)).c_str(), &m_layerMatrix[i][j]);
                    ImGui::EndDisabled();
                } else {
                    ImGui::PushID(i * MAX_LAYERS + j);
                    if (ImGui::Checkbox("##", &m_layerMatrix[i][j])) {
                        // Keep symmetric
                        m_layerMatrix[j][i] = m_layerMatrix[i][j];
                    }
                    ImGui::PopID();
                }
            }
        }
        
        ImGui::EndTable();
    }
    
    ImGui::PopStyleVar();
    
    ImGui::Spacing();
    
    if (ImGui::Button("Enable All")) {
        for (int i = 0; i < MAX_LAYERS; i++) {
            for (int j = 0; j < MAX_LAYERS; j++) {
                m_layerMatrix[i][j] = true;
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Disable All")) {
        for (int i = 0; i < MAX_LAYERS; i++) {
            for (int j = 0; j < MAX_LAYERS; j++) {
                m_layerMatrix[i][j] = false;
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Default")) {
        initLayerMatrix();
    }
}

void PhysicsPanel::update(float) {
    // Update from actual physics world if connected
}

void PhysicsPanel::setPhysicsWorld(std::shared_ptr<engine::physics::PhysicsWorld> world) {
    m_physicsWorld = world;
}

} // namespace editor

