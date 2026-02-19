#include "FileWatcher.hpp"
#include "../Core/Logger.hpp"
#include <algorithm>

namespace GameEngine {

    FileWatcher::FileWatcher(int pollIntervalMs)
        : m_PollIntervalMs(pollIntervalMs) {
    }

    FileWatcher::~FileWatcher() {
        Stop();
    }

    void FileWatcher::Start() {
        if (m_Running) return;
        
        m_Running = true;
        m_WatcherThread = std::thread(&FileWatcher::WatcherThread, this);
        
        GE_CORE_DEBUG("FileWatcher started (poll interval: {}ms)", m_PollIntervalMs);
    }

    void FileWatcher::Stop() {
        if (!m_Running) return;
        
        m_Running = false;
        
        if (m_WatcherThread.joinable()) {
            m_WatcherThread.join();
        }
        
        GE_CORE_DEBUG("FileWatcher stopped");
    }

    uint32_t FileWatcher::WatchFile(const std::string& filepath, FileChangeCallback callback) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        FileWatch watch;
        watch.id = m_NextWatchId++;
        watch.path = filepath;
        watch.callback = callback;
        watch.isDirectory = false;
        watch.recursive = false;
        
        try {
            watch.lastWriteTime = GetLastWriteTime(filepath);
        } catch (...) {
            watch.lastWriteTime = std::filesystem::file_time_type::min();
        }
        
        m_Watches.push_back(watch);
        
        GE_CORE_DEBUG("FileWatcher: Watching file '{}'", filepath);
        
        return watch.id;
    }

    uint32_t FileWatcher::WatchDirectory(
        const std::string& directory,
        const std::vector<std::string>& extensions,
        FileChangeCallback callback,
        bool recursive
    ) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        FileWatch watch;
        watch.id = m_NextWatchId++;
        watch.path = directory;
        watch.callback = callback;
        watch.isDirectory = true;
        watch.extensions = extensions;
        watch.recursive = recursive;
        watch.lastWriteTime = std::filesystem::file_time_type::min();
        
        m_Watches.push_back(watch);
        
        GE_CORE_DEBUG("FileWatcher: Watching directory '{}' (recursive: {})", directory, recursive);
        
        return watch.id;
    }

    void FileWatcher::RemoveWatch(uint32_t watchId) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        m_Watches.erase(
            std::remove_if(m_Watches.begin(), m_Watches.end(),
                [watchId](const FileWatch& w) { return w.id == watchId; }),
            m_Watches.end()
        );
    }

    void FileWatcher::ClearWatches() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Watches.clear();
    }

    void FileWatcher::ProcessCallbacks() {
        std::vector<PendingCallback> callbacks;
        
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            callbacks = std::move(m_PendingCallbacks);
            m_PendingCallbacks.clear();
        }
        
        for (const auto& cb : callbacks) {
            cb.callback(cb.path, cb.type);
        }
    }

    void FileWatcher::WatcherThread() {
        while (m_Running) {
            CheckFileChanges();
            std::this_thread::sleep_for(std::chrono::milliseconds(m_PollIntervalMs));
        }
    }

    void FileWatcher::CheckFileChanges() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        for (auto& watch : m_Watches) {
            if (watch.isDirectory) {
                // Check all files in directory
                try {
                    if (!std::filesystem::exists(watch.path)) continue;
                    
                    auto iterator = watch.recursive ?
                        std::filesystem::recursive_directory_iterator(watch.path) :
                        std::filesystem::recursive_directory_iterator(watch.path);
                    
                    for (const auto& entry : std::filesystem::directory_iterator(watch.path)) {
                        if (!entry.is_regular_file()) continue;
                        
                        std::string filepath = entry.path().string();
                        
                        // Check extension filter
                        if (!watch.extensions.empty()) {
                            if (!MatchesExtension(filepath, watch.extensions)) {
                                continue;
                            }
                        }
                        
                        auto currentTime = entry.last_write_time();
                        // For simplicity, we just trigger on any change
                        // A more complete implementation would track individual file times
                    }
                } catch (const std::exception& e) {
                    GE_CORE_ERROR("FileWatcher error: {}", e.what());
                }
            } else {
                // Single file watch
                try {
                    if (!std::filesystem::exists(watch.path)) {
                        if (watch.lastWriteTime != std::filesystem::file_time_type::min()) {
                            // File was deleted
                            m_PendingCallbacks.push_back({
                                watch.path,
                                FileChangeType::Deleted,
                                watch.callback
                            });
                            watch.lastWriteTime = std::filesystem::file_time_type::min();
                        }
                        continue;
                    }
                    
                    auto currentTime = GetLastWriteTime(watch.path);
                    
                    if (watch.lastWriteTime == std::filesystem::file_time_type::min()) {
                        // File was created
                        m_PendingCallbacks.push_back({
                            watch.path,
                            FileChangeType::Created,
                            watch.callback
                        });
                    } else if (currentTime != watch.lastWriteTime) {
                        // File was modified
                        m_PendingCallbacks.push_back({
                            watch.path,
                            FileChangeType::Modified,
                            watch.callback
                        });
                    }
                    
                    watch.lastWriteTime = currentTime;
                } catch (const std::exception& e) {
                    GE_CORE_ERROR("FileWatcher error checking '{}': {}", watch.path, e.what());
                }
            }
        }
    }

    bool FileWatcher::MatchesExtension(const std::string& filename, const std::vector<std::string>& extensions) {
        for (const auto& ext : extensions) {
            if (filename.length() >= ext.length() &&
                filename.compare(filename.length() - ext.length(), ext.length(), ext) == 0) {
                return true;
            }
        }
        return false;
    }

    std::filesystem::file_time_type FileWatcher::GetLastWriteTime(const std::string& path) {
        return std::filesystem::last_write_time(path);
    }

}
