#include "GPUMarker.hpp"
#include "../Core/Logger.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    // -------------------------------------------------------------------------
    // Static definitions
    // -------------------------------------------------------------------------

    bool GPUMarker::s_Initialized = false;
    bool GPUMarker::s_Supported   = false;

    PFN_vkCmdBeginDebugUtilsLabelEXT  GPUMarker::s_BeginLabel  = nullptr;
    PFN_vkCmdEndDebugUtilsLabelEXT    GPUMarker::s_EndLabel    = nullptr;
    PFN_vkCmdInsertDebugUtilsLabelEXT GPUMarker::s_InsertLabel = nullptr;

    // -------------------------------------------------------------------------
    // Init — load function pointers from the Vulkan instance
    // -------------------------------------------------------------------------

    void GPUMarker::Init(VkInstance instance) {
        if (s_Initialized) return;
        s_Initialized = true;

        s_BeginLabel  = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
            vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
        s_EndLabel    = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
            vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
        s_InsertLabel = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(
            vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));

        s_Supported = (s_BeginLabel != nullptr && s_EndLabel != nullptr);

        if (s_Supported) {
            GE_CORE_INFO("GPUMarker: VK_EXT_debug_utils available");
        } else {
            GE_CORE_WARN("GPUMarker: VK_EXT_debug_utils not available — markers will be no-ops");
        }
    }

    // -------------------------------------------------------------------------
    // Begin
    // -------------------------------------------------------------------------

    void GPUMarker::Begin(VkCommandBuffer cmd,
                          const std::string& name,
                          glm::vec4 color) {
        if (!s_Supported || !s_BeginLabel) return;

        VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
        label.pLabelName = name.c_str();
        label.color[0]   = color.r;
        label.color[1]   = color.g;
        label.color[2]   = color.b;
        label.color[3]   = color.a;

        s_BeginLabel(cmd, &label);
    }

    // -------------------------------------------------------------------------
    // End
    // -------------------------------------------------------------------------

    void GPUMarker::End(VkCommandBuffer cmd) {
        if (!s_Supported || !s_EndLabel) return;
        s_EndLabel(cmd);
    }

    // -------------------------------------------------------------------------
    // Insert (single point, no begin/end pair)
    // -------------------------------------------------------------------------

    void GPUMarker::Insert(VkCommandBuffer cmd,
                           const std::string& name,
                           glm::vec4 color) {
        if (!s_Supported || !s_InsertLabel) return;

        VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
        label.pLabelName = name.c_str();
        label.color[0]   = color.r;
        label.color[1]   = color.g;
        label.color[2]   = color.b;
        label.color[3]   = color.a;

        s_InsertLabel(cmd, &label);
    }

} // namespace GameEngine
