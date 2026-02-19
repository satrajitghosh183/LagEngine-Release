/**
 * @file UUIDTests.cpp
 * @brief Unit tests for UUID generation
 */

#include <gtest/gtest.h>
#include "Core/UUID.hpp"
#include <unordered_set>
#include <thread>
#include <vector>

using namespace GameEngine;

// Test UUID uniqueness
TEST(UUIDTest, Uniqueness) {
    std::unordered_set<uint64_t> uuids;
    const int count = 10000;
    
    for (int i = 0; i < count; i++) {
        UUID uuid;
        auto [it, inserted] = uuids.insert(static_cast<uint64_t>(uuid));
        EXPECT_TRUE(inserted) << "Duplicate UUID generated at iteration " << i;
    }
    
    EXPECT_EQ(uuids.size(), count);
}

// Test UUID non-zero
TEST(UUIDTest, NonZero) {
    for (int i = 0; i < 100; i++) {
        UUID uuid;
        EXPECT_NE(static_cast<uint64_t>(uuid), 0);
    }
}

// Test UUID from value
TEST(UUIDTest, FromValue) {
    uint64_t value = 12345678901234567890ULL;
    UUID uuid(value);
    EXPECT_EQ(static_cast<uint64_t>(uuid), value);
}

// Test UUID copy
TEST(UUIDTest, Copy) {
    UUID uuid1;
    UUID uuid2 = uuid1;
    EXPECT_EQ(static_cast<uint64_t>(uuid1), static_cast<uint64_t>(uuid2));
}

// Test UUID comparison
TEST(UUIDTest, Comparison) {
    UUID uuid1;
    UUID uuid2;
    UUID uuid3 = uuid1;
    
    EXPECT_NE(uuid1, uuid2);
    EXPECT_EQ(uuid1, uuid3);
}

// Test UUID hashing (for use in containers)
TEST(UUIDTest, Hashing) {
    std::unordered_set<UUID> uuidSet;
    
    UUID uuid1;
    UUID uuid2;
    
    uuidSet.insert(uuid1);
    uuidSet.insert(uuid2);
    uuidSet.insert(uuid1); // Duplicate
    
    EXPECT_EQ(uuidSet.size(), 2);
}

// Test thread-safe UUID generation
TEST(UUIDTest, ThreadSafety) {
    const int numThreads = 10;
    const int uuidsPerThread = 1000;
    
    std::vector<std::vector<UUID>> threadUUIDs(numThreads);
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back([i, &threadUUIDs, uuidsPerThread]() {
            for (int j = 0; j < uuidsPerThread; j++) {
                threadUUIDs[i].push_back(UUID());
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Check all UUIDs are unique
    std::unordered_set<uint64_t> allUUIDs;
    for (const auto& vec : threadUUIDs) {
        for (const auto& uuid : vec) {
            auto [it, inserted] = allUUIDs.insert(static_cast<uint64_t>(uuid));
            EXPECT_TRUE(inserted) << "Duplicate UUID in multi-threaded test";
        }
    }
    
    EXPECT_EQ(allUUIDs.size(), numThreads * uuidsPerThread);
}

// Test zero UUID
TEST(UUIDTest, ZeroUUID) {
    UUID uuid(0);
    EXPECT_EQ(static_cast<uint64_t>(uuid), 0);
    
    // Check that default constructor never produces zero
    for (int i = 0; i < 1000; i++) {
        UUID newUuid;
        EXPECT_NE(static_cast<uint64_t>(newUuid), 0);
    }
}
