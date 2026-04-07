#pragma once

#include "../../Core/Base.hpp"
#include "VisualScriptNode.hpp"
#include <nlohmann/json.hpp>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <queue>
#include <cstdint>

namespace GameEngine {

    /**
     * @brief A directed link between two pins on different nodes.
     */
    struct VisualScriptLink {
        uint32_t ID           = 0;
        uint32_t SourceNodeID = 0;
        uint32_t SourcePinID  = 0;
        uint32_t TargetNodeID = 0;
        uint32_t TargetPinID  = 0;
    };

    /**
     * @brief The top-level data structure that owns every node and link in a
     *        visual script.
     *
     * The graph is a hybrid control-flow / data-flow graph (like UE Blueprints).
     * Flow links determine execution order; data links determine how values
     * propagate between nodes.
     *
     * Thread safety: NOT thread-safe.  Access from one thread at a time.
     */
    class VisualScriptGraph {
    public:
        VisualScriptGraph() = default;
        ~VisualScriptGraph() = default;

        // ── Name / metadata ─────────────────────────────────────────

        void setName(const std::string& name) { m_Name = name; }
        const std::string& getName() const     { return m_Name; }

        // ── Node management ─────────────────────────────────────────

        /**
         * @brief Add a node to the graph.  The graph takes shared ownership.
         * @return The ID assigned to the node.
         */
        uint32_t addNode(Ref<VisualScriptNode> node) {
            node->ID = m_NextNodeID++;
            m_Nodes.push_back(node);
            m_NodeMap[node->ID] = node;
            return node->ID;
        }

        /**
         * @brief Remove a node and all its connected links.
         */
        void removeNode(uint32_t nodeID) {
            // Remove links that reference this node
            m_Links.erase(
                std::remove_if(m_Links.begin(), m_Links.end(),
                    [nodeID](const VisualScriptLink& l) {
                        return l.SourceNodeID == nodeID || l.TargetNodeID == nodeID;
                    }),
                m_Links.end());

            m_Nodes.erase(
                std::remove_if(m_Nodes.begin(), m_Nodes.end(),
                    [nodeID](const Ref<VisualScriptNode>& n) { return n->ID == nodeID; }),
                m_Nodes.end());
            m_NodeMap.erase(nodeID);
        }

        /**
         * @brief Find a node by its ID.
         */
        Ref<VisualScriptNode> findNode(uint32_t nodeID) const {
            auto it = m_NodeMap.find(nodeID);
            return it != m_NodeMap.end() ? it->second : nullptr;
        }

        const std::vector<Ref<VisualScriptNode>>& getNodes() const { return m_Nodes; }

        // ── Link management ─────────────────────────────────────────

        /**
         * @brief Create a link between two pins.
         * @return The link ID, or 0 if the connection is invalid.
         */
        uint32_t addLink(uint32_t srcNodeID, uint32_t srcPinID,
                         uint32_t dstNodeID, uint32_t dstPinID) {
            // Validate nodes exist
            auto srcNode = findNode(srcNodeID);
            auto dstNode = findNode(dstNodeID);
            if (!srcNode || !dstNode) return 0;

            // Validate pins exist
            auto* srcPin = srcNode->getPin(srcPinID);
            auto* dstPin = dstNode->getPin(dstPinID);
            if (!srcPin || !dstPin) return 0;

            // Source must be output, target must be input
            if (srcPin->Direction != PinDirection::Output) return 0;
            if (dstPin->Direction != PinDirection::Input)  return 0;

            // Types must match (Flow-Flow or data type match)
            if (srcPin->Type != dstPin->Type) return 0;

            // For data inputs, only one link per input pin
            if (dstPin->Type != PinType::Flow) {
                m_Links.erase(
                    std::remove_if(m_Links.begin(), m_Links.end(),
                        [dstPinID](const VisualScriptLink& l) {
                            return l.TargetPinID == dstPinID;
                        }),
                    m_Links.end());
            }

            VisualScriptLink link;
            link.ID           = m_NextLinkID++;
            link.SourceNodeID = srcNodeID;
            link.SourcePinID  = srcPinID;
            link.TargetNodeID = dstNodeID;
            link.TargetPinID  = dstPinID;
            m_Links.push_back(link);
            return link.ID;
        }

        /**
         * @brief Remove a link by its ID.
         */
        void removeLink(uint32_t linkID) {
            m_Links.erase(
                std::remove_if(m_Links.begin(), m_Links.end(),
                    [linkID](const VisualScriptLink& l) { return l.ID == linkID; }),
                m_Links.end());
        }

        const std::vector<VisualScriptLink>& getLinks() const { return m_Links; }

        // ── Queries ─────────────────────────────────────────────────

        /**
         * @brief Find the link that feeds a given input pin.
         * @return Pointer into the link vector, or nullptr.
         */
        const VisualScriptLink* findLinkToInputPin(uint32_t pinID) const {
            for (auto& l : m_Links)
                if (l.TargetPinID == pinID) return &l;
            return nullptr;
        }

        /**
         * @brief Find all links leaving a given output pin (mainly for Flow pins).
         */
        std::vector<const VisualScriptLink*> findLinksFromOutputPin(uint32_t pinID) const {
            std::vector<const VisualScriptLink*> result;
            for (auto& l : m_Links)
                if (l.SourcePinID == pinID) result.push_back(&l);
            return result;
        }

        /**
         * @brief Find the node that owns a given pin ID.
         */
        Ref<VisualScriptNode> findNodeByPinID(uint32_t pinID) const {
            for (auto& n : m_Nodes) {
                if (n->getPin(pinID)) return n;
            }
            return nullptr;
        }

        // ── Execution order (topological sort of data-flow) ─────────

        /**
         * @brief Compute a topological order over the data-flow edges.
         *
         * This is used to evaluate data dependencies before executing a
         * flow node.  Nodes with no data inputs come first.
         */
        std::vector<Ref<VisualScriptNode>> getExecutionOrder() const {
            // Build in-degree map (data links only)
            std::unordered_map<uint32_t, int> inDegree;
            for (auto& n : m_Nodes) inDegree[n->ID] = 0;

            for (auto& l : m_Links) {
                auto srcNode = findNode(l.SourceNodeID);
                if (!srcNode) continue;
                auto* srcPin = srcNode->getPin(l.SourcePinID);
                if (srcPin && srcPin->Type != PinType::Flow) {
                    inDegree[l.TargetNodeID]++;
                }
            }

            // Kahn's algorithm
            std::queue<uint32_t> ready;
            for (auto& [id, deg] : inDegree)
                if (deg == 0) ready.push(id);

            std::vector<Ref<VisualScriptNode>> order;
            order.reserve(m_Nodes.size());
            while (!ready.empty()) {
                uint32_t curr = ready.front();
                ready.pop();
                auto node = findNode(curr);
                if (node) order.push_back(node);

                for (auto& l : m_Links) {
                    if (l.SourceNodeID != curr) continue;
                    auto srcNode = findNode(l.SourceNodeID);
                    if (!srcNode) continue;
                    auto* srcPin = srcNode->getPin(l.SourcePinID);
                    if (srcPin && srcPin->Type != PinType::Flow) {
                        if (--inDegree[l.TargetNodeID] == 0)
                            ready.push(l.TargetNodeID);
                    }
                }
            }
            return order;
        }

        // ── ID generators (public so serializer can restore them) ───

        void setNextNodeID(uint32_t id) { m_NextNodeID = id; }
        void setNextLinkID(uint32_t id) { m_NextLinkID = id; }
        uint32_t getNextNodeID() const  { return m_NextNodeID; }
        uint32_t getNextLinkID() const  { return m_NextLinkID; }

    private:
        std::string m_Name = "Untitled";

        std::vector<Ref<VisualScriptNode>> m_Nodes;
        std::unordered_map<uint32_t, Ref<VisualScriptNode>> m_NodeMap;
        std::vector<VisualScriptLink> m_Links;

        uint32_t m_NextNodeID = 1;
        uint32_t m_NextLinkID = 1;
    };

} // namespace GameEngine
