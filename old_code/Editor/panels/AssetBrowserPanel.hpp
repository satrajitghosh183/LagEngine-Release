#pragma once

#include "EditorPanel.hpp"
#include <vector>
#include <string>
#include <filesystem>
#include <functional>

namespace editor {

/**
 * @brief Asset Browser Panel - browse and manage project assets
 * Similar to Unity's Project window or LumixEngine's Asset Browser
 */
class AssetBrowserPanel : public EditorPanel {
public:
    AssetBrowserPanel();
    
    void render() override;
    void update(float dt) override;
    
    // Set project root directory
    void setRootDirectory(const std::filesystem::path& path);
    
    // Callbacks
    std::function<void(const std::filesystem::path&)> onAssetSelected;
    std::function<void(const std::filesystem::path&)> onAssetDoubleClicked;

private:
    enum class AssetType {
        Unknown,
        Folder,
        Mesh,
        Texture,
        Shader,
        Material,
        Scene,
        Script,
        Audio,
        Font,
        Prefab
    };
    
    struct AssetEntry {
        std::filesystem::path path;
        std::string name;
        AssetType type;
        bool isDirectory;
        size_t size = 0;
    };
    
    void renderToolbar();
    void renderDirectoryTree();
    void renderDirectoryNode(const std::filesystem::path& path);
    void renderAssetGrid();
    void renderAssetList();
    void renderAssetItem(const AssetEntry& entry, bool gridView);
    void renderContextMenu();
    void renderAssetPreview();
    
    void refreshDirectory();
    AssetType getAssetType(const std::filesystem::path& path) const;
    const char* getAssetIcon(AssetType type) const;
    ImVec4 getAssetColor(AssetType type) const;
    std::string formatFileSize(size_t bytes) const;
    
    // Navigation
    void navigateTo(const std::filesystem::path& path);
    void navigateBack();
    void navigateForward();
    void navigateUp();
    
    std::filesystem::path m_rootDirectory;
    std::filesystem::path m_currentDirectory;
    std::vector<std::filesystem::path> m_navigationHistory;
    size_t m_historyIndex = 0;
    
    std::vector<AssetEntry> m_currentEntries;
    
    // Selection
    std::filesystem::path m_selectedAsset;
    std::filesystem::path m_hoveredAsset;
    
    // View options
    bool m_gridView = true;
    float m_iconSize = 80.0f;
    int m_iconSizeIndex = 1; // 0=Small, 1=Medium, 2=Large
    
    // Search
    char m_searchBuffer[256] = "";
    
    // Preview
    bool m_showPreview = true;
    unsigned int m_previewTexture = 0;
    
    // Demo assets (for visualization without actual filesystem)
    void initDemoAssets();
    std::vector<AssetEntry> m_demoAssets;
    bool m_useDemoAssets = true;
};

} // namespace editor

