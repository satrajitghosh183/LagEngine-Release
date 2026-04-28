#pragma once

#include "../JobSystem.hpp"
#include <functional>
#include <atomic>
#include <vector>
#include <cstdint>
#include <algorithm>

namespace GameEngine {

    /**
     * @brief Parallel for-loop utilities built on the JobSystem
     *
     * Splits a range [begin, end) into chunks and distributes across workers.
     * Blocks until all iterations complete.
     */
    class ParallelFor {
    public:
        /**
         * @brief Execute func(index) for each index in [begin, end)
         * @param begin Start index (inclusive)
         * @param end End index (exclusive)
         * @param func Function taking a single index
         * @param grainSize Minimum iterations per chunk (0 = auto)
         */
        static void Execute(int64_t begin, int64_t end,
                            const std::function<void(int64_t)>& func,
                            int64_t grainSize = 0) {
            if (begin >= end) return;

            int64_t count = end - begin;
            uint32_t workerCount = JobSystem::GetWorkerCount();

            if (workerCount == 0 || count <= 1) {
                for (int64_t i = begin; i < end; i++) func(i);
                return;
            }

            if (grainSize <= 0) {
                grainSize = std::max<int64_t>(1, count / (workerCount * 4));
            }

            int64_t numChunks = (count + grainSize - 1) / grainSize;
            std::vector<JobHandle> handles;
            handles.reserve(static_cast<size_t>(numChunks));

            for (int64_t chunk = 0; chunk < numChunks; chunk++) {
                int64_t chunkBegin = begin + chunk * grainSize;
                int64_t chunkEnd = std::min(chunkBegin + grainSize, end);

                handles.push_back(JobSystem::Enqueue([=]() {
                    for (int64_t i = chunkBegin; i < chunkEnd; i++) {
                        func(i);
                    }
                }));
            }

            for (auto& h : handles) {
                JobSystem::Wait(h);
            }
        }

        /**
         * @brief Execute func(begin, end) for each chunk in [begin, end)
         * @param begin Start index (inclusive)
         * @param end End index (exclusive)
         * @param func Function taking (chunkBegin, chunkEnd)
         * @param numChunks Number of chunks (0 = auto based on worker count)
         */
        static void ExecuteChunked(int64_t begin, int64_t end,
                                    const std::function<void(int64_t, int64_t)>& func,
                                    int64_t numChunks = 0) {
            if (begin >= end) return;

            int64_t count = end - begin;
            uint32_t workerCount = JobSystem::GetWorkerCount();

            if (workerCount == 0) {
                func(begin, end);
                return;
            }

            if (numChunks <= 0) {
                numChunks = static_cast<int64_t>(workerCount * 2);
            }
            numChunks = std::min(numChunks, count);

            int64_t chunkSize = count / numChunks;
            int64_t remainder = count % numChunks;

            std::vector<JobHandle> handles;
            handles.reserve(static_cast<size_t>(numChunks));

            int64_t current = begin;
            for (int64_t i = 0; i < numChunks; i++) {
                int64_t thisChunk = chunkSize + (i < remainder ? 1 : 0);
                int64_t chunkEnd = current + thisChunk;

                handles.push_back(JobSystem::Enqueue([=]() {
                    func(current, chunkEnd);
                }));
                current = chunkEnd;
            }

            for (auto& h : handles) {
                JobSystem::Wait(h);
            }
        }

        /**
         * @brief Parallel reduce: split range, apply func per chunk, merge results
         * @param begin Start index (inclusive)
         * @param end End index (exclusive)
         * @param identity Identity element for the reduction
         * @param func (begin, end) -> partial result
         * @param merge (a, b) -> combined result
         */
        template <typename T>
        static T Reduce(int64_t begin, int64_t end,
                        T identity,
                        const std::function<T(int64_t, int64_t)>& func,
                        const std::function<T(T, T)>& merge) {
            if (begin >= end) return identity;

            uint32_t workerCount = JobSystem::GetWorkerCount();
            if (workerCount == 0) return func(begin, end);

            int64_t count = end - begin;
            int64_t numChunks = std::min(static_cast<int64_t>(workerCount * 2), count);
            int64_t chunkSize = count / numChunks;
            int64_t remainder = count % numChunks;

            std::vector<T> results(static_cast<size_t>(numChunks), identity);
            std::vector<JobHandle> handles;
            handles.reserve(static_cast<size_t>(numChunks));

            int64_t current = begin;
            for (int64_t i = 0; i < numChunks; i++) {
                int64_t thisChunk = chunkSize + (i < remainder ? 1 : 0);
                int64_t chunkEnd = current + thisChunk;
                size_t idx = static_cast<size_t>(i);

                handles.push_back(JobSystem::Enqueue([&results, idx, current, chunkEnd, &func]() {
                    results[idx] = func(current, chunkEnd);
                }));
                current = chunkEnd;
            }

            for (auto& h : handles) {
                JobSystem::Wait(h);
            }

            T result = identity;
            for (auto& partial : results) {
                result = merge(result, partial);
            }
            return result;
        }
    };

} // namespace GameEngine
