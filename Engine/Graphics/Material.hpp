#pragma once

#include "../Core/Base.hpp"
#include "Texture2D.hpp"
#include "Vulkan/VulkanDescriptors.hpp"
#include "UniformBuffer.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <memory>

namespace GameEngine {

    class Shader; // forward declaration for the legacy ctor shim

    /**
     * @brief PBR Material (Vulkan)
     *
     * Stores all PBR parameters and texture maps. Instead of calling
     * SetUniform*() on a shader, the material exposes its data through
     * two mechanisms that match the Vulkan pipeline model:
     *
     *   1. GetMaterialBufferData() — fills a MaterialUBO struct that the
     *      caller uploads to a UniformBuffer. The layout matches the
     *      std140 block in the PBR fragment shader.
     *
     *   2. WriteDescriptors() — writes all texture descriptors (albedo, normal,
     *      metallic, roughness, AO, plus any custom textures) into the
     *      provided VulkanDescriptorWriter. Slots 0-4 are reserved for PBR
     *      maps; custom textures start at slot 5.
     *
     * Usage:
     *   auto mat = CreateRef<Material>();
     *   mat->SetAlbedo({1, 0, 0});
     *   mat->SetAlbedoMap(albedoTex);
     *   // per-frame:
     *   auto data = mat->GetMaterialBufferData();
     *   materialUBO.SetData(&data, sizeof(data));
     *   mat->WriteDescriptors(writer, defaultSampler);
     */
    class Material {
    public:
        // ---- std140 UBO layout (matches shader MaterialData block) ----
        struct MaterialUBO {
            alignas(16) glm::vec4 Albedo;        // xyz = albedo, w = 1
            alignas(4)  float     Metallic;
            alignas(4)  float     Roughness;
            alignas(4)  float     AO;
            alignas(4)  int       HasAlbedoMap;
            alignas(4)  int       HasNormalMap;
            alignas(4)  int       HasMetallicMap;
            alignas(4)  int       HasRoughnessMap;
            alignas(4)  int       HasAOMap;
        };

        // Texture binding slots (must match descriptor set layout)
        static constexpr uint32_t kSlotAlbedo    = 0;
        static constexpr uint32_t kSlotNormal     = 1;
        static constexpr uint32_t kSlotMetallic   = 2;
        static constexpr uint32_t kSlotRoughness  = 3;
        static constexpr uint32_t kSlotAO         = 4;
        static constexpr uint32_t kSlotCustomBase = 5;

        Material();
        ~Material() = default;

        /**
         * @brief Legacy constructor for code that pre-dates the Vulkan
         * migration. Vulkan materials don't bind a Shader directly — the
         * shader/pipeline is selected by the renderer based on material
         * properties. The shader argument is ignored.
         */
        explicit Material(const Ref<Shader>& /*shader*/) : Material() {}

        // ---- Name ----
        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }

        // ---- PBR scalar parameters ----
        void      SetAlbedo(const glm::vec3& color) { m_Albedo    = color; }
        glm::vec3 GetAlbedo()    const { return m_Albedo;    }

        void  SetMetallic(float v)  { m_Metallic  = v; }
        float GetMetallic()  const  { return m_Metallic;  }

        void  SetRoughness(float v) { m_Roughness = v; }
        float GetRoughness() const  { return m_Roughness; }

        void  SetAO(float v)        { m_AO        = v; }
        float GetAO()        const  { return m_AO;        }

        // ---- PBR texture maps ----
        void         SetAlbedoMap(const Ref<Texture2D>& t)    { m_AlbedoMap    = t; }
        Ref<Texture2D> GetAlbedoMap()    const { return m_AlbedoMap;    }

        void         SetNormalMap(const Ref<Texture2D>& t)    { m_NormalMap    = t; }
        Ref<Texture2D> GetNormalMap()    const { return m_NormalMap;    }

        void         SetMetallicMap(const Ref<Texture2D>& t)  { m_MetallicMap  = t; }
        Ref<Texture2D> GetMetallicMap()  const { return m_MetallicMap;  }

        void         SetRoughnessMap(const Ref<Texture2D>& t) { m_RoughnessMap = t; }
        Ref<Texture2D> GetRoughnessMap() const { return m_RoughnessMap; }

        void         SetAOMap(const Ref<Texture2D>& t)        { m_AOMap        = t; }
        Ref<Texture2D> GetAOMap()        const { return m_AOMap;        }

        // ---- Custom / extra textures ----
        void SetTexture(const std::string& name, const Ref<Texture2D>& texture);

        // ---- Generic scalar/vector parameters (stored for serialization / editor) ----
        void SetInt(const std::string& name, int value);
        void SetFloat(const std::string& name, float value);
        void SetVec2(const std::string& name, const glm::vec2& value);
        void SetVec3(const std::string& name, const glm::vec3& value);
        void SetVec4(const std::string& name, const glm::vec4& value);
        void SetMat4(const std::string& name, const glm::mat4& value);

        int       GetInt(const std::string& name)   const;
        float     GetFloat(const std::string& name) const;
        glm::vec3 GetVec3(const std::string& name)  const;
        glm::vec4 GetVec4(const std::string& name)  const;

        // ---- Vulkan integration ----

        /**
         * @brief Build the UBO payload for this material.
         *        Upload the returned struct to a UniformBuffer<MaterialUBO>.
         */
        MaterialUBO GetMaterialBufferData() const;

        /**
         * @brief Write all texture descriptors into the writer.
         * @param writer A VulkanDescriptorWriter already bound to the correct layout.
         * @param fallbackSampler Sampler to use when a texture has VK_NULL_HANDLE sampler.
         * @param fallbackView    1x1 white image view used when a PBR map is absent.
         */
        void WriteDescriptors(VulkanDescriptorWriter& writer,
                              VkSampler fallbackSampler,
                              VkImageView fallbackView) const;

    private:
        std::string m_Name = "Material";

        // PBR scalars
        glm::vec3 m_Albedo    = glm::vec3(1.0f);
        float     m_Metallic  = 0.0f;
        float     m_Roughness = 0.5f;
        float     m_AO        = 1.0f;

        // PBR maps
        Ref<Texture2D> m_AlbedoMap;
        Ref<Texture2D> m_NormalMap;
        Ref<Texture2D> m_MetallicMap;
        Ref<Texture2D> m_RoughnessMap;
        Ref<Texture2D> m_AOMap;

        // Custom named textures (extra slots beyond kSlotAO)
        std::vector<std::pair<std::string, Ref<Texture2D>>> m_CustomTextures;

        // Generic params (for editor / serialization only; not pushed as uniforms)
        std::unordered_map<std::string, int>       m_IntParams;
        std::unordered_map<std::string, float>     m_FloatParams;
        std::unordered_map<std::string, glm::vec2> m_Vec2Params;
        std::unordered_map<std::string, glm::vec3> m_Vec3Params;
        std::unordered_map<std::string, glm::vec4> m_Vec4Params;
        std::unordered_map<std::string, glm::mat4> m_Mat4Params;
    };

} // namespace GameEngine
