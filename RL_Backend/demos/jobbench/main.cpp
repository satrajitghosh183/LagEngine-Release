/**
 * RL_Backend - JobBench microbenchmark
 * Fan-out/fan-in graphs, measures steals, throughput, scaling
 *
 * CLI (optional):
 *   jobbench [--workers W] [--depth D] [--breadth B] [--machine]
 *   --workers: comma-separated list, e.g. 1,2,4,8
 *   --depth, --breadth: single value or comma-separated; if both given, runs that pair only
 *   --machine: print KEY=value lines for parsing (workers,depth,breadth,ms,jobs,steals,throughput)
 */

#include "rldemo/JobSystem.hpp"
#include <spdlog/spdlog.h>
#include <chrono>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>

using namespace rldemo;

static bool s_MachineOutput = false;

static std::vector<uint32_t> parseList(const char* s) {
    std::vector<uint32_t> out;
    std::istringstream iss(s);
    std::string part;
    while (std::getline(iss, part, ',')) {
        unsigned long v = 0;
        try { v = std::stoul(part); } catch (...) { continue; }
        if (v > 0 && v <= 64) out.push_back(static_cast<uint32_t>(v));
    }
    return out;
}

void runFanOutFanIn(uint32_t workers, uint32_t depth, uint32_t breadth) {
    JobSystem::ResetStats();
    auto start = std::chrono::high_resolution_clock::now();
    std::atomic<int> counter{0};
    std::function<void(int, int)> spawn;
    spawn = [&spawn, &counter, breadth](int d, int maxDepth) {
        if (d >= maxDepth) {
            counter++;
            return;
        }
        std::vector<JobHandle> children;
        for (uint32_t i = 0; i < breadth; ++i) {
            auto h = JobSystem::Spawn([&spawn, d, maxDepth, breadth]() {
                spawn(d + 1, maxDepth);
            }, {});
            children.push_back(h);
        }
        for (auto& h : children) JobSystem::Wait(h);
    };
    JobHandle root = JobSystem::Enqueue([&spawn, depth, breadth]() { spawn(0, depth); }, {});
    JobSystem::Wait(root);
    auto end = std::chrono::high_resolution_clock::now();
    auto stats = JobSystem::GetStats();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    double throughput = (ms > 0) ? (stats.jobCount / (ms / 1000.0)) : 0;

    if (s_MachineOutput) {
        std::cout << "workers=" << workers << " depth=" << depth << " breadth=" << breadth
                  << " ms=" << ms << " jobs=" << stats.jobCount
                  << " steals=" << stats.steals << " steal_attempts=" << stats.stealAttempts
                  << " throughput=" << throughput << " pushes=" << stats.pushes << " pops=" << stats.pops << "\n";
    } else {
        std::cout << "  FanOut(d=" << depth << ",b=" << breadth << "): "
                  << ms << " ms, jobs=" << stats.jobCount
                  << ", steals=" << stats.steals
                  << ", throughput=" << throughput << " jobs/s\n";
    }
}

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::warn);

    std::vector<uint32_t> workerList = {2, 4, 8};
    std::vector<uint32_t> depthList, breadthList;
    bool customPair = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--machine") == 0) {
            s_MachineOutput = true;
        } else if (std::strcmp(argv[i], "--workers") == 0 && i + 1 < argc) {
            workerList = parseList(argv[++i]);
            if (workerList.empty()) workerList = {2, 4, 8};
        } else if (std::strcmp(argv[i], "--depth") == 0 && i + 1 < argc) {
            depthList = parseList(argv[++i]);
            customPair = true;
        } else if (std::strcmp(argv[i], "--breadth") == 0 && i + 1 < argc) {
            breadthList = parseList(argv[++i]);
            customPair = true;
        }
    }

    if (!s_MachineOutput) {
        std::cout << "JobBench - Work-stealing job system microbenchmark\n";
    }

    std::vector<std::pair<uint32_t, uint32_t>> runs;
    if (customPair && !depthList.empty() && !breadthList.empty()) {
        for (uint32_t d : depthList)
            for (uint32_t b : breadthList)
                runs.push_back({d, b});
    } else if (customPair && depthList.size() == 1 && breadthList.size() == 1) {
        runs.push_back({depthList[0], breadthList[0]});
    } else {
        runs = {{3, 4}, {4, 3}, {5, 2}};
    }

    for (uint32_t workers : workerList) {
        JobSystem::Shutdown();
        JobSystem::Initialize(workers);
        if (!s_MachineOutput) {
            std::cout << "\nWorkers: " << workers << "\n";
        }
        for (const auto& [depth, breadth] : runs) {
            runFanOutFanIn(workers, depth, breadth);
        }
    }

    JobSystem::Shutdown();
    if (!s_MachineOutput) {
        std::cout << "\nDone.\n";
    }
    return 0;
}
