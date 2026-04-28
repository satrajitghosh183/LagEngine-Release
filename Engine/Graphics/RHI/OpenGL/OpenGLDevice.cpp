// =============================================================================
// OpenGLDevice.cpp — STUB (Vulkan-only build)
//
// The engine has migrated entirely to Vulkan.  All OpenGL implementation code
// has been removed.  This translation unit now only provides the RHIDevice
// factory override that returns a null (or stub) device when the OpenGL
// backend is (incorrectly) requested at runtime, so that existing call sites
// fail gracefully rather than referencing deleted symbols.
// =============================================================================

#include "OpenGLDevice.hpp"

namespace GameEngine { namespace RHI {

    Ref<RHIDevice> RHIDevice::Create(RHIBackend backend) {
        // The Vulkan backend is handled by VulkanDevice (Engine/Graphics/Vulkan/).
        // If this factory is called with OpenGL (legacy path), return the no-op stub
        // rather than crashing, so transition-period code keeps compiling.
        (void)backend;
        return CreateRef<OpenGLDevice>();
    }

}} // namespace GameEngine::RHI
