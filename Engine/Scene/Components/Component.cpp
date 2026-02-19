#include "Component.hpp"
#include "../Entity.hpp"
#include "../Scene.hpp"

namespace GameEngine {

    Entity Component::GetOwnerEntity() const {
        if (!OwnerScene || OwnerUUID == UUID(0))
            return Entity();
        return Entity(OwnerUUID, OwnerScene);
    }
}
