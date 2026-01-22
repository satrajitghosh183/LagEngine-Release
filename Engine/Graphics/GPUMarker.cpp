#include "GPUMarker.hpp"
#include "../Core/Logger.hpp"
#include <glad/glad.h>
#include <cstring>

namespace GameEngine {

    bool GPUMarker::s_Initialized = false;
    bool GPUMarker::s_Supported = false;

    void GPUMarker::Begin(const std::string& name) {
        if (!s_Initialized) {
            s_Supported = IsSupported();
            s_Initialized = true;
        }

        if (!s_Supported) {
            return;
        }

#ifdef GL_KHR_debug
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name.c_str());
#else
        // Fallback: use glPushGroupMarkerEXT if available
        // For now, just log
        GE_CORE_TRACE("GPU Marker Begin: {0}", name);
#endif
    }

    void GPUMarker::End() {
        if (!s_Supported) {
            return;
        }

#ifdef GL_KHR_debug
        glPopDebugGroup();
#else
        GE_CORE_TRACE("GPU Marker End");
#endif
    }

    bool GPUMarker::IsSupported() {
        // Check for GL_KHR_debug extension
        const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        if (extensions) {
            return strstr(extensions, "GL_KHR_debug") != nullptr;
        }
        return false;
    }

}

