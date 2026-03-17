#include "AnimationPanel.hpp"
#include "../../Engine/Scene/Components/SpriteAnimatorComponent.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <string>

namespace GameEngine {

    // ──────────────────────────────────────────────────────────────────────────
    // Public entry point
    // ──────────────────────────────────────────────────────────────────────────

    void AnimationPanel::OnImGuiRender() {
        ImGui::Begin("Animation");

        if (!m_SelectedEntity.IsValid()) {
            ImGui::TextDisabled("No entity selected.");
            ImGui::Spacing();
            ImGui::TextDisabled("Select an entity in the Hierarchy to view its animation data.");
            ImGui::End();
            return;
        }

        if (m_SelectedEntity.HasComponent<SpriteAnimatorComponent>()) {
            RenderSpriteAnimator();
        } else {
            RenderNoAnimationMsg();
        }

        ImGui::End();
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Sprite animator
    // ──────────────────────────────────────────────────────────────────────────

    void AnimationPanel::RenderSpriteAnimator() {
        auto& comp = m_SelectedEntity.GetComponent<SpriteAnimatorComponent>();
        auto& anim = comp.GetAnimation();

        ImGui::Text("Sprite Animator");
        ImGui::Separator();

        // ── Playback controls ──────────────────────────────────────────
        bool playing = comp.Playing;

        if (playing) {
            if (ImGui::Button("  ||  ")) { comp.Playing = false; anim.Stop(); }
            ImGui::SameLine();
            ImGui::TextDisabled("Playing");
        } else {
            if (ImGui::Button("  >   ")) { comp.Playing = true;  anim.Play(); }
            ImGui::SameLine();
            ImGui::TextDisabled("Stopped");
        }
        ImGui::SameLine(0, 20);
        ImGui::Checkbox("Loop", &comp.Looping);

        // ── Properties ────────────────────────────────────────────────
        ImGui::Spacing();
        ImGui::PushItemWidth(120.0f);
        if (ImGui::DragFloat("Frame Duration (s)", &comp.FrameDuration, 0.005f, 0.005f, 2.0f, "%.3f"))
            comp.FrameDuration = std::max(comp.FrameDuration, 0.005f);

        int cols = comp.FrameColumns, rows = comp.FrameRows;
        if (ImGui::DragInt("Columns", &cols, 1, 1, 64)) comp.FrameColumns = std::max(cols, 1);
        if (ImGui::DragInt("Rows",    &rows, 1, 1, 64)) comp.FrameRows    = std::max(rows, 1);
        ImGui::PopItemWidth();

        int total   = anim.GetFrameCount();
        int current = anim.GetCurrentFrameIndex();

        ImGui::Spacing();
        ImGui::Text("Frame  %d / %d", current + 1, std::max(total, 1));

        // ── Timeline ──────────────────────────────────────────────────
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextUnformatted("Timeline");
        ImGui::Spacing();

        float scrubVal = (total > 1) ? static_cast<float>(current) / (total - 1) : 0.0f;
        DrawTimeline(current, total, scrubVal);
    }

    // ──────────────────────────────────────────────────────────────────────────
    // No animation message
    // ──────────────────────────────────────────────────────────────────────────

    void AnimationPanel::RenderNoAnimationMsg() {
        ImGui::TextDisabled("This entity has no animation component.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped("To animate this entity, add a component in the Components panel:");
        ImGui::Spacing();
        ImGui::BulletText("SpriteAnimatorComponent  –  2D sprite-sheet animation");
        ImGui::BulletText("Transform keyframe animation coming soon");
        ImGui::Spacing();

        // Quick hint about the demo scenes
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Tip:");
        ImGui::SameLine();
        ImGui::TextWrapped("Use  Entity > Create Demo Scene  to load a pre-built scene with animated objects.");
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Timeline widget
    // ──────────────────────────────────────────────────────────────────────────

    void AnimationPanel::DrawTimeline(int currentFrame, int totalFrames, float& /*scrubValue*/) {
        if (totalFrames <= 0) {
            ImGui::TextDisabled("(no frames)");
            return;
        }

        ImDrawList* draw  = ImGui::GetWindowDrawList();
        ImVec2      pos   = ImGui::GetCursorScreenPos();
        float       avail = ImGui::GetContentRegionAvail().x;
        float       tlH   = 24.0f;  // height of each frame cell
        float       fw    = std::max(avail / static_cast<float>(totalFrames), 4.0f);
        float       tlW   = fw * totalFrames;

        // ── Background ──────────────────────────────────────────────────
        draw->AddRectFilled(pos, ImVec2(pos.x + tlW, pos.y + tlH),
                            IM_COL32(30, 30, 30, 220), 3.0f);

        // ── Frame cells ─────────────────────────────────────────────────
        for (int i = 0; i < totalFrames; i++) {
            float x0 = pos.x + i * fw;
            float x1 = x0 + fw - 1.0f;

            bool isActive = (i == currentFrame);

            ImU32 bg = isActive ? IM_COL32(60, 140, 255, 220)
                                : (i % 2 == 0 ? IM_COL32(45, 45, 45, 220)
                                              : IM_COL32(55, 55, 55, 220));
            draw->AddRectFilled(ImVec2(x0, pos.y), ImVec2(x1, pos.y + tlH), bg, 2.0f);

            // Border
            draw->AddRect(ImVec2(x0, pos.y), ImVec2(x1, pos.y + tlH),
                          isActive ? IM_COL32(100, 180, 255, 255)
                                   : IM_COL32(70, 70, 70, 200), 2.0f);

            // Frame number label (only if cells are wide enough)
            if (fw >= 18.0f) {
                char label[16];
                snprintf(label, sizeof(label), "%d", i + 1);
                draw->AddText(ImVec2(x0 + 3.0f, pos.y + 5.0f),
                              isActive ? IM_COL32(255, 255, 255, 255)
                                       : IM_COL32(180, 180, 180, 200), label);
            }
        }

        // ── Playhead line ────────────────────────────────────────────────
        float headX = pos.x + (currentFrame + 0.5f) * fw;
        draw->AddLine(ImVec2(headX, pos.y - 4),
                      ImVec2(headX, pos.y + tlH + 2),
                      IM_COL32(255, 220, 50, 230), 2.0f);
        // Playhead triangle
        draw->AddTriangleFilled(
            ImVec2(headX - 5, pos.y - 4),
            ImVec2(headX + 5, pos.y - 4),
            ImVec2(headX,     pos.y + 3),
            IM_COL32(255, 220, 50, 230));

        // Invisible button so the user can click to scrub
        ImGui::InvisibleButton("##timeline", ImVec2(tlW, tlH));
        // (scrubbing is read-only for now since Stop() resets the frame counter)

        ImGui::Spacing();
    }

}
