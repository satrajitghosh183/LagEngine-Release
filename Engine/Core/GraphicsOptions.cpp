#include "GraphicsOptions.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cstring>

namespace GameEngine {

    GraphicsOptions* GraphicsOptions::s_Instance = nullptr;

    GraphicsOptions GraphicsOptions::Parse(int argc, char* argv[]) {
        std::vector<std::string> args;
        for (int i = 0; i < argc; ++i) {
            args.push_back(argv[i]);
        }
        return Parse(args);
    }

    GraphicsOptions GraphicsOptions::Parse(const std::vector<std::string>& args) {
        GraphicsOptions options;
        
        for (size_t i = 0; i < args.size(); ++i) {
            std::string arg = args[i];
            std::transform(arg.begin(), arg.end(), arg.begin(), ::toupper);
            
            // Graphics backend
            if (arg == "OPENGL") {
                options.BackendType = Backend::OpenGL;
            } else if (arg == "VULKAN") {
                options.BackendType = Backend::Vulkan;
            }
            // Debug flags
            else if (arg == "DEBUGDEVICE") {
                options.DebugDevice = true;
            } else if (arg == "GPUVALIDATION") {
                options.GPUValidation = true;
            } else if (arg == "GPU_VERBOSE" || arg == "GPUVERBOSE") {
                options.GPUVerbose = true;
            }
            // GPU adapter hints
            else if (arg == "IGPU") {
                options.AdapterHint = GPUHint::Integrated;
            } else if (arg == "AMDGPU") {
                options.AdapterHint = GPUHint::AMD;
            } else if (arg == "NVIDIAGPU") {
                options.AdapterHint = GPUHint::NVIDIA;
            } else if (arg == "INTELGPU") {
                options.AdapterHint = GPUHint::Intel;
            }
            // Runtime options
            else if (arg == "ALWAYSACTIVE") {
                options.AlwaysActive = true;
            }
        }
        
        return options;
    }

    GraphicsOptions& GraphicsOptions::Get() {
        if (!s_Instance) {
            s_Instance = new GraphicsOptions();
        }
        return *s_Instance;
    }

    void GraphicsOptions::Initialize(int argc, char* argv[]) {
        if (!s_Instance) {
            s_Instance = new GraphicsOptions(Parse(argc, argv));
            GE_CORE_INFO("GraphicsOptions initialized from command-line");
        }
    }

}
