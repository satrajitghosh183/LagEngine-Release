#include "AssetBrowserPanel.hpp"
#include "../../Engine/Core/Logger.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cstring>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace GameEngine {

    AssetBrowserPanel::AssetBrowserPanel() {
        // Initialize extension mapping
        m_ExtensionMap[".scene"] = AssetType::Scene;
        m_ExtensionMap[".png"] = AssetType::Texture;
        m_ExtensionMap[".jpg"] = AssetType::Texture;
        m_ExtensionMap[".jpeg"] = AssetType::Texture;
        m_ExtensionMap[".tga"] = AssetType::Texture;
        m_ExtensionMap[".bmp"] = AssetType::Texture;
        m_ExtensionMap[".hdr"] = AssetType::Texture;
        m_ExtensionMap[".obj"] = AssetType::Model;
        m_ExtensionMap[".fbx"] = AssetType::Model;
        m_ExtensionMap[".gltf"] = AssetType::Model;
        m_ExtensionMap[".glb"] = AssetType::Model;
        m_ExtensionMap[".dae"] = AssetType::Model;
        m_ExtensionMap[".wav"] = AssetType::Audio;
        m_ExtensionMap[".mp3"] = AssetType::Audio;
        m_ExtensionMap[".ogg"] = AssetType::Audio;
        m_ExtensionMap[".lua"] = AssetType::Script;
        m_ExtensionMap[".mat"] = AssetType::Material;
        m_ExtensionMap[".glsl"] = AssetType::Shader;
        m_ExtensionMap[".vert"] = AssetType::Shader;
        m_ExtensionMap[".frag"] = AssetType::Shader;
        
        // Set default assets directory
        m_AssetsRoot = std::filesystem::current_path() / "Assets";
        m_CurrentDirectory = m_AssetsRoot;
    }

    void AssetBrowserPanel::SetAssetsDirectory(const std::filesystem::path& path) {
        m_AssetsRoot = path;
        m_CurrentDirectory = path;
        m_NeedsRefresh = true;
    }

    void AssetBrowserPanel::Refresh() {
        m_NeedsRefresh = true;
    }

    void AssetBrowserPanel::OnImGuiRender() {
        ImGui::Begin("Asset Browser");

        if (!m_PendingDeletePath.empty()) ImGui::OpenPopup("Confirm Delete");
        if (m_IsRenaming && !m_PendingRenamePath.empty()) ImGui::OpenPopup("Rename");
        
        // Top toolbar
        if (ImGui::Button("<##Back")) {
            if (m_CurrentDirectory != m_AssetsRoot) {
                m_CurrentDirectory = m_CurrentDirectory.parent_path();
                m_NeedsRefresh = true;
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Go back");
        
        ImGui::SameLine();
        if (ImGui::Button("^##Up")) {
            if (m_CurrentDirectory != m_AssetsRoot) {
                m_CurrentDirectory = m_CurrentDirectory.parent_path();
                m_NeedsRefresh = true;
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Go to parent");
        
        ImGui::SameLine();
        if (ImGui::Button("R##Refresh")) {
            m_NeedsRefresh = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Refresh");
        
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();
        
        // View toggle
        if (ImGui::Button(m_GridView ? "List" : "Grid")) {
            m_GridView = !m_GridView;
        }
        
        ImGui::SameLine();
        
        // Thumbnail size slider (only in grid view)
        if (m_GridView) {
            ImGui::SetNextItemWidth(100);
            ImGui::SliderFloat("##Size", &m_ThumbnailSize, 32.0f, 128.0f, "%.0f");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Thumbnail size");
            ImGui::SameLine();
        }
        
        // Filter and Sort
        RenderFilterAndSort();
        
        // Search with case-sensitive toggle
        ImGui::Text("Search:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("##Search", m_SearchBuffer, sizeof(m_SearchBuffer));
        ImGui::SameLine();
        if (ImGui::Button("X##ClearSearch")) {
            m_SearchBuffer[0] = '\0';
            m_NeedsRefresh = true;
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Case", &m_CaseSensitive)) {
            m_NeedsRefresh = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Case-sensitive search");
        }
        
        // Breadcrumb
        RenderBreadcrumb();
        
        ImGui::Separator();
        
        // Main content area with tree and contents
        float treeWidth = 200.0f;
        
        // Directory tree (left panel)
        ImGui::BeginChild("DirectoryTree", ImVec2(treeWidth, 0), true);
        RenderDirectoryTree();
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Directory contents (right panel)
        ImGui::BeginChild("DirectoryContents", ImVec2(0, 0), true);
        RenderDirectoryContents();
        ImGui::EndChild();
        
        // Context menu for empty space
        if (ImGui::BeginPopupContextWindow("AssetBrowserContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::BeginMenu("Create")) {
                if (ImGui::MenuItem("Folder")) {
                    CreateNewFolder();
                }
                if (ImGui::MenuItem("Lua Script")) {
                    CreateNewScript();
                }
                if (ImGui::MenuItem("Scene")) {
                    CreateNewScene();
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Refresh")) {
                m_NeedsRefresh = true;
            }
            ImGui::EndPopup();
        }

        RenderModals();
        ImGui::End();
    }

    void AssetBrowserPanel::RenderBreadcrumb() {
        std::filesystem::path relativePath = std::filesystem::relative(m_CurrentDirectory, m_AssetsRoot);
        
        ImGui::Text("Assets");
        
        std::filesystem::path currentBuild = m_AssetsRoot;
        for (const auto& part : relativePath) {
            if (part == ".") continue;
            
            ImGui::SameLine();
            ImGui::Text("/");
            ImGui::SameLine();
            
            currentBuild /= part;
            
            if (ImGui::Button(part.string().c_str())) {
                m_CurrentDirectory = currentBuild;
                m_NeedsRefresh = true;
            }
        }
    }

    void AssetBrowserPanel::RenderDirectoryTree() {
        if (std::filesystem::exists(m_AssetsRoot)) {
            DrawDirectoryNode(m_AssetsRoot);
        }
    }

    void AssetBrowserPanel::DrawDirectoryNode(const std::filesystem::path& path) {
        std::string name = path.filename().string();
        if (name.empty()) name = path.string();
        
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        
        if (m_CurrentDirectory == path) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        
        // Check if has subdirectories
        bool hasSubdirs = false;
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                if (entry.is_directory()) {
                    hasSubdirs = true;
                    break;
                }
            }
        }
        
        if (!hasSubdirs) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        
        bool opened = ImGui::TreeNodeEx(name.c_str(), flags);
        
        if (ImGui::IsItemClicked()) {
            m_CurrentDirectory = path;
            AddToRecentFolders(path);
            m_NeedsRefresh = true;
        }
        
        if (opened) {
            if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                for (const auto& entry : std::filesystem::directory_iterator(path)) {
                    if (entry.is_directory()) {
                        DrawDirectoryNode(entry.path());
                    }
                }
            }
            ImGui::TreePop();
        }
    }

    void AssetBrowserPanel::RenderDirectoryContents() {
        // Refresh cache if needed
        if (m_NeedsRefresh) {
            m_CachedEntries.clear();
            
            if (std::filesystem::exists(m_CurrentDirectory) && std::filesystem::is_directory(m_CurrentDirectory)) {
                for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory)) {
                    m_CachedEntries.push_back(entry);
                }
            }
            
            // Apply filter and sort
            ApplyFilter();
            ApplySort();
            
            // Add current directory to recent folders
            AddToRecentFolders(m_CurrentDirectory);
            
            m_NeedsRefresh = false;
        }
        
        if (m_GridView) {
            // Grid view
            float panelWidth = ImGui::GetContentRegionAvail().x;
            int columnCount = (int)(panelWidth / (m_ThumbnailSize + m_Padding));
            if (columnCount < 1) columnCount = 1;
            
            ImGui::Columns(columnCount, nullptr, false);
            
            for (const auto& entry : m_FilteredEntries) {
                DrawAssetEntry(entry, true);
                ImGui::NextColumn();
            }
            
            ImGui::Columns(1);
        } else {
            // List view
            for (const auto& entry : m_FilteredEntries) {
                DrawAssetEntry(entry, false);
            }
        }
    }

    void AssetBrowserPanel::DrawAssetEntry(const std::filesystem::directory_entry& entry, bool isGridView) {
        const auto& path = entry.path();
        std::string filename = path.filename().string();
        
        AssetType type = entry.is_directory() ? AssetType::Folder : GetAssetType(path);
        const char* icon = GetAssetIcon(type);
        ImVec4 color = GetAssetColor(type);
        
        bool isSelected = (m_SelectedPath == path);
        
        ImGui::PushID(filename.c_str());
        
        if (isGridView) {
            // Grid item
            ImGui::BeginGroup();
            
            ImGui::PushStyleColor(ImGuiCol_Button, isSelected ? ImVec4(0.3f, 0.5f, 0.8f, 0.5f) : ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.8f, 0.3f));
            
            if (ImGui::Button(icon, ImVec2(m_ThumbnailSize, m_ThumbnailSize))) {
                m_SelectedPath = path;
            }
            
            ImGui::PopStyleColor(2);
            
            // Handle double-click
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                HandleDoubleClick(path);
            }
            
            // Drag source
            HandleDragDrop(path);
            
            // Context menu
            HandleContextMenu(path);
            
            // Filename
            ImGui::TextWrapped("%s", filename.c_str());
            
            ImGui::EndGroup();
        } else {
            // List item
            ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick;
            
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            
            char label[512];
            snprintf(label, sizeof(label), "%s  %s", icon, filename.c_str());
            
            if (ImGui::Selectable(label, isSelected, flags)) {
                m_SelectedPath = path;
                
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    HandleDoubleClick(path);
                }
            }
            
            ImGui::PopStyleColor();
            
            // Drag source
            HandleDragDrop(path);
            
            // Context menu
            HandleContextMenu(path);
        }
        
        ImGui::PopID();
    }

    AssetType AssetBrowserPanel::GetAssetType(const std::filesystem::path& path) const {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        auto it = m_ExtensionMap.find(ext);
        if (it != m_ExtensionMap.end()) {
            return it->second;
        }
        
        return AssetType::Unknown;
    }

    const char* AssetBrowserPanel::GetAssetIcon(AssetType type) const {
        switch (type) {
            case AssetType::Folder:   return "[D]";
            case AssetType::Scene:    return "[S]";
            case AssetType::Texture:  return "[T]";
            case AssetType::Model:    return "[M]";
            case AssetType::Audio:    return "[A]";
            case AssetType::Script:   return "[L]";
            case AssetType::Material: return "[m]";
            case AssetType::Shader:   return "[#]";
            default:                  return "[?]";
        }
    }

    ImVec4 AssetBrowserPanel::GetAssetColor(AssetType type) const {
        switch (type) {
            case AssetType::Folder:   return ImVec4(1.0f, 0.9f, 0.4f, 1.0f);  // Yellow
            case AssetType::Scene:    return ImVec4(0.4f, 0.8f, 1.0f, 1.0f);  // Cyan
            case AssetType::Texture:  return ImVec4(0.4f, 1.0f, 0.4f, 1.0f);  // Green
            case AssetType::Model:    return ImVec4(1.0f, 0.6f, 0.2f, 1.0f);  // Orange
            case AssetType::Audio:    return ImVec4(1.0f, 0.4f, 1.0f, 1.0f);  // Magenta
            case AssetType::Script:   return ImVec4(0.6f, 0.6f, 1.0f, 1.0f);  // Blue
            case AssetType::Material: return ImVec4(0.8f, 0.8f, 0.8f, 1.0f);  // Gray
            case AssetType::Shader:   return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);  // Red
            default:                  return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);  // Gray
        }
    }

    void AssetBrowserPanel::HandleDragDrop(const std::filesystem::path& path) {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            std::string pathStr = path.string();
            ImGui::SetDragDropPayload("ASSET_PATH", pathStr.c_str(), pathStr.size() + 1);
            ImGui::Text("%s", path.filename().string().c_str());
            ImGui::EndDragDropSource();
        }
    }

    void AssetBrowserPanel::HandleDoubleClick(const std::filesystem::path& path) {
        if (std::filesystem::is_directory(path)) {
            m_CurrentDirectory = path;
            m_NeedsRefresh = true;
        } else {
            // Open file based on type
            AssetType type = GetAssetType(path);
            switch (type) {
                case AssetType::Scene:
                    if (m_OnOpenScene) {
                        m_OnOpenScene(path.string());
                    } else {
                        GE_CORE_INFO("Opening scene: {0}", path.string());
                    }
                    break;
                case AssetType::Script:
                    OpenInExternalEditor(path.string());
                    break;
                default:
                    GE_CORE_INFO("Selected asset: {0}", path.string());
                    break;
            }
        }
    }

    void AssetBrowserPanel::HandleContextMenu(const std::filesystem::path& path) {
        if (ImGui::BeginPopupContextItem()) {
            if (std::filesystem::is_directory(path)) {
                if (ImGui::MenuItem("Open")) {
                    m_CurrentDirectory = path;
                    m_NeedsRefresh = true;
                }
            }
            
            if (ImGui::MenuItem("Rename")) {
                m_PendingRenamePath = path;
                m_IsRenaming = true;
                memset(m_RenameBuffer, 0, sizeof(m_RenameBuffer));
                strncpy(m_RenameBuffer, path.filename().string().c_str(), sizeof(m_RenameBuffer) - 1);
            }
            
            if (ImGui::MenuItem("Delete")) {
                m_PendingDeletePath = path;
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Show in Explorer")) {
                // Platform-specific: open file explorer
                #ifdef _WIN32
                std::string cmd = "explorer /select,\"" + path.string() + "\"";
                system(cmd.c_str());
                #endif
            }
            
            ImGui::EndPopup();
        }
    }

    void AssetBrowserPanel::CreateNewFolder() {
        std::filesystem::path newPath = m_CurrentDirectory / "New Folder";
        int counter = 1;
        while (std::filesystem::exists(newPath)) {
            newPath = m_CurrentDirectory / ("New Folder " + std::to_string(counter++));
        }
        
        try {
            std::filesystem::create_directory(newPath);
            m_NeedsRefresh = true;
            GE_CORE_INFO("Created folder: {0}", newPath.string());
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to create folder: {0}", e.what());
        }
    }

    void AssetBrowserPanel::CreateNewScript() {
        std::filesystem::path newPath = m_CurrentDirectory / "NewScript.lua";
        int counter = 1;
        while (std::filesystem::exists(newPath)) {
            newPath = m_CurrentDirectory / ("NewScript" + std::to_string(counter++) + ".lua");
        }
        
        try {
            std::ofstream file(newPath);
            file << "-- " << newPath.filename().string() << "\n\n";
            file << "function OnStart()\n";
            file << "    -- Called when the entity is created\n";
            file << "end\n\n";
            file << "function OnUpdate(deltaTime)\n";
            file << "    -- Called every frame\n";
            file << "end\n\n";
            file << "function OnDestroy()\n";
            file << "    -- Called when the entity is destroyed\n";
            file << "end\n";
            file.close();
            
            m_NeedsRefresh = true;
            GE_CORE_INFO("Created script: {0}", newPath.string());
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to create script: {0}", e.what());
        }
    }

    void AssetBrowserPanel::CreateNewScene() {
        std::filesystem::path newPath = m_CurrentDirectory / "NewScene.scene";
        int counter = 1;
        while (std::filesystem::exists(newPath)) {
            newPath = m_CurrentDirectory / ("NewScene" + std::to_string(counter++) + ".scene");
        }
        
        try {
            std::ofstream file(newPath);
            file << "{\n";
            file << "  \"version\": \"1.0.0\",\n";
            file << "  \"name\": \"" << newPath.stem().string() << "\",\n";
            file << "  \"entities\": []\n";
            file << "}\n";
            file.close();
            
            m_NeedsRefresh = true;
            GE_CORE_INFO("Created scene: {0}", newPath.string());
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to create scene: {0}", e.what());
        }
    }

    void AssetBrowserPanel::DeleteSelected() {
        if (m_SelectedPath.empty()) return;
        
        try {
            std::filesystem::remove_all(m_SelectedPath);
            m_SelectedPath.clear();
            m_NeedsRefresh = true;
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to delete: {0}", e.what());
        }
    }

    void AssetBrowserPanel::RenameSelected() {
        if (m_SelectedPath.empty()) return;
        m_PendingRenamePath = m_SelectedPath;
        m_IsRenaming = true;
        memset(m_RenameBuffer, 0, sizeof(m_RenameBuffer));
        strncpy(m_RenameBuffer, m_SelectedPath.filename().string().c_str(), sizeof(m_RenameBuffer) - 1);
    }

    void AssetBrowserPanel::OpenInExternalEditor(const std::string& path) {
#ifdef _WIN32
        ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#elif __APPLE__
        std::string cmd = "open \"" + path + "\"";
        (void)system(cmd.c_str());
#else
        std::string cmd = "xdg-open \"" + path + "\"";
        (void)system(cmd.c_str());
#endif
    }

    void AssetBrowserPanel::RenderModals() {
        if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (!m_PendingDeletePath.empty()) {
                ImGui::Text("Delete \"%s\"?", m_PendingDeletePath.filename().string().c_str());
                ImGui::TextUnformatted("This cannot be undone.");
                if (ImGui::Button("Delete")) {
                    try {
                        std::filesystem::remove_all(m_PendingDeletePath);
                        m_NeedsRefresh = true;
                        GE_CORE_INFO("Deleted: {0}", m_PendingDeletePath.string());
                    } catch (const std::exception& e) {
                        GE_CORE_ERROR("Failed to delete: {0}", e.what());
                    }
                    m_PendingDeletePath.clear();
                    m_SelectedPath.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    m_PendingDeletePath.clear();
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Rename", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (!m_PendingRenamePath.empty()) {
                ImGui::Text("Rename to:");
                ImGui::SetNextItemWidth(300.0f);
                bool enterPressed = ImGui::InputText("##Rename", m_RenameBuffer, sizeof(m_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
                if (ImGui::Button("OK") || enterPressed) {
                    std::string newName(m_RenameBuffer);
                    if (!newName.empty()) {
                        std::filesystem::path newPath = m_PendingRenamePath.parent_path() / newName;
                        try {
                            std::filesystem::rename(m_PendingRenamePath, newPath);
                            m_NeedsRefresh = true;
                            if (m_SelectedPath == m_PendingRenamePath) m_SelectedPath = newPath;
                            GE_CORE_INFO("Renamed to: {0}", newPath.string());
                        } catch (const std::exception& e) {
                            GE_CORE_ERROR("Failed to rename: {0}", e.what());
                        }
                    }
                    m_PendingRenamePath.clear();
                    m_IsRenaming = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    m_PendingRenamePath.clear();
                    m_IsRenaming = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }
    }

    void AssetBrowserPanel::RenderFilterAndSort() {
        // Filter dropdown with label
        ImGui::Text("Filter:");
        ImGui::SameLine();
        const char* filterItems[] = {
            "All", "Models", "Scripts", "Textures", "Audio", "Scenes", "Shaders"
        };
        int currentFilter = static_cast<int>(m_CurrentFilter);
        ImGui::SetNextItemWidth(120);
        if (ImGui::Combo("##Filter", &currentFilter, filterItems, 7)) {
            m_CurrentFilter = static_cast<SelectionFilter>(currentFilter);
            m_NeedsRefresh = true;
        }
        
        ImGui::SameLine();
        
        // Sort dropdown with label
        ImGui::Text("Sort:");
        ImGui::SameLine();
        const char* sortItems[] = {
            "Recent", "Name", "Path", "Size", "Type"
        };
        int currentSort = static_cast<int>(m_CurrentSorting);
        ImGui::SetNextItemWidth(120);
        if (ImGui::Combo("##Sort", &currentSort, sortItems, 5)) {
            m_CurrentSorting = static_cast<SortingMode>(currentSort);
            m_NeedsRefresh = true;
        }
        
        ImGui::SameLine();
        
        // Recent folders dropdown
        if (!m_RecentFolders.empty()) {
            ImGui::Text("Recent:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(150);
            if (ImGui::BeginCombo("##RecentFolders", "Select...")) {
                for (const auto& folder : m_RecentFolders) {
                    std::string displayName = folder.filename().string();
                    if (displayName.empty()) {
                        displayName = folder.string();
                    }
                    
                    if (ImGui::Selectable(displayName.c_str())) {
                        m_CurrentDirectory = folder;
                        AddToRecentFolders(folder);
                        m_NeedsRefresh = true;
                    }
                }
                ImGui::EndCombo();
            }
        }
    }

    void AssetBrowserPanel::ApplyFilter() {
        m_FilteredEntries.clear();
        
        for (const auto& entry : m_CachedEntries) {
            if (MatchesFilter(entry)) {
                m_FilteredEntries.push_back(entry);
            }
        }
    }

    void AssetBrowserPanel::ApplySort() {
        std::sort(m_FilteredEntries.begin(), m_FilteredEntries.end(),
            [this](const auto& a, const auto& b) {
                // Directories always first
                if (a.is_directory() != b.is_directory()) {
                    return a.is_directory();
                }
                
                switch (m_CurrentSorting) {
                    case SortingMode::Name:
                        return a.path().filename() < b.path().filename();
                    case SortingMode::Path:
                        return a.path() < b.path();
                    case SortingMode::Size:
                        if (a.is_directory() || b.is_directory()) {
                            return a.is_directory(); // Directories first
                        }
                        return std::filesystem::file_size(a.path()) < std::filesystem::file_size(b.path());
                    case SortingMode::Type:
                        {
                            AssetType typeA = GetAssetType(a.path());
                            AssetType typeB = GetAssetType(b.path());
                            if (typeA != typeB) {
                                return typeA < typeB;
                            }
                            return a.path().filename() < b.path().filename();
                        }
                    case SortingMode::Recent:
                    default:
                        // For recent, use modification time
                        {
                            auto timeA = std::filesystem::last_write_time(a.path());
                            auto timeB = std::filesystem::last_write_time(b.path());
                            return timeA > timeB; // Newer first
                        }
                }
            });
    }

    bool AssetBrowserPanel::MatchesFilter(const std::filesystem::directory_entry& entry) const {
        // Always show directories
        if (entry.is_directory()) {
            return true;
        }
        
        // Apply selection filter
        if (m_CurrentFilter != SelectionFilter::All) {
            AssetType type = GetAssetType(entry.path());
            switch (m_CurrentFilter) {
                case SelectionFilter::Models:
                    if (type != AssetType::Model) return false;
                    break;
                case SelectionFilter::Scripts:
                    if (type != AssetType::Script) return false;
                    break;
                case SelectionFilter::Textures:
                    if (type != AssetType::Texture) return false;
                    break;
                case SelectionFilter::Audio:
                    if (type != AssetType::Audio) return false;
                    break;
                case SelectionFilter::Scenes:
                    if (type != AssetType::Scene) return false;
                    break;
                case SelectionFilter::Shaders:
                    if (type != AssetType::Shader) return false;
                    break;
                default:
                    break;
            }
        }
        
        // Apply search filter
        if (m_SearchBuffer[0] != '\0') {
            std::string name = entry.path().filename().string();
            std::string search = m_SearchBuffer;
            
            if (!m_CaseSensitive) {
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                std::transform(search.begin(), search.end(), search.begin(), ::tolower);
            }
            
            if (name.find(search) == std::string::npos) {
                return false;
            }
        }
        
        return true;
    }

    void AssetBrowserPanel::AddToRecentFolders(const std::filesystem::path& path) {
        // Remove if already exists
        m_RecentFolders.erase(
            std::remove(m_RecentFolders.begin(), m_RecentFolders.end(), path),
            m_RecentFolders.end()
        );
        
        // Add to front
        m_RecentFolders.insert(m_RecentFolders.begin(), path);
        
        // Limit size
        if (m_RecentFolders.size() > MAX_RECENT_FOLDERS) {
            m_RecentFolders.pop_back();
        }
    }

}
