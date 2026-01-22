#pragma once

#include "../Core/Base.hpp"
#include <string>

namespace GameEngine {

    /**
     * @brief GPU marker for profiling
     * 
     * Uses GL_KHR_debug or glPushDebugGroup for pass timing
     */
    class GPUMarker {
    public:
        /**
         * @brief Begin GPU marker
         */
        static void Begin(const std::string& name);

        /**
         * @brief End GPU marker
         */
        static void End();

        /**
         * @brief Check if GPU markers are supported
         */
        static bool IsSupported();

    private:
        static bool s_Initialized;
        static bool s_Supported;
    };

}

