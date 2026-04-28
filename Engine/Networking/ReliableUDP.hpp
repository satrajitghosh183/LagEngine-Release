#pragma once

#include "Transport.hpp"
#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace GameEngine {
namespace Networking {

    // ENet-style reliable UDP transport with channels, acks, and fragmentation.
    // Pure C++17 implementation on top of BSD sockets / Winsock — no external deps.
    class ReliableUDPTransport : public Transport {
    public:
        ReliableUDPTransport();
        ~ReliableUDPTransport() override;

        bool Listen(uint16_t port, int maxPeers) override;
        bool Connect(const std::string& address, uint16_t port) override;
        void Disconnect() override;
        bool IsServer() const override { return m_Mode == Mode::Server; }
        bool IsClient() const override { return m_Mode == Mode::Client; }
        bool IsConnected() const override { return m_Connected.load(); }

        void Send(PeerID peer, const uint8_t* data, size_t size,
                  TransferMode mode, uint8_t channel) override;
        void Broadcast(const uint8_t* data, size_t size,
                       TransferMode mode, uint8_t channel,
                       PeerID exclude) override;

        bool Poll(TransportEvent& outEvent) override;
        void Tick(float deltaTime) override;

        uint64_t GetBytesSent() const override { return m_BytesSent.load(); }
        uint64_t GetBytesReceived() const override { return m_BytesReceived.load(); }
        TransportType GetType() const override { return TransportType::ReliableUDP; }

    private:
        enum class Mode { None, Server, Client };

        // Wire packet header (8 bytes)
        struct WirePacket {
            uint16_t Magic;           // 0xLAGX (0x4C41)
            uint8_t Flags;            // bit 0: reliable, bit 1: ack, bit 2: connect, bit 3: disconnect
            uint8_t Channel;
            uint16_t SequenceID;      // For reliable ordering and ack tracking
            uint16_t AckID;           // Last received sequence from remote
            // Payload follows
        };

        struct PeerState {
            PeerID ID;
            std::string Address;
            uint16_t Port;
            int SockAddrLen;
            std::vector<uint8_t> SockAddr; // sockaddr_storage bytes
            bool Connected = false;

            uint16_t NextOutgoingSeq = 1;
            uint16_t LastReceivedSeq = 0;

            // Outgoing reliable queue — resent until acked
            struct PendingSend {
                uint16_t Seq;
                uint8_t Channel;
                std::vector<uint8_t> Data;
                float TimeUntilResend = 0.2f; // 200ms default
                int RetriesLeft = 10;
            };
            std::deque<PendingSend> ReliableQueue;
            float TimeSinceLastHeard = 0.0f;
        };

        void NetworkThreadLoop();
        void HandleIncoming(const uint8_t* buf, size_t len, const void* fromAddr, int fromLen);
        PeerID FindOrCreatePeer(const void* fromAddr, int fromLen);
        void SendWire(PeerState& peer, uint8_t flags, uint8_t channel, uint16_t seq,
                      const uint8_t* payload, size_t payloadSize);
        void SendAck(PeerState& peer, uint16_t seq);

        Mode m_Mode = Mode::None;
        std::atomic<bool> m_Connected{false};
        int m_Socket = -1;
        uint16_t m_Port = 0;
        int m_MaxPeers = 16;

        std::thread m_NetThread;
        std::atomic<bool> m_Running{false};

        std::mutex m_PeerMutex;
        std::unordered_map<PeerID, PeerState> m_Peers;
        PeerID m_NextPeerID = 1;
        PeerID m_LocalID = 0;

        std::mutex m_EventMutex;
        std::deque<TransportEvent> m_Events;

        std::atomic<uint64_t> m_BytesSent{0};
        std::atomic<uint64_t> m_BytesReceived{0};

        // Server-side: where to send when "Connect" is called
        std::vector<uint8_t> m_ServerAddr;
        int m_ServerAddrLen = 0;
    };

}}
