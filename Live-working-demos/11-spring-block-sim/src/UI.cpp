#include "UI.h"
#include <imgui.h>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// Helper: coloured label
// ─────────────────────────────────────────────────────────────────────────────
static void colLabel(const char* text, ImVec4 col = {0.4f,0.8f,1.0f,1.0f}) {
    ImGui::TextColored(col, "%s", text);
}

// ─────────────────────────────────────────────────────────────────────────────
void UI::draw(PhysicsWorld& world, RenderConfig& rendCfg,
              int& selectedBlock, int& selectedSpring) {
    actions = {};   // reset every frame

    ImGuiIO& io = ImGui::GetIO();
    float panelW = 280.0f;

    // ── Left panel: Simulation controls + Hierarchy ──────────────────────────
    ImGui::SetNextWindowPos ({0, 0}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({panelW, io.DisplaySize.y}, ImGuiCond_Always);
    ImGui::Begin("##Left", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

    colLabel("Spring & Block Simulator", {1.0f, 0.8f, 0.3f, 1.0f});
    ImGui::Separator();

    drawSimControls(world);
    ImGui::Spacing();
    ImGui::Separator();

    colLabel("Scene Hierarchy");
    drawHierarchy(world, selectedBlock, selectedSpring);

    ImGui::End();

    // ── Right panel: Properties ───────────────────────────────────────────────
    ImGui::SetNextWindowPos ({io.DisplaySize.x - panelW, 0}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({panelW, io.DisplaySize.y}, ImGuiCond_Always);
    ImGui::Begin("##Right", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

    colLabel("Properties");
    ImGui::Separator();
    drawProperties(world, selectedBlock, selectedSpring);

    ImGui::Spacing(); ImGui::Separator();
    colLabel("Render Settings");
    drawRenderConfig(rendCfg);

    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────────────────
void UI::drawSimControls(PhysicsWorld& world) {
    colLabel("Simulation");

    // Play / Pause
    if (world.running) {
        if (ImGui::Button("  Pause  ")) world.running = false;
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f,0.7f,0.2f,1.0f));
        if (ImGui::Button("  Play   ")) world.running = true;
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::Button("Step")) actions.stepOnce = true;
    }

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f,0.2f,0.2f,1.0f));
    if (ImGui::Button("Reset")) actions.resetSim = true;
    ImGui::PopStyleColor();

    ImGui::SliderFloat("Time Scale",  &world.timeScale, 0.01f, 4.0f);
    ImGui::SliderInt  ("Sub-steps",   &world.subSteps,   1,    20);

    glm::vec3 g = world.gravity;
    float gY = g.y;
    if (ImGui::SliderFloat("Gravity Y", &gY, -30.0f, 0.0f))
        world.gravity.y = gY;

    ImGui::Spacing();
    colLabel("Add Objects");
    if (ImGui::Button("+ Block"))  actions.addBlock  = true;
    ImGui::SameLine();
    if (ImGui::Button("+ Spring")) actions.addSpring = true;
}

// ─────────────────────────────────────────────────────────────────────────────
void UI::drawHierarchy(const PhysicsWorld& world,
                       int& selectedBlock, int& selectedSpring) {
    if (ImGui::TreeNodeEx("Blocks", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (int i = 0; i < (int)world.blocks.size(); ++i) {
            const auto& b = world.blocks[i];
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf |
                                       ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (selectedBlock == i) flags |= ImGuiTreeNodeFlags_Selected;
            ImGui::TreeNodeEx((void*)(intptr_t)i, flags, "%s", b.name.c_str());
            if (ImGui::IsItemClicked()) {
                selectedBlock  = i;
                selectedSpring = -1;
            }
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Springs", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (int i = 0; i < (int)world.springs.size(); ++i) {
            const auto& s = world.springs[i];
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf |
                                       ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (selectedSpring == i) flags |= ImGuiTreeNodeFlags_Selected;
            ImGui::TreeNodeEx((void*)(intptr_t)(i + 10000), flags, "%s", s.name.c_str());
            if (ImGui::IsItemClicked()) {
                selectedSpring = i;
                selectedBlock  = -1;
            }
        }
        ImGui::TreePop();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
static void editVec3(const char* label, glm::vec3& v,
                     float vmin = -100.f, float vmax = 100.f) {
    float f[3] = {v.x, v.y, v.z};
    if (ImGui::DragFloat3(label, f, 0.01f, vmin, vmax))
        v = {f[0], f[1], f[2]};
}

void UI::drawProperties(PhysicsWorld& world,
                        int selectedBlock, int selectedSpring) {
    // ── Block properties ──────────────────────────────────────────────────────
    if (selectedBlock >= 0 && selectedBlock < (int)world.blocks.size()) {
        Block& b = world.blocks[selectedBlock];
        colLabel("Block Properties", {0.6f, 1.0f, 0.6f, 1.0f});

        char nameBuf[64]; std::copy(b.name.begin(), b.name.end(), nameBuf);
        nameBuf[b.name.size()] = '\0';
        if (ImGui::InputText("Name##B", nameBuf, 64)) b.name = nameBuf;

        ImGui::Checkbox("Anchored", &b.isAnchored);
        ImGui::SameLine();
        ImGui::TextDisabled("(fixed in space)");

        editVec3("Position##B", b.position, -50.f, 50.f);
        editVec3("Velocity##B", b.velocity, -50.f, 50.f);

        ImGui::DragFloat("Mass",        &b.mass,        0.01f, 0.01f, 1000.f);
        editVec3("Half Extents", b.halfExtents, 0.05f, 10.f);
        ImGui::DragFloat("Restitution", &b.restitution, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Friction",    &b.friction,    0.01f, 0.0f, 2.0f);

        ImGui::Spacing();
        colLabel("Material");
        float col[3] = {b.albedo.x, b.albedo.y, b.albedo.z};
        if (ImGui::ColorEdit3("Albedo##B", col)) b.albedo = {col[0],col[1],col[2]};
        ImGui::SliderFloat("Metallic##B",  &b.metallic,  0.0f, 1.0f);
        ImGui::SliderFloat("Roughness##B", &b.roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("AO##B",        &b.ao,        0.0f, 1.0f);

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f,0.2f,0.2f,1.0f));
        if (ImGui::Button("Delete Block")) actions.deleteBlock = true;
        ImGui::PopStyleColor();
    }

    // ── Spring properties ─────────────────────────────────────────────────────
    else if (selectedSpring >= 0 && selectedSpring < (int)world.springs.size()) {
        Spring& s = world.springs[selectedSpring];
        colLabel("Spring Properties", {1.0f, 0.8f, 0.4f, 1.0f});

        char nameBuf[64]; std::copy(s.name.begin(), s.name.end(), nameBuf);
        nameBuf[s.name.size()] = '\0';
        if (ImGui::InputText("Name##S", nameBuf, 64)) s.name = nameBuf;

        // Endpoint selectors
        auto endpointUI = [&](const char* label, int& blockIdx, glm::vec3& anchor) {
            ImGui::Text("%s", label);
            ImGui::SameLine();
            if (blockIdx < 0) {
                ImGui::Text("Fixed Anchor");
                editVec3(("##anch_" + std::string(label)).c_str(), anchor, -50.f, 50.f);
            } else {
                if (blockIdx < (int)world.blocks.size())
                    ImGui::Text("Block: %s", world.blocks[blockIdx].name.c_str());
                else
                    ImGui::TextDisabled("(invalid)");
            }
            // Combo to pick block or anchor
            std::string preview = (blockIdx < 0) ? "Anchor" :
                (blockIdx < (int)world.blocks.size() ? world.blocks[blockIdx].name : "?");
            if (ImGui::BeginCombo(("##ep_" + std::string(label)).c_str(), preview.c_str())) {
                if (ImGui::Selectable("Fixed Anchor")) blockIdx = -1;
                for (int bi = 0; bi < (int)world.blocks.size(); ++bi)
                    if (ImGui::Selectable(world.blocks[bi].name.c_str()))
                        blockIdx = bi;
                ImGui::EndCombo();
            }
        };

        endpointUI("End A", s.blockA, s.anchorA);
        ImGui::Spacing();
        endpointUI("End B", s.blockB, s.anchorB);
        ImGui::Spacing();

        ImGui::DragFloat("Stiffness (k)",  &s.k,         0.5f, 0.0f, 5000.0f);
        ImGui::DragFloat("Damping",        &s.damping,   0.05f, 0.0f, 100.0f);
        ImGui::DragFloat("Rest Length",    &s.restLength,0.05f, 0.05f, 30.0f);

        ImGui::Spacing();
        colLabel("Material");
        float col[3] = {s.albedo.x, s.albedo.y, s.albedo.z};
        if (ImGui::ColorEdit3("Albedo##S", col)) s.albedo = {col[0],col[1],col[2]};
        ImGui::SliderFloat("Metallic##S",  &s.metallic,  0.0f, 1.0f);
        ImGui::SliderFloat("Roughness##S", &s.roughness, 0.0f, 1.0f);

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f,0.2f,0.2f,1.0f));
        if (ImGui::Button("Delete Spring")) actions.deleteSpring = true;
        ImGui::PopStyleColor();
    } else {
        ImGui::TextDisabled("Nothing selected.\nClick a block or spring\nin the hierarchy.");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void UI::drawRenderConfig(RenderConfig& cfg) {
    ImGui::SliderFloat("Exposure",      &cfg.exposure,      0.1f, 5.0f);
    ImGui::Checkbox   ("Bloom",         &cfg.bloomEnabled);
    if (cfg.bloomEnabled) {
        ImGui::SliderFloat("Bloom Strength",&cfg.bloomStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("Bloom Thresh",  &cfg.bloomThreshold,0.3f, 3.0f);
    }
    ImGui::Checkbox("Wireframe", &cfg.wireframe);
}
