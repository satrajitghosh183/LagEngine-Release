#pragma once

#include "../Core/Base.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace GameEngine {
namespace Networking {

    using PeerID = uint32_t;
    constexpr PeerID kInvalidPeer = 0;

    enum class TransferMode : uint8_t {
        Reliable = 0,
        Unreliable = 1,
        UnreliableOrdered = 2
    };

    struct TransportEvent {
        enum Kind { None, Connected, Disconnected, Receive } Kind = None;
        PeerID Peer = kInvalidPeer;
        std::vector<uint8_t> Data;
        uint8_t Channel = 0;
    };

    // Abstract transport. All concrete transports (UDP, ENet-like, WebSocket)
    // implement this interface.
    class Transport {
    public:
        virtual ~Transport() = default;

        virtual bool Listen(uint16_t port, int maxPeers) = 0;
        virtual bool Connect(const std::string& address, uint16_t port) = 0;
        virtual void Disconnect() = 0;
        virtual bool IsServer() const = 0;
        virtual bool IsClient() const = 0;
        virtual bool IsConnected() const = 0;

        // Send data to a specific peer. Server uses 'peer' to select target.
        // Client ignores 'peer' (sends to server).
        virtual void Send(PeerID peer, const uint8_t* data, size_t size,
                          TransferMode mode = TransferMode::Reliable,
                          uint8_t channel = 0) = 0;

        // Send to all peers (server-only)
        virtual void Broadcast(const uint8_t* data, size_t size,
                               TransferMode mode = TransferMode::Reliable,
                               uint8_t channel = 0,
                               PeerID exclude = kInvalidPeer) = 0;

        // Poll events. Returns true if an event was produced. Call in a loop.
        virtual bool Poll(TransportEvent& outEvent) = 0;

        // Pump I/O — call once per frame
        virtual void Tick(float deltaTime) = 0;

        // Stats
        virtual uint64_t GetBytesSent() const = 0;
        virtual uint64_t GetBytesReceived() const = 0;

        enum class TransportType { UDP, ReliableUDP, WebSocket };
        virtual TransportType GetType() const = 0;
    };

    Scope<Transport> CreateUDPTransport();
    Scope<Transport> CreateReliableUDPTransport();
    Scope<Transport> CreateWebSocketTransport();

}}
