#pragma once

#include "NetworkSession.hpp"
#include "RPC.hpp"
#include "../Core/Base.hpp"
#include "../Core/UUID.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>
#include <vector>

namespace GameEngine {

    class Scene;
    class Entity;

    namespace Networking {

        // Per-entity replicated state snapshot.
        struct ReplicatedState {
            uint64_t NetworkID = 0;
            glm::vec3 Position{0};
            glm::quat Rotation{1, 0, 0, 0};
            glm::vec3 Scale{1};
            glm::vec3 Velocity{0};
            uint32_t Tick = 0;
        };

        // Server-authoritative replication manager.
        // Server sends snapshots of replicated entities every tick; clients interpolate.
        class ReplicationManager {
        public:
            ReplicationManager();
            ~ReplicationManager();

            void SetSession(NetworkSession* session);
            void SetScene(Scene* scene);

            // Called each frame:
            //   On server: gathers replicated entity state and broadcasts snapshots.
            //   On client: applies received snapshots (with interpolation).
            void Tick(float deltaTime);

            // Register/unregister a networked entity. NetworkID must be globally unique.
            void RegisterEntity(uint64_t networkID, Entity* entity);
            void UnregisterEntity(uint64_t networkID);

            // Tuning
            void SetSnapshotInterval(float seconds) { m_SnapshotInterval = seconds; }
            void SetInterpolationDelay(float seconds) { m_InterpDelay = seconds; }
            void SetAuthoritativeMode(bool serverAuth) { m_ServerAuthoritative = serverAuth; }

        private:
            void ServerTick(float deltaTime);
            void ClientTick(float deltaTime);
            void OnReplicationReceived(PeerID sender, BinaryReader& reader);

            NetworkSession* m_Session = nullptr;
            Scene* m_Scene = nullptr;

            // Entity registry: NetworkID → entity
            std::unordered_map<uint64_t, Entity*> m_Entities;

            // Client-side interpolation buffer: NetworkID → recent states
            struct InterpBuffer {
                std::vector<ReplicatedState> States; // sorted by Tick
            };
            std::unordered_map<uint64_t, InterpBuffer> m_InterpBuffers;

            float m_SnapshotTimer = 0.0f;
            float m_SnapshotInterval = 1.0f / 20.0f; // 20Hz default
            float m_InterpDelay = 0.1f;              // 100ms interpolation window
            uint32_t m_ServerTick = 0;
            bool m_ServerAuthoritative = true;
        };

    }
}
