#include "RPC.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {
namespace Networking {

    uint32_t RPCRegistry::Register(const std::string& name, RPCAuthority auth,
                                    TransferMode mode, RPCHandler handler) {
        auto it = m_NameToID.find(name);
        if (it != m_NameToID.end()) {
            GE_CORE_WARN("RPC: '{}' already registered, replacing handler", name);
            m_Infos[it->second].Handler = std::move(handler);
            m_Infos[it->second].Authority = auth;
            m_Infos[it->second].Mode = mode;
            return it->second;
        }

        uint32_t id = m_NextID++;
        RPCInfo info;
        info.Name = name;
        info.ID = id;
        info.Authority = auth;
        info.Mode = mode;
        info.Handler = std::move(handler);
        m_Infos[id] = std::move(info);
        m_NameToID[name] = id;
        return id;
    }

    const RPCInfo* RPCRegistry::Find(const std::string& name) const {
        auto it = m_NameToID.find(name);
        if (it == m_NameToID.end()) return nullptr;
        auto jt = m_Infos.find(it->second);
        return jt == m_Infos.end() ? nullptr : &jt->second;
    }

    const RPCInfo* RPCRegistry::Find(uint32_t id) const {
        auto it = m_Infos.find(id);
        return it == m_Infos.end() ? nullptr : &it->second;
    }

    bool RPCRegistry::Dispatch(PeerID sender, uint32_t rpcID, BinaryReader& args) const {
        const RPCInfo* info = Find(rpcID);
        if (!info) {
            GE_CORE_WARN("RPC: unknown ID {}", rpcID);
            return false;
        }
        try {
            info->Handler(sender, args);
            return true;
        } catch (const std::exception& e) {
            GE_CORE_ERROR("RPC '{}' threw: {}", info->Name, e.what());
            return false;
        }
    }

}}
