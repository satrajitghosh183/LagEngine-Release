#include "WebSocketTransport.hpp"
#include "../Core/Logger.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <random>
#include <sstream>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using socklen_t = int;
    static inline void closeSocket(int s) { closesocket(s); }
    static int sendSocket(int s, const char* b, int n) { return send(s, b, n, 0); }
    static int recvSocket(int s, char* b, int n) { return recv(s, b, n, 0); }
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    static inline void closeSocket(int s) { ::close(s); }
    static int sendSocket(int s, const char* b, int n) { return (int)send(s, b, n, 0); }
    static int recvSocket(int s, char* b, int n) { return (int)recv(s, b, n, 0); }
#endif

namespace GameEngine {
namespace Networking {

    static constexpr uint8_t OP_CONT  = 0x0;
    static constexpr uint8_t OP_TEXT  = 0x1;
    static constexpr uint8_t OP_BIN   = 0x2;
    static constexpr uint8_t OP_CLOSE = 0x8;
    static constexpr uint8_t OP_PING  = 0x9;
    static constexpr uint8_t OP_PONG  = 0xA;

    WebSocketTransport::WebSocketTransport() {
#ifdef _WIN32
        static bool inited = false;
        if (!inited) { WSADATA w; WSAStartup(MAKEWORD(2,2), &w); inited = true; }
#endif
    }

    WebSocketTransport::~WebSocketTransport() { Disconnect(); }

    bool WebSocketTransport::Listen(uint16_t port, int maxPeers) {
        m_ListenSocket = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_ListenSocket < 0) return false;

        int opt = 1;
        setsockopt(m_ListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        if (bind(m_ListenSocket, (sockaddr*)&addr, sizeof(addr)) < 0) {
            closeSocket(m_ListenSocket);
            return false;
        }
        if (listen(m_ListenSocket, 16) < 0) {
            closeSocket(m_ListenSocket);
            return false;
        }

        m_MaxPeers = maxPeers;
        m_Mode = Mode::Server;
        m_Running.store(true);
        m_Connected.store(true);
        m_AcceptThread = std::thread([this]() { AcceptLoop(); });

        GE_CORE_INFO("WebSocket: listening on port {}", port);
        return true;
    }

    bool WebSocketTransport::Connect(const std::string& address, uint16_t port) {
        int sock = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) return false;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, address.c_str(), &addr.sin_addr);

        if (::connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
            closeSocket(sock);
            return false;
        }

        if (!PerformClientHandshake(sock, address, port, "/")) {
            closeSocket(sock);
            return false;
        }

        PeerID id = m_NextPeerID++;
        Connection conn;
        conn.ID = id;
        conn.Socket = sock;
        conn.Handshaked = true;
        conn.ClientSide = true;
        {
            std::lock_guard<std::mutex> lock(m_ConnMutex);
            m_Connections[id] = std::move(conn);
        }

        m_Mode = Mode::Client;
        m_Running.store(true);
        m_Connected.store(true);
        m_ReadThreads.emplace_back([this, id]() { ReadLoop(id); });

        {
            TransportEvent ev;
            ev.Kind = TransportEvent::Connected;
            ev.Peer = id;
            std::lock_guard<std::mutex> elock(m_EventMutex);
            m_Events.push_back(std::move(ev));
        }

        GE_CORE_INFO("WebSocket: connected to {}:{}", address, port);
        return true;
    }

    void WebSocketTransport::Disconnect() {
        if (!m_Running.exchange(false)) return;

        if (m_ListenSocket >= 0) {
            closeSocket(m_ListenSocket);
            m_ListenSocket = -1;
        }
        {
            std::lock_guard<std::mutex> lock(m_ConnMutex);
            for (auto& [id, conn] : m_Connections) {
                if (conn.Socket >= 0) {
                    uint8_t closeFrame[2] = { 0x03, 0xE8 }; // 1000 = normal close
                    SendFrame(conn.Socket, closeFrame, 2, OP_CLOSE, conn.ClientSide);
                    closeSocket(conn.Socket);
                }
            }
            m_Connections.clear();
        }

        if (m_AcceptThread.joinable()) m_AcceptThread.join();
        for (auto& t : m_ReadThreads) if (t.joinable()) t.join();
        m_ReadThreads.clear();

        m_Connected.store(false);
        m_Mode = Mode::None;
    }

    void WebSocketTransport::AcceptLoop() {
        while (m_Running.load()) {
            sockaddr_in client{};
            socklen_t clen = sizeof(client);
            int clientSock = (int)accept(m_ListenSocket, (sockaddr*)&client, &clen);
            if (clientSock < 0) {
                if (m_Running.load())
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            std::string clientKey;
            if (!PerformServerHandshake(clientSock, clientKey)) {
                closeSocket(clientSock);
                continue;
            }

            PeerID id = m_NextPeerID++;
            {
                std::lock_guard<std::mutex> lock(m_ConnMutex);
                if ((int)m_Connections.size() >= m_MaxPeers) {
                    closeSocket(clientSock);
                    continue;
                }
                Connection conn;
                conn.ID = id;
                conn.Socket = clientSock;
                conn.Handshaked = true;
                conn.ClientSide = false;
                m_Connections[id] = std::move(conn);
            }

            {
                TransportEvent ev;
                ev.Kind = TransportEvent::Connected;
                ev.Peer = id;
                std::lock_guard<std::mutex> elock(m_EventMutex);
                m_Events.push_back(std::move(ev));
            }

            m_ReadThreads.emplace_back([this, id]() { ReadLoop(id); });
        }
    }

    void WebSocketTransport::ReadLoop(PeerID peerID) {
        while (m_Running.load()) {
            int sock;
            {
                std::lock_guard<std::mutex> lock(m_ConnMutex);
                auto it = m_Connections.find(peerID);
                if (it == m_Connections.end()) return;
                sock = it->second.Socket;
            }

            char buf[4096];
            int n = recvSocket(sock, buf, sizeof(buf));
            if (n <= 0) {
                TransportEvent ev;
                ev.Kind = TransportEvent::Disconnected;
                ev.Peer = peerID;
                {
                    std::lock_guard<std::mutex> elock(m_EventMutex);
                    m_Events.push_back(std::move(ev));
                }
                std::lock_guard<std::mutex> lock(m_ConnMutex);
                m_Connections.erase(peerID);
                return;
            }

            m_BytesReceived.fetch_add((uint64_t)n);

            std::lock_guard<std::mutex> lock(m_ConnMutex);
            auto it = m_Connections.find(peerID);
            if (it == m_Connections.end()) return;
            it->second.RecvBuffer.insert(it->second.RecvBuffer.end(),
                                          buf, buf + n);

            // Try to parse all frames available
            std::vector<uint8_t> payload;
            uint8_t opcode;
            while (ParseFrame(it->second, payload, opcode)) {
                if (opcode == OP_BIN || opcode == OP_TEXT) {
                    TransportEvent ev;
                    ev.Kind = TransportEvent::Receive;
                    ev.Peer = peerID;
                    ev.Channel = 0;
                    ev.Data = std::move(payload);
                    std::lock_guard<std::mutex> elock(m_EventMutex);
                    m_Events.push_back(std::move(ev));
                } else if (opcode == OP_PING) {
                    SendFrame(sock, payload.data(), payload.size(), OP_PONG, it->second.ClientSide);
                } else if (opcode == OP_CLOSE) {
                    TransportEvent ev;
                    ev.Kind = TransportEvent::Disconnected;
                    ev.Peer = peerID;
                    std::lock_guard<std::mutex> elock(m_EventMutex);
                    m_Events.push_back(std::move(ev));
                    closeSocket(sock);
                    m_Connections.erase(peerID);
                    return;
                }
            }
        }
    }

    void WebSocketTransport::Send(PeerID peer, const uint8_t* data, size_t size,
                                   TransferMode, uint8_t) {
        std::lock_guard<std::mutex> lock(m_ConnMutex);
        auto it = m_Connections.find(peer);
        if (it == m_Connections.end()) return;
        SendFrame(it->second.Socket, data, size, OP_BIN, it->second.ClientSide);
    }

    void WebSocketTransport::Broadcast(const uint8_t* data, size_t size,
                                        TransferMode, uint8_t, PeerID exclude) {
        std::lock_guard<std::mutex> lock(m_ConnMutex);
        for (auto& [id, conn] : m_Connections) {
            if (id == exclude) continue;
            SendFrame(conn.Socket, data, size, OP_BIN, conn.ClientSide);
        }
    }

    bool WebSocketTransport::Poll(TransportEvent& outEvent) {
        std::lock_guard<std::mutex> lock(m_EventMutex);
        if (m_Events.empty()) return false;
        outEvent = std::move(m_Events.front());
        m_Events.pop_front();
        return true;
    }

    void WebSocketTransport::Tick(float) {}

    void WebSocketTransport::SendFrame(int sock, const uint8_t* payload, size_t size,
                                        uint8_t opcode, bool masked) {
        std::vector<uint8_t> frame;
        frame.push_back(0x80 | (opcode & 0x0F)); // FIN + opcode

        uint8_t maskBit = masked ? 0x80 : 0x00;
        if (size < 126) {
            frame.push_back(maskBit | (uint8_t)size);
        } else if (size < 65536) {
            frame.push_back(maskBit | 126);
            frame.push_back((size >> 8) & 0xFF);
            frame.push_back(size & 0xFF);
        } else {
            frame.push_back(maskBit | 127);
            for (int i = 7; i >= 0; i--)
                frame.push_back((size >> (i * 8)) & 0xFF);
        }

        if (masked) {
            uint8_t mask[4];
            std::random_device rd;
            for (int i = 0; i < 4; i++) mask[i] = (uint8_t)(rd() & 0xFF);
            frame.insert(frame.end(), mask, mask + 4);
            size_t off = frame.size();
            frame.resize(off + size);
            for (size_t i = 0; i < size; i++)
                frame[off + i] = payload[i] ^ mask[i % 4];
        } else {
            frame.insert(frame.end(), payload, payload + size);
        }

        int sent = sendSocket(sock, (const char*)frame.data(), (int)frame.size());
        if (sent > 0) m_BytesSent.fetch_add((uint64_t)sent);
    }

    bool WebSocketTransport::ParseFrame(Connection& conn,
                                         std::vector<uint8_t>& outPayload,
                                         uint8_t& outOpcode) {
        auto& buf = conn.RecvBuffer;
        if (buf.size() < 2) return false;

        uint8_t b0 = buf[0], b1 = buf[1];
        outOpcode = b0 & 0x0F;
        bool masked = (b1 & 0x80) != 0;
        uint64_t len = b1 & 0x7F;
        size_t headerSize = 2;

        if (len == 126) {
            if (buf.size() < 4) return false;
            len = (buf[2] << 8) | buf[3];
            headerSize = 4;
        } else if (len == 127) {
            if (buf.size() < 10) return false;
            len = 0;
            for (int i = 0; i < 8; i++) len = (len << 8) | buf[2 + i];
            headerSize = 10;
        }

        uint8_t mask[4] = {};
        if (masked) {
            if (buf.size() < headerSize + 4) return false;
            for (int i = 0; i < 4; i++) mask[i] = buf[headerSize + i];
            headerSize += 4;
        }

        if (buf.size() < headerSize + len) return false;

        outPayload.resize(len);
        for (uint64_t i = 0; i < len; i++) {
            outPayload[i] = buf[headerSize + i];
            if (masked) outPayload[i] ^= mask[i % 4];
        }

        buf.erase(buf.begin(), buf.begin() + headerSize + len);
        return true;
    }

    // ---- HTTP handshake (server) -------------------------------------------

    static std::string recvHttpHeader(int sock) {
        std::string buf;
        char c;
        int total = 0;
        while (total < 4096) {
            int n = recvSocket(sock, &c, 1);
            if (n <= 0) break;
            buf += c;
            total++;
            if (buf.size() >= 4 && buf.substr(buf.size() - 4) == "\r\n\r\n") break;
        }
        return buf;
    }

    bool WebSocketTransport::PerformServerHandshake(int sock, std::string& outKey) {
        std::string header = recvHttpHeader(sock);
        size_t keyPos = header.find("Sec-WebSocket-Key:");
        if (keyPos == std::string::npos) return false;
        keyPos += 18;
        while (keyPos < header.size() && (header[keyPos] == ' ' || header[keyPos] == '\t')) keyPos++;
        size_t endPos = header.find("\r\n", keyPos);
        if (endPos == std::string::npos) return false;
        outKey = header.substr(keyPos, endPos - keyPos);

        std::string acceptKey = ComputeAcceptKey(outKey);
        std::ostringstream resp;
        resp << "HTTP/1.1 101 Switching Protocols\r\n"
             << "Upgrade: websocket\r\n"
             << "Connection: Upgrade\r\n"
             << "Sec-WebSocket-Accept: " << acceptKey << "\r\n\r\n";
        std::string s = resp.str();
        return sendSocket(sock, s.c_str(), (int)s.size()) > 0;
    }

    bool WebSocketTransport::PerformClientHandshake(int sock, const std::string& host,
                                                      uint16_t port, const std::string& path) {
        // Generate 16-byte random key, base64-encode
        uint8_t key[16];
        std::random_device rd;
        for (int i = 0; i < 16; i++) key[i] = (uint8_t)(rd() & 0xFF);
        std::string keyB64 = Base64Encode(key, 16);

        std::ostringstream req;
        req << "GET " << path << " HTTP/1.1\r\n"
            << "Host: " << host << ":" << port << "\r\n"
            << "Upgrade: websocket\r\n"
            << "Connection: Upgrade\r\n"
            << "Sec-WebSocket-Key: " << keyB64 << "\r\n"
            << "Sec-WebSocket-Version: 13\r\n\r\n";
        std::string s = req.str();
        if (sendSocket(sock, s.c_str(), (int)s.size()) <= 0) return false;

        std::string resp = recvHttpHeader(sock);
        return resp.find("101 Switching Protocols") != std::string::npos;
    }

    // ---- SHA-1 (minimal implementation for WebSocket accept key) -----------

    void WebSocketTransport::SHA1Compute(const uint8_t* data, size_t len, uint8_t out[20]) {
        uint32_t h[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
        uint64_t bitLen = (uint64_t)len * 8;

        std::vector<uint8_t> padded(data, data + len);
        padded.push_back(0x80);
        while (padded.size() % 64 != 56) padded.push_back(0);
        for (int i = 7; i >= 0; i--) padded.push_back((bitLen >> (i * 8)) & 0xFF);

        auto rotl = [](uint32_t v, int n) -> uint32_t { return (v << n) | (v >> (32 - n)); };

        for (size_t chunk = 0; chunk < padded.size(); chunk += 64) {
            uint32_t w[80];
            for (int i = 0; i < 16; i++) {
                w[i] = ((uint32_t)padded[chunk + i*4] << 24)
                     | ((uint32_t)padded[chunk + i*4 + 1] << 16)
                     | ((uint32_t)padded[chunk + i*4 + 2] << 8)
                     |  (uint32_t)padded[chunk + i*4 + 3];
            }
            for (int i = 16; i < 80; i++)
                w[i] = rotl(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);

            uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
            for (int i = 0; i < 80; i++) {
                uint32_t f, k;
                if (i < 20)      { f = (b & c) | ((~b) & d); k = 0x5A827999; }
                else if (i < 40) { f = b ^ c ^ d;             k = 0x6ED9EBA1; }
                else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
                else             { f = b ^ c ^ d;             k = 0xCA62C1D6; }
                uint32_t temp = rotl(a, 5) + f + e + k + w[i];
                e = d; d = c; c = rotl(b, 30); b = a; a = temp;
            }
            h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
        }

        for (int i = 0; i < 5; i++) {
            out[i*4]   = (h[i] >> 24) & 0xFF;
            out[i*4+1] = (h[i] >> 16) & 0xFF;
            out[i*4+2] = (h[i] >> 8)  & 0xFF;
            out[i*4+3] =  h[i]        & 0xFF;
        }
    }

    std::string WebSocketTransport::Base64Encode(const uint8_t* data, size_t len) {
        static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        int val = 0, valb = -6;
        for (size_t i = 0; i < len; i++) {
            val = (val << 8) + data[i];
            valb += 8;
            while (valb >= 0) {
                out.push_back(chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }

    std::string WebSocketTransport::ComputeAcceptKey(const std::string& clientKey) {
        static const char* guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        std::string concat = clientKey + guid;
        uint8_t hash[20];
        SHA1Compute(reinterpret_cast<const uint8_t*>(concat.data()), concat.size(), hash);
        return Base64Encode(hash, 20);
    }

    Scope<Transport> CreateWebSocketTransport() {
        return CreateScope<WebSocketTransport>();
    }

}}
