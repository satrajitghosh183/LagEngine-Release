/**
 * @file ScriptEngineTests.cpp
 * @brief Unit tests for the Lua scripting engine
 */

#include <gtest/gtest.h>
#include "Scripting/ScriptEngine.hpp"
#include <thread>
#include <vector>

using namespace GameEngine;

class ScriptEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        ScriptEngine::Init();
    }

    void TearDown() override {
        ScriptEngine::Shutdown();
    }
};

// ==================== Basic Execution ====================

TEST_F(ScriptEngineTest, ExecuteSimpleScript) {
    bool result = ScriptEngine::ExecuteString("x = 1 + 1");
    EXPECT_TRUE(result);
}

TEST_F(ScriptEngineTest, ExecuteMultipleStatements) {
    bool result = ScriptEngine::ExecuteString(R"(
        local a = 10
        local b = 20
        local c = a + b
    )");
    EXPECT_TRUE(result);
}

TEST_F(ScriptEngineTest, ExecuteWithError) {
    bool result = ScriptEngine::ExecuteString("this is not valid lua");
    EXPECT_FALSE(result);
}

TEST_F(ScriptEngineTest, ExecuteEmptyString) {
    bool result = ScriptEngine::ExecuteString("");
    EXPECT_TRUE(result);  // Empty string is valid Lua
}

// ==================== Built-in Functions ====================

TEST_F(ScriptEngineTest, PrintFunction) {
    bool result = ScriptEngine::ExecuteString("print('Hello from Lua')");
    EXPECT_TRUE(result);
}

TEST_F(ScriptEngineTest, MathFunctions) {
    bool result = ScriptEngine::ExecuteString(R"(
        local a = math.sin(0)
        local b = math.cos(0)
        local c = math.sqrt(4)
        local d = math.floor(3.7)
        local e = math.ceil(3.2)
    )");
    EXPECT_TRUE(result);
}

TEST_F(ScriptEngineTest, StringFunctions) {
    bool result = ScriptEngine::ExecuteString(R"(
        local s = "hello world"
        local upper = string.upper(s)
        local sub = string.sub(s, 1, 5)
        local len = string.len(s)
    )");
    EXPECT_TRUE(result);
}

TEST_F(ScriptEngineTest, TableFunctions) {
    bool result = ScriptEngine::ExecuteString(R"(
        local t = {1, 2, 3, 4, 5}
        table.insert(t, 6)
        table.remove(t, 1)
        local sorted = {5, 3, 1, 4, 2}
        table.sort(sorted)
    )");
    EXPECT_TRUE(result);
}

// ==================== Sandboxing ====================

TEST_F(ScriptEngineTest, NoOSAccess) {
    // os library should not be available
    bool result = ScriptEngine::ExecuteString("os.execute('echo test')");
    EXPECT_FALSE(result);
}

TEST_F(ScriptEngineTest, NoIOAccess) {
    // io library should not be available
    bool result = ScriptEngine::ExecuteString("io.open('/etc/passwd', 'r')");
    EXPECT_FALSE(result);
}

TEST_F(ScriptEngineTest, NoPackageAccess) {
    // package/require should not be available for loading arbitrary modules
    bool result = ScriptEngine::ExecuteString("require('os')");
    EXPECT_FALSE(result);
}

// ==================== Engine API ====================

TEST_F(ScriptEngineTest, VectorCreation) {
    bool result = ScriptEngine::ExecuteString(R"(
        local v = Vector(1, 2, 3)
    )");
    EXPECT_TRUE(result);
}

TEST_F(ScriptEngineTest, VectorOperations) {
    bool result = ScriptEngine::ExecuteString(R"(
        local v1 = Vector(1, 0, 0)
        local v2 = Vector(0, 1, 0)
        local v3 = v1 + v2
        local dot = v1:dot(v2)
        local cross = v1:cross(v2)
        local len = v1:length()
        local norm = v1:normalize()
    )");
    EXPECT_TRUE(result);
}

TEST_F(ScriptEngineTest, GetPropsFunction) {
    bool result = ScriptEngine::ExecuteString("getprops(Vector)");
    EXPECT_TRUE(result);
}

// ==================== Error Handling ====================

TEST_F(ScriptEngineTest, RuntimeError) {
    bool result = ScriptEngine::ExecuteString(R"(
        local t = nil
        local x = t.field  -- nil access
    )");
    EXPECT_FALSE(result);
}

TEST_F(ScriptEngineTest, StackOverflow) {
    bool result = ScriptEngine::ExecuteString(R"(
        function infinite()
            infinite()
        end
        infinite()
    )");
    EXPECT_FALSE(result);  // Should fail with stack overflow
}

// ==================== Thread Safety ====================

TEST_F(ScriptEngineTest, ConcurrentExecution) {
    const int numThreads = 5;
    const int executionsPerThread = 20;
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    std::atomic<int> failCount{0};
    
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back([i, executionsPerThread, &successCount, &failCount]() {
            for (int j = 0; j < executionsPerThread; j++) {
                std::string script = "local x = " + std::to_string(i * 100 + j);
                if (ScriptEngine::ExecuteString(script)) {
                    successCount++;
                } else {
                    failCount++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // All executions should succeed
    EXPECT_EQ(successCount.load(), numThreads * executionsPerThread);
    EXPECT_EQ(failCount.load(), 0);
}

// ==================== Function Calling ====================

TEST_F(ScriptEngineTest, CallFunction) {
    ScriptEngine::ExecuteString(R"(
        function testFunc()
            return true
        end
    )");
    
    bool result = ScriptEngine::CallFunction("testFunc");
    EXPECT_TRUE(result);
}

TEST_F(ScriptEngineTest, CallNonexistentFunction) {
    bool result = ScriptEngine::CallFunction("nonExistentFunction");
    EXPECT_FALSE(result);
}

// ==================== Global State ====================

TEST_F(ScriptEngineTest, GlobalVariablePersistence) {
    ScriptEngine::ExecuteString("globalVar = 42");
    
    bool result = ScriptEngine::ExecuteString(R"(
        assert(globalVar == 42, "Global variable not persisted")
    )");
    EXPECT_TRUE(result);
}

TEST_F(ScriptEngineTest, FunctionPersistence) {
    ScriptEngine::ExecuteString(R"(
        function add(a, b)
            return a + b
        end
    )");
    
    bool result = ScriptEngine::ExecuteString(R"(
        local result = add(2, 3)
        assert(result == 5, "Function not persisted")
    )");
    EXPECT_TRUE(result);
}

// ==================== Lua State Management ====================

TEST_F(ScriptEngineTest, GetLuaState) {
    lua_State* L = ScriptEngine::GetLuaState();
    EXPECT_NE(L, nullptr);
}

TEST_F(ScriptEngineTest, ReInitialization) {
    ScriptEngine::Shutdown();
    ScriptEngine::Init();
    
    lua_State* L = ScriptEngine::GetLuaState();
    EXPECT_NE(L, nullptr);
    
    bool result = ScriptEngine::ExecuteString("x = 1");
    EXPECT_TRUE(result);
}
