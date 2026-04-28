#include "ReplicationManager.hpp"
#include "../Scene/Scene.hpp"
#include "../Scene/Entity.hpp"
#include "../Scene/Components/TransformComponent.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {
namespace Networking {

    ReplicationManager::ReplicationManager() = default;
    ReplicationManager::~ReplicationManager() = default;

    void ReplicationManager::SetSession(NetworkSession* session) {
        m_Session = session;
        if (m_Session) {
            m_Session->OnReplicationPacket = [this](PeerID p, BinaryReader& r) {
                OnReplicationReceived(p, r);
            };
        }
    }

    void ReplicationManager::SetScene(Scene* scene) {
        m_Scene = scene;
    }

    void ReplicationManager::RegisterEntity(uint64_t networkID, Entity* entity) {
        m_Entities[networkID] = entity;
    }

    void ReplicationManager::UnregisterEntity(uint64_t networkID) {
        m_Entities.erase(networkID);
        m_InterpBuffers.erase(networkID);
    }

    void ReplicationManager::Tick(float deltaTime) {
        if (!m_Session) return;
        if (m_Session->IsServer()) ServerTick(deltaTime);
        else if (m_Session->IsClient()) ClientTick(deltaTime);
    }

    void ReplicationManager::ServerTick(float deltaTime) {
        m_SnapshotTimer += deltaTime;
        if (m_SnapshotTimer < m_SnapshotInterval) return;
        m_SnapshotTimer = 0.0f;
        m_ServerTick++;

        if (m_Entities.empty()) return;

        BinaryWriter w;
        w.WriteU8(RPCRegistry::kReplMarker);
        w.WriteU32(m_ServerTick);
        w.WriteU16(static_cast<uint16_t>(m_Entities.size()));

        for (auto& [netID, entity] : m_Entities) {
            if (!entity || !entity->HasComponent<TransformComponent>()) continue;
            auto& xform = entity->GetComponent<TransformComponent>();

            w.WriteU64(netID);
            w.WriteVec3(xform.Position);
            w.WriteQuat(xform.Rotation);
            w.WriteVec3(xform.Scale);
        }

        auto data = w.TakeData();
        m_Session->BroadcastRaw(data.data(), data.size(),
                                TransferMode::UnreliableOrdered, 0);
    }

    void ReplicationManager::ClientTick(float /*deltaTime*/) {
        // Apply interpolated state from buffered snapshots
        for (auto& [netID, buf] : m_InterpBuffers) {
            auto it = m_Entities.find(netID);
            if (it == m_Entities.end() || !it->second) continue;
            if (!it->second->HasComponent<TransformComponent>() || buf.States.empty()) continue;
            auto& xform = it->second->GetComponent<TransformComponent>();

            // Simple: use most recent state (interp window extension would go here)
            const auto& latest = buf.States.back();
            xform.Position = latest.Position;
            xform.Rotation = latest.Rotation;
            xform.Scale = latest.Scale;
        }
    }

    void ReplicationManager::OnReplicationReceived(PeerID /*sender*/, BinaryReader& r) {
        try {
            uint32_t tick = r.ReadU32();
            uint16_t count = r.ReadU16();
            for (uint16_t i = 0; i < count; i++) {
                ReplicatedState s;
                s.NetworkID = r.ReadU64();
                s.Position = r.ReadVec3();
                s.Rotation = r.ReadQuat();
                s.Scale = r.ReadVec3();
                s.Tick = tick;

                auto& buf = m_InterpBuffers[s.NetworkID];
                buf.States.push_back(s);
                if (buf.States.size() > 16) buf.States.erase(buf.States.begin());
            }
        } catch (const std::exception& e) {
            GE_CORE_WARN("Replication: malformed snapshot: {}", e.what());
        }
    }

}}
