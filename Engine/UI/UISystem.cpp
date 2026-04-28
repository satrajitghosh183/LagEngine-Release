#include "UISystem.hpp"
#include "UILayout.hpp"
#include "../Graphics/BatchRenderer2D.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {
namespace UI {

    Ref<UICanvas> UISystem::s_Canvas;
    Ref<UITheme>  UISystem::s_Theme;
    UIDrawList    UISystem::s_DrawList;
    UIWidget*     UISystem::s_FocusedWidget = nullptr;
    UIWidget*     UISystem::s_HoveredWidget = nullptr;
    glm::vec2     UISystem::s_MousePos{0.0f};

    void UISystem::Init() {
        if (!s_Theme) s_Theme = UITheme::CreateDefault();
        GE_CORE_INFO("UISystem initialized");
    }

    void UISystem::Shutdown() {
        s_Canvas.reset();
        s_Theme.reset();
        s_DrawList.Clear();
    }

    void UISystem::SetCanvas(Ref<UICanvas> canvas) { s_Canvas = canvas; }
    void UISystem::SetTheme(Ref<UITheme> theme)    { s_Theme = theme; }

    void UISystem::Update(float deltaTime, const glm::vec2& screenSize) {
        if (!s_Canvas) return;
        s_Canvas->update(deltaTime, screenSize);
    }

    static void WalkAndRecord(UIWidget* w, UIDrawList& dl, UITheme* theme);

    void UISystem::Render(VkCommandBuffer cmd) {
        if (!s_Canvas) return;

        s_DrawList.Clear();
        BuildDrawList();

        if (s_DrawList.Commands().empty()) return;

        // Submit to BatchRenderer2D — each command becomes draw calls on cmd.
        // BatchRenderer2D already abstracts textured quads through Vulkan.
        for (const auto& c : s_DrawList.Commands()) {
            switch (c.Type) {
                case UIDrawCommand::Kind::Rect:
                    BatchRenderer2D::DrawQuad(
                        glm::vec3(c.Rect.x + c.Rect.z * 0.5f, c.Rect.y + c.Rect.w * 0.5f, 0.0f),
                        glm::vec2(c.Rect.z, c.Rect.w),
                        c.Color);
                    break;
                case UIDrawCommand::Kind::BorderedRect:
                    BatchRenderer2D::DrawQuad(
                        glm::vec3(c.Rect.x + c.Rect.z * 0.5f, c.Rect.y + c.Rect.w * 0.5f, 0.0f),
                        glm::vec2(c.Rect.z, c.Rect.w),
                        c.Color);
                    // Border — draw as 4 thin rects around the fill
                    {
                        float bw = c.BorderWidth;
                        // Top
                        BatchRenderer2D::DrawQuad(
                            glm::vec3(c.Rect.x + c.Rect.z*0.5f, c.Rect.y + bw*0.5f, 0.0f),
                            glm::vec2(c.Rect.z, bw), c.Color2);
                        // Bottom
                        BatchRenderer2D::DrawQuad(
                            glm::vec3(c.Rect.x + c.Rect.z*0.5f, c.Rect.y + c.Rect.w - bw*0.5f, 0.0f),
                            glm::vec2(c.Rect.z, bw), c.Color2);
                        // Left
                        BatchRenderer2D::DrawQuad(
                            glm::vec3(c.Rect.x + bw*0.5f, c.Rect.y + c.Rect.w*0.5f, 0.0f),
                            glm::vec2(bw, c.Rect.w), c.Color2);
                        // Right
                        BatchRenderer2D::DrawQuad(
                            glm::vec3(c.Rect.x + c.Rect.z - bw*0.5f, c.Rect.y + c.Rect.w*0.5f, 0.0f),
                            glm::vec2(bw, c.Rect.w), c.Color2);
                    }
                    break;
                case UIDrawCommand::Kind::Line: {
                    // Simple axis-aligned line as a thin quad
                    glm::vec2 a(c.Rect.x, c.Rect.y);
                    glm::vec2 b(c.Rect.z, c.Rect.w);
                    glm::vec2 mid = (a + b) * 0.5f;
                    glm::vec2 d = b - a;
                    float len = glm::length(d);
                    BatchRenderer2D::DrawQuad(glm::vec3(mid, 0.0f),
                                              glm::vec2(len, c.LineThickness), c.Color);
                    break;
                }
                case UIDrawCommand::Kind::TexturedRect:
                case UIDrawCommand::Kind::Text:
                case UIDrawCommand::Kind::PushScissor:
                case UIDrawCommand::Kind::PopScissor:
                    // Text and textured rendering require a font atlas + BatchRenderer2D
                    // extensions. Stubbed for now — they're recorded in the draw list
                    // so the integration point is in place.
                    break;
            }
        }
    }

    void UISystem::BuildDrawList() {
        if (!s_Canvas) return;
        auto& roots = s_Canvas->GetRoots();
        for (auto& root : roots) WalkAndRecord(root.get(), s_DrawList, s_Theme.get());
    }

    static void WalkAndRecord(UIWidget* w, UIDrawList& dl, UITheme* theme) {
        if (!w || !w->isVisible()) return;

        glm::vec4 rect = w->getWorldRect();
        const UIStyle* style = nullptr;
        if (theme) {
            // Dynamic type dispatch — could replace with a GetTypeName() virtual
            if (dynamic_cast<UIButton*>(w))       style = &theme->GetStyle("Button");
            else if (dynamic_cast<UIPanel*>(w))   style = &theme->GetStyle("Panel");
            else if (dynamic_cast<UILabel*>(w))   style = &theme->GetStyle("Label");
            else if (dynamic_cast<UISlider*>(w))  style = &theme->GetStyle("Slider");
            else if (dynamic_cast<UICheckBox*>(w))style = &theme->GetStyle("CheckBox");
            else if (dynamic_cast<UITextEdit*>(w))style = &theme->GetStyle("TextEdit");
        }

        // Draw based on widget type
        if (auto* panel = dynamic_cast<UIPanel*>(w)) {
            glm::vec4 fill = style ? style->BackgroundColor : panel->BackgroundColor;
            glm::vec4 border = style ? style->BorderColor : panel->BorderColor;
            if (panel->DrawBorder) {
                dl.AddBorderedRect(rect, fill, border, style ? style->BorderWidth : 1.0f,
                                   style ? style->CornerRadius : panel->CornerRadius);
            } else {
                dl.AddRect(rect, fill, style ? style->CornerRadius : panel->CornerRadius);
            }
        } else if (auto* btn = dynamic_cast<UIButton*>(w)) {
            glm::vec4 col = btn->NormalColor;
            if (style) col = btn->IsPressed ? style->PressedColor
                       : btn->IsHovered ? style->HoverColor
                       : style->BackgroundColor;
            else col = btn->IsPressed ? btn->PressedColor
                     : btn->IsHovered ? btn->HoverColor
                     : btn->NormalColor;
            dl.AddRect(rect, col, style ? style->CornerRadius : 4.0f);
            dl.AddText(btn->Label,
                       { rect.x + (style ? style->PaddingX : 8.0f),
                         rect.y + rect.w * 0.5f },
                       style ? style->FontSize : 14.0f,
                       style ? style->TextColor : btn->TextColor);
        } else if (auto* lbl = dynamic_cast<UILabel*>(w)) {
            dl.AddText(lbl->Text, { rect.x, rect.y }, lbl->FontSize,
                       style ? style->TextColor : lbl->TextColor);
        } else if (auto* img = dynamic_cast<UIImage*>(w)) {
            if (img->TextureID != 0) {
                dl.AddTexturedRect(rect, img->TextureID, img->Tint);
            } else {
                dl.AddRect(rect, img->Tint);
            }
        } else if (auto* sl = dynamic_cast<UISlider*>(w)) {
            // Track
            dl.AddRect({ rect.x, rect.y + rect.w * 0.4f, rect.z, rect.w * 0.2f },
                       style ? style->BackgroundColor : sl->TrackColor, 2.0f);
            // Fill
            float t = (sl->Value - sl->MinValue) / std::max(0.0001f, sl->MaxValue - sl->MinValue);
            dl.AddRect({ rect.x, rect.y + rect.w * 0.4f, rect.z * t, rect.w * 0.2f },
                       style ? style->AccentColor : sl->FillColor, 2.0f);
            // Handle
            dl.AddRect({ rect.x + rect.z * t - 6.0f, rect.y, 12.0f, rect.w },
                       style ? style->TextColor : sl->HandleColor, 3.0f);
        } else if (auto* pb = dynamic_cast<UIProgressBar*>(w)) {
            dl.AddRect(rect, style ? style->BackgroundColor : pb->BackgroundColor, 2.0f);
            dl.AddRect({ rect.x, rect.y, rect.z * pb->Value, rect.w },
                       style ? style->AccentColor : pb->FillColor, 2.0f);
        } else if (auto* cb = dynamic_cast<UICheckBox*>(w)) {
            // Box
            float boxSize = rect.w;
            dl.AddBorderedRect({ rect.x, rect.y, boxSize, boxSize },
                               style ? style->BackgroundColor : glm::vec4(0.15f, 0.15f, 0.18f, 1.0f),
                               style ? style->BorderColor : glm::vec4(0.5f),
                               1.0f, 2.0f);
            if (cb->IsChecked) {
                dl.AddRect({ rect.x + 4.0f, rect.y + 4.0f, boxSize - 8.0f, boxSize - 8.0f },
                           style ? style->AccentColor : glm::vec4(0.35f, 0.65f, 1.0f, 1.0f), 1.0f);
            }
            // Label
            dl.AddText(cb->Label, { rect.x + boxSize + 6.0f, rect.y + boxSize * 0.5f },
                       style ? style->FontSize : 14.0f,
                       style ? style->TextColor : glm::vec4(0.95f));
        } else if (auto* te = dynamic_cast<UITextEdit*>(w)) {
            dl.AddBorderedRect(rect,
                               style ? style->BackgroundColor : glm::vec4(0.08f, 0.08f, 0.10f, 1.0f),
                               style ? (te->HasFocus ? style->AccentColor : style->BorderColor)
                                     : glm::vec4(0.30f, 0.35f, 0.45f, 1.0f),
                               1.0f, 3.0f);
            const std::string& display = te->Text.empty() ? te->Placeholder : te->Text;
            glm::vec4 textCol = te->Text.empty() ? glm::vec4(0.5f) :
                (style ? style->TextColor : glm::vec4(0.95f));
            dl.AddText(display,
                       { rect.x + (style ? style->PaddingX : 6.0f),
                         rect.y + rect.w * 0.5f },
                       style ? style->FontSize : 14.0f, textCol);
        } else if (auto* dd = dynamic_cast<UIDropdown*>(w)) {
            dl.AddBorderedRect(rect,
                               style ? style->BackgroundColor : glm::vec4(0.20f),
                               style ? style->BorderColor : glm::vec4(0.40f),
                               1.0f, 3.0f);
            std::string display = (dd->SelectedIndex >= 0 &&
                                   dd->SelectedIndex < (int)dd->Options.size())
                                      ? dd->Options[dd->SelectedIndex] : "Select...";
            dl.AddText(display, { rect.x + 8.0f, rect.y + rect.w * 0.5f },
                       style ? style->FontSize : 14.0f,
                       style ? style->TextColor : glm::vec4(0.95f));
            if (dd->IsOpen) {
                float y = rect.y + rect.w;
                for (const auto& opt : dd->Options) {
                    dl.AddRect({ rect.x, y, rect.z, rect.w },
                               style ? style->BackgroundColor : glm::vec4(0.18f), 0.0f);
                    dl.AddText(opt, { rect.x + 8.0f, y + rect.w * 0.5f },
                               style ? style->FontSize : 14.0f,
                               style ? style->TextColor : glm::vec4(0.95f));
                    y += rect.w;
                }
            }
        } else if (auto* sc = dynamic_cast<UIScrollContainer*>(w)) {
            dl.PushScissor(rect);
        }

        // Recurse into children
        for (const auto& child : w->getChildren()) {
            WalkAndRecord(child.get(), dl, theme);
        }

        if (dynamic_cast<UIScrollContainer*>(w)) dl.PopScissor();
    }

    void UISystem::OnMouseMove(float x, float y) {
        s_MousePos = { x, y };
        if (s_Canvas) s_Canvas->onMouseMove(s_MousePos);
    }

    void UISystem::OnMouseButton(int button, bool pressed) {
        if (!s_Canvas) return;
        if (pressed) s_Canvas->onMouseClick(s_MousePos);
        else s_Canvas->onMouseRelease(s_MousePos);
    }

    void UISystem::OnKey(int key, bool pressed) {
        if (auto* te = dynamic_cast<UITextEdit*>(s_FocusedWidget)) {
            te->OnKey(key, pressed);
        }
    }

    void UISystem::OnCharTyped(uint32_t codepoint) {
        if (auto* te = dynamic_cast<UITextEdit*>(s_FocusedWidget)) {
            te->OnCharTyped(codepoint);
        }
    }

    void UISystem::OnScroll(float /*dx*/, float dy) {
        // Scroll the hovered UIScrollContainer if any
        if (auto* sc = dynamic_cast<UIScrollContainer*>(s_HoveredWidget)) {
            sc->ScrollY -= dy * 20.0f;
        }
    }

}}
