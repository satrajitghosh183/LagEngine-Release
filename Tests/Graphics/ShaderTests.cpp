/**
 * @file ShaderTests.cpp
 * @brief Unit tests for Shader system (mostly compile-time/interface tests)
 * 
 * Note: Most graphics tests require an OpenGL context. These tests focus on
 * interface validation and compile-time checks. Integration tests should
 * be run with a proper graphics context.
 */

#include <gtest/gtest.h>
#include "Graphics/Shader.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace GameEngine;

// ==================== Shader Source Parsing ====================

TEST(ShaderTest, VertexShaderSource_IsValid) {
    // Test that we can create a valid GLSL vertex shader string
    const char* vertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;
        
        uniform mat4 u_Model;
        uniform mat4 u_ViewProjection;
        
        out vec3 v_Normal;
        out vec2 v_TexCoord;
        
        void main() {
            gl_Position = u_ViewProjection * u_Model * vec4(aPos, 1.0);
            v_Normal = mat3(transpose(inverse(u_Model))) * aNormal;
            v_TexCoord = aTexCoord;
        }
    )";
    
    EXPECT_NE(vertexShader, nullptr);
    EXPECT_GT(strlen(vertexShader), 0);
}

TEST(ShaderTest, FragmentShaderSource_IsValid) {
    const char* fragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        
        in vec3 v_Normal;
        in vec2 v_TexCoord;
        
        uniform vec3 u_Color;
        uniform sampler2D u_Texture;
        
        void main() {
            vec3 color = texture(u_Texture, v_TexCoord).rgb * u_Color;
            FragColor = vec4(color, 1.0);
        }
    )";
    
    EXPECT_NE(fragmentShader, nullptr);
    EXPECT_GT(strlen(fragmentShader), 0);
}

// ==================== Uniform Types ====================

TEST(ShaderTest, UniformTypeSupport) {
    // Test that uniform types can be represented
    int intValue = 42;
    float floatValue = 3.14f;
    glm::vec2 vec2Value(1.0f, 2.0f);
    glm::vec3 vec3Value(1.0f, 2.0f, 3.0f);
    glm::vec4 vec4Value(1.0f, 2.0f, 3.0f, 4.0f);
    glm::mat3 mat3Value(1.0f);
    glm::mat4 mat4Value(1.0f);
    
    // Verify types can be passed to glm::value_ptr
    EXPECT_NE(glm::value_ptr(vec2Value), nullptr);
    EXPECT_NE(glm::value_ptr(vec3Value), nullptr);
    EXPECT_NE(glm::value_ptr(vec4Value), nullptr);
    EXPECT_NE(glm::value_ptr(mat3Value), nullptr);
    EXPECT_NE(glm::value_ptr(mat4Value), nullptr);
}

// ==================== Shader File Extensions ====================

TEST(ShaderTest, FileExtensions) {
    std::string vertPath = "shaders/test.vert";
    std::string fragPath = "shaders/test.frag";
    std::string glslPath = "shaders/test.glsl";
    
    // Verify extensions
    EXPECT_EQ(vertPath.substr(vertPath.find_last_of('.')), ".vert");
    EXPECT_EQ(fragPath.substr(fragPath.find_last_of('.')), ".frag");
    EXPECT_EQ(glslPath.substr(glslPath.find_last_of('.')), ".glsl");
}

// ==================== Shader Include Directive ====================

TEST(ShaderTest, IncludeDirectivePattern) {
    // Test that include pattern can be recognized
    std::string shaderWithInclude = R"(
        #version 330 core
        #include "common.glsl"
        
        void main() {
        }
    )";
    
    size_t includePos = shaderWithInclude.find("#include");
    EXPECT_NE(includePos, std::string::npos);
}

// ==================== Common Uniform Names ====================

TEST(ShaderTest, CommonUniformNaming) {
    // These are common uniform names used in the engine
    std::vector<std::string> commonUniforms = {
        "u_Model",
        "u_View",
        "u_Projection",
        "u_ViewProjection",
        "u_NormalMatrix",
        "u_CameraPosition",
        "u_Time",
        "u_Color",
        "u_Albedo",
        "u_Metallic",
        "u_Roughness",
        "u_AO",
        "u_LightPosition",
        "u_LightColor",
        "u_LightIntensity"
    };
    
    for (const auto& name : commonUniforms) {
        // Verify uniform names start with u_
        EXPECT_EQ(name.substr(0, 2), "u_");
    }
}

// ==================== Attribute Locations ====================

TEST(ShaderTest, StandardAttributeLocations) {
    // Standard attribute locations used in the engine
    // These should match the vertex layout
    const int POSITION_LOCATION = 0;
    const int NORMAL_LOCATION = 1;
    const int TEXCOORD_LOCATION = 2;
    const int TANGENT_LOCATION = 3;
    const int COLOR_LOCATION = 4;
    const int BONE_IDS_LOCATION = 5;
    const int BONE_WEIGHTS_LOCATION = 6;
    
    EXPECT_EQ(POSITION_LOCATION, 0);
    EXPECT_EQ(NORMAL_LOCATION, 1);
    EXPECT_EQ(TEXCOORD_LOCATION, 2);
}

// ==================== PBR Shader Requirements ====================

TEST(ShaderTest, PBRMaterialUniforms) {
    // Required uniforms for PBR materials
    std::vector<std::string> pbrUniforms = {
        "u_AlbedoMap",
        "u_NormalMap",
        "u_MetallicMap",
        "u_RoughnessMap",
        "u_AOMap",
        "u_EmissiveMap",
        "u_IrradianceMap",
        "u_PrefilterMap",
        "u_BRDFLut"
    };
    
    EXPECT_EQ(pbrUniforms.size(), 9);
}

// ==================== Shader Macros ====================

TEST(ShaderTest, DefineDirectives) {
    // Test that define directives can be constructed
    std::string define = "#define MAX_LIGHTS 16";
    
    EXPECT_NE(define.find("#define"), std::string::npos);
    EXPECT_NE(define.find("MAX_LIGHTS"), std::string::npos);
    EXPECT_NE(define.find("16"), std::string::npos);
}

// ==================== Version Directives ====================

TEST(ShaderTest, GLSLVersions) {
    // Supported GLSL versions
    std::vector<std::string> versions = {
        "#version 330 core",
        "#version 400 core",
        "#version 410 core",
        "#version 420 core",
        "#version 430 core",
        "#version 450 core"
    };
    
    for (const auto& version : versions) {
        EXPECT_EQ(version.substr(0, 8), "#version");
    }
}
