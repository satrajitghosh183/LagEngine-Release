#pragma once

#include "../Core/Base.hpp"
#include <string>
#include <functional>
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

namespace GameEngine {

    /**
     * @brief File change event type
     */
    enum class FileChangeType {
        Created,
        Modified,
        Deleted
    };

    /**
     * @brief File change callback
     */
    using FileChangeCallback = std::function<void(const std::string& path, FileChangeType type)>;

    /**
     * @brief Watches files for changes and notifies via callbacks
     * 
     * Features:
     * - Background thread monitoring
     * - Per-file callbacks
     * - Directory watching
     * - Hot-reload support for scripts/assets
     */
    class FileWatcher {
    public:
        /**
         * @brief Create file watcher
         * @param pollInterval How often to check for changes (milliseconds)
         */
        FileWatcher(int pollIntervalMs = 500);
        ~FileWatcher();
        
        /**
         * @brief Start watching
         */
        void Start();
        
        /**
         * @brief Stop watching
         */
        void Stop();
        
        /**
         * @brief Watch a specific file
         * @param filepath Path to file to watch
         * @param callback Function to call when file changes
         * @return Watch ID (for removal)
         */
        uint32_t WatchFile(const std::string& filepath, FileChangeCallback callback);
        
        /**
         * @brief Watch all files in a directory (non-recursive by default)
         * @param directory Path to directory
         * @param extensions Filter by file extensions (empty = all files)
         * @param callback Function to call when any file changes
         * @param recursive Watch subdirectories
         * @return Watch ID
         */
        uint32_t WatchDirectory(
            const std::string& directory,
            const std::vector<std::string>& extensions,
            FileChangeCallback callback,
            bool recursive = false
        );
        
        /**
         * @brief Remove watch
         */
        void RemoveWatch(uint32_t watchId);
        
        /**
         * @brief Clear all watches
         */
        void ClearWatches();
        
        /**
         * @brief Process any pending callbacks (call from main thread)
         */
        void ProcessCallbacks();
        
        /**
         * @brief Check if running
         */
        bool IsRunning() const { return m_Running; }
        
    private:
        struct FileWatch {
            uint32_t id;
            std::string path;
            FileChangeCallback callback;
            std::filesystem::file_time_type lastWriteTime;
            bool isDirectory;
            std::vector<std::string> extensions;
            bool recursive;
        };
        
        struct PendingCallback {
            std::string path;
            FileChangeType type;
            FileChangeCallback callback;
        };
        
        void WatcherThread();
        void CheckFileChanges();
        bool MatchesExtension(const std::string& filename, const std::vector<std::string>& extensions);
        std::filesystem::file_time_type GetLastWriteTime(const std::string& path);
        
    private:
        std::vector<FileWatch> m_Watches;
        std::vector<PendingCallback> m_PendingCallbacks;
        
        std::thread m_WatcherThread;
        std::atomic<bool> m_Running{false};
        std::mutex m_Mutex;
        
        int m_PollIntervalMs;
        uint32_t m_NextWatchId = 1;
    };

}
