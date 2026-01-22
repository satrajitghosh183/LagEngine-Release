#include "FrameScheduler.hpp"
#include "JobSystem.hpp"
#include "../Scene/SceneManager.hpp"
#include "../Scene/Scene.hpp"
#include "Logger.hpp"

namespace GameEngine {

    FrameScheduler::FrameScheduler() {
    }

    FrameScheduler::~FrameScheduler() {
    }

    void FrameScheduler::ScheduleFrame(float deltaTime, float fixedDeltaTime, SceneManager* sceneManager) {
        if (!sceneManager) {
            return;
        }

        Ref<Scene> sceneRef = sceneManager->GetActiveScene();
        if (!sceneRef) {
            return;
        }

        Scene* scene = sceneRef.get();
        m_FrameJobs.clear();

        if (!m_UseJobs) {
            // Single-threaded mode - execute directly
            if (scene) {
                scene->FixedUpdate(fixedDeltaTime);
                scene->Update(deltaTime);
            }
            return;
        }

        // Schedule frame stages as jobs
        // Note: We capture sceneRef by value to keep the scene alive during job execution
        JobHandle inputJob = JobSystem::Enqueue([this]() {
            ScheduleInput();
        });

        JobHandle scriptingJob = JobSystem::Enqueue([this, deltaTime, sceneRef]() {
            ScheduleScripting(deltaTime, sceneRef.get());
        }, inputJob);

        JobHandle physicsJob = JobSystem::Enqueue([this, fixedDeltaTime, sceneRef]() {
            SchedulePhysics(fixedDeltaTime, sceneRef.get());
        }, inputJob);

        JobHandle sceneUpdateJob = JobSystem::Enqueue([this, deltaTime, sceneRef]() {
            ScheduleSceneUpdate(deltaTime, sceneRef.get());
        }, scriptingJob);

        JobHandle cullingJob = JobSystem::Enqueue([this, sceneRef]() {
            ScheduleCulling(sceneRef.get());
        }, sceneUpdateJob);

        // Render graph build and GPU submissions run on main thread
        // (after all other jobs complete)
        m_FrameJobs.push_back(cullingJob);
    }

    void FrameScheduler::WaitForFrame() {
        if (!m_UseJobs) {
            return;
        }

        // Wait for all frame jobs
        for (auto& handle : m_FrameJobs) {
            JobSystem::Wait(handle);
        }

        // Now run render graph build and GPU submissions on main thread
        ScheduleRenderGraphBuild();
        ScheduleGPUSubmissions();
    }

    void FrameScheduler::ScheduleInput() {
        // Input polling happens on main thread, so this is a no-op in job mode
        // In the future, input could be polled in parallel
    }

    void FrameScheduler::ScheduleScripting(float deltaTime, Scene* scene) {
        if (!scene) {
            return;
        }
        // TODO: Call script lifecycle functions
        // This will be implemented when scripting is integrated with jobs
    }

    void FrameScheduler::SchedulePhysics(float fixedDeltaTime, Scene* scene) {
        if (!scene) {
            return;
        }
        scene->FixedUpdate(fixedDeltaTime);
    }

    void FrameScheduler::ScheduleSceneUpdate(float deltaTime, Scene* scene) {
        if (!scene) {
            return;
        }
        scene->Update(deltaTime);
    }

    void FrameScheduler::ScheduleCulling(Scene* scene) {
        if (!scene) {
            return;
        }
        // TODO: Implement culling as a job
        // For now, culling happens during render
    }

    void FrameScheduler::ScheduleRenderGraphBuild() {
        // Render graph build happens on main thread
        // TODO: Implement when render graph is added
    }

    void FrameScheduler::ScheduleGPUSubmissions() {
        // GPU submissions happen on main thread
        // TODO: Implement when render graph is added
    }

}

