#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace GameEngine {
namespace UI {

    // Anchor presets
    enum class Anchor {
        TopLeft, TopCenter, TopRight,
        MiddleLeft, Center, MiddleRight,
        BottomLeft, BottomCenter, BottomRight,
        StretchHorizontal, StretchVertical, StretchAll
    };

    // UI Events
    struct UIEvent {
        enum class Type { Click, Hover, HoverExit, Focus, Blur, ValueChanged, DragStart, Drag, DragEnd };
        Type EventType;
        glm::vec2 Position{0.0f};
        glm::vec2 Delta{0.0f};
        bool Consumed = false;
    };

    // Base widget
    class UIWidget {
    public:
        virtual ~UIWidget() = default;

        virtual void update(float dt);
        virtual void render() = 0;
        virtual bool hitTest(const glm::vec2& point) const;

        // Transform
        void setPosition(const glm::vec2& pos) { m_Position = pos; }
        glm::vec2 getPosition() const { return m_Position; }
        void setSize(const glm::vec2& size) { m_Size = size; }
        glm::vec2 getSize() const { return m_Size; }
        void setAnchor(Anchor anchor) { m_Anchor = anchor; }
        void setVisible(bool v) { m_Visible = v; }
        bool isVisible() const { return m_Visible; }
        void setInteractable(bool i) { m_Interactable = i; }
        bool isInteractable() const { return m_Interactable; }

        // Hierarchy
        void addChild(Ref<UIWidget> child);
        void removeChild(UIWidget* child);
        const std::vector<Ref<UIWidget>>& getChildren() const { return m_Children; }
        UIWidget* getParent() const { return m_Parent; }

        // Computed world-space rect
        glm::vec2 getWorldPosition() const;
        glm::vec4 getWorldRect() const; // x, y, w, h

        // Name/tag for lookups
        std::string Name;
        std::string Tag;

        // Tween support
        void tweenPosition(const glm::vec2& target, float duration);
        void tweenAlpha(float target, float duration);
        float getAlpha() const { return m_Alpha; }
        void setAlpha(float a) { m_Alpha = a; }

    protected:
        glm::vec2 m_Position{0.0f};
        glm::vec2 m_Size{100.0f, 30.0f};
        Anchor m_Anchor = Anchor::TopLeft;
        bool m_Visible = true;
        bool m_Interactable = true;
        float m_Alpha = 1.0f;

        UIWidget* m_Parent = nullptr;
        std::vector<Ref<UIWidget>> m_Children;

        // Tween state
        struct Tween {
            enum class Property { Position, Alpha };
            Property Prop;
            glm::vec2 StartPos, EndPos;
            float StartAlpha = 1.0f, EndAlpha = 1.0f;
            float Duration = 0.0f, Timer = 0.0f;
            bool Active = false;
        };
        std::vector<Tween> m_Tweens;
    };

    // Label (text display)
    class UILabel : public UIWidget {
    public:
        UILabel(const std::string& text = "");
        void render() override;

        std::string Text;
        glm::vec4 TextColor = glm::vec4(1.0f);
        float FontSize = 16.0f;
        enum class Alignment { Left, Center, Right };
        Alignment TextAlignment = Alignment::Left;
    };

    // Button
    class UIButton : public UIWidget {
    public:
        UIButton(const std::string& label = "Button");
        void render() override;

        std::string Label;
        glm::vec4 NormalColor = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
        glm::vec4 HoverColor = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
        glm::vec4 PressedColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        glm::vec4 TextColor = glm::vec4(1.0f);

        std::function<void()> OnClick;

        bool IsHovered = false;
        bool IsPressed = false;
    };

    // Image
    class UIImage : public UIWidget {
    public:
        UIImage(uint32_t textureID = 0);
        void render() override;

        uint32_t TextureID = 0;
        glm::vec4 Tint = glm::vec4(1.0f);
        bool PreserveAspect = false;
    };

    // Slider
    class UISlider : public UIWidget {
    public:
        UISlider(float minVal = 0.0f, float maxVal = 1.0f, float value = 0.5f);
        void render() override;

        float MinValue = 0.0f;
        float MaxValue = 1.0f;
        float Value = 0.5f;
        glm::vec4 TrackColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        glm::vec4 FillColor = glm::vec4(0.3f, 0.6f, 1.0f, 1.0f);
        glm::vec4 HandleColor = glm::vec4(1.0f);

        std::function<void(float)> OnValueChanged;
    };

    // Progress bar
    class UIProgressBar : public UIWidget {
    public:
        UIProgressBar(float value = 0.0f);
        void render() override;

        float Value = 0.0f; // 0-1
        glm::vec4 BackgroundColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        glm::vec4 FillColor = glm::vec4(0.3f, 0.8f, 0.3f, 1.0f);
    };

    // Panel (container)
    class UIPanel : public UIWidget {
    public:
        UIPanel();
        void render() override;

        glm::vec4 BackgroundColor = glm::vec4(0.15f, 0.15f, 0.15f, 0.9f);
        float CornerRadius = 4.0f;
        bool DrawBorder = true;
        glm::vec4 BorderColor = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
    };

    // Canvas (root container for a UI hierarchy)
    class UICanvas {
    public:
        UICanvas(float referenceWidth = 1920.0f, float referenceHeight = 1080.0f);

        void update(float dt, const glm::vec2& screenSize);
        void render();

        // Input forwarding
        void onMouseMove(const glm::vec2& screenPos);
        void onMouseClick(const glm::vec2& screenPos);
        void onMouseRelease(const glm::vec2& screenPos);

        // Widget management
        void addWidget(Ref<UIWidget> widget);
        void removeWidget(UIWidget* widget);
        UIWidget* findByName(const std::string& name) const;

        // Reference resolution (for resolution-independent UI)
        void setReferenceResolution(float w, float h) { m_RefWidth = w; m_RefHeight = h; }
        glm::vec2 screenToCanvas(const glm::vec2& screenPos) const;

        // Access to root widgets (used by UISystem for rendering)
        const std::vector<Ref<UIWidget>>& GetRoots() const { return m_RootWidgets; }

        // World-space canvas mode
        bool WorldSpace = false;
        glm::mat4 WorldTransform = glm::mat4(1.0f);

    private:
        UIWidget* hitTestRecursive(UIWidget* widget, const glm::vec2& point);

        float m_RefWidth, m_RefHeight;
        float m_ScreenWidth = 0, m_ScreenHeight = 0;
        float m_ScaleX = 1.0f, m_ScaleY = 1.0f;
        std::vector<Ref<UIWidget>> m_RootWidgets;
        UIWidget* m_HoveredWidget = nullptr;
        UIWidget* m_PressedWidget = nullptr;
    };

}}
