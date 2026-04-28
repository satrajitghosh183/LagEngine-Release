#include "Terrain.hpp"
#include "../Core/Logger.hpp"
#include <stb_image.h>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace GameEngine {

    // =========================================================================
    // Helpers — allocate/destroy GPU buffers via VMA
    // =========================================================================

    static void CreateGPUBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                 VkBuffer& outBuffer, VmaAllocation& outAlloc,
                                 const void* initialData = nullptr) {
        VmaAllocator allocator = VulkanDevice::Get().GetAllocator();

        // Staging buffer (host-visible)
        VkBuffer        stagingBuf;
        VmaAllocation   stagingAlloc;
        VmaAllocationInfo stagingInfo;

        VkBufferCreateInfo stagingCI{};
        stagingCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingCI.size  = size;
        stagingCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingAllocCI{};
        stagingAllocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                               VMA_ALLOCATION_CREATE_MAPPED_BIT;
        stagingAllocCI.usage = VMA_MEMORY_USAGE_AUTO;

        vmaCreateBuffer(allocator, &stagingCI, &stagingAllocCI,
                        &stagingBuf, &stagingAlloc, &stagingInfo);

        if (initialData) {
            memcpy(stagingInfo.pMappedData, initialData, static_cast<size_t>(size));
        }

        // Device-local destination buffer
        VkBufferCreateInfo deviceCI{};
        deviceCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        deviceCI.size  = size;
        deviceCI.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo deviceAllocCI{};
        deviceAllocCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        vmaCreateBuffer(allocator, &deviceCI, &deviceAllocCI,
                        &outBuffer, &outAlloc, nullptr);

        // Copy via a single-time command buffer
        VkCommandBuffer cmd = VulkanDevice::Get().BeginSingleTimeCommands();

        VkBufferCopy region{};
        region.size = size;
        vkCmdCopyBuffer(cmd, stagingBuf, outBuffer, 1, &region);

        VulkanDevice::Get().EndSingleTimeCommands(cmd);

        // Free staging resources
        vmaDestroyBuffer(allocator, stagingBuf, stagingAlloc);
    }

    // =========================================================================
    // Terrain
    // =========================================================================

    Terrain::~Terrain() {
        destroyGPUResources();
    }

    void Terrain::destroyGPUResources() {
        VmaAllocator allocator = VulkanDevice::Get().GetAllocator();
        if (m_VertexBuffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, m_VertexBuffer, m_VertexAllocation);
            m_VertexBuffer     = VK_NULL_HANDLE;
            m_VertexAllocation = VK_NULL_HANDLE;
        }
        if (m_IndexBuffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, m_IndexBuffer, m_IndexAllocation);
            m_IndexBuffer     = VK_NULL_HANDLE;
            m_IndexAllocation = VK_NULL_HANDLE;
        }
    }

    bool Terrain::loadFromHeightmap(const std::string& path, float width, float depth, float maxHeight) {
        m_Width     = width;
        m_Depth     = depth;
        m_MaxHeight = maxHeight;

        int w, h, channels;
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 1);
        if (!data) {
            GE_CORE_ERROR("Terrain: failed to load heightmap '{}'", path);
            return false;
        }

        m_Resolution    = std::min(w, h) - 1;
        int gridSize    = m_Resolution + 1;
        m_HeightData.resize(gridSize * gridSize);

        for (int z = 0; z < gridSize; z++) {
            for (int x = 0; x < gridSize; x++) {
                int imgX = static_cast<int>(x * (w - 1.0f) / m_Resolution);
                int imgZ = static_cast<int>(z * (h - 1.0f) / m_Resolution);
                imgX = std::clamp(imgX, 0, w - 1);
                imgZ = std::clamp(imgZ, 0, h - 1);
                m_HeightData[z * gridSize + x] = data[imgZ * w + imgX] / 255.0f;
            }
        }

        stbi_image_free(data);
        buildMesh();
        uploadToGPU();
        return true;
    }

    void Terrain::generateFromNoise(float width, float depth, int resolution, uint32_t seed) {
        m_Width      = width;
        m_Depth      = depth;
        m_Resolution = resolution;

        int gridSize = m_Resolution + 1;
        m_HeightData.resize(gridSize * gridSize);

        for (int z = 0; z < gridSize; z++) {
            for (int x = 0; x < gridSize; x++) {
                float nx = static_cast<float>(x) / m_Resolution;
                float nz = static_cast<float>(z) / m_Resolution;
                m_HeightData[z * gridSize + x] =
                    fbmNoise(nx * 4.0f, nz * 4.0f, 6, 0.5f, 2.0f, seed);
            }
        }

        buildMesh();
        uploadToGPU();
    }

    float Terrain::sampleHeight(int x, int z) const {
        int gridSize = m_Resolution + 1;
        x = std::clamp(x, 0, m_Resolution);
        z = std::clamp(z, 0, m_Resolution);
        return m_HeightData[z * gridSize + x];
    }

    float Terrain::getHeightAt(float worldX, float worldZ) const {
        float gx = (worldX / m_Width + 0.5f) * m_Resolution;
        float gz = (worldZ / m_Depth + 0.5f) * m_Resolution;

        int x0  = static_cast<int>(std::floor(gx));
        int z0  = static_cast<int>(std::floor(gz));
        float fx = gx - x0;
        float fz = gz - z0;

        float h00 = sampleHeight(x0,     z0);
        float h10 = sampleHeight(x0 + 1, z0);
        float h01 = sampleHeight(x0,     z0 + 1);
        float h11 = sampleHeight(x0 + 1, z0 + 1);

        float h0 = h00 * (1.0f - fx) + h10 * fx;
        float h1 = h01 * (1.0f - fx) + h11 * fx;
        return (h0 * (1.0f - fz) + h1 * fz) * m_MaxHeight;
    }

    glm::vec3 Terrain::getNormalAt(float worldX, float worldZ) const {
        float gx = (worldX / m_Width + 0.5f) * m_Resolution;
        float gz = (worldZ / m_Depth + 0.5f) * m_Resolution;
        int ix = std::clamp(static_cast<int>(gx), 0, m_Resolution);
        int iz = std::clamp(static_cast<int>(gz), 0, m_Resolution);
        return computeNormal(ix, iz);
    }

    glm::vec3 Terrain::computeNormal(int x, int z) const {
        float hL = sampleHeight(x - 1, z) * m_MaxHeight;
        float hR = sampleHeight(x + 1, z) * m_MaxHeight;
        float hD = sampleHeight(x, z - 1) * m_MaxHeight;
        float hU = sampleHeight(x, z + 1) * m_MaxHeight;

        float stepX = m_Width / m_Resolution;
        float stepZ = m_Depth / m_Resolution;

        glm::vec3 normal(hL - hR, 2.0f * stepX, hD - hU);
        return glm::normalize(normal);
    }

    void Terrain::buildMesh() {
        int gridSize = m_Resolution + 1;
        m_Vertices.clear();
        m_Indices.clear();
        m_Vertices.reserve(gridSize * gridSize);
        m_Indices.reserve(m_Resolution * m_Resolution * 6);

        float halfW = m_Width * 0.5f;
        float halfD = m_Depth * 0.5f;

        for (int z = 0; z < gridSize; z++) {
            for (int x = 0; x < gridSize; x++) {
                float fx = static_cast<float>(x) / m_Resolution;
                float fz = static_cast<float>(z) / m_Resolution;

                TerrainVertex v;
                v.Position = glm::vec3(fx * m_Width - halfW,
                                       m_HeightData[z * gridSize + x] * m_MaxHeight,
                                       fz * m_Depth - halfD);
                v.Normal   = computeNormal(x, z);
                v.TexCoord = glm::vec2(fx, fz);
                m_Vertices.push_back(v);
            }
        }

        for (int z = 0; z < m_Resolution; z++) {
            for (int x = 0; x < m_Resolution; x++) {
                uint32_t tl = z * gridSize + x;
                uint32_t tr = tl + 1;
                uint32_t bl = (z + 1) * gridSize + x;
                uint32_t br = bl + 1;

                m_Indices.push_back(tl); m_Indices.push_back(bl); m_Indices.push_back(tr);
                m_Indices.push_back(tr); m_Indices.push_back(bl); m_Indices.push_back(br);
            }
        }
        m_IndexCount = static_cast<uint32_t>(m_Indices.size());
    }

    void Terrain::uploadToGPU() {
        destroyGPUResources();

        VkDeviceSize vbSize = m_Vertices.size() * sizeof(TerrainVertex);
        VkDeviceSize ibSize = m_Indices.size()  * sizeof(uint32_t);

        CreateGPUBuffer(vbSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        m_VertexBuffer, m_VertexAllocation, m_Vertices.data());

        CreateGPUBuffer(ibSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                        m_IndexBuffer, m_IndexAllocation, m_Indices.data());

        GE_CORE_INFO("Terrain: uploaded {} vertices, {} indices to GPU",
                     m_Vertices.size(), m_Indices.size());
    }

    void Terrain::setTextureLayer(int index, const std::string& texturePath, float scale) {
        if (index < 0 || index >= 4) return;
        m_TextureLayers[index].Scale   = scale;
        m_TextureLayers[index].Texture = CreateScope<Texture2D>(texturePath);
    }

    void Terrain::setBlendMap(const std::string& path) {
        m_BlendMap = CreateScope<Texture2D>(path);
    }

    void Terrain::render(VkCommandBuffer cmd) const {
        if (m_VertexBuffer == VK_NULL_HANDLE || m_IndexCount == 0) return;

        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_VertexBuffer, offsets);
        vkCmdBindIndexBuffer(cmd, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, m_IndexCount, 1, 0, 0, 0);
    }

    VkDescriptorImageInfo Terrain::getBlendMapDescriptor() const {
        if (!m_BlendMap) return {};
        return m_BlendMap->GetDescriptorInfo();
    }

    VkDescriptorImageInfo Terrain::getLayerDescriptor(int index) const {
        if (index < 0 || index >= 4 || !m_TextureLayers[index].Texture) return {};
        return m_TextureLayers[index].Texture->GetDescriptorInfo();
    }

    // =========================================================================
    // Perlin Noise Implementation
    // =========================================================================

    float Terrain::fade(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    float Terrain::grad2D(int hash, float x, float y) {
        int h = hash & 7;
        float u = h < 4 ? x : y;
        float v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }

    float Terrain::perlinNoise2D(float x, float y, uint32_t seed) {
        auto hashFn = [seed](int px, int py) -> int {
            uint32_t h = seed;
            h ^= static_cast<uint32_t>(px) * 374761393u;
            h ^= static_cast<uint32_t>(py) * 668265263u;
            h = (h ^ (h >> 13)) * 1274126177u;
            return static_cast<int>(h & 255);
        };

        int xi  = static_cast<int>(std::floor(x));
        int yi  = static_cast<int>(std::floor(y));
        float xf = x - xi;
        float yf = y - yi;

        float u = fade(xf);
        float v = fade(yf);

        int aa = hashFn(xi,     yi);
        int ab = hashFn(xi,     yi + 1);
        int ba = hashFn(xi + 1, yi);
        int bb = hashFn(xi + 1, yi + 1);

        float x1 = grad2D(aa, xf,       yf)       * (1.0f - u) + grad2D(ba, xf - 1.0f, yf)       * u;
        float x2 = grad2D(ab, xf,       yf - 1.0f) * (1.0f - u) + grad2D(bb, xf - 1.0f, yf - 1.0f) * u;

        return (x1 * (1.0f - v) + x2 * v) * 0.5f + 0.5f;
    }

    float Terrain::fbmNoise(float x, float y, int octaves,
                              float persistence, float lacunarity, uint32_t seed) {
        float total    = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue  = 0.0f;

        for (int i = 0; i < octaves; i++) {
            total    += perlinNoise2D(x * frequency, y * frequency, seed + i) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return total / maxValue;
    }

} // namespace GameEngine
