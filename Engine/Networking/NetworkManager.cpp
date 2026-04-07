#include "NetworkManager.hpp"
#include <cstring>
#include <algorithm>

#ifdef GE_PLATFORM_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using SocketType = SOCKET;
    #define INVALID_SOCK INVALID_SOCKET
    #define CLOSE_SOCKET closesocket
    static bool s_WinsockInitialized = false;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <poll.h>
    using SocketType = int;
    #define INVALID_SOCK (-1)
    #define CLOSE_SOCKET ::close
#endif

namespace GameEngine {
namespace Networking {

    NetworkManager::NetworkManager() {
#ifdef GE_PLATFORM_WINDOWS
        if (!s_WinsockInitialized) {
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
            s_WinsockInitialized = true;
        }
#endif
    }

    NetworkManager::~NetworkManager() {
        disconnect();
    }

    bool NetworkManager::startServer(uint16_t port, int /*maxClients*/) {
        m_Socket = static_cast<int>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
        if (m_Socket == static_cast<int>(INVALID_SOCK)) return false;

        // Set non-blocking
#ifdef GE_PLATFORM_WINDOWS
        unsigned long mode = 1;
        ioctlsocket(static_cast<SocketType>(m_Socket), FIONBIO, &mode);
#else
        int flags = fcntl(m_Socket, F_GETFL, 0);
        fcntl(m_Socket, F_SETFL, flags | O_NONBLOCK);
#endif

        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(static_cast<SocketType>(m_Socket), (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            CLOSE_SOCKET(static_cast<SocketType>(m_Socket));
            m_Socket = -1;
            return false;
        }

        m_Mode = Mode::Server;
        m_Port = port;
        m_LocalID = 0; // Server is always ID 0
        m_Connected = true;
        m_Running = true;

        m_NetworkThread = std::thread(&NetworkManager::networkThreadFunc, this);
        return true;
    }

    bool NetworkManager::connectToServer(const std::string& address, uint16_t port) {
        m_Socket = static_cast<int>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
        if (m_Socket == static_cast<int>(INVALID_SOCK)) return false;

        // Set non-blocking
#ifdef GE_PLATFORM_WINDOWS
        unsigned long mode = 1;
        ioctlsocket(static_cast<SocketType>(m_Socket), FIONBIO, &mode);
#else
        int flags = fcntl(m_Socket, F_GETFL, 0);
        fcntl(m_Socket, F_SETFL, flags | O_NONBLOCK);
#endif

        struct sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, address.c_str(), &serverAddr.sin_addr);

        // For UDP, connect sets the default destination
        ::connect(static_cast<SocketType>(m_Socket), (struct sockaddr*)&serverAddr, sizeof(serverAddr));

        m_Mode = Mode::Client;
        m_Port = port;
        m_Running = true;

        // Send connect packet
        NetworkPacket connectPacket;
        connectPacket.Type = PacketType::Connect;
        connectPacket.Reliable = true;
        sendToServer(connectPacket);

        m_NetworkThread = std::thread(&NetworkManager::networkThreadFunc, this);
        m_Connected = true;
        return true;
    }

    void NetworkManager::disconnect() {
        if (!m_Running) return;

        // Send disconnect to peers
        NetworkPacket disconnectPacket;
        disconnectPacket.Type = PacketType::Disconnect;
        disconnectPacket.SenderID = m_LocalID;
        if (m_Mode == Mode::Client) {
            sendToServer(disconnectPacket);
        } else if (m_Mode == Mode::Server) {
            broadcast(disconnectPacket);
        }

        m_Running = false;
        m_Connected = false;

        if (m_NetworkThread.joinable()) {
            m_NetworkThread.join();
        }

        if (m_Socket >= 0) {
            CLOSE_SOCKET(static_cast<SocketType>(m_Socket));
            m_Socket = -1;
        }

        m_Peers.clear();
        m_Mode = Mode::None;
    }

    void NetworkManager::update(float dt) {
        if (!m_Connected) return;

        processIncomingPackets();
        pingPeers(dt);
    }

    void NetworkManager::sendToServer(const NetworkPacket& packet) {
        std::lock_guard<std::mutex> lock(m_PacketMutex);
        m_OutgoingPackets.push(packet);
        m_BytesSent += packet.Data.size() + 8; // header overhead estimate
    }

    void NetworkManager::sendToClient(uint32_t clientID, const NetworkPacket& packet) {
        // In a real implementation, this would route to a specific client's address
        std::lock_guard<std::mutex> lock(m_PacketMutex);
        NetworkPacket routed = packet;
        routed.SenderID = m_LocalID;
        m_OutgoingPackets.push(routed);
        m_BytesSent += packet.Data.size() + 8;
    }

    void NetworkManager::broadcast(const NetworkPacket& packet, uint32_t excludeID) {
        for (const auto& peer : m_Peers) {
            if (peer.ID != excludeID && peer.Connected) {
                sendToClient(peer.ID, packet);
            }
        }
    }

    void NetworkManager::registerRPC(const std::string& name, RPCHandler handler) {
        m_RPCHandlers[name] = std::move(handler);
    }

    void NetworkManager::callRPC(const std::string& name, const NetworkPacket& args, uint32_t targetID) {
        NetworkPacket rpcPacket;
        rpcPacket.Type = PacketType::RPC;
        rpcPacket.SenderID = m_LocalID;
        rpcPacket.Reliable = true;
        rpcPacket.writeString(name);
        rpcPacket.Data.insert(rpcPacket.Data.end(), args.Data.begin(), args.Data.end());

        if (targetID == 0) {
            broadcast(rpcPacket);
        } else {
            sendToClient(targetID, rpcPacket);
        }
    }

    float NetworkManager::getAverageRTT() const {
        if (m_Peers.empty()) return 0.0f;
        float total = 0.0f;
        for (const auto& peer : m_Peers) total += peer.RTT;
        return total / static_cast<float>(m_Peers.size());
    }

    void NetworkManager::processIncomingPackets() {
        std::lock_guard<std::mutex> lock(m_PacketMutex);
        while (!m_IncomingPackets.empty()) {
            NetworkPacket packet = m_IncomingPackets.front();
            m_IncomingPackets.pop();
            handlePacket(packet);
        }
    }

    void NetworkManager::handlePacket(const NetworkPacket& packet) {
        switch (packet.Type) {
            case PacketType::Connect: {
                if (m_Mode == Mode::Server) {
                    NetworkPeer peer;
                    peer.ID = m_NextPeerID++;
                    peer.Connected = true;
                    m_Peers.push_back(peer);
                    if (OnPeerConnected) OnPeerConnected(peer.ID);
                } else {
                    // Client received connection acknowledgment
                    if (!packet.Data.empty()) {
                        auto reader = packet.createReader();
                        m_LocalID = reader.readUint32();
                    }
                }
                break;
            }
            case PacketType::Disconnect: {
                auto it = std::find_if(m_Peers.begin(), m_Peers.end(),
                    [&](const NetworkPeer& p) { return p.ID == packet.SenderID; });
                if (it != m_Peers.end()) {
                    uint32_t peerID = it->ID;
                    m_Peers.erase(it);
                    if (OnPeerDisconnected) OnPeerDisconnected(peerID);
                }
                break;
            }
            case PacketType::Ping: {
                NetworkPacket pong;
                pong.Type = PacketType::Pong;
                pong.SenderID = m_LocalID;
                pong.Data = packet.Data; // Echo timestamp back
                if (m_Mode == Mode::Client) sendToServer(pong);
                else sendToClient(packet.SenderID, pong);
                break;
            }
            case PacketType::Pong: {
                auto it = std::find_if(m_Peers.begin(), m_Peers.end(),
                    [&](const NetworkPeer& p) { return p.ID == packet.SenderID; });
                if (it != m_Peers.end() && packet.Data.size() >= 4) {
                    auto reader = packet.createReader();
                    float sentTime = reader.readFloat();
                    it->RTT = it->LastPingTime - sentTime;
                }
                break;
            }
            case PacketType::RPC: {
                auto reader = packet.createReader();
                std::string rpcName = reader.readString();
                auto handlerIt = m_RPCHandlers.find(rpcName);
                if (handlerIt != m_RPCHandlers.end()) {
                    handlerIt->second(packet.SenderID, reader);
                }
                break;
            }
            default: {
                if (OnPacketReceived) OnPacketReceived(packet.SenderID, packet);
                break;
            }
        }
    }

    void NetworkManager::pingPeers(float dt) {
        m_PingTimer += dt;
        if (m_PingTimer < 1.0f) return; // Ping every second
        m_PingTimer = 0.0f;

        NetworkPacket ping;
        ping.Type = PacketType::Ping;
        ping.SenderID = m_LocalID;
        ping.Reliable = false;
        ping.writeFloat(m_PingTimer);

        if (m_Mode == Mode::Client) {
            sendToServer(ping);
        } else {
            broadcast(ping);
        }
    }

    void NetworkManager::networkThreadFunc() {
        char buffer[4096];
        struct sockaddr_in fromAddr{};
        socklen_t fromLen = sizeof(fromAddr);

        while (m_Running) {
            // Receive
            int received = recvfrom(static_cast<SocketType>(m_Socket), buffer, sizeof(buffer), 0,
                                    (struct sockaddr*)&fromAddr, &fromLen);

            if (received > 0) {
                std::lock_guard<std::mutex> lock(m_PacketMutex);
                NetworkPacket packet;
                if (received >= 5) {
                    packet.Type = static_cast<PacketType>(buffer[0]);
                    memcpy(&packet.SenderID, buffer + 1, 4);
                    packet.Data.assign(buffer + 5, buffer + received);
                }
                m_IncomingPackets.push(packet);
                m_BytesReceived += received;
            }

            // Send
            {
                std::lock_guard<std::mutex> lock(m_PacketMutex);
                while (!m_OutgoingPackets.empty()) {
                    const auto& packet = m_OutgoingPackets.front();
                    std::vector<uint8_t> wire;
                    wire.push_back(static_cast<uint8_t>(packet.Type));
                    uint32_t id = packet.SenderID;
                    wire.insert(wire.end(), reinterpret_cast<uint8_t*>(&id),
                                reinterpret_cast<uint8_t*>(&id) + 4);
                    wire.insert(wire.end(), packet.Data.begin(), packet.Data.end());

                    send(static_cast<SocketType>(m_Socket),
                         reinterpret_cast<const char*>(wire.data()),
                         static_cast<int>(wire.size()), 0);

                    m_OutgoingPackets.pop();
                }
            }

            // Don't spin the CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

}}
