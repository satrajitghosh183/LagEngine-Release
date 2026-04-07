#pragma once

namespace engine::scene {

    // Optional base interface — empty for now, extensible later
    class Scene {
    public:
        virtual ~Scene() = default;
    };

} // namespace engine::scene
