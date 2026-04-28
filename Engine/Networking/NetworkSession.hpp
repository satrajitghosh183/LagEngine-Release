#pragma once

#include "Transport.hpp"
#include "RPC.hpp"
#include "../Core/Base.hpp"
#include <functional>
#include <string>
#include <unordered_map>

namespace GameEngine {
namespace Networking {

    // High-level networking session combining Transport + RPC + Replication.
    // Use this from game code; don't talk to transports directly.
    class NetworkSession {
    public:
        NetworkSession();
        ~NetworkSession();

        bool StartServer(Transport::TransportType type, uint16_t port, int maxClients = 16);
        bool ConnectToServer(Transport::TransportType type, const std::string& address, uint16_t port);
        void Disconnect();
        void Tick(float deltaTime);

        bool IsServer() const    { return m_Transport && m_Transport->IsServer(); }
        bool IsClient() const    { return m_Transport && m_Transport->IsClient(); }
        bool IsConnected() const { return m_Transport && m_Transport->IsConnected(); }

        RPCRegistry& GetRPCRegistry() { return m_RPC; }

        // RPC invocation: send an RPC call to one peer or broadcast to all.
        // Use RPCRegistry::Serialize to build the payload.
        void SendRPC(PeerID peer, uint32_t rpcID, const std::vector<uint8_t>& payload,
                     TransferMode mode = TransferMode::Reliable);
        void BroadcastRPC(uint32_t rpcID, const std::vector<uint8_t>& payload,
                          TransferMode mode = TransferMode::Reliable,
                          PeerID exclude = kInvalidPeer);

        // Raw send (non-RPC) — e.g., replication snapshots
        void SendRaw(PeerID peer, const uint8_t* data, size_t size,
                     TransferMode mode = TransferMode::Unreliable,
                     uint8_t channel = 0);
        void BroadcastRaw(const uint8_t* data, size_t size,
                          TransferMode mode = TransferMode::Unreliable,
                          uint8_t channel = 0, PeerID exclude = kInvalidPeer);

        // Event hooks — set before starting the session
        std::function<void(PeerID)> OnPeerConnected;
        std::function<void(PeerID)> OnPeerDisconnected;
        // Called for non-RPC, non-replication raw packets
        std::function<void(PeerID, const uint8_t*, size_t, uint8_t channel)> OnRawPacket;
        // Called for replication snapshot packets (kReplMarker)
        std::function<void(PeerID, BinaryReader&)> OnReplicationPacket;

    private:
        Scope<Transport> m_Transport;
        RPCRegistry m_RPC;
    };

}}
