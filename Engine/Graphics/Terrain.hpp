#pragma once

#include "../Core/Base.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include "VertexArray.hpp"
#include "Texture2D.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <array>
#include <cstdint>

namespace GameEngine {

    struct TerrainVertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoord;
    };

    /**
     * @brief Heightmap / noise-generated terrain mesh backed by Vulkan buffers.
     *
     * All geometry lives in a VkBuffer (vertex + index) allocated via VMA.
     * Textures are Texture2D objects (VkImage + VkSampler).
     * Rendering is done by calling Render(VkCommandBuffer) which records
     * vkCmdDraw* into an already-active render pass.
     */
    class Terrain {
    public:
        Terrain() = default;
        ~Terrain();

        Terrain(const Terrain&) = delete;
        Terrain& operator=(const Terrain&) = delete;

        bool loadFromHeightmap(const std::string& path, float width, float depth, float maxHeight);
        void generateFromNoise(float width, float depth, int resolution, uint32_t seed);

        float getHeightAt(float x, float z) const;
        glm::vec3 getNormalAt(float x, float z) const;

        /** Load a texture into a terrain layer (index 0-3). */
        void setTextureLayer(int index, const std::string& texturePath, float scale);

        /** Load the blend map texture. */
        void setBlendMap(const std::string& path);

        /**
         * @brief Record draw commands into cmd.
         *
         * The caller must have already:
         *  - begun a render pass
         *  - bound the terrain pipeline and descriptor sets (textures, UBOs)
         *
         * This method calls:
         *  vkCmdBindVertexBuffers / vkCmdBindIndexBuffer / vkCmdDrawIndexed
         */
        void render(VkCommandBuffer cmd) const;

        float    getWidth()      const { return m_Width; }
        float    getDepth()      const { return m_Depth; }
        float    getMaxHeight()  const { return m_MaxHeight; }
        int      getResolution() const { return m_Resolution; }
        uint32_t getIndexCount() const { return m_IndexCount; }

        // Descriptor info for each texture layer and the blend map
        VkDescriptorImageInfo getBlendMapDescriptor() const;
        VkDescriptorImageInfo getLayerDescriptor(int index) const;

    private:
        void buildMesh();
        void uploadToGPU();
        void destroyGPUResources();

        float sampleHeight(int x, int z) const;
        glm::vec3 computeNormal(int x, int z) const;

        // Perlin noise helpers
        static float perlinNoise2D(float x, float y, uint32_t seed);
        static float fbmNoise(float x, float y, int octaves, float persistence, float lacunarity, uint32_t seed);
        static float grad2D(int hash, float x, float y);
        static float fade(float t);

        float m_Width      = 256.0f;
        float m_Depth      = 256.0f;
        float m_MaxHeight  = 50.0f;
        int   m_Resolution = 256;

        std::vector<float>         m_HeightData; // (resolution+1)*(resolution+1)
        std::vector<TerrainVertex> m_Vertices;
        std::vector<uint32_t>      m_Indices;
        uint32_t                   m_IndexCount = 0;

        // Vulkan geometry buffers
        VkBuffer      m_VertexBuffer     = VK_NULL_HANDLE;
        VmaAllocation m_VertexAllocation = VK_NULL_HANDLE;
        VkBuffer      m_IndexBuffer      = VK_NULL_HANDLE;
        VmaAllocation m_IndexAllocation  = VK_NULL_HANDLE;

        // Textures
        struct TextureLayer {
            Scope<Texture2D> Texture;
            float Scale = 10.0f;
        };
        std::array<TextureLayer, 4> m_TextureLayers;
        Scope<Texture2D>            m_BlendMap;
    };

    struct TerrainComponent {
        std::string HeightmapPath;
        float Width    = 256.0f;
        float Depth    = 256.0f;
        float MaxHeight = 50.0f;
        int   Resolution = 256;
        std::array<std::string, 4> TextureLayers;
        std::array<float, 4>       TextureScales = {10.0f, 10.0f, 10.0f, 10.0f};
        std::string BlendMapPath;
        Ref<Terrain> TerrainData;
    };

} // namespace GameEngine
