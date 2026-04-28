#include "VulkanPipelineCache.hpp"
#include "VulkanDevice.hpp"
#include "../../Core/Logger.hpp"

#include <filesystem>
#include <fstream>
#include <vector>

namespace GameEngine {

    VkPipelineCache VulkanPipelineCache::s_Cache = VK_NULL_HANDLE;
    std::string     VulkanPipelineCache::s_CachePath;

    static std::string DefaultCachePath() {
        // Honor common cache-dir conventions, with a working-directory fallback.
        if (const char* xdg = std::getenv("XDG_CACHE_HOME"))    return std::string(xdg) + "/lag_pipeline_cache.bin";
        if (const char* home = std::getenv("HOME"))             return std::string(home) + "/.cache/lag_pipeline_cache.bin";
        if (const char* tmp = std::getenv("LOCALAPPDATA"))      return std::string(tmp) + "/lag_pipeline_cache.bin";
        return "lag_pipeline_cache.bin";
    }

    void VulkanPipelineCache::Init() {
        if (s_Cache != VK_NULL_HANDLE) return;

        if (s_CachePath.empty()) s_CachePath = DefaultCachePath();

        std::vector<uint8_t> initialData;
        std::error_code ec;
        if (std::filesystem::exists(s_CachePath, ec)) {
            std::ifstream f(s_CachePath, std::ios::binary | std::ios::ate);
            if (f.is_open()) {
                std::streamsize sz = f.tellg();
                if (sz > 0) {
                    f.seekg(0, std::ios::beg);
                    initialData.resize(static_cast<size_t>(sz));
                    f.read(reinterpret_cast<char*>(initialData.data()), sz);
                }
            }
        }

        VkPipelineCacheCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        ci.initialDataSize = initialData.size();
        ci.pInitialData    = initialData.empty() ? nullptr : initialData.data();

        VkDevice dev = VulkanDevice::Get().GetDevice();
        if (vkCreatePipelineCache(dev, &ci, nullptr, &s_Cache) == VK_SUCCESS) {
            GE_CORE_INFO("Vulkan pipeline cache: loaded {} bytes from {}",
                         initialData.size(), s_CachePath);
        } else {
            GE_CORE_WARN("Vulkan pipeline cache: failed to create — pipelines will be uncached");
            s_Cache = VK_NULL_HANDLE;
        }
    }

    void VulkanPipelineCache::Save() {
        if (s_Cache == VK_NULL_HANDLE) return;

        VkDevice dev = VulkanDevice::Get().GetDevice();
        size_t size = 0;
        if (vkGetPipelineCacheData(dev, s_Cache, &size, nullptr) != VK_SUCCESS || size == 0) return;

        std::vector<uint8_t> data(size);
        if (vkGetPipelineCacheData(dev, s_Cache, &size, data.data()) != VK_SUCCESS) return;

        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(s_CachePath).parent_path(), ec);

        std::ofstream f(s_CachePath, std::ios::binary | std::ios::trunc);
        if (f.is_open()) {
            f.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(size));
            GE_CORE_INFO("Vulkan pipeline cache: saved {} bytes to {}", size, s_CachePath);
        }
    }

    void VulkanPipelineCache::Shutdown() {
        if (s_Cache == VK_NULL_HANDLE) return;
        Save();
        vkDestroyPipelineCache(VulkanDevice::Get().GetDevice(), s_Cache, nullptr);
        s_Cache = VK_NULL_HANDLE;
    }

}
