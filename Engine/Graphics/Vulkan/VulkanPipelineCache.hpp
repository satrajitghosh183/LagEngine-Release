#pragma once

#include <vulkan/vulkan.h>
#include <string>

namespace GameEngine {

    /**
     * VulkanPipelineCache — process-wide VkPipelineCache.
     *
     * Vulkan pipeline creation is one of the most expensive driver
     * operations (often 10–100ms per pipeline). A pipeline cache lets the
     * driver memoize compiled state across pipelines and persist it to
     * disk between runs, cutting startup time by 5–10x in production.
     *
     * Usage:
     *   VulkanPipelineCache::Init();   // once at engine startup
     *   ...                            // create pipelines passing GetCache()
     *   VulkanPipelineCache::Save();   // before shutdown
     *   VulkanPipelineCache::Shutdown();
     *
     * The cache file is written to <user-cache-dir>/lag_pipeline_cache.bin
     * by default; override with SetCachePath() before Init().
     */
    class VulkanPipelineCache {
    public:
        static void Init();
        static void Shutdown();
        static void Save();

        static VkPipelineCache GetCache() { return s_Cache; }

        static void SetCachePath(const std::string& path) { s_CachePath = path; }
        static const std::string& GetCachePath() { return s_CachePath; }

    private:
        static VkPipelineCache s_Cache;
        static std::string s_CachePath;
    };

}
