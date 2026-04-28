#pragma once

#include "Component.hpp"
#include <cstdint>

namespace GameEngine {

    /**
     * NetworkComponent — tags an entity for network replication.
     *
     * Server replicates entities tagged with this component every tick.
     * Clients apply received snapshots to the tagged entity's TransformComponent.
     */
    class NetworkComponent : public Component {
    public:
        NetworkComponent() = default;

        uint64_t NetworkID = 0;      // Globally unique ID; assigned by server on spawn
        bool ReplicateTransform = true;
        bool ReplicateVelocity = false;
        bool ClientAuthoritative = false;

        const char* GetTypeName() const override { return "NetworkComponent"; }
    };

}
