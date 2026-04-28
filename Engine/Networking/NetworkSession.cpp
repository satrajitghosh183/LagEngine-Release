#include "NetworkSession.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {
namespace Networking {

    NetworkSession::NetworkSession() = default;
    NetworkSession::~NetworkSession() { Disconnect(); }

    static Scope<Transport> MakeTransport(Transport::TransportType type) {
        switch (type) {
            case Transport::TransportType::UDP:
            case Transport::TransportType::ReliableUDP:
                return CreateReliableUDPTransport();
            case Transport::TransportType::WebSocket:
                return CreateWebSocketTransport();
        }
        return nullptr;
    }

    bool NetworkSession::StartServer(Transport::TransportType type, uint16_t port, int maxClients) {
        m_Transport = MakeTransport(type);
        if (!m_Transport) return false;
        return m_Transport->Listen(port, maxClients);
    }

    bool NetworkSession::ConnectToServer(Transport::TransportType type,
                                          const std::string& address, uint16_t port) {
        m_Transport = MakeTransport(type);
        if (!m_Transport) return false;
        return m_Transport->Connect(address, port);
    }

    void NetworkSession::Disconnect() {
        if (m_Transport) m_Transport->Disconnect();
        m_Transport.reset();
    }

    void NetworkSession::Tick(float deltaTime) {
        if (!m_Transport) return;

        m_Transport->Tick(deltaTime);

        TransportEvent ev;
        while (m_Transport->Poll(ev)) {
            switch (ev.Kind) {
                case TransportEvent::Connected:
                    if (OnPeerConnected) OnPeerConnected(ev.Peer);
                    break;
                case TransportEvent::Disconnected:
                    if (OnPeerDisconnected) OnPeerDisconnected(ev.Peer);
                    break;
                case TransportEvent::Receive: {
                    if (ev.Data.empty()) break;
                    uint8_t marker = ev.Data[0];
                    BinaryReader r(ev.Data);
                    r.ReadU8(); // consume marker
                    if (marker == RPCRegistry::kRPCMarker) {
                        if (r.HasBytes(4)) {
                            uint32_t rpcID = r.ReadU32();
                            m_RPC.Dispatch(ev.Peer, rpcID, r);
                        }
                    } else if (marker == RPCRegistry::kReplMarker) {
                        if (OnReplicationPacket) OnReplicationPacket(ev.Peer, r);
                    } else {
                        if (OnRawPacket)
                            OnRawPacket(ev.Peer, ev.Data.data(), ev.Data.size(), ev.Channel);
                    }
                    break;
                }
                default: break;
            }
        }
    }

    void NetworkSession::SendRPC(PeerID peer, uint32_t rpcID,
                                  const std::vector<uint8_t>& payload, TransferMode mode) {
        if (!m_Transport) return;
        m_Transport->Send(peer, payload.data(), payload.size(), mode, 0);
    }

    void NetworkSession::BroadcastRPC(uint32_t rpcID, const std::vector<uint8_t>& payload,
                                       TransferMode mode, PeerID exclude) {
        if (!m_Transport) return;
        m_Transport->Broadcast(payload.data(), payload.size(), mode, 0, exclude);
    }

    void NetworkSession::SendRaw(PeerID peer, const uint8_t* data, size_t size,
                                  TransferMode mode, uint8_t channel) {
        if (!m_Transport) return;
        m_Transport->Send(peer, data, size, mode, channel);
    }

    void NetworkSession::BroadcastRaw(const uint8_t* data, size_t size,
                                       TransferMode mode, uint8_t channel, PeerID exclude) {
        if (!m_Transport) return;
        m_Transport->Broadcast(data, size, mode, channel, exclude);
    }

}}
