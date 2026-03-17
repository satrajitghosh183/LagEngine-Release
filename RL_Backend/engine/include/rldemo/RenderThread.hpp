#pragma once

#include "Types.hpp"
#include "Window.hpp"
#include <thread>
#include <atomic>
#include <functional>
#include <queue>
#include <mutex>

namespace rldemo {

/**
 * RenderThread - owns GL context, only does submit + present
 * Workers do: culling, building draw lists, instance buffers, indirect commands
 */
class RenderThread {
public:
    using RenderFunc = std::function<void()>;

    explicit RenderThread(Ref<Window> window);
    ~RenderThread();

    /** Submit work to run on render thread (before present) */
    void Submit(RenderFunc func);

    /** Run one frame: execute submitted work, swap buffers */
    void Frame();

    /** Request shutdown */
    void RequestStop() { m_StopRequested = true; }

    bool IsStopRequested() const { return m_StopRequested; }
    Window& GetWindow() { return *m_Window; }

private:
    void Run();

    Ref<Window> m_Window;
    std::thread m_Thread;
    std::queue<RenderFunc> m_Queue;
    std::mutex m_QueueMutex;
    std::atomic<bool> m_StopRequested{false};
    std::atomic<bool> m_Running{false};
};

} // namespace rldemo
