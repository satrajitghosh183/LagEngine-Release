#pragma once

#include "Base.hpp"
#include "JobSystem.hpp"
#include <functional>
#include <vector>

namespace GameEngine {

    class Scene;
    class SceneManager;

    /**
     * @brief Frame scheduler - decomposes frame into jobs
     * 
     * Converts frame stages into jobs with dependencies:
     * - Input polling (main thread)
     * - Scripting update (jobs)
     * - Physics step (jobs)
     * - Scene update (jobs)
     * - Culling (jobs)
     * - Render graph build (main thread)
     * - GPU submissions (main thread)
     */
    class FrameScheduler {
    public:
        FrameScheduler();
        ~FrameScheduler();

        /**
         * @brief Schedule frame stages
         */
        void ScheduleFrame(float deltaTime, float fixedDeltaTime, SceneManager* sceneManager);

        /**
         * @brief Wait for frame completion
         */
        void WaitForFrame();

        /**
         * @brief Enable/disable job system (for debugging)
         */
        void SetUseJobs(bool useJobs) { m_UseJobs = useJobs; }
        bool GetUseJobs() const { return m_UseJobs; }

    private:
        void ScheduleInput();
        void ScheduleScripting(float deltaTime, Scene* scene);
        void SchedulePhysics(float fixedDeltaTime, Scene* scene);
        void ScheduleSceneUpdate(float deltaTime, Scene* scene);
        void ScheduleCulling(Scene* scene);
        void ScheduleRenderGraphBuild();
        void ScheduleGPUSubmissions();

        bool m_UseJobs = true;
        std::vector<JobHandle> m_FrameJobs;
    };

}

