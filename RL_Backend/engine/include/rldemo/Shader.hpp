#pragma once

#include "Types.hpp"
#include <string>
#include <cstdint>

namespace rldemo {

class Shader {
public:
    Shader(const char* vertexSrc, const char* fragmentSrc);
    ~Shader();

    void Bind() const;
    void SetMat4(const char* name, const float* m);
    void SetVec4(const char* name, float x, float y, float z, float w);

    uint32_t GetId() const { return m_Program; }

private:
    uint32_t m_Program = 0;
};

} // namespace rldemo
