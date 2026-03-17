#pragma once

#include <string>
#include <cstdint>
#include <functional>

struct GLFWwindow;

namespace rldemo {

class Window {
public:
    using ResizeCallback = std::function<void(uint32_t, uint32_t)>;

    Window(const std::string& title, uint32_t width, uint32_t height);
    ~Window();

    void PollEvents();
    void SwapBuffers();
    bool ShouldClose() const;

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    float GetAspect() const { return static_cast<float>(m_Width) / static_cast<float>(m_Height ? m_Height : 1); }

    void SetResizeCallback(ResizeCallback cb) { m_ResizeCallback = std::move(cb); }
    GLFWwindow* GetNative() { return m_Window; }

private:
    GLFWwindow* m_Window = nullptr;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    ResizeCallback m_ResizeCallback;
};

} // namespace rldemo
