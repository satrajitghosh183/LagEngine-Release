#pragma once

#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Core/WickedCompat.hpp"
#include "../../Engine/Graphics/RenderPath.hpp"

namespace GameEngine {

    class WickedStyleTemplateApp : public Application {
    public:
        WickedStyleTemplateApp();
        virtual ~WickedStyleTemplateApp() = default;

    protected:
        void OnInit() override;
        void OnUpdate(float deltaTime) override;
        void OnRender() override;

    private:
        Ref<RenderPath3D> m_RenderPath3D;
        Ref<RenderPath2D> m_RenderPath2D;
        bool m_Use3D = true;
    };

}
