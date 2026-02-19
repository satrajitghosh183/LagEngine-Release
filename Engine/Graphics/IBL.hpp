#pragma once

#include "../Core/Base.hpp"
#include "Shader.hpp"
#include "Texture2D.hpp"
#include <glm/glm.hpp>
#include <string>

namespace GameEngine {

    /**
     * @brief Image-Based Lighting utility
     *
     * Generates the textures needed for PBR IBL rendering:
     * - Environment cubemap (from equirectangular HDR or fallback procedural sky)
     * - Irradiance convolution map (diffuse IBL)
     * - Prefiltered specular map (specular IBL with roughness mip levels)
     * - BRDF integration LUT (2D lookup for split-sum approximation)
     */
    class IBL {
    public:
        /**
         * @brief Initialize IBL with a procedural sky (no HDR file needed)
         */
        static void Init();

        /**
         * @brief Load an HDR equirectangular environment map
         */
        static void LoadEnvironment(const std::string& hdrPath);

        /**
         * @brief Shutdown and release resources
         */
        static void Shutdown();

        /**
         * @brief Bind IBL textures for rendering
         * @param irradianceSlot Texture slot for irradiance map
         * @param prefilterSlot Texture slot for prefiltered specular
         * @param brdfLUTSlot Texture slot for BRDF LUT
         */
        static void Bind(uint32_t irradianceSlot = 8,
                         uint32_t prefilterSlot = 9,
                         uint32_t brdfLUTSlot = 10);

        /**
         * @brief Apply IBL uniforms to a shader
         */
        static void ApplyToShader(const Ref<Shader>& shader,
                                  uint32_t irradianceSlot = 8,
                                  uint32_t prefilterSlot = 9,
                                  uint32_t brdfLUTSlot = 10);

        static uint32_t GetIrradianceMap() { return s_IrradianceMap; }
        static uint32_t GetPrefilterMap() { return s_PrefilterMap; }
        static uint32_t GetBRDFLUT() { return s_BRDFLUT; }
        static bool IsReady() { return s_Initialized; }

    private:
        static void GenerateBRDFLUT();
        static void GenerateProceduralCubemap();
        static void GenerateIrradianceMap();
        static void GeneratePrefilterMap();

        static uint32_t s_EnvCubemap;
        static uint32_t s_IrradianceMap;
        static uint32_t s_PrefilterMap;
        static uint32_t s_BRDFLUT;
        static bool s_Initialized;

        static Ref<Shader> s_EquirectToCubeShader;
        static Ref<Shader> s_IrradianceShader;
        static Ref<Shader> s_PrefilterShader;
        static Ref<Shader> s_BRDFShader;
    };

}
