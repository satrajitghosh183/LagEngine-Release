#pragma once

#include "../../Core/Base.hpp"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include <functional>

namespace GameEngine {

    // Forward declarations
    class Scene;
    class Entity;
    class VisualScriptVM;

    /// Pin value type: the variant that every data pin carries
    using PinValue = std::variant<bool, int, float, glm::vec2, glm::vec3, std::string, uint32_t>;

    /// Enumeration of pin data types
    enum class PinType : uint8_t {
        Flow,     ///< Control-flow pin (no data payload)
        Bool,
        Int,
        Float,
        Vec2,
        Vec3,
        String,
        Entity    ///< Stored as uint32_t (entity UUID low bits)
    };

    /// Whether a pin is on the input or output side of a node
    enum class PinDirection : uint8_t {
        Input,
        Output
    };

    /**
     * @brief A single pin on a visual-script node.
     *
     * Pins are the connection points on nodes.  Flow pins control which
     * node runs next; data pins carry values between nodes.
     */
    struct VisualScriptPin {
        uint32_t    ID        = 0;
        std::string Name;
        PinType     Type      = PinType::Flow;
        PinDirection Direction = PinDirection::Input;
        PinValue    DefaultValue;

        /// Convenience: return a sensible default for the pin type
        static PinValue defaultForType(PinType type) {
            switch (type) {
                case PinType::Flow:   return bool(false);
                case PinType::Bool:   return bool(false);
                case PinType::Int:    return int(0);
                case PinType::Float:  return float(0.0f);
                case PinType::Vec2:   return glm::vec2(0.0f);
                case PinType::Vec3:   return glm::vec3(0.0f);
                case PinType::String: return std::string();
                case PinType::Entity: return uint32_t(0);
            }
            return int(0);
        }
    };

    /**
     * @brief Abstract base class for every node in a visual-script graph.
     *
     * Subclasses define their pins in the constructor and implement
     * execute() which reads input pin values and writes output pin values.
     */
    class VisualScriptNode {
    public:
        VisualScriptNode() = default;
        virtual ~VisualScriptNode() = default;

        /// Unique numeric ID within the graph
        uint32_t    ID       = 0;
        /// Position in the editor canvas (pixels)
        glm::vec2   Position = glm::vec2(0.0f);
        /// Human-readable name shown in the editor
        std::string Name;
        /// Category for the node palette (e.g. "Math", "Entity")
        std::string Category;

        // ── Pin management ──────────────────────────────────────────

        uint32_t addInputPin(const std::string& name, PinType type,
                             const PinValue& defaultVal = {}) {
            uint32_t pinID = m_NextPinID++;
            VisualScriptPin pin;
            pin.ID           = pinID;
            pin.Name         = name;
            pin.Type         = type;
            pin.Direction    = PinDirection::Input;
            pin.DefaultValue = defaultVal.index() == 0 && type != PinType::Bool && type != PinType::Flow
                                   ? VisualScriptPin::defaultForType(type)
                                   : defaultVal;
            m_InputPins.push_back(pin);
            return pinID;
        }

        uint32_t addOutputPin(const std::string& name, PinType type,
                              const PinValue& defaultVal = {}) {
            uint32_t pinID = m_NextPinID++;
            VisualScriptPin pin;
            pin.ID           = pinID;
            pin.Name         = name;
            pin.Type         = type;
            pin.Direction    = PinDirection::Output;
            pin.DefaultValue = defaultVal.index() == 0 && type != PinType::Bool && type != PinType::Flow
                                   ? VisualScriptPin::defaultForType(type)
                                   : defaultVal;
            m_OutputPins.push_back(pin);
            return pinID;
        }

        const std::vector<VisualScriptPin>& getInputPins()  const { return m_InputPins; }
        const std::vector<VisualScriptPin>& getOutputPins() const { return m_OutputPins; }

        VisualScriptPin* getPin(uint32_t pinID) {
            for (auto& p : m_InputPins)  if (p.ID == pinID) return &p;
            for (auto& p : m_OutputPins) if (p.ID == pinID) return &p;
            return nullptr;
        }

        const VisualScriptPin* getPin(uint32_t pinID) const {
            for (auto& p : m_InputPins)  if (p.ID == pinID) return &p;
            for (auto& p : m_OutputPins) if (p.ID == pinID) return &p;
            return nullptr;
        }

        // ── Pin value access (used by the VM) ───────────────────────

        void setPinValue(uint32_t pinID, const PinValue& value) {
            m_PinValues[pinID] = value;
        }

        PinValue getPinValue(uint32_t pinID) const {
            auto it = m_PinValues.find(pinID);
            if (it != m_PinValues.end()) return it->second;
            // Fall back to the pin's default
            if (auto* pin = getPin(pinID)) return pin->DefaultValue;
            return PinValue{};
        }

        /// Helper: read a typed value from a pin, returning defaultVal on type mismatch
        template <typename T>
        T getPinValueAs(uint32_t pinID, const T& defaultVal = T{}) const {
            PinValue v = getPinValue(pinID);
            if (auto* ptr = std::get_if<T>(&v)) return *ptr;
            return defaultVal;
        }

        // ── Execution ──────────────────────────────────────────────

        /**
         * @brief Execute this node.
         * @param vm  The virtual machine (provides variable storage, scene access, etc.)
         *
         * Implementations should:
         *   1. Read from input pins via getPinValue / getPinValueAs
         *   2. Perform their computation
         *   3. Write results to output pins via setPinValue
         *   4. (Flow nodes) Set the next flow output via vm
         */
        virtual void execute(VisualScriptVM& vm) = 0;

        /**
         * @brief Return the name that identifies this node type in serialization.
         */
        virtual std::string getNodeTypeName() const = 0;

        /// Assign pin IDs starting from a given base (used after deserialization)
        void setNextPinID(uint32_t id) { m_NextPinID = id; }
        uint32_t getNextPinID() const { return m_NextPinID; }

    protected:
        std::vector<VisualScriptPin> m_InputPins;
        std::vector<VisualScriptPin> m_OutputPins;
        uint32_t m_NextPinID = 1;

        /// Runtime pin values keyed by pin ID
        std::unordered_map<uint32_t, PinValue> m_PinValues;
    };

} // namespace GameEngine
