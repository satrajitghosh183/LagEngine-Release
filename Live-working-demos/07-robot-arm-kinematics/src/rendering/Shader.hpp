#pragma once

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <stdexcept>

// Compiles GLSL source to SPIR-V at runtime using shaderc
inline std::vector<uint32_t> compileGLSLToSPIRV(
    const std::string& source,
    shaderc_shader_kind kind,
    const std::string& name)
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
        source, kind, name.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << "Shader compilation error (" << name << "): "
                  << result.GetErrorMessage() << std::endl;
        throw std::runtime_error("Failed to compile shader: " + name);
    }

    return {result.cbegin(), result.cend()};
}

// Reads a text file and returns its contents as a string
inline std::string readShaderFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: cannot open shader file: " << path << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}
