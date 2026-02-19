#pragma once

#include "Base.hpp"
#include <string>
#include <vector>

namespace GameEngine {

    /**
     * @brief Graphics options parsed from command-line arguments
     */
    struct GraphicsOptions {
        // Graphics backend
        enum class Backend {
            OpenGL,
            Vulkan,
            Auto  // Default - use what's available
        };
        Backend BackendType = Backend::Auto;
        
        // Debug flags
        bool DebugDevice = false;
        bool GPUValidation = false;
        bool GPUVerbose = false;
        
        // GPU adapter hints
        enum class GPUHint {
            None,
            Integrated,  // igpu
            AMD,         // amdgpu
            NVIDIA,      // nvidiagpu
            Intel        // intelgpu
        };
        GPUHint AdapterHint = GPUHint::None;
        
        // Runtime options
        bool AlwaysActive = false;  // Don't pause when window unfocused
        
        /**
         * @brief Parse command-line arguments
         * @param argc Argument count
         * @param argv Argument values
         * @return GraphicsOptions with parsed values
         */
        static GraphicsOptions Parse(int argc, char* argv[]);
        
        /**
         * @brief Parse command-line arguments from vector
         * @param args Argument vector
         * @return GraphicsOptions with parsed values
         */
        static GraphicsOptions Parse(const std::vector<std::string>& args);
        
        /**
         * @brief Get singleton instance
         */
        static GraphicsOptions& Get();
        
        /**
         * @brief Initialize singleton from command-line arguments
         */
        static void Initialize(int argc, char* argv[]);
        
    private:
        static GraphicsOptions* s_Instance;
    };

}
