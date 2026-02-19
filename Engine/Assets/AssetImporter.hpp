#pragma once

#include "../Core/Base.hpp"
#include "../Graphics/Mesh3D.hpp"
#include "../Graphics/Material.hpp"
#include "../Graphics/Texture2D.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace GameEngine {

    /**
     * @brief Result of mesh import
     */
    struct MeshImportResult {
        bool Success = false;
        std::string ErrorMessage;
        
        std::vector<Ref<Mesh3D>> Meshes;
        std::vector<Ref<Material>> Materials;
        std::vector<Ref<Texture2D>> Textures;
        
        // Hierarchy information
        struct Node {
            std::string Name;
            glm::mat4 Transform;
            int MeshIndex = -1;
            std::vector<int> Children;
        };
        std::vector<Node> Nodes;
    };

    /**
     * @brief Import options for mesh loading
     */
    struct MeshImportOptions {
        bool FlipUVs = true;
        bool GenerateNormals = true;
        bool GenerateTangents = true;
        bool OptimizeMeshes = true;
        bool JoinIdenticalVertices = true;
        bool Triangulate = true;
        float Scale = 1.0f;
        bool ImportMaterials = true;
        bool ImportTextures = true;
        std::string TextureSearchPath;
    };

    /**
     * @brief Asset import pipeline using Assimp
     * 
     * Supports:
     * - FBX, OBJ, GLTF/GLB, DAE (Collada), 3DS, Blend, etc.
     * - Mesh geometry with UVs, normals, tangents
     * - Material properties
     * - Texture references
     * - Scene hierarchy
     */
    class AssetImporter {
    public:
        /**
         * @brief Import mesh from file
         * @param filepath Path to 3D model file
         * @param options Import options
         * @return Import result containing meshes, materials, and textures
         */
        static MeshImportResult ImportMesh(
            const std::string& filepath,
            const MeshImportOptions& options = MeshImportOptions()
        );
        
        /**
         * @brief Check if file format is supported
         */
        static bool IsSupported(const std::string& filepath);
        
        /**
         * @brief Get list of supported formats
         */
        static std::vector<std::string> GetSupportedFormats();
        
        /**
         * @brief Import only textures from a material description
         */
        static Ref<Texture2D> ImportTexture(
            const std::string& filepath,
            bool generateMipmaps = true
        );
        
    private:
        static void ProcessNode(
            void* aiNode, 
            void* aiScene,
            MeshImportResult& result,
            const MeshImportOptions& options,
            const std::string& directory,
            int parentIndex
        );
        
        static Ref<Mesh3D> ProcessMesh(
            void* aiMesh,
            void* aiScene,
            const MeshImportOptions& options
        );
        
        static Ref<Material> ProcessMaterial(
            void* aiMaterial,
            void* aiScene,
            const std::string& directory,
            const MeshImportOptions& options,
            std::unordered_map<std::string, Ref<Texture2D>>& textureCache
        );
        
        static Ref<Texture2D> LoadMaterialTexture(
            void* aiMaterial,
            int textureType,
            const std::string& directory,
            std::unordered_map<std::string, Ref<Texture2D>>& textureCache
        );
    };

}
