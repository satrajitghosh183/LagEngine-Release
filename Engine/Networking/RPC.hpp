#pragma once

#include "Transport.hpp"
#include "../Core/Base.hpp"
#include <cstring>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace GameEngine {
namespace Networking {

    // ---- Binary writer/reader ----------------------------------------------

    class BinaryWriter {
    public:
        const std::vector<uint8_t>& Data() const { return m_Data; }
        std::vector<uint8_t> TakeData() { return std::move(m_Data); }

        void WriteU8 (uint8_t  v) { m_Data.push_back(v); }
        void WriteU16(uint16_t v) { append(&v, 2); }
        void WriteU32(uint32_t v) { append(&v, 4); }
        void WriteU64(uint64_t v) { append(&v, 8); }
        void WriteI32(int32_t v)  { append(&v, 4); }
        void WriteF32(float v)    { append(&v, 4); }
        void WriteF64(double v)   { append(&v, 8); }
        void WriteBool(bool v)    { m_Data.push_back(v ? 1 : 0); }

        void WriteString(const std::string& s) {
            WriteU16(static_cast<uint16_t>(s.size()));
            m_Data.insert(m_Data.end(), s.begin(), s.end());
        }
        void WriteBytes(const uint8_t* p, size_t n) {
            WriteU32(static_cast<uint32_t>(n));
            m_Data.insert(m_Data.end(), p, p + n);
        }

        void WriteVec2(const glm::vec2& v) { WriteF32(v.x); WriteF32(v.y); }
        void WriteVec3(const glm::vec3& v) { WriteF32(v.x); WriteF32(v.y); WriteF32(v.z); }
        void WriteVec4(const glm::vec4& v) { WriteF32(v.x); WriteF32(v.y); WriteF32(v.z); WriteF32(v.w); }
        void WriteQuat(const glm::quat& q) { WriteF32(q.x); WriteF32(q.y); WriteF32(q.z); WriteF32(q.w); }
        void WriteMat4(const glm::mat4& m) {
            for (int c = 0; c < 4; c++) for (int r = 0; r < 4; r++) WriteF32(m[c][r]);
        }

    private:
        template<typename T>
        void append(const T* v, size_t n) {
            const uint8_t* p = reinterpret_cast<const uint8_t*>(v);
            m_Data.insert(m_Data.end(), p, p + n);
        }
        std::vector<uint8_t> m_Data;
    };

    class BinaryReader {
    public:
        BinaryReader(const uint8_t* data, size_t size) : m_Data(data), m_Size(size) {}
        BinaryReader(const std::vector<uint8_t>& v) : m_Data(v.data()), m_Size(v.size()) {}

        bool HasBytes(size_t n) const { return m_Pos + n <= m_Size; }
        size_t Remaining() const { return m_Size - m_Pos; }
        bool IsAtEnd() const { return m_Pos >= m_Size; }

        uint8_t  ReadU8 () { check(1); return m_Data[m_Pos++]; }
        uint16_t ReadU16() { uint16_t v; read(&v, 2); return v; }
        uint32_t ReadU32() { uint32_t v; read(&v, 4); return v; }
        uint64_t ReadU64() { uint64_t v; read(&v, 8); return v; }
        int32_t  ReadI32() { int32_t v;  read(&v, 4); return v; }
        float    ReadF32() { float v;    read(&v, 4); return v; }
        double   ReadF64() { double v;   read(&v, 8); return v; }
        bool     ReadBool(){ return ReadU8() != 0; }

        std::string ReadString() {
            uint16_t len = ReadU16();
            check(len);
            std::string s(reinterpret_cast<const char*>(m_Data + m_Pos), len);
            m_Pos += len;
            return s;
        }
        std::vector<uint8_t> ReadBytes() {
            uint32_t len = ReadU32();
            check(len);
            std::vector<uint8_t> v(m_Data + m_Pos, m_Data + m_Pos + len);
            m_Pos += len;
            return v;
        }

        glm::vec2 ReadVec2() { return {ReadF32(), ReadF32()}; }
        glm::vec3 ReadVec3() { return {ReadF32(), ReadF32(), ReadF32()}; }
        glm::vec4 ReadVec4() { return {ReadF32(), ReadF32(), ReadF32(), ReadF32()}; }
        glm::quat ReadQuat() { float x = ReadF32(), y = ReadF32(), z = ReadF32(), w = ReadF32(); return glm::quat(w, x, y, z); }
        glm::mat4 ReadMat4() {
            glm::mat4 m;
            for (int c = 0; c < 4; c++) for (int r = 0; r < 4; r++) m[c][r] = ReadF32();
            return m;
        }

    private:
        void check(size_t n) const { if (!HasBytes(n)) throw std::runtime_error("BinaryReader: out of bounds"); }
        template<typename T>
        void read(T* out, size_t n) { check(n); std::memcpy(out, m_Data + m_Pos, n); m_Pos += n; }

        const uint8_t* m_Data;
        size_t m_Size;
        size_t m_Pos = 0;
    };

    // ---- RPC authority / transfer modes ------------------------------------

    enum class RPCAuthority : uint8_t {
        Authority = 0,   // Server only can call
        AnyPeer   = 1,   // Any peer can call
        CallLocal = 2,   // Execute on sender locally + remotes
    };

    // ---- RPC handler signature ---------------------------------------------

    using RPCHandler = std::function<void(PeerID sender, BinaryReader& args)>;

    struct RPCInfo {
        std::string Name;
        uint32_t ID;
        RPCAuthority Authority;
        TransferMode Mode;
        RPCHandler Handler;
    };

    // ---- RPC registry ------------------------------------------------------

    class RPCRegistry {
    public:
        // Register an RPC. Returns the numeric ID assigned.
        uint32_t Register(const std::string& name, RPCAuthority auth,
                          TransferMode mode, RPCHandler handler);

        const RPCInfo* Find(const std::string& name) const;
        const RPCInfo* Find(uint32_t id) const;

        // Dispatch a received RPC packet. Reader is positioned after the RPC header.
        bool Dispatch(PeerID sender, uint32_t rpcID, BinaryReader& args) const;

        // Serialize an RPC call into a byte stream ready to send via Transport.
        // Format: [u8 RPC_MARKER=0xAA][u32 rpcID][payload...]
        template<typename... Args>
        static std::vector<uint8_t> Serialize(uint32_t rpcID, const Args&... args) {
            BinaryWriter w;
            w.WriteU8(kRPCMarker);
            w.WriteU32(rpcID);
            (WriteArg(w, args), ...);
            return w.TakeData();
        }

        static constexpr uint8_t kRPCMarker = 0xAA;
        static constexpr uint8_t kReplMarker = 0xBB;

    private:
        static void WriteArg(BinaryWriter& w, int32_t v)            { w.WriteI32(v); }
        static void WriteArg(BinaryWriter& w, uint32_t v)           { w.WriteU32(v); }
        static void WriteArg(BinaryWriter& w, uint64_t v)           { w.WriteU64(v); }
        static void WriteArg(BinaryWriter& w, float v)              { w.WriteF32(v); }
        static void WriteArg(BinaryWriter& w, bool v)               { w.WriteBool(v); }
        static void WriteArg(BinaryWriter& w, const std::string& v) { w.WriteString(v); }
        static void WriteArg(BinaryWriter& w, const glm::vec3& v)   { w.WriteVec3(v); }
        static void WriteArg(BinaryWriter& w, const glm::quat& v)   { w.WriteQuat(v); }

        std::unordered_map<std::string, uint32_t> m_NameToID;
        std::unordered_map<uint32_t, RPCInfo> m_Infos;
        uint32_t m_NextID = 1;
    };

}}
