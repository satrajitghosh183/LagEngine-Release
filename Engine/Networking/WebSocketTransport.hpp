#pragma once

#include "Transport.hpp"
#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace GameEngine {
namespace Networking {

    // Minimal WebSocket (RFC 6455) server + client.
    // Supports text + binary frames, ping/pong, proper handshake.
    // Uses TCP sockets directly. Single-threaded reader per connection + event queue.
    class WebSocketTransport : public Transport {
    public:
        WebSocketTransport();
        ~WebSocketTransport() override;

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
        TransportType GetType() const override { return TransportType::WebSocket; }

    private:
        enum class Mode { None, Server, Client };

        struct Connection {
            PeerID ID = 0;
            int Socket = -1;
            std::vector<uint8_t> RecvBuffer;
            bool Handshaked = false;
            bool ClientSide = false; // masked frames when sending
        };

        void AcceptLoop();
        void ReadLoop(PeerID peerID);
        bool PerformServerHandshake(int sock, std::string& outKey);
        bool PerformClientHandshake(int sock, const std::string& host, uint16_t port, const std::string& path);

        void SendFrame(int sock, const uint8_t* payload, size_t size,
                       uint8_t opcode, bool masked);
        bool ParseFrame(Connection& conn, std::vector<uint8_t>& outPayload, uint8_t& outOpcode);

        static std::string ComputeAcceptKey(const std::string& clientKey);
        static std::string Base64Encode(const uint8_t* data, size_t len);
        static void SHA1Compute(const uint8_t* data, size_t len, uint8_t out[20]);

        Mode m_Mode = Mode::None;
        std::atomic<bool> m_Connected{false};
        std::atomic<bool> m_Running{false};
        int m_ListenSocket = -1;
        int m_MaxPeers = 16;

        std::mutex m_ConnMutex;
        std::unordered_map<PeerID, Connection> m_Connections;
        PeerID m_NextPeerID = 1;

        std::mutex m_EventMutex;
        std::deque<TransportEvent> m_Events;

        std::thread m_AcceptThread;
        std::vector<std::thread> m_ReadThreads;

        std::atomic<uint64_t> m_BytesSent{0};
        std::atomic<uint64_t> m_BytesReceived{0};
    };

}}
