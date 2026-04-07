#include "rldemo/RenderThread.hpp"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <spdlog/spdlog.h>

namespace rldemo {

RenderThread::RenderThread(Ref<Window> window) : m_Window(std::move(window)) {
    m_Thread = std::thread(&RenderThread::Run, this);
    while (!m_Running.load()) {
        std::this_thread::yield();
    }
}

RenderThread::~RenderThread() {
    m_StopRequested = true;
    if (m_Thread.joinable()) m_Thread.join();
}

void RenderThread::Submit(RenderFunc func) {
    if (func) {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_Queue.push(std::move(func));
    }
}

void RenderThread::Frame() {
    std::queue<RenderFunc> batch;
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        batch.swap(m_Queue);
    }
    while (!batch.empty()) {
        auto& f = batch.front();
        if (f) f();
        batch.pop();
    }
}

void RenderThread::Run() {
    m_Window->PollEvents();
    glfwMakeContextCurrent(m_Window->GetNative());
    m_Running = true;
    while (!m_StopRequested && !m_Window->ShouldClose()) {
        Frame();
        m_Window->SwapBuffers();
        m_Window->PollEvents();
    }
    m_Running = false;
}

} // namespace rldemo
