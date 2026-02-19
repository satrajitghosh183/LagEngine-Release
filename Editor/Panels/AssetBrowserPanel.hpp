#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Assets/AssetDatabase.hpp"
#include <imgui.h>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace GameEngine {

    /**
     * @brief Asset browser panel - file/asset management
     * 
     * Features:
     * - Directory tree navigation
     * - Grid/list view toggle
     * - Thumbnail previews (icons for now)
     * - Drag-and-drop onto scene
     * - Create/rename/delete assets
     * - Import detection
     * - Filtering and sorting
     * - Recent folders
     */
    class AssetBrowserPanel {
    public:
        enum class SelectionFilter {
            All,
            Models,
            Scripts,
            Textures,
            Audio,
            Scenes,
            Shaders,
            RecentFolderBegin = 10,
            RecentFolderEnd = 20,
            Count
        };
        
        enum class SortingMode {
            Recent,
            Name,
            Path,
            Size,
            Type,
            Count
        };
    public:
        AssetBrowserPanel();
        ~AssetBrowserPanel() = default;
        
        void OnImGuiRender();
        
        /**
         * @brief Set the root assets directory
         */
        void SetAssetsDirectory(const std::filesystem::path& path);
        
        /**
         * @brief Get current directory
         */
        const std::filesystem::path& GetCurrentDirectory() const { return m_CurrentDirectory; }
        
        /**
         * @brief Refresh the current directory view
         */
        void Refresh();

        /** @brief Callback when user double-clicks a scene file (path as string) */
        void SetOnOpenScene(std::function<void(const std::string&)> cb) { m_OnOpenScene = std::move(cb); }
        
    private:
        void RenderDirectoryTree();
        void RenderDirectoryContents();
        void RenderBreadcrumb();
        void RenderFilterAndSort();
        
        void ApplyFilter();
        void ApplySort();
        bool MatchesFilter(const std::filesystem::directory_entry& entry) const;
        void AddToRecentFolders(const std::filesystem::path& path);
        
        void DrawDirectoryNode(const std::filesystem::path& path);
        void DrawAssetEntry(const std::filesystem::directory_entry& entry, bool isGridView);
        
        AssetType GetAssetType(const std::filesystem::path& path) const;
        const char* GetAssetIcon(AssetType type) const;
        ImVec4 GetAssetColor(AssetType type) const;
        
        void HandleDragDrop(const std::filesystem::path& path);
        void HandleDoubleClick(const std::filesystem::path& path);
        void HandleContextMenu(const std::filesystem::path& path);
        
        void CreateNewFolder();
        void CreateNewScript();
        void CreateNewScene();
        void DeleteSelected();
        void RenameSelected();
        void OpenInExternalEditor(const std::string& path);
        void RenderModals();
        
    private:
        std::filesystem::path m_AssetsRoot;
        std::filesystem::path m_CurrentDirectory;
        std::filesystem::path m_SelectedPath;
        
        // View settings
        bool m_GridView = true;
        float m_ThumbnailSize = 64.0f;
        float m_Padding = 16.0f;
        
        // Search/filter
        char m_SearchBuffer[256] = "";
        bool m_CaseSensitive = false;
        SelectionFilter m_CurrentFilter = SelectionFilter::All;
        SortingMode m_CurrentSorting = SortingMode::Recent;
        
        // Filtered and sorted entries
        std::vector<std::filesystem::directory_entry> m_FilteredEntries;
        
        // Recent folders
        std::vector<std::filesystem::path> m_RecentFolders;
        static constexpr size_t MAX_RECENT_FOLDERS = 10;
        
        // Cached directory entries
        std::vector<std::filesystem::directory_entry> m_CachedEntries;
        bool m_NeedsRefresh = true;
        
        // File extension to asset type mapping
        std::unordered_map<std::string, AssetType> m_ExtensionMap;
        
        // Rename state
        bool m_IsRenaming = false;
        char m_RenameBuffer[256] = "";
        std::filesystem::path m_PendingRenamePath;

        // Delete confirmation
        std::filesystem::path m_PendingDeletePath;

        std::function<void(const std::string&)> m_OnOpenScene;
    };

}
