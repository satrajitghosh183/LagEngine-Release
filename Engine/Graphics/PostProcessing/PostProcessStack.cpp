#include "PostProcessStack.hpp"
#include <glad/glad.h>
#include <algorithm>
#include <string>

namespace GameEngine {

    // =====================================================================
    // Shader compilation helper (local to this TU)
    // =====================================================================

    static uint32_t compileShaderProgram(const char* vertSrc, const char* fragSrc) {
        uint32_t vert = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert, 1, &vertSrc, nullptr);
        glCompileShader(vert);

        uint32_t frag = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 1, &fragSrc, nullptr);
        glCompileShader(frag);

        uint32_t prog = glCreateProgram();
        glAttachShader(prog, vert);
        glAttachShader(prog, frag);
        glLinkProgram(prog);

        glDeleteShader(vert);
        glDeleteShader(frag);
        return prog;
    }

    static const char* s_FullscreenQuadVert = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            TexCoord = aTexCoord;
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";

    // =====================================================================
    // PostProcessStack
    // =====================================================================

    PostProcessStack::~PostProcessStack() {
        if (m_PingPongFBO[0]) glDeleteFramebuffers(2, m_PingPongFBO);
        if (m_PingPongTextures[0]) glDeleteTextures(2, m_PingPongTextures);
        if (m_QuadVAO) glDeleteVertexArrays(1, &m_QuadVAO);
        if (m_QuadVBO) glDeleteBuffers(1, &m_QuadVBO);
    }

    void PostProcessStack::init(int width, int height) {
        m_Width = width;
        m_Height = height;

        // Create fullscreen quad
        float quadVerts[] = {
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 1.0f
        };

        glGenVertexArrays(1, &m_QuadVAO);
        glGenBuffers(1, &m_QuadVBO);
        glBindVertexArray(m_QuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);

        createPingPongBuffers();
    }

    void PostProcessStack::resize(int width, int height) {
        m_Width = width;
        m_Height = height;
        createPingPongBuffers();
        for (auto& effect : m_Effects) {
            effect->resize(width, height);
        }
    }

    void PostProcessStack::createPingPongBuffers() {
        if (m_PingPongFBO[0]) glDeleteFramebuffers(2, m_PingPongFBO);
        if (m_PingPongTextures[0]) glDeleteTextures(2, m_PingPongTextures);

        glGenFramebuffers(2, m_PingPongFBO);
        glGenTextures(2, m_PingPongTextures);

        for (int i = 0; i < 2; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, m_PingPongFBO[i]);
            glBindTexture(GL_TEXTURE_2D, m_PingPongTextures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_PingPongTextures[i], 0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void PostProcessStack::sortEffects() {
        std::sort(m_Effects.begin(), m_Effects.end(),
            [](const Scope<PostProcessEffect>& a, const Scope<PostProcessEffect>& b) {
                return a->Priority < b->Priority;
            });
    }

    void PostProcessStack::removeEffect(PostProcessEffect* effect) {
        m_Effects.erase(
            std::remove_if(m_Effects.begin(), m_Effects.end(),
                [effect](const Scope<PostProcessEffect>& e) { return e.get() == effect; }),
            m_Effects.end());
    }

    uint32_t PostProcessStack::apply(uint32_t sceneTexture) {
        uint32_t currentInput = sceneTexture;
        int pingPongIndex = 0;

        glBindVertexArray(m_QuadVAO);

        for (auto& effect : m_Effects) {
            if (!effect->Enabled) continue;

            uint32_t outputFBO = m_PingPongFBO[pingPongIndex];
            effect->apply(currentInput, outputFBO, m_Width, m_Height);
            currentInput = m_PingPongTextures[pingPongIndex];
            pingPongIndex = 1 - pingPongIndex;
        }

        glBindVertexArray(0);
        return currentInput;
    }

    // =====================================================================
    // ToneMappingEffect
    // =====================================================================

    static const char* s_ToneMappingFrag = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D uTexture;
        uniform float uExposure;
        uniform float uGamma;
        uniform int uAlgorithm;

        vec3 reinhardToneMap(vec3 color) {
            return color / (color + vec3(1.0));
        }

        vec3 acesToneMap(vec3 color) {
            float a = 2.51;
            float b = 0.03;
            float c = 2.43;
            float d = 0.59;
            float e = 0.14;
            return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
        }

        vec3 filmicToneMap(vec3 color) {
            vec3 x = max(vec3(0.0), color - 0.004);
            return (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
        }

        void main() {
            vec3 color = texture(uTexture, TexCoord).rgb * uExposure;

            if (uAlgorithm == 0) color = reinhardToneMap(color);
            else if (uAlgorithm == 1) color = acesToneMap(color);
            else color = filmicToneMap(color);

            color = pow(color, vec3(1.0 / uGamma));
            FragColor = vec4(color, 1.0);
        }
    )";

    void ToneMappingEffect::init(int /*width*/, int /*height*/) {
        Priority = 90;
        m_Shader = compileShaderProgram(s_FullscreenQuadVert, s_ToneMappingFrag);
    }

    void ToneMappingEffect::resize(int /*width*/, int /*height*/) {}

    void ToneMappingEffect::apply(uint32_t inputTexture, uint32_t outputFBO, int width, int height) {
        glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(m_Shader);
        glUniform1f(glGetUniformLocation(m_Shader, "uExposure"), Exposure);
        glUniform1f(glGetUniformLocation(m_Shader, "uGamma"), Gamma);
        glUniform1i(glGetUniformLocation(m_Shader, "uAlgorithm"), static_cast<int>(CurrentAlgorithm));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);
        glUniform1i(glGetUniformLocation(m_Shader, "uTexture"), 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void ToneMappingEffect::renderUI() {
        // Can be used by editor panel to show ImGui controls
    }

    // =====================================================================
    // FXAAEffect
    // =====================================================================

    static const char* s_FXAAFrag = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D uTexture;
        uniform vec2 uTexelSize;
        uniform float uSubpixelQuality;
        uniform float uEdgeThreshold;
        uniform float uEdgeThresholdMin;

        float luminance(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

        void main() {
            vec3 rgbM = texture(uTexture, TexCoord).rgb;
            float lumaM = luminance(rgbM);
            float lumaN = luminance(textureOffset(uTexture, TexCoord, ivec2(0, 1)).rgb);
            float lumaS = luminance(textureOffset(uTexture, TexCoord, ivec2(0, -1)).rgb);
            float lumaE = luminance(textureOffset(uTexture, TexCoord, ivec2(1, 0)).rgb);
            float lumaW = luminance(textureOffset(uTexture, TexCoord, ivec2(-1, 0)).rgb);

            float rangeMax = max(max(lumaN, lumaS), max(lumaE, lumaW));
            float rangeMin = min(min(lumaN, lumaS), min(lumaE, lumaW));
            float range = rangeMax - rangeMin;

            if (range < max(uEdgeThresholdMin, rangeMax * uEdgeThreshold)) {
                FragColor = vec4(rgbM, 1.0);
                return;
            }

            float lumaNW = luminance(textureOffset(uTexture, TexCoord, ivec2(-1, 1)).rgb);
            float lumaNE = luminance(textureOffset(uTexture, TexCoord, ivec2(1, 1)).rgb);
            float lumaSW = luminance(textureOffset(uTexture, TexCoord, ivec2(-1, -1)).rgb);
            float lumaSE = luminance(textureOffset(uTexture, TexCoord, ivec2(1, -1)).rgb);

            float lumaAvg = (lumaN + lumaS + lumaE + lumaW) * 0.25;
            float subpixel = clamp(abs(lumaAvg - lumaM) / range, 0.0, 1.0);
            subpixel = smoothstep(0.0, 1.0, subpixel) * subpixel * uSubpixelQuality;

            bool isHorizontal = abs(lumaN + lumaS - 2.0 * lumaM) * 2.0 +
                                abs(lumaE + lumaW - 2.0 * lumaM) >=
                                abs(lumaE + lumaW - 2.0 * lumaM) * 2.0 +
                                abs(lumaN + lumaS - 2.0 * lumaM);

            float stepLength = isHorizontal ? uTexelSize.y : uTexelSize.x;
            float gradientPos = isHorizontal ? abs(lumaN - lumaM) : abs(lumaE - lumaM);
            float gradientNeg = isHorizontal ? abs(lumaS - lumaM) : abs(lumaW - lumaM);

            if (gradientNeg > gradientPos) stepLength = -stepLength;

            vec2 offset = isHorizontal ? vec2(0.0, stepLength * 0.5) : vec2(stepLength * 0.5, 0.0);
            vec3 result = texture(uTexture, TexCoord + offset).rgb;
            FragColor = vec4(mix(result, rgbM, subpixel), 1.0);
        }
    )";

    void FXAAEffect::init(int /*width*/, int /*height*/) {
        Priority = 100;
        m_Shader = compileShaderProgram(s_FullscreenQuadVert, s_FXAAFrag);
    }

    void FXAAEffect::resize(int /*width*/, int /*height*/) {}

    void FXAAEffect::apply(uint32_t inputTexture, uint32_t outputFBO, int width, int height) {
        glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(m_Shader);
        glUniform2f(glGetUniformLocation(m_Shader, "uTexelSize"), 1.0f / width, 1.0f / height);
        glUniform1f(glGetUniformLocation(m_Shader, "uSubpixelQuality"), SubpixelQuality);
        glUniform1f(glGetUniformLocation(m_Shader, "uEdgeThreshold"), EdgeThreshold);
        glUniform1f(glGetUniformLocation(m_Shader, "uEdgeThresholdMin"), EdgeThresholdMin);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);
        glUniform1i(glGetUniformLocation(m_Shader, "uTexture"), 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // =====================================================================
    // VignetteEffect
    // =====================================================================

    static const char* s_VignetteFrag = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D uTexture;
        uniform float uIntensity;
        uniform float uSmoothness;
        uniform vec3 uColor;

        void main() {
            vec4 color = texture(uTexture, TexCoord);
            vec2 uv = TexCoord * 2.0 - 1.0;
            float d = length(uv);
            float vignette = smoothstep(1.0, 1.0 - uSmoothness, d * uIntensity);
            color.rgb = mix(uColor, color.rgb, vignette);
            FragColor = color;
        }
    )";

    void VignetteEffect::init(int /*width*/, int /*height*/) {
        Priority = 95;
        m_Shader = compileShaderProgram(s_FullscreenQuadVert, s_VignetteFrag);
    }

    void VignetteEffect::resize(int /*width*/, int /*height*/) {}

    void VignetteEffect::apply(uint32_t inputTexture, uint32_t outputFBO, int width, int height) {
        glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(m_Shader);
        glUniform1f(glGetUniformLocation(m_Shader, "uIntensity"), Intensity);
        glUniform1f(glGetUniformLocation(m_Shader, "uSmoothness"), Smoothness);
        glUniform3f(glGetUniformLocation(m_Shader, "uColor"), Color.r, Color.g, Color.b);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);
        glUniform1i(glGetUniformLocation(m_Shader, "uTexture"), 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // =====================================================================
    // ColorGradingEffect
    // =====================================================================

    static const char* s_ColorGradingFrag = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D uTexture;
        uniform float uSaturation;
        uniform float uContrast;
        uniform float uBrightness;
        uniform vec3 uColorFilter;
        uniform float uTemperature;

        void main() {
            vec3 color = texture(uTexture, TexCoord).rgb;

            // Brightness
            color += uBrightness;

            // Contrast
            color = (color - 0.5) * uContrast + 0.5;

            // Saturation
            float luma = dot(color, vec3(0.299, 0.587, 0.114));
            color = mix(vec3(luma), color, uSaturation);

            // Color filter
            color *= uColorFilter;

            // Temperature
            if (uTemperature != 0.0) {
                color.r += uTemperature * 0.1;
                color.b -= uTemperature * 0.1;
            }

            FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
        }
    )";

    void ColorGradingEffect::init(int /*width*/, int /*height*/) {
        Priority = 85;
        m_Shader = compileShaderProgram(s_FullscreenQuadVert, s_ColorGradingFrag);
    }

    void ColorGradingEffect::resize(int /*width*/, int /*height*/) {}

    void ColorGradingEffect::apply(uint32_t inputTexture, uint32_t outputFBO, int width, int height) {
        glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(m_Shader);
        glUniform1f(glGetUniformLocation(m_Shader, "uSaturation"), Saturation);
        glUniform1f(glGetUniformLocation(m_Shader, "uContrast"), Contrast);
        glUniform1f(glGetUniformLocation(m_Shader, "uBrightness"), Brightness);
        glUniform3f(glGetUniformLocation(m_Shader, "uColorFilter"), ColorFilter.r, ColorFilter.g, ColorFilter.b);
        glUniform1f(glGetUniformLocation(m_Shader, "uTemperature"), Temperature);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);
        glUniform1i(glGetUniformLocation(m_Shader, "uTexture"), 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void ColorGradingEffect::renderUI() {}

    // =====================================================================
    // BloomEffect
    // =====================================================================

    static const char* s_BrightPassFrag = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D uTexture;
        uniform float uThreshold;

        void main() {
            vec3 color = texture(uTexture, TexCoord).rgb;
            float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
            if (brightness > uThreshold)
                FragColor = vec4(color, 1.0);
            else
                FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
    )";

    static const char* s_GaussianBlurFrag = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D uTexture;
        uniform bool uHorizontal;
        uniform vec2 uTexelSize;

        const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

        void main() {
            vec3 result = texture(uTexture, TexCoord).rgb * weights[0];
            vec2 offset = uHorizontal ? vec2(uTexelSize.x, 0.0) : vec2(0.0, uTexelSize.y);

            for (int i = 1; i < 5; i++) {
                result += texture(uTexture, TexCoord + offset * float(i)).rgb * weights[i];
                result += texture(uTexture, TexCoord - offset * float(i)).rgb * weights[i];
            }
            FragColor = vec4(result, 1.0);
        }
    )";

    static const char* s_BloomCombineFrag = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D uScene;
        uniform sampler2D uBloom;
        uniform float uIntensity;

        void main() {
            vec3 scene = texture(uScene, TexCoord).rgb;
            vec3 bloom = texture(uBloom, TexCoord).rgb;
            FragColor = vec4(scene + bloom * uIntensity, 1.0);
        }
    )";

    void BloomEffect::init(int width, int height) {
        Priority = 10;
        m_Width = width;
        m_Height = height;

        m_BrightPassShader = compileShaderProgram(s_FullscreenQuadVert, s_BrightPassFrag);
        m_BlurShader = compileShaderProgram(s_FullscreenQuadVert, s_GaussianBlurFrag);
        m_CombineShader = compileShaderProgram(s_FullscreenQuadVert, s_BloomCombineFrag);

        resize(width, height);
    }

    void BloomEffect::resize(int width, int height) {
        m_Width = width;
        m_Height = height;
        int halfW = width / 2, halfH = height / 2;

        auto createFBOTexture = [](uint32_t& fbo, uint32_t& tex, int w, int h) {
            if (fbo) glDeleteFramebuffers(1, &fbo);
            if (tex) glDeleteTextures(1, &tex);
            glGenFramebuffers(1, &fbo);
            glGenTextures(1, &tex);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        };

        createFBOTexture(m_BrightPassFBO, m_BrightPassTexture, halfW, halfH);
        createFBOTexture(m_BlurFBO[0], m_BlurTextures[0], halfW, halfH);
        createFBOTexture(m_BlurFBO[1], m_BlurTextures[1], halfW, halfH);
    }

    void BloomEffect::apply(uint32_t inputTexture, uint32_t outputFBO, int width, int height) {
        int halfW = width / 2, halfH = height / 2;

        // 1. Bright pass (extract bright pixels)
        glBindFramebuffer(GL_FRAMEBUFFER, m_BrightPassFBO);
        glViewport(0, 0, halfW, halfH);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(m_BrightPassShader);
        glUniform1f(glGetUniformLocation(m_BrightPassShader, "uThreshold"), Threshold);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 2. Gaussian blur (ping-pong)
        uint32_t blurInput = m_BrightPassTexture;
        bool horizontal = true;
        for (int i = 0; i < BlurPasses * 2; i++) {
            int idx = horizontal ? 0 : 1;
            glBindFramebuffer(GL_FRAMEBUFFER, m_BlurFBO[idx]);
            glViewport(0, 0, halfW, halfH);
            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(m_BlurShader);
            glUniform1i(glGetUniformLocation(m_BlurShader, "uHorizontal"), horizontal);
            glUniform2f(glGetUniformLocation(m_BlurShader, "uTexelSize"), 1.0f / halfW, 1.0f / halfH);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, blurInput);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            blurInput = m_BlurTextures[idx];
            horizontal = !horizontal;
        }

        // 3. Combine bloom with scene
        glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(m_CombineShader);
        glUniform1f(glGetUniformLocation(m_CombineShader, "uIntensity"), Intensity);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);
        glUniform1i(glGetUniformLocation(m_CombineShader, "uScene"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, blurInput);
        glUniform1i(glGetUniformLocation(m_CombineShader, "uBloom"), 1);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void BloomEffect::renderUI() {}

}
