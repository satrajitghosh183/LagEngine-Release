#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>
#include <glad/glad.h>
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

    class Terrain {
    public:
        Terrain() = default;
        ~Terrain();

        bool loadFromHeightmap(const std::string& path, float width, float depth, float maxHeight);
        void generateFromNoise(float width, float depth, int resolution, uint32_t seed);

        float getHeightAt(float x, float z) const;
        glm::vec3 getNormalAt(float x, float z) const;

        void setTextureLayer(int index, const std::string& texturePath, float scale);
        void setBlendMap(const std::string& path);

        void render() const;

        float getWidth() const { return m_Width; }
        float getDepth() const { return m_Depth; }
        float getMaxHeight() const { return m_MaxHeight; }
        int getResolution() const { return m_Resolution; }

    private:
        void buildMesh();
        void uploadToGPU();
        float sampleHeight(int x, int z) const;
        glm::vec3 computeNormal(int x, int z) const;

        // Perlin noise helpers
        static float perlinNoise2D(float x, float y, uint32_t seed);
        static float fbmNoise(float x, float y, int octaves, float persistence, float lacunarity, uint32_t seed);
        static float grad2D(int hash, float x, float y);
        static float fade(float t);

        float m_Width = 256.0f;
        float m_Depth = 256.0f;
        float m_MaxHeight = 50.0f;
        int m_Resolution = 256;

        std::vector<float> m_HeightData; // (resolution+1) x (resolution+1)
        std::vector<TerrainVertex> m_Vertices;
        std::vector<uint32_t> m_Indices;

        GLuint m_VAO = 0, m_VBO = 0, m_IBO = 0;
        uint32_t m_IndexCount = 0;

        struct TextureLayer {
            GLuint TextureID = 0;
            float Scale = 10.0f;
            std::string Path;
        };
        std::array<TextureLayer, 4> m_TextureLayers;
        GLuint m_BlendMapTexture = 0;
    };

    struct TerrainComponent {
        std::string HeightmapPath;
        float Width = 256.0f;
        float Depth = 256.0f;
        float MaxHeight = 50.0f;
        int Resolution = 256;
        std::array<std::string, 4> TextureLayers;
        std::array<float, 4> TextureScales = {10.0f, 10.0f, 10.0f, 10.0f};
        std::string BlendMapPath;
        Ref<Terrain> TerrainData;
    };

} // namespace GameEngine
