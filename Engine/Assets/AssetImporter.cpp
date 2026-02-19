#include "AssetImporter.hpp"
#include "../Core/Logger.hpp"
#include "../Platform/FileSystem.hpp"
#include <glm/gtc/matrix_transform.hpp>

// Assimp includes
#ifdef GE_HAS_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif

#include <filesystem>

namespace GameEngine {

    std::vector<std::string> AssetImporter::GetSupportedFormats() {
        return {
            ".fbx", ".obj", ".gltf", ".glb", ".dae",
            ".3ds", ".blend", ".stl", ".ply", ".x3d"
        };
    }

    bool AssetImporter::IsSupported(const std::string& filepath) {
        std::filesystem::path path(filepath);
        std::string ext = path.extension().string();
        
        // Convert to lowercase
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        auto formats = GetSupportedFormats();
        return std::find(formats.begin(), formats.end(), ext) != formats.end();
    }

    MeshImportResult AssetImporter::ImportMesh(
        const std::string& filepath,
        const MeshImportOptions& options
    ) {
        MeshImportResult result;
        
#ifdef GE_HAS_ASSIMP
        // Create Assimp importer
        Assimp::Importer importer;
        
        // Configure import flags
        unsigned int importFlags = 0;
        
        if (options.Triangulate) {
            importFlags |= aiProcess_Triangulate;
        }
        if (options.FlipUVs) {
            importFlags |= aiProcess_FlipUVs;
        }
        if (options.GenerateNormals) {
            importFlags |= aiProcess_GenSmoothNormals;
        }
        if (options.GenerateTangents) {
            importFlags |= aiProcess_CalcTangentSpace;
        }
        if (options.OptimizeMeshes) {
            importFlags |= aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph;
        }
        if (options.JoinIdenticalVertices) {
            importFlags |= aiProcess_JoinIdenticalVertices;
        }
        
        // Add common processing steps
        importFlags |= aiProcess_ValidateDataStructure;
        importFlags |= aiProcess_ImproveCacheLocality;
        importFlags |= aiProcess_RemoveRedundantMaterials;
        importFlags |= aiProcess_SortByPType;
        
        // Load the model
        const aiScene* scene = importer.ReadFile(filepath, importFlags);
        
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            result.ErrorMessage = "Assimp error: " + std::string(importer.GetErrorString());
            GE_CORE_ERROR("AssetImporter: {}", result.ErrorMessage);
            return result;
        }
        
        // Get directory for texture loading
        std::filesystem::path path(filepath);
        std::string directory = path.parent_path().string();
        if (!options.TextureSearchPath.empty()) {
            directory = options.TextureSearchPath;
        }
        
        // Texture cache to avoid loading duplicates
        std::unordered_map<std::string, Ref<Texture2D>> textureCache;
        
        // Process materials first
        if (options.ImportMaterials && scene->HasMaterials()) {
            for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
                auto material = ProcessMaterial(
                    scene->mMaterials[i],
                    (void*)scene,
                    directory,
                    options,
                    textureCache
                );
                result.Materials.push_back(material);
            }
        }
        
        // Store loaded textures in result
        for (const auto& [path, texture] : textureCache) {
            result.Textures.push_back(texture);
        }
        
        // Process scene hierarchy starting from root
        ProcessNode(
            scene->mRootNode,
            (void*)scene,
            result,
            options,
            directory,
            -1
        );
        
        result.Success = true;
        GE_CORE_INFO("AssetImporter: Loaded '{}' - {} meshes, {} materials",
            filepath, result.Meshes.size(), result.Materials.size());
        
#else
        // Assimp not available
        result.ErrorMessage = "Assimp library not available. Build with GE_HAS_ASSIMP defined.";
        GE_CORE_ERROR("AssetImporter: {}", result.ErrorMessage);
#endif
        
        return result;
    }

#ifdef GE_HAS_ASSIMP
    void AssetImporter::ProcessNode(
        void* node,
        void* scene,
        MeshImportResult& result,
        const MeshImportOptions& options,
        const std::string& directory,
        int parentIndex
    ) {
        aiNode* aiN = static_cast<aiNode*>(node);
        aiScene* aiS = static_cast<aiScene*>(scene);
        
        // Create node info
        MeshImportResult::Node nodeInfo;
        nodeInfo.Name = aiN->mName.C_Str();
        
        // Convert Assimp matrix to glm
        aiMatrix4x4& m = aiN->mTransformation;
        nodeInfo.Transform = glm::mat4(
            m.a1, m.b1, m.c1, m.d1,
            m.a2, m.b2, m.c2, m.d2,
            m.a3, m.b3, m.c3, m.d3,
            m.a4, m.b4, m.c4, m.d4
        );
        
        // Apply scale
        if (options.Scale != 1.0f) {
            nodeInfo.Transform = glm::scale(nodeInfo.Transform, glm::vec3(options.Scale));
        }
        
        int currentNodeIndex = static_cast<int>(result.Nodes.size());
        
        // Process meshes for this node
        if (aiN->mNumMeshes > 0) {
            nodeInfo.MeshIndex = static_cast<int>(result.Meshes.size());
            
            for (unsigned int i = 0; i < aiN->mNumMeshes; i++) {
                aiMesh* mesh = aiS->mMeshes[aiN->mMeshes[i]];
                auto processedMesh = ProcessMesh(mesh, scene, options);
                if (processedMesh) {
                    result.Meshes.push_back(processedMesh);
                }
            }
        }
        
        // Add node to result
        result.Nodes.push_back(nodeInfo);
        
        // Add as child to parent
        if (parentIndex >= 0) {
            result.Nodes[parentIndex].Children.push_back(currentNodeIndex);
        }
        
        // Process children
        for (unsigned int i = 0; i < aiN->mNumChildren; i++) {
            ProcessNode(aiN->mChildren[i], scene, result, options, directory, currentNodeIndex);
        }
    }

    Ref<Mesh3D> AssetImporter::ProcessMesh(
        void* mesh,
        void* scene,
        const MeshImportOptions& options
    ) {
        aiMesh* aiM = static_cast<aiMesh*>(mesh);
        
        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;
        
        // Process vertices
        vertices.reserve(aiM->mNumVertices);
        
        for (unsigned int i = 0; i < aiM->mNumVertices; i++) {
            Vertex3D vertex;
            
            // Position
            vertex.Position.x = aiM->mVertices[i].x;
            vertex.Position.y = aiM->mVertices[i].y;
            vertex.Position.z = aiM->mVertices[i].z;
            
            // Normal
            if (aiM->HasNormals()) {
                vertex.Normal.x = aiM->mNormals[i].x;
                vertex.Normal.y = aiM->mNormals[i].y;
                vertex.Normal.z = aiM->mNormals[i].z;
            }
            
            // Texture coordinates
            if (aiM->mTextureCoords[0]) {
                vertex.TexCoords.x = aiM->mTextureCoords[0][i].x;
                vertex.TexCoords.y = aiM->mTextureCoords[0][i].y;
            }
            
            // Tangent
            if (aiM->HasTangentsAndBitangents()) {
                vertex.Tangent.x = aiM->mTangents[i].x;
                vertex.Tangent.y = aiM->mTangents[i].y;
                vertex.Tangent.z = aiM->mTangents[i].z;
            }
            
            vertices.push_back(vertex);
        }
        
        // Process indices
        for (unsigned int i = 0; i < aiM->mNumFaces; i++) {
            aiFace& face = aiM->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }
        
        // Create mesh
        auto result = CreateRef<Mesh3D>(vertices, indices);
        result->SetName(aiM->mName.C_Str());
        
        return result;
    }

    Ref<Material> AssetImporter::ProcessMaterial(
        void* material,
        void* scene,
        const std::string& directory,
        const MeshImportOptions& options,
        std::unordered_map<std::string, Ref<Texture2D>>& textureCache
    ) {
        aiMaterial* aiMat = static_cast<aiMaterial*>(material);
        
        auto result = CreateRef<Material>();
        
        // Material name
        aiString name;
        if (aiMat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
            result->SetName(name.C_Str());
        }
        
        // Diffuse color
        aiColor3D color(1.0f, 1.0f, 1.0f);
        if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            result->SetAlbedo(glm::vec3(color.r, color.g, color.b));
        }
        
        // Metallic
        float metallic = 0.0f;
        if (aiMat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
            result->SetMetallic(metallic);
        }
        
        // Roughness
        float roughness = 0.5f;
        if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
            result->SetRoughness(roughness);
        }
        
        // Load textures if enabled
        if (options.ImportTextures) {
            // Albedo/Diffuse texture
            auto albedoTex = LoadMaterialTexture(material, aiTextureType_DIFFUSE, directory, textureCache);
            if (albedoTex) {
                result->SetAlbedoMap(albedoTex);
            }
            
            // Normal map
            auto normalTex = LoadMaterialTexture(material, aiTextureType_NORMALS, directory, textureCache);
            if (!normalTex) {
                normalTex = LoadMaterialTexture(material, aiTextureType_HEIGHT, directory, textureCache);
            }
            if (normalTex) {
                result->SetNormalMap(normalTex);
            }
            
            // Metallic/Roughness (often combined in PBR workflows)
            auto metallicTex = LoadMaterialTexture(material, aiTextureType_METALNESS, directory, textureCache);
            if (metallicTex) {
                result->SetMetallicMap(metallicTex);
            }
            
            // Ambient occlusion
            auto aoTex = LoadMaterialTexture(material, aiTextureType_AMBIENT_OCCLUSION, directory, textureCache);
            if (aoTex) {
                result->SetAOMap(aoTex);
            }
        }
        
        return result;
    }

    Ref<Texture2D> AssetImporter::LoadMaterialTexture(
        void* material,
        int textureType,
        const std::string& directory,
        std::unordered_map<std::string, Ref<Texture2D>>& textureCache
    ) {
        aiMaterial* aiMat = static_cast<aiMaterial*>(material);
        aiTextureType type = static_cast<aiTextureType>(textureType);
        
        if (aiMat->GetTextureCount(type) == 0) {
            return nullptr;
        }
        
        aiString texPath;
        if (aiMat->GetTexture(type, 0, &texPath) != AI_SUCCESS) {
            return nullptr;
        }
        
        std::string fullPath = directory + "/" + texPath.C_Str();
        
        // Check cache
        auto it = textureCache.find(fullPath);
        if (it != textureCache.end()) {
            return it->second;
        }
        
        // Load texture
        if (FileSystem::Exists(fullPath)) {
            auto texture = CreateRef<Texture2D>(fullPath);
            if (texture) {
                textureCache[fullPath] = texture;
                return texture;
            }
        }
        
        GE_CORE_WARN("AssetImporter: Texture not found: {}", fullPath);
        return nullptr;
    }
#else
    // Stub implementations when Assimp is not available
    void AssetImporter::ProcessNode(void*, void*, MeshImportResult&, const MeshImportOptions&, const std::string&, int) {}
    Ref<Mesh3D> AssetImporter::ProcessMesh(void*, void*, const MeshImportOptions&) { return nullptr; }
    Ref<Material> AssetImporter::ProcessMaterial(void*, void*, const std::string&, const MeshImportOptions&, std::unordered_map<std::string, Ref<Texture2D>>&) { return nullptr; }
    Ref<Texture2D> AssetImporter::LoadMaterialTexture(void*, int, const std::string&, std::unordered_map<std::string, Ref<Texture2D>>&) { return nullptr; }
#endif

    Ref<Texture2D> AssetImporter::ImportTexture(const std::string& filepath, bool generateMipmaps) {
        if (!FileSystem::Exists(filepath)) {
            GE_CORE_ERROR("AssetImporter: Texture file not found: {}", filepath);
            return nullptr;
        }
        
        auto texture = CreateRef<Texture2D>(filepath);
        if (texture && generateMipmaps) {
            texture->GenerateMipmaps();
        }
        return texture;
    }

}
