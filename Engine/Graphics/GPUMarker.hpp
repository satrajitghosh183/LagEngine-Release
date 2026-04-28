#pragma once

#include "../Core/Base.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>

namespace GameEngine {

    /**
     * @brief GPU debug markers via VK_EXT_debug_utils
     *
     * Wraps vkCmdBeginDebugUtilsLabelEXT / vkCmdEndDebugUtilsLabelEXT.
     * When the extension is absent (e.g., in release builds without validation
     * layers) all calls are no-ops.
     *
     * The function pointers are loaded once on first use from the Vulkan
     * instance dispatch table so no linking to an extension loader is required.
     *
     * Usage:
     *   GPUMarker::Begin(cmd, "Shadow Pass", {1, 0.5f, 0, 1});
     *   // ... draw calls ...
     *   GPUMarker::End(cmd);
     *
     * Or use the RAII helper:
     *   {
     *       GPUMarker::Scope scope(cmd, "Lighting Pass");
     *       // ...
     *   } // End() called automatically
     */
    class GPUMarker {
    public:
        /**
         * @brief Insert a labeled region begin marker into the command buffer.
         * @param cmd   Command buffer to record into.
         * @param name  Label string (shown in RenderDoc / Nsight).
         * @param color RGBA label color in [0, 1]. Defaults to white.
         */
        static void Begin(VkCommandBuffer cmd,
                          const std::string& name,
                          glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f});

        /**
         * @brief End the most recently opened marker region.
         */
        static void End(VkCommandBuffer cmd);

        /**
         * @brief Insert a single-point label (no Begin/End pair).
         */
        static void Insert(VkCommandBuffer cmd,
                           const std::string& name,
                           glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f});

        /** @brief Returns true if VK_EXT_debug_utils is available. */
        static bool IsSupported() { return s_Supported; }

        /**
         * @brief Load extension function pointers from the instance.
         *        Called automatically on first use; may also be called
         *        explicitly after VkInstance creation.
         */
        static void Init(VkInstance instance);

        /**
         * @brief RAII scope wrapper — calls Begin in ctor, End in dtor.
         */
        struct Scope {
            Scope(VkCommandBuffer cmd, const std::string& name,
                  glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f})
                : m_Cmd(cmd) {
                GPUMarker::Begin(cmd, name, color);
            }
            ~Scope() { GPUMarker::End(m_Cmd); }
            Scope(const Scope&)            = delete;
            Scope& operator=(const Scope&) = delete;
        private:
            VkCommandBuffer m_Cmd;
        };

    private:
        static bool s_Initialized;
        static bool s_Supported;

        // Loaded function pointers
        static PFN_vkCmdBeginDebugUtilsLabelEXT  s_BeginLabel;
        static PFN_vkCmdEndDebugUtilsLabelEXT    s_EndLabel;
        static PFN_vkCmdInsertDebugUtilsLabelEXT s_InsertLabel;
    };

} // namespace GameEngine
