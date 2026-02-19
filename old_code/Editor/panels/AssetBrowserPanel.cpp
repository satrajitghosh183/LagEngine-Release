#include "AssetBrowserPanel.hpp"
#include "../core/EditorTheme.hpp"
#include <imgui.h>
#include <algorithm>

namespace editor {

AssetBrowserPanel::AssetBrowserPanel() 
    : EditorPanel("Asset Browser", true) {
    initDemoAssets();
}

void AssetBrowserPanel::initDemoAssets() {
    // Create demo asset structure
    m_demoAssets = {
        // Folders
        {"Assets/Meshes", "Meshes", AssetType::Folder, true, 0},
        {"Assets/Textures", "Textures", AssetType::Folder, true, 0},
        {"Assets/Materials", "Materials", AssetType::Folder, true, 0},
        {"Assets/Shaders", "Shaders", AssetType::Folder, true, 0},
        {"Assets/Scenes", "Scenes", AssetType::Folder, true, 0},
        {"Assets/Scripts", "Scripts", AssetType::Folder, true, 0},
        {"Assets/Audio", "Audio", AssetType::Folder, true, 0},
        
        // Meshes
        {"Assets/cube.obj", "cube.obj", AssetType::Mesh, false, 4096},
        {"Assets/sphere.obj", "sphere.obj", AssetType::Mesh, false, 8192},
        {"Assets/cloth_plane.obj", "cloth_plane.obj", AssetType::Mesh, false, 2048},
        {"Assets/character.fbx", "character.fbx", AssetType::Mesh, false, 524288},
        
        // Textures
        {"Assets/cloth_diffuse.png", "cloth_diffuse.png", AssetType::Texture, false, 1048576},
        {"Assets/cloth_normal.png", "cloth_normal.png", AssetType::Texture, false, 1048576},
        {"Assets/ground_albedo.jpg", "ground_albedo.jpg", AssetType::Texture, false, 524288},
        {"Assets/skybox_hdr.exr", "skybox_hdr.exr", AssetType::Texture, false, 4194304},
        
        // Shaders
        {"Assets/cloth.vert", "cloth.vert", AssetType::Shader, false, 1024},
        {"Assets/cloth.frag", "cloth.frag", AssetType::Shader, false, 2048},
        {"Assets/pbr.vert", "pbr.vert", AssetType::Shader, false, 1536},
        {"Assets/pbr.frag", "pbr.frag", AssetType::Shader, false, 4096},
        
        // Materials
        {"Assets/cloth_material.mat", "cloth_material.mat", AssetType::Material, false, 512},
        {"Assets/metal.mat", "metal.mat", AssetType::Material, false, 512},
        {"Assets/wood.mat", "wood.mat", AssetType::Material, false, 512},
        
        // Scenes
        {"Assets/main_scene.scene", "main_scene.scene", AssetType::Scene, false, 8192},
        {"Assets/physics_demo.scene", "physics_demo.scene", AssetType::Scene, false, 4096},
        
        // Scripts
        {"Assets/ClothController.cpp", "ClothController.cpp", AssetType::Script, false, 2048},
        {"Assets/PhysicsManager.hpp", "PhysicsManager.hpp", AssetType::Script, false, 1024},
    };
    
    m_currentEntries = m_demoAssets;
}

void AssetBrowserPanel::render() {
    if (!beginPanel(ImGuiWindowFlags_MenuBar)) {
        endPanel();
        return;
    }
    
    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Folder")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Material")) {}
            if (ImGui::MenuItem("Shader")) {}
            if (ImGui::MenuItem("Scene")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Grid View", nullptr, m_gridView)) {
                m_gridView = true;
            }
            if (ImGui::MenuItem("List View", nullptr, !m_gridView)) {
                m_gridView = false;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Show Preview", nullptr, &m_showPreview)) {}
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    
    renderToolbar();
    
    // Main content area with optional preview panel
    float previewWidth = m_showPreview ? 200.0f : 0.0f;
    
    // Directory tree (left side)
    ImGui::BeginChild("DirectoryTree", ImVec2(180, 0), true);
    renderDirectoryTree();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Asset view (center)
    ImGui::BeginChild("AssetView", ImVec2(-previewWidth - 10, 0), true);
    
    if (m_gridView) {
        renderAssetGrid();
    } else {
        renderAssetList();
    }
    
    ImGui::EndChild();
    
    // Preview panel (right side)
    if (m_showPreview) {
        ImGui::SameLine();
        ImGui::BeginChild("Preview", ImVec2(0, 0), true);
        renderAssetPreview();
        ImGui::EndChild();
    }
    
    // Context menu
    if (ImGui::BeginPopupContextWindow("AssetContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        renderContextMenu();
        ImGui::EndPopup();
    }
    
    endPanel();
}

void AssetBrowserPanel::renderToolbar() {
    // Navigation buttons
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    
    if (ImGui::Button("\xef\x81\x88##Back")) { navigateBack(); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Back");
    
    ImGui::SameLine();
    
    if (ImGui::Button("\xef\x81\x89##Forward")) { navigateForward(); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Forward");
    
    ImGui::SameLine();
    
    if (ImGui::Button("\xef\x81\xa2##Up")) { navigateUp(); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Parent Directory");
    
    ImGui::SameLine();
    
    if (ImGui::Button("\xef\x80\xa1##Refresh")) { refreshDirectory(); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Refresh");
    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(10, 0));
    ImGui::SameLine();
    
    // Path breadcrumb
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
    ImGui::Button("Assets");
    ImGui::SameLine(0, 2);
    ImGui::Text("/");
    ImGui::SameLine(0, 2);
    ImGui::Button("Current");
    ImGui::PopStyleColor();
    
    ImGui::SameLine(ImGui::GetWindowWidth() - 350);
    
    // Search bar
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::SetNextItemWidth(180);
    ImGui::InputTextWithHint("##AssetSearch", "\xef\x80\x82 Search assets...", m_searchBuffer, sizeof(m_searchBuffer));
    ImGui::PopStyleVar();
    
    ImGui::SameLine();
    
    // View toggle
    ImGui::PushStyleColor(ImGuiCol_Button, m_gridView ? ImVec4(0.3f, 0.5f, 0.7f, 1.0f) : ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
    if (ImGui::Button("\xef\x80\x89##GridView")) { m_gridView = true; }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Grid View");
    
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, !m_gridView ? ImVec4(0.3f, 0.5f, 0.7f, 1.0f) : ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
    if (ImGui::Button("\xef\x80\xb9##ListView")) { m_gridView = false; }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("List View");
    
    ImGui::SameLine();
    
    // Icon size slider
    ImGui::SetNextItemWidth(80);
    if (ImGui::SliderInt("##IconSize", &m_iconSizeIndex, 0, 2, "")) {
        float sizes[] = {48.0f, 80.0f, 120.0f};
        m_iconSize = sizes[m_iconSizeIndex];
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Icon Size");
    
    ImGui::PopStyleVar();
    
    ImGui::Separator();
}

void AssetBrowserPanel::renderDirectoryTree() {
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.6f, 0.5f));
    
    // Root folder
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | 
                               ImGuiTreeNodeFlags_DefaultOpen |
                               ImGuiTreeNodeFlags_SpanAvailWidth;
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.3f, 1.0f));
    if (ImGui::TreeNodeEx("\xef\x81\xbb Assets", flags)) {
        ImGui::PopStyleColor();
        
        // Subfolders
        ImGuiTreeNodeFlags leafFlags = flags | ImGuiTreeNodeFlags_Leaf;
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.75f, 0.9f, 1.0f));
        
        if (ImGui::TreeNodeEx("\xef\x81\xbb Meshes", leafFlags)) { ImGui::TreePop(); }
        if (ImGui::TreeNodeEx("\xef\x81\xbb Textures", leafFlags)) { ImGui::TreePop(); }
        if (ImGui::TreeNodeEx("\xef\x81\xbb Materials", leafFlags)) { ImGui::TreePop(); }
        if (ImGui::TreeNodeEx("\xef\x81\xbb Shaders", leafFlags)) { ImGui::TreePop(); }
        if (ImGui::TreeNodeEx("\xef\x81\xbb Scenes", leafFlags)) { ImGui::TreePop(); }
        if (ImGui::TreeNodeEx("\xef\x81\xbb Scripts", leafFlags)) { ImGui::TreePop(); }
        if (ImGui::TreeNodeEx("\xef\x81\xbb Audio", leafFlags)) { ImGui::TreePop(); }
        
        ImGui::PopStyleColor();
        
        ImGui::TreePop();
    } else {
        ImGui::PopStyleColor();
    }
    
    ImGui::PopStyleColor();
}

void AssetBrowserPanel::renderAssetGrid() {
    float padding = 8.0f;
    float cellSize = m_iconSize + padding * 2;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columns = std::max(1, static_cast<int>(panelWidth / cellSize));
    
    std::string searchStr = m_searchBuffer;
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);
    
    ImGui::Columns(columns, nullptr, false);
    
    for (const auto& entry : m_useDemoAssets ? m_demoAssets : m_currentEntries) {
        // Filter by search
        if (!searchStr.empty()) {
            std::string nameLower = entry.name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            if (nameLower.find(searchStr) == std::string::npos) continue;
        }
        
        renderAssetItem(entry, true);
        ImGui::NextColumn();
    }
    
    ImGui::Columns(1);
}

void AssetBrowserPanel::renderAssetList() {
    std::string searchStr = m_searchBuffer;
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);
    
    // Table header
    if (ImGui::BeginTable("AssetTable", 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        
        for (const auto& entry : m_useDemoAssets ? m_demoAssets : m_currentEntries) {
            // Filter by search
            if (!searchStr.empty()) {
                std::string nameLower = entry.name;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                if (nameLower.find(searchStr) == std::string::npos) continue;
            }
            
            ImGui::TableNextRow();
            
            ImGui::TableSetColumnIndex(0);
            
            ImVec4 color = getAssetColor(entry.type);
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::Text("%s", getAssetIcon(entry.type));
            ImGui::PopStyleColor();
            ImGui::SameLine();
            
            bool selected = (m_selectedAsset == entry.path);
            if (ImGui::Selectable(entry.name.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
                m_selectedAsset = entry.path;
                if (onAssetSelected) onAssetSelected(entry.path);
            }
            
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                if (entry.isDirectory) {
                    navigateTo(entry.path);
                } else if (onAssetDoubleClicked) {
                    onAssetDoubleClicked(entry.path);
                }
            }
            
            ImGui::TableSetColumnIndex(1);
            ImGui::TextDisabled("%s", entry.isDirectory ? "Folder" : 
                               (entry.type == AssetType::Mesh ? "Mesh" :
                                entry.type == AssetType::Texture ? "Texture" :
                                entry.type == AssetType::Shader ? "Shader" :
                                entry.type == AssetType::Material ? "Material" :
                                entry.type == AssetType::Scene ? "Scene" :
                                entry.type == AssetType::Script ? "Script" : "Unknown"));
            
            ImGui::TableSetColumnIndex(2);
            if (!entry.isDirectory) {
                ImGui::TextDisabled("%s", formatFileSize(entry.size).c_str());
            }
            
            ImGui::TableSetColumnIndex(3);
            ImGui::TextDisabled("%s", entry.path.string().c_str());
        }
        
        ImGui::EndTable();
    }
}

void AssetBrowserPanel::renderAssetItem(const AssetEntry& entry, bool /*gridView*/) {
    ImGui::PushID(entry.name.c_str());
    
    ImVec4 iconColor = getAssetColor(entry.type);
    const char* icon = getAssetIcon(entry.type);
    
    bool selected = (m_selectedAsset == entry.path);
    
    // Background
    ImGui::PushStyleColor(ImGuiCol_Button, selected ? ImVec4(0.3f, 0.5f, 0.7f, 0.5f) : ImVec4(0.12f, 0.12f, 0.15f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.25f, 0.8f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    
    if (ImGui::Button("##AssetBtn", ImVec2(m_iconSize, m_iconSize + 20))) {
        m_selectedAsset = entry.path;
        if (onAssetSelected) onAssetSelected(entry.path);
    }
    
    // Double-click handling
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        if (entry.isDirectory) {
            navigateTo(entry.path);
        } else if (onAssetDoubleClicked) {
            onAssetDoubleClicked(entry.path);
        }
    }
    
    // Drag source
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        std::string pathStr = entry.path.string();
        ImGui::SetDragDropPayload("ASSET_PATH", pathStr.c_str(), pathStr.size() + 1);
        
        ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
        ImGui::Text("%s %s", icon, entry.name.c_str());
        ImGui::PopStyleColor();
        
        ImGui::EndDragDropSource();
    }
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
    
    // Draw icon on top of button
    ImVec2 pos = ImGui::GetItemRectMin();
    ImVec2 size = ImGui::GetItemRectSize();
    
    ImGui::GetWindowDrawList()->AddText(
        ImGui::GetFont(), m_iconSize * 0.5f,
        ImVec2(pos.x + (size.x - m_iconSize * 0.3f) * 0.5f, pos.y + 10),
        ImGui::ColorConvertFloat4ToU32(iconColor),
        icon
    );
    
    // Draw name below
    ImGui::SetCursorPosX(ImGui::GetCursorPosX());
    
    std::string displayName = entry.name;
    if (displayName.length() > 12) {
        displayName = displayName.substr(0, 10) + "...";
    }
    
    float textWidth = ImGui::CalcTextSize(displayName.c_str()).x;
    float offsetX = (m_iconSize - textWidth) * 0.5f;
    if (offsetX < 0) offsetX = 0;
    
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
    ImGui::TextWrapped("%s", displayName.c_str());
    
    ImGui::PopID();
}

void AssetBrowserPanel::renderAssetPreview() {
    ImGui::Text("Preview");
    ImGui::Separator();
    
    if (m_selectedAsset.empty()) {
        ImGui::TextDisabled("Select an asset to preview");
        return;
    }
    
    // Find selected asset
    const AssetEntry* selected = nullptr;
    for (const auto& entry : m_useDemoAssets ? m_demoAssets : m_currentEntries) {
        if (entry.path == m_selectedAsset) {
            selected = &entry;
            break;
        }
    }
    
    if (!selected) return;
    
    // Asset icon
    ImVec4 color = getAssetColor(selected->type);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::SetWindowFontScale(2.0f);
    ImGui::Text("%s", getAssetIcon(selected->type));
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    
    // Asset info
    ImGui::Text("Name:");
    ImGui::TextWrapped("%s", selected->name.c_str());
    
    ImGui::Spacing();
    
    ImGui::Text("Type:");
    ImGui::TextDisabled("%s", selected->isDirectory ? "Folder" : "Asset");
    
    if (!selected->isDirectory) {
        ImGui::Spacing();
        ImGui::Text("Size:");
        ImGui::TextDisabled("%s", formatFileSize(selected->size).c_str());
    }
    
    ImGui::Spacing();
    
    ImGui::Text("Path:");
    ImGui::TextWrapped("%s", selected->path.string().c_str());
}

void AssetBrowserPanel::renderContextMenu() {
    if (ImGui::MenuItem("Create Folder")) {}
    ImGui::Separator();
    if (ImGui::MenuItem("Import Asset...")) {}
    ImGui::Separator();
    if (ImGui::BeginMenu("Create")) {
        if (ImGui::MenuItem("Material")) {}
        if (ImGui::MenuItem("Shader")) {}
        if (ImGui::MenuItem("Scene")) {}
        ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Refresh")) { refreshDirectory(); }
    if (ImGui::MenuItem("Show in Explorer")) {}
}

const char* AssetBrowserPanel::getAssetIcon(AssetType type) const {
    switch (type) {
        case AssetType::Folder:   return "\xef\x81\xbb";
        case AssetType::Mesh:     return "\xef\x86\x92";
        case AssetType::Texture:  return "\xef\x80\xbe";
        case AssetType::Shader:   return "\xef\x84\xa1";
        case AssetType::Material: return "\xef\x88\x93";
        case AssetType::Scene:    return "\xef\x86\x88";
        case AssetType::Script:   return "\xef\x84\xa1";
        case AssetType::Audio:    return "\xef\x80\xa8";
        case AssetType::Font:     return "\xef\x80\xb5";
        case AssetType::Prefab:   return "\xef\x87\xb1";
        default:                  return "\xef\x85\x9b";
    }
}

ImVec4 AssetBrowserPanel::getAssetColor(AssetType type) const {
    switch (type) {
        case AssetType::Folder:   return ImVec4(0.9f, 0.7f, 0.3f, 1.0f);
        case AssetType::Mesh:     return ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
        case AssetType::Texture:  return ImVec4(0.8f, 0.5f, 0.7f, 1.0f);
        case AssetType::Shader:   return ImVec4(0.5f, 0.7f, 0.9f, 1.0f);
        case AssetType::Material: return ImVec4(0.9f, 0.6f, 0.4f, 1.0f);
        case AssetType::Scene:    return ImVec4(0.6f, 0.8f, 0.95f, 1.0f);
        case AssetType::Script:   return ImVec4(0.8f, 0.8f, 0.4f, 1.0f);
        case AssetType::Audio:    return ImVec4(0.6f, 0.4f, 0.8f, 1.0f);
        case AssetType::Font:     return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        case AssetType::Prefab:   return ImVec4(0.4f, 0.6f, 0.9f, 1.0f);
        default:                  return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    }
}

std::string AssetBrowserPanel::formatFileSize(size_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024 && unit < 3) {
        size /= 1024;
        unit++;
    }
    
    char buffer[32];
    if (unit == 0) {
        snprintf(buffer, sizeof(buffer), "%d %s", static_cast<int>(size), units[unit]);
    } else {
        snprintf(buffer, sizeof(buffer), "%.1f %s", size, units[unit]);
    }
    
    return buffer;
}

void AssetBrowserPanel::navigateTo(const std::filesystem::path& path) {
    m_currentDirectory = path;
    refreshDirectory();
}

void AssetBrowserPanel::navigateBack() {
    if (m_historyIndex > 0) {
        m_historyIndex--;
        m_currentDirectory = m_navigationHistory[m_historyIndex];
        refreshDirectory();
    }
}

void AssetBrowserPanel::navigateForward() {
    if (m_historyIndex < m_navigationHistory.size() - 1) {
        m_historyIndex++;
        m_currentDirectory = m_navigationHistory[m_historyIndex];
        refreshDirectory();
    }
}

void AssetBrowserPanel::navigateUp() {
    if (m_currentDirectory != m_rootDirectory && m_currentDirectory.has_parent_path()) {
        navigateTo(m_currentDirectory.parent_path());
    }
}

void AssetBrowserPanel::refreshDirectory() {
    // Would refresh m_currentEntries from filesystem
}

void AssetBrowserPanel::setRootDirectory(const std::filesystem::path& path) {
    m_rootDirectory = path;
    m_currentDirectory = path;
    refreshDirectory();
}

void AssetBrowserPanel::update(float) {
    // Update logic
}

AssetBrowserPanel::AssetType AssetBrowserPanel::getAssetType(const std::filesystem::path& path) const {
    if (std::filesystem::is_directory(path)) return AssetType::Folder;
    
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb") return AssetType::Mesh;
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".exr" || ext == ".hdr") return AssetType::Texture;
    if (ext == ".vert" || ext == ".frag" || ext == ".glsl" || ext == ".hlsl") return AssetType::Shader;
    if (ext == ".mat") return AssetType::Material;
    if (ext == ".scene") return AssetType::Scene;
    if (ext == ".cpp" || ext == ".hpp" || ext == ".h" || ext == ".c") return AssetType::Script;
    if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") return AssetType::Audio;
    if (ext == ".ttf" || ext == ".otf") return AssetType::Font;
    if (ext == ".prefab") return AssetType::Prefab;
    
    return AssetType::Unknown;
}

} // namespace editor

