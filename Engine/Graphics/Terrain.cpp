#include "Terrain.hpp"
#include <stb_image.h>
#include <cmath>
#include <algorithm>

namespace GameEngine {

    Terrain::~Terrain() {
        if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
        if (m_VBO) glDeleteBuffers(1, &m_VBO);
        if (m_IBO) glDeleteBuffers(1, &m_IBO);
        if (m_BlendMapTexture) glDeleteTextures(1, &m_BlendMapTexture);
        for (auto& layer : m_TextureLayers) {
            if (layer.TextureID) glDeleteTextures(1, &layer.TextureID);
        }
    }

    bool Terrain::loadFromHeightmap(const std::string& path, float width, float depth, float maxHeight) {
        m_Width = width;
        m_Depth = depth;
        m_MaxHeight = maxHeight;

        int w, h, channels;
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 1);
        if (!data) return false;

        m_Resolution = std::min(w, h) - 1;
        int gridSize = m_Resolution + 1;
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
        m_Width = width;
        m_Depth = depth;
        m_Resolution = resolution;

        int gridSize = m_Resolution + 1;
        m_HeightData.resize(gridSize * gridSize);

        for (int z = 0; z < gridSize; z++) {
            for (int x = 0; x < gridSize; x++) {
                float nx = static_cast<float>(x) / m_Resolution;
                float nz = static_cast<float>(z) / m_Resolution;
                m_HeightData[z * gridSize + x] = fbmNoise(nx * 4.0f, nz * 4.0f, 6, 0.5f, 2.0f, seed);
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
        // Convert world position to grid position
        float gx = (worldX / m_Width + 0.5f) * m_Resolution;
        float gz = (worldZ / m_Depth + 0.5f) * m_Resolution;

        int x0 = static_cast<int>(std::floor(gx));
        int z0 = static_cast<int>(std::floor(gz));
        float fx = gx - x0;
        float fz = gz - z0;

        // Bilinear interpolation
        float h00 = sampleHeight(x0, z0);
        float h10 = sampleHeight(x0 + 1, z0);
        float h01 = sampleHeight(x0, z0 + 1);
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
                v.Normal = computeNormal(x, z);
                v.TexCoord = glm::vec2(fx, fz);
                m_Vertices.push_back(v);
            }
        }

        for (int z = 0; z < m_Resolution; z++) {
            for (int x = 0; x < m_Resolution; x++) {
                uint32_t topLeft = z * gridSize + x;
                uint32_t topRight = topLeft + 1;
                uint32_t bottomLeft = (z + 1) * gridSize + x;
                uint32_t bottomRight = bottomLeft + 1;

                m_Indices.push_back(topLeft);
                m_Indices.push_back(bottomLeft);
                m_Indices.push_back(topRight);
                m_Indices.push_back(topRight);
                m_Indices.push_back(bottomLeft);
                m_Indices.push_back(bottomRight);
            }
        }
        m_IndexCount = static_cast<uint32_t>(m_Indices.size());
    }

    void Terrain::uploadToGPU() {
        if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
        if (m_VBO) glDeleteBuffers(1, &m_VBO);
        if (m_IBO) glDeleteBuffers(1, &m_IBO);

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_IBO);

        glBindVertexArray(m_VAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(TerrainVertex),
                     m_Vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(uint32_t),
                     m_Indices.data(), GL_STATIC_DRAW);

        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                              reinterpret_cast<void*>(offsetof(TerrainVertex, Position)));
        // Normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                              reinterpret_cast<void*>(offsetof(TerrainVertex, Normal)));
        // TexCoord
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                              reinterpret_cast<void*>(offsetof(TerrainVertex, TexCoord)));

        glBindVertexArray(0);
    }

    void Terrain::setTextureLayer(int index, const std::string& texturePath, float scale) {
        if (index < 0 || index >= 4) return;
        m_TextureLayers[index].Path = texturePath;
        m_TextureLayers[index].Scale = scale;

        int w, h, channels;
        unsigned char* data = stbi_load(texturePath.c_str(), &w, &h, &channels, 4);
        if (!data) return;

        if (m_TextureLayers[index].TextureID)
            glDeleteTextures(1, &m_TextureLayers[index].TextureID);

        glGenTextures(1, &m_TextureLayers[index].TextureID);
        glBindTexture(GL_TEXTURE_2D, m_TextureLayers[index].TextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }

    void Terrain::setBlendMap(const std::string& path) {
        int w, h, channels;
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
        if (!data) return;

        if (m_BlendMapTexture) glDeleteTextures(1, &m_BlendMapTexture);

        glGenTextures(1, &m_BlendMapTexture);
        glBindTexture(GL_TEXTURE_2D, m_BlendMapTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        stbi_image_free(data);
    }

    void Terrain::render() const {
        if (!m_VAO || m_IndexCount == 0) return;

        // Bind blend map
        if (m_BlendMapTexture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_BlendMapTexture);
        }

        // Bind texture layers
        for (int i = 0; i < 4; i++) {
            if (m_TextureLayers[i].TextureID) {
                glActiveTexture(GL_TEXTURE1 + i);
                glBindTexture(GL_TEXTURE_2D, m_TextureLayers[i].TextureID);
            }
        }

        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    // =====================================================================
    // Perlin Noise Implementation
    // =====================================================================

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
        // Hash function
        auto hash = [seed](int x, int y) -> int {
            uint32_t h = seed;
            h ^= static_cast<uint32_t>(x) * 374761393u;
            h ^= static_cast<uint32_t>(y) * 668265263u;
            h = (h ^ (h >> 13)) * 1274126177u;
            return static_cast<int>(h & 255);
        };

        int xi = static_cast<int>(std::floor(x));
        int yi = static_cast<int>(std::floor(y));
        float xf = x - xi;
        float yf = y - yi;

        float u = fade(xf);
        float v = fade(yf);

        int aa = hash(xi, yi);
        int ab = hash(xi, yi + 1);
        int ba = hash(xi + 1, yi);
        int bb = hash(xi + 1, yi + 1);

        float x1 = grad2D(aa, xf, yf) * (1.0f - u) + grad2D(ba, xf - 1.0f, yf) * u;
        float x2 = grad2D(ab, xf, yf - 1.0f) * (1.0f - u) + grad2D(bb, xf - 1.0f, yf - 1.0f) * u;

        return (x1 * (1.0f - v) + x2 * v) * 0.5f + 0.5f;
    }

    float Terrain::fbmNoise(float x, float y, int octaves, float persistence, float lacunarity, uint32_t seed) {
        float total = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; i++) {
            total += perlinNoise2D(x * frequency, y * frequency, seed + i) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return total / maxValue;
    }

} // namespace GameEngine
