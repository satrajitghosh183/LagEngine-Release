#include "ReliableUDP.hpp"
#include "../Core/Logger.hpp"

#include <cstring>
#include <chrono>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using socklen_t = int;
    static inline void closeSocket(int s) { closesocket(s); }
    static bool g_WinsockInited = false;
    static void initWinsock() {
        if (!g_WinsockInited) {
            WSADATA wsa;
            WSAStartup(MAKEWORD(2, 2), &wsa);
            g_WinsockInited = true;
        }
    }
#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <netdb.h>
    static inline void closeSocket(int s) { ::close(s); }
    static void initWinsock() {}
#endif

namespace GameEngine {
namespace Networking {

    constexpr uint16_t kMagic = 0x4C41; // 'LA'
    constexpr size_t kHeaderSize = 8;
    constexpr size_t kMaxPayloadSize = 1400; // MTU-safe

    enum PacketFlags : uint8_t {
        FLAG_RELIABLE   = 1 << 0,
        FLAG_ACK        = 1 << 1,
        FLAG_CONNECT    = 1 << 2,
        FLAG_DISCONNECT = 1 << 3,
        FLAG_PING       = 1 << 4,
    };

    ReliableUDPTransport::ReliableUDPTransport() {
        initWinsock();
    }

    ReliableUDPTransport::~ReliableUDPTransport() {
        Disconnect();
    }

    static bool setNonBlocking(int sock) {
#ifdef _WIN32
        u_long mode = 1;
        return ioctlsocket(sock, FIONBIO, &mode) == 0;
#else
        int flags = fcntl(sock, F_GETFL, 0);
        return fcntl(sock, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
    }

    bool ReliableUDPTransport::Listen(uint16_t port, int maxPeers) {
        m_Socket = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_Socket < 0) {
            GE_CORE_ERROR("ReliableUDP: socket() failed");
            return false;
        }
        if (!setNonBlocking(m_Socket)) {
            GE_CORE_ERROR("ReliableUDP: failed to set non-blocking");
            closeSocket(m_Socket);
            return false;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        if (bind(m_Socket, (sockaddr*)&addr, sizeof(addr)) < 0) {
            GE_CORE_ERROR("ReliableUDP: bind failed on port {}", port);
            closeSocket(m_Socket);
            return false;
        }

        m_Port = port;
        m_MaxPeers = maxPeers;
        m_Mode = Mode::Server;
        m_Connected.store(true);
        m_Running.store(true);
        m_NetThread = std::thread([this]() { NetworkThreadLoop(); });

        GE_CORE_INFO("ReliableUDP: listening on port {}", port);
        return true;
    }

    bool ReliableUDPTransport::Connect(const std::string& address, uint16_t port) {
        m_Socket = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_Socket < 0) return false;
        setNonBlocking(m_Socket);

        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_port = 0; // any ephemeral port
        bind(m_Socket, (sockaddr*)&local, sizeof(local));

        sockaddr_in remote{};
        remote.sin_family = AF_INET;
        remote.sin_port = htons(port);
        inet_pton(AF_INET, address.c_str(), &remote.sin_addr);

        m_ServerAddr.assign((uint8_t*)&remote, (uint8_t*)&remote + sizeof(remote));
        m_ServerAddrLen = sizeof(remote);
        m_Mode = Mode::Client;
        m_Running.store(true);
        m_NetThread = std::thread([this]() { NetworkThreadLoop(); });

        // Send Connect packet
        {
            std::lock_guard<std::mutex> lock(m_PeerMutex);
            PeerState peer;
            peer.ID = 1; // server is peer 1 on client side
            peer.Address = address;
            peer.Port = port;
            peer.SockAddr = m_ServerAddr;
            peer.SockAddrLen = m_ServerAddrLen;
            peer.Connected = true;
            m_Peers[1] = peer;
            SendWire(m_Peers[1], FLAG_CONNECT | FLAG_RELIABLE, 0, m_Peers[1].NextOutgoingSeq++, nullptr, 0);
        }

        m_Connected.store(true);
        GE_CORE_INFO("ReliableUDP: connecting to {}:{}", address, port);
        return true;
    }

    void ReliableUDPTransport::Disconnect() {
        if (!m_Running.exchange(false)) return;

        {
            std::lock_guard<std::mutex> lock(m_PeerMutex);
            for (auto& [id, peer] : m_Peers) {
                if (peer.Connected) {
                    SendWire(peer, FLAG_DISCONNECT, 0, 0, nullptr, 0);
                }
            }
            m_Peers.clear();
        }

        if (m_NetThread.joinable()) m_NetThread.join();
        if (m_Socket >= 0) {
            closeSocket(m_Socket);
            m_Socket = -1;
        }
        m_Connected.store(false);
        m_Mode = Mode::None;
    }

    void ReliableUDPTransport::Send(PeerID peer, const uint8_t* data, size_t size,
                                     TransferMode mode, uint8_t channel) {
        if (size > kMaxPayloadSize) {
            GE_CORE_WARN("ReliableUDP: payload {} exceeds MTU {}", size, kMaxPayloadSize);
            return;
        }

        std::lock_guard<std::mutex> lock(m_PeerMutex);
        PeerID targetID = (m_Mode == Mode::Client) ? 1 : peer;
        auto it = m_Peers.find(targetID);
        if (it == m_Peers.end()) return;

        PeerState& p = it->second;
        uint8_t flags = (mode == TransferMode::Reliable) ? FLAG_RELIABLE : 0;
        uint16_t seq = p.NextOutgoingSeq++;

        SendWire(p, flags, channel, seq, data, size);

        if (mode == TransferMode::Reliable) {
            PeerState::PendingSend pending;
            pending.Seq = seq;
            pending.Channel = channel;
            pending.Data.assign(data, data + size);
            p.ReliableQueue.push_back(std::move(pending));
        }
    }

    void ReliableUDPTransport::Broadcast(const uint8_t* data, size_t size,
                                          TransferMode mode, uint8_t channel,
                                          PeerID exclude) {
        std::lock_guard<std::mutex> lock(m_PeerMutex);
        for (auto& [id, peer] : m_Peers) {
            if (id == exclude || !peer.Connected) continue;
            uint8_t flags = (mode == TransferMode::Reliable) ? FLAG_RELIABLE : 0;
            uint16_t seq = peer.NextOutgoingSeq++;
            SendWire(peer, flags, channel, seq, data, size);
            if (mode == TransferMode::Reliable) {
                PeerState::PendingSend pending;
                pending.Seq = seq;
                pending.Channel = channel;
                pending.Data.assign(data, data + size);
                peer.ReliableQueue.push_back(std::move(pending));
            }
        }
    }

    bool ReliableUDPTransport::Poll(TransportEvent& outEvent) {
        std::lock_guard<std::mutex> lock(m_EventMutex);
        if (m_Events.empty()) return false;
        outEvent = std::move(m_Events.front());
        m_Events.pop_front();
        return true;
    }

    void ReliableUDPTransport::Tick(float deltaTime) {
        std::lock_guard<std::mutex> lock(m_PeerMutex);
        for (auto& [id, peer] : m_Peers) {
            peer.TimeSinceLastHeard += deltaTime;

            // Resend unacked reliable packets
            for (auto it = peer.ReliableQueue.begin(); it != peer.ReliableQueue.end();) {
                it->TimeUntilResend -= deltaTime;
                if (it->TimeUntilResend <= 0.0f) {
                    if (it->RetriesLeft > 0) {
                        SendWire(peer, FLAG_RELIABLE, it->Channel, it->Seq, it->Data.data(), it->Data.size());
                        it->TimeUntilResend = 0.2f * (11 - it->RetriesLeft); // backoff
                        it->RetriesLeft--;
                        ++it;
                    } else {
                        // Give up — peer probably gone
                        it = peer.ReliableQueue.erase(it);
                    }
                } else {
                    ++it;
                }
            }
        }
    }

    void ReliableUDPTransport::NetworkThreadLoop() {
        uint8_t buf[2048];
        while (m_Running.load()) {
            sockaddr_storage from{};
            socklen_t fromLen = sizeof(from);
            int n = recvfrom(m_Socket, (char*)buf, sizeof(buf), 0,
                             (sockaddr*)&from, &fromLen);
            if (n > 0) {
                m_BytesReceived.fetch_add((uint64_t)n);
                HandleIncoming(buf, (size_t)n, &from, (int)fromLen);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    void ReliableUDPTransport::HandleIncoming(const uint8_t* buf, size_t len,
                                               const void* fromAddr, int fromLen) {
        if (len < kHeaderSize) return;

        uint16_t magic = buf[0] | (buf[1] << 8);
        if (magic != kMagic) return;

        uint8_t flags = buf[2];
        uint8_t channel = buf[3];
        uint16_t seq = buf[4] | (buf[5] << 8);
        uint16_t ack = buf[6] | (buf[7] << 8);

        const uint8_t* payload = buf + kHeaderSize;
        size_t payloadSize = len - kHeaderSize;

        PeerID peerID = FindOrCreatePeer(fromAddr, fromLen);
        if (peerID == kInvalidPeer) return;

        std::lock_guard<std::mutex> lock(m_PeerMutex);
        auto it = m_Peers.find(peerID);
        if (it == m_Peers.end()) return;
        PeerState& peer = it->second;
        peer.TimeSinceLastHeard = 0.0f;

        // Process ack if present
        if (flags & FLAG_ACK) {
            for (auto qit = peer.ReliableQueue.begin(); qit != peer.ReliableQueue.end();) {
                if (qit->Seq == ack) {
                    qit = peer.ReliableQueue.erase(qit);
                    break;
                } else {
                    ++qit;
                }
            }
            return;
        }

        // Handle connection lifecycle
        if (flags & FLAG_CONNECT) {
            if (!peer.Connected) {
                peer.Connected = true;
                TransportEvent ev;
                ev.Kind = TransportEvent::Connected;
                ev.Peer = peerID;
                std::lock_guard<std::mutex> elock(m_EventMutex);
                m_Events.push_back(std::move(ev));
            }
            if (flags & FLAG_RELIABLE) SendAck(peer, seq);
            return;
        }

        if (flags & FLAG_DISCONNECT) {
            peer.Connected = false;
            TransportEvent ev;
            ev.Kind = TransportEvent::Disconnected;
            ev.Peer = peerID;
            std::lock_guard<std::mutex> elock(m_EventMutex);
            m_Events.push_back(std::move(ev));
            return;
        }

        // Data packet
        if (flags & FLAG_RELIABLE) {
            SendAck(peer, seq);
            // Drop if already received (simple duplicate suppression)
            if (seq <= peer.LastReceivedSeq && peer.LastReceivedSeq != 0) return;
            peer.LastReceivedSeq = seq;
        }

        TransportEvent ev;
        ev.Kind = TransportEvent::Receive;
        ev.Peer = peerID;
        ev.Channel = channel;
        ev.Data.assign(payload, payload + payloadSize);
        std::lock_guard<std::mutex> elock(m_EventMutex);
        m_Events.push_back(std::move(ev));
    }

    PeerID ReliableUDPTransport::FindOrCreatePeer(const void* fromAddr, int fromLen) {
        const sockaddr_in* from = (const sockaddr_in*)fromAddr;

        std::lock_guard<std::mutex> lock(m_PeerMutex);
        for (auto& [id, peer] : m_Peers) {
            const sockaddr_in* pa = (const sockaddr_in*)peer.SockAddr.data();
            if (pa->sin_port == from->sin_port &&
                pa->sin_addr.s_addr == from->sin_addr.s_addr) {
                return id;
            }
        }

        if (m_Mode != Mode::Server) return kInvalidPeer;
        if ((int)m_Peers.size() >= m_MaxPeers) return kInvalidPeer;

        PeerID newID = m_NextPeerID++;
        PeerState peer;
        peer.ID = newID;
        char addrStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from->sin_addr, addrStr, INET_ADDRSTRLEN);
        peer.Address = addrStr;
        peer.Port = ntohs(from->sin_port);
        peer.SockAddr.assign((const uint8_t*)fromAddr, (const uint8_t*)fromAddr + fromLen);
        peer.SockAddrLen = fromLen;
        m_Peers[newID] = std::move(peer);
        return newID;
    }

    void ReliableUDPTransport::SendWire(PeerState& peer, uint8_t flags, uint8_t channel,
                                         uint16_t seq, const uint8_t* payload, size_t payloadSize) {
        uint8_t buf[2048];
        buf[0] = kMagic & 0xFF;
        buf[1] = (kMagic >> 8) & 0xFF;
        buf[2] = flags;
        buf[3] = channel;
        buf[4] = seq & 0xFF;
        buf[5] = (seq >> 8) & 0xFF;
        buf[6] = peer.LastReceivedSeq & 0xFF;
        buf[7] = (peer.LastReceivedSeq >> 8) & 0xFF;
        if (payloadSize > 0 && payload) {
            memcpy(buf + kHeaderSize, payload, payloadSize);
        }
        size_t total = kHeaderSize + payloadSize;
        int sent = sendto(m_Socket, (const char*)buf, (int)total, 0,
                          (const sockaddr*)peer.SockAddr.data(), peer.SockAddrLen);
        if (sent > 0) m_BytesSent.fetch_add((uint64_t)sent);
    }

    void ReliableUDPTransport::SendAck(PeerState& peer, uint16_t seq) {
        uint8_t buf[8];
        buf[0] = kMagic & 0xFF;
        buf[1] = (kMagic >> 8) & 0xFF;
        buf[2] = FLAG_ACK;
        buf[3] = 0;
        buf[4] = 0; buf[5] = 0;
        buf[6] = seq & 0xFF;
        buf[7] = (seq >> 8) & 0xFF;
        sendto(m_Socket, (const char*)buf, 8, 0,
               (const sockaddr*)peer.SockAddr.data(), peer.SockAddrLen);
        m_BytesSent.fetch_add(8);
    }

    Scope<Transport> CreateReliableUDPTransport() {
        return CreateScope<ReliableUDPTransport>();
    }

}}
