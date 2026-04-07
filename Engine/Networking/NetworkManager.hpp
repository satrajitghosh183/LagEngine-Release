#pragma once

#include "../Core/Base.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>
#include <queue>

namespace GameEngine {
namespace Networking {

    // =====================================================================
    // Packet
    // =====================================================================

    enum class PacketType : uint8_t {
        Connect = 0,
        Disconnect,
        Ping,
        Pong,
        Data,
        RPC,
        EntityCreate,
        EntityDestroy,
        EntityUpdate,
        Custom
    };

    struct NetworkPacket {
        PacketType Type = PacketType::Data;
        uint32_t SenderID = 0;
        bool Reliable = true;
        std::vector<uint8_t> Data;

        // Serialization helpers
        void writeUint8(uint8_t v) { Data.push_back(v); }
        void writeUint16(uint16_t v) { Data.push_back(v & 0xFF); Data.push_back((v >> 8) & 0xFF); }
        void writeUint32(uint32_t v) { for (int i = 0; i < 4; i++) Data.push_back((v >> (i * 8)) & 0xFF); }
        void writeFloat(float v) { uint32_t u; memcpy(&u, &v, 4); writeUint32(u); }
        void writeString(const std::string& s) {
            writeUint16(static_cast<uint16_t>(s.size()));
            Data.insert(Data.end(), s.begin(), s.end());
        }

        // Deserialization
        struct Reader {
            const std::vector<uint8_t>& data;
            size_t pos = 0;
            uint8_t readUint8() { return data[pos++]; }
            uint16_t readUint16() { uint16_t v = data[pos] | (data[pos+1] << 8); pos += 2; return v; }
            uint32_t readUint32() { uint32_t v = 0; for (int i = 0; i < 4; i++) v |= data[pos++] << (i*8); return v; }
            float readFloat() { uint32_t u = readUint32(); float f; memcpy(&f, &u, 4); return f; }
            std::string readString() { uint16_t len = readUint16(); std::string s(data.begin()+pos, data.begin()+pos+len); pos += len; return s; }
        };
        Reader createReader() const { return {Data, 0}; }
    };

    // =====================================================================
    // Peer
    // =====================================================================

    struct NetworkPeer {
        uint32_t ID = 0;
        std::string Address;
        uint16_t Port = 0;
        float RTT = 0.0f; // Round-trip time in ms
        float LastPingTime = 0.0f;
        bool Connected = false;
    };

    // =====================================================================
    // RPC System
    // =====================================================================

    using RPCHandler = std::function<void(uint32_t senderID, NetworkPacket::Reader& reader)>;

    // =====================================================================
    // Network Manager
    // =====================================================================

    class NetworkManager {
    public:
        enum class Mode { None, Server, Client };

        NetworkManager();
        ~NetworkManager();

        // Lifecycle
        bool startServer(uint16_t port, int maxClients = 16);
        bool connectToServer(const std::string& address, uint16_t port);
        void disconnect();
        void update(float dt);

        // State
        Mode getMode() const { return m_Mode; }
        bool isServer() const { return m_Mode == Mode::Server; }
        bool isClient() const { return m_Mode == Mode::Client; }
        bool isConnected() const { return m_Connected; }
        uint32_t getLocalID() const { return m_LocalID; }

        // Peers
        const std::vector<NetworkPeer>& getPeers() const { return m_Peers; }
        int getPeerCount() const { return static_cast<int>(m_Peers.size()); }

        // Send
        void sendToServer(const NetworkPacket& packet);
        void sendToClient(uint32_t clientID, const NetworkPacket& packet);
        void broadcast(const NetworkPacket& packet, uint32_t excludeID = 0);

        // RPC
        void registerRPC(const std::string& name, RPCHandler handler);
        void callRPC(const std::string& name, const NetworkPacket& args, uint32_t targetID = 0);

        // Callbacks
        std::function<void(uint32_t peerID)> OnPeerConnected;
        std::function<void(uint32_t peerID)> OnPeerDisconnected;
        std::function<void(uint32_t senderID, const NetworkPacket&)> OnPacketReceived;

        // Stats
        float getAverageRTT() const;
        uint64_t getBytesSent() const { return m_BytesSent; }
        uint64_t getBytesReceived() const { return m_BytesReceived; }

    private:
        void processIncomingPackets();
        void handlePacket(const NetworkPacket& packet);
        void pingPeers(float dt);

        Mode m_Mode = Mode::None;
        bool m_Connected = false;
        uint32_t m_LocalID = 0;
        uint32_t m_NextPeerID = 1;
        uint16_t m_Port = 0;

        std::vector<NetworkPeer> m_Peers;
        std::unordered_map<std::string, RPCHandler> m_RPCHandlers;

        // Thread-safe packet queue
        std::mutex m_PacketMutex;
        std::queue<NetworkPacket> m_IncomingPackets;
        std::queue<NetworkPacket> m_OutgoingPackets;

        // Stats
        uint64_t m_BytesSent = 0;
        uint64_t m_BytesReceived = 0;
        float m_PingTimer = 0.0f;

        // Socket handle (platform-specific)
        int m_Socket = -1;
        bool m_Running = false;
        std::thread m_NetworkThread;

        void networkThreadFunc();
    };

}}
