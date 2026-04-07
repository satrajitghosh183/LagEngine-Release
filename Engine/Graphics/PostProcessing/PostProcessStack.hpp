#pragma once

#include "../../Core/Base.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <cstdint>

namespace GameEngine {

    class PostProcessEffect {
    public:
        virtual ~PostProcessEffect() = default;
        virtual std::string getName() const = 0;
        virtual void init(int width, int height) = 0;
        virtual void resize(int width, int height) = 0;
        virtual void apply(uint32_t inputTexture, uint32_t outputFBO, int width, int height) = 0;
        virtual void renderUI() {} // ImGui settings

        bool Enabled = true;
        int Priority = 0; // Lower = earlier in chain
    };

    class PostProcessStack {
    public:
        PostProcessStack() = default;
        ~PostProcessStack();

        void init(int width, int height);
        void resize(int width, int height);

        // Add an effect to the stack
        template<typename T, typename... Args>
        T* addEffect(Args&&... args) {
            auto effect = CreateScope<T>(std::forward<Args>(args)...);
            effect->init(m_Width, m_Height);
            T* ptr = effect.get();
            m_Effects.push_back(std::move(effect));
            sortEffects();
            return ptr;
        }

        void removeEffect(PostProcessEffect* effect);

        // Apply all enabled effects in order
        // Returns the final texture ID
        uint32_t apply(uint32_t sceneTexture);

        // Access effects
        const std::vector<Scope<PostProcessEffect>>& getEffects() const { return m_Effects; }

    private:
        void createPingPongBuffers();
        void sortEffects();

        int m_Width = 0, m_Height = 0;
        uint32_t m_PingPongFBO[2] = {0, 0};
        uint32_t m_PingPongTextures[2] = {0, 0};
        uint32_t m_QuadVAO = 0;
        uint32_t m_QuadVBO = 0;
        std::vector<Scope<PostProcessEffect>> m_Effects;
    };

    // =====================================================================
    // Built-in effects
    // =====================================================================

    class BloomEffect : public PostProcessEffect {
    public:
        std::string getName() const override { return "Bloom"; }
        void init(int width, int height) override;
        void resize(int width, int height) override;
        void apply(uint32_t inputTexture, uint32_t outputFBO, int width, int height) override;
        void renderUI() override;

        float Threshold = 1.0f;
        float Intensity = 1.0f;
        int BlurPasses = 5;

    private:
        uint32_t m_BrightPassFBO = 0;
        uint32_t m_BrightPassTexture = 0;
        uint32_t m_BlurFBO[2] = {0, 0};
        uint32_t m_BlurTextures[2] = {0, 0};
        uint32_t m_BrightPassShader = 0;
        uint32_t m_BlurShader = 0;
        uint32_t m_CombineShader = 0;
        int m_Width = 0, m_Height = 0;
    };

    class ToneMappingEffect : public PostProcessEffect {
    public:
        enum class Algorithm { Reinhard, ACES, Filmic, Uncharted2 };

        std::string getName() const override { return "Tone Mapping"; }
        void init(int width, int height) override;
        void resize(int width, int height) override;
        void apply(uint32_t inputTexture, uint32_t outputFBO, int width, int height) override;
        void renderUI() override;

        Algorithm CurrentAlgorithm = Algorithm::ACES;
        float Exposure = 1.0f;
        float Gamma = 2.2f;

    private:
        uint32_t m_Shader = 0;
    };

    class FXAAEffect : public PostProcessEffect {
    public:
        std::string getName() const override { return "FXAA"; }
        void init(int width, int height) override;
        void resize(int width, int height) override;
        void apply(uint32_t inputTexture, uint32_t outputFBO, int width, int height) override;

        float SubpixelQuality = 0.75f;
        float EdgeThreshold = 0.166f;
        float EdgeThresholdMin = 0.0833f;

    private:
        uint32_t m_Shader = 0;
    };

    class VignetteEffect : public PostProcessEffect {
    public:
        std::string getName() const override { return "Vignette"; }
        void init(int width, int height) override;
        void resize(int width, int height) override;
        void apply(uint32_t inputTexture, uint32_t outputFBO, int width, int height) override;

        float Intensity = 0.5f;
        float Smoothness = 2.0f;
        glm::vec3 Color = glm::vec3(0.0f);

    private:
        uint32_t m_Shader = 0;
    };

    class ColorGradingEffect : public PostProcessEffect {
    public:
        std::string getName() const override { return "Color Grading"; }
        void init(int width, int height) override;
        void resize(int width, int height) override;
        void apply(uint32_t inputTexture, uint32_t outputFBO, int width, int height) override;
        void renderUI() override;

        float Saturation = 1.0f;
        float Contrast = 1.0f;
        float Brightness = 0.0f;
        glm::vec3 ColorFilter = glm::vec3(1.0f);
        float Temperature = 0.0f; // -1 cool to +1 warm

    private:
        uint32_t m_Shader = 0;
    };

}
