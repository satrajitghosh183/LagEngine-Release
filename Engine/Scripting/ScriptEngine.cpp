#include "ScriptEngine.hpp"
#include "../Core/Logger.hpp"
#include "../Core/Application.hpp"
#include "../Core/WickedCompat.hpp"
#include "../Platform/FileSystem.hpp"
#include "../Scene/Components/TransformComponent.hpp"
#include "../Scene/Components/RigidBodyComponent.hpp"
#include "../Scene/Components/LightComponent.hpp"
#include "../Scene/SceneHelpers.hpp"
#include "../Audio/AudioHelpers.hpp"
#include "../Platform/InputHelpers.hpp"
#include "../Graphics/RenderPath.hpp"
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>
#include <sstream>
#include <mutex>

namespace GameEngine {

    lua_State* ScriptEngine::s_LuaState = nullptr;
    std::mutex ScriptEngine::s_LuaMutex;
    std::function<void(const std::string&)> ScriptEngine::s_PrintCallback;

    static int LuaPrint(lua_State* L) {
        int n = lua_gettop(L);
        std::string s;
        for (int i = 1; i <= n; i++) {
            if (i > 1) s += "\t";
            luaL_tolstring(L, i, nullptr);
            const char* str = lua_tostring(L, -1);
            if (str) s += str;
            lua_pop(L, 1);
        }
        // Safe: LuaPrint only runs inside ExecuteString/CallFunction which hold s_LuaMutex
        if (ScriptEngine::s_PrintCallback) ScriptEngine::s_PrintCallback(s);
        return 0;
    }

    void ScriptEngine::SetPrintCallback(std::function<void(const std::string&)> cb) {
        std::lock_guard<std::mutex> lock(s_LuaMutex);
        s_PrintCallback = std::move(cb);
    }

    // Registry for storing RenderPath references (must be before Shutdown)
    static std::unordered_map<int, Ref<RenderPath>> s_RenderPathRegistry;
    static int s_RenderPathCounter = 0;
    
    // Registry for storing Sound references
    static std::unordered_map<int, AudioHelpers::Sound> s_SoundRegistry;
    static int s_SoundCounter = 0;
    
    // Registry for storing SoundInstance references
    static std::unordered_map<int, AudioHelpers::SoundInstance> s_SoundInstanceRegistry;
    static int s_SoundInstanceCounter = 0;

    // Registry for storing Scene references
    static std::unordered_map<int, Ref<Scene>> s_SceneRegistry;
    static int s_SceneCounter = 0;

    // Forward declaration for LuaGetProps
    static int LuaGetProps(lua_State* L);
    
    // Helper to push vec3 to Lua as table
    static void PushVec3(lua_State* L, const glm::vec3& v) {
        lua_newtable(L);
        lua_pushnumber(L, v.x); lua_setfield(L, -2, "x");
        lua_pushnumber(L, v.y); lua_setfield(L, -2, "y");
        lua_pushnumber(L, v.z); lua_setfield(L, -2, "z");
    }
    
    // Helper to get vec3 from Lua table
    static glm::vec3 ToVec3(lua_State* L, int index) {
        glm::vec3 v(0.0f);
        if (lua_istable(L, index)) {
            lua_getfield(L, index, "x"); v.x = static_cast<float>(lua_tonumber(L, -1)); lua_pop(L, 1);
            lua_getfield(L, index, "y"); v.y = static_cast<float>(lua_tonumber(L, -1)); lua_pop(L, 1);
            lua_getfield(L, index, "z"); v.z = static_cast<float>(lua_tonumber(L, -1)); lua_pop(L, 1);
        }
        return v;
    }
    
    // Helper to push quat to Lua as table
    static void PushQuat(lua_State* L, const glm::quat& q) {
        lua_newtable(L);
        lua_pushnumber(L, q.x); lua_setfield(L, -2, "x");
        lua_pushnumber(L, q.y); lua_setfield(L, -2, "y");
        lua_pushnumber(L, q.z); lua_setfield(L, -2, "z");
        lua_pushnumber(L, q.w); lua_setfield(L, -2, "w");
    }
    
    // Helper to get quat from Lua table
    static glm::quat ToQuat(lua_State* L, int index) {
        glm::quat q(1, 0, 0, 0);
        if (lua_istable(L, index)) {
            lua_getfield(L, index, "x"); q.x = static_cast<float>(lua_tonumber(L, -1)); lua_pop(L, 1);
            lua_getfield(L, index, "y"); q.y = static_cast<float>(lua_tonumber(L, -1)); lua_pop(L, 1);
            lua_getfield(L, index, "z"); q.z = static_cast<float>(lua_tonumber(L, -1)); lua_pop(L, 1);
            lua_getfield(L, index, "w"); q.w = static_cast<float>(lua_tonumber(L, -1)); lua_pop(L, 1);
        }
        return q;
    }

    void ScriptEngine::Init() {
        s_LuaState = luaL_newstate();
        // Sandbox: load only safe libraries (exclude os, io, package, debug to prevent security issues)
        luaL_requiref(s_LuaState, LUA_GNAME, luaopen_base, 1);
        lua_pop(s_LuaState, 1);
        luaL_requiref(s_LuaState, LUA_COLIBNAME, luaopen_coroutine, 1);
        lua_pop(s_LuaState, 1);
        luaL_requiref(s_LuaState, LUA_TABLIBNAME, luaopen_table, 1);
        lua_pop(s_LuaState, 1);
        luaL_requiref(s_LuaState, LUA_STRLIBNAME, luaopen_string, 1);
        lua_pop(s_LuaState, 1);
        luaL_requiref(s_LuaState, LUA_MATHLIBNAME, luaopen_math, 1);
        lua_pop(s_LuaState, 1);
        luaL_requiref(s_LuaState, LUA_UTF8LIBNAME, luaopen_utf8, 1);
        lua_pop(s_LuaState, 1);
        
        RegisterCoreFunctions();
        RegisterComponents();
        RegisterMathTypes();
        RegisterApplicationAPI();
        RegisterSceneAPI();
        RegisterAudioAPI();
        RegisterInputAPI();
        
        // Load startup.lua if it exists
        LoadStartupScript();
        
        GE_CORE_INFO("ScriptEngine initialized");
    }

    void ScriptEngine::Shutdown() {
        std::lock_guard<std::mutex> lock(s_LuaMutex);
        if (s_LuaState) {
            lua_close(s_LuaState);
            s_LuaState = nullptr;
        }
        // Clear registries to prevent leaks
        s_RenderPathRegistry.clear();
        s_SoundRegistry.clear();
        s_SoundInstanceRegistry.clear();
        s_SceneRegistry.clear();
        s_RenderPathCounter = 0;
        s_SoundCounter = 0;
        s_SoundInstanceCounter = 0;
        s_SceneCounter = 0;
        
        GE_CORE_INFO("ScriptEngine shutdown");
    }

    bool ScriptEngine::LoadScript(const std::string& filepath) {
        if (!s_LuaState) {
            GE_CORE_ERROR("ScriptEngine not initialized");
            return false;
        }
        if (!FileSystem::Exists(filepath)) {
            GE_CORE_ERROR("Script file not found: {0}", filepath);
            return false;
        }
        
        std::string code = FileSystem::ReadFileAsString(filepath);
        return ExecuteString(code);
    }

    bool ScriptEngine::ExecuteString(const std::string& code) {
        if (!s_LuaState) {
            GE_CORE_ERROR("ScriptEngine not initialized");
            return false;
        }
        std::lock_guard<std::mutex> lock(s_LuaMutex);
        int result = luaL_dostring(s_LuaState, code.c_str());
        
        if (result != LUA_OK) {
            const char* error = lua_tostring(s_LuaState, -1);
            GE_CORE_ERROR("Lua error: {0}", error ? error : "unknown");
            lua_pop(s_LuaState, 1);
            return false;
        }
        
        return true;
    }

    bool ScriptEngine::CallFunction(const std::string& functionName) {
        if (!s_LuaState) {
            GE_CORE_ERROR("ScriptEngine not initialized");
            return false;
        }
        std::lock_guard<std::mutex> lock(s_LuaMutex);

        lua_getglobal(s_LuaState, functionName.c_str());
        
        if (!lua_isfunction(s_LuaState, -1)) {
            GE_CORE_ERROR("Lua function not found: {0}", functionName);
            lua_pop(s_LuaState, 1);
            return false;
        }
        
        int result = lua_pcall(s_LuaState, 0, 0, 0);
        
        if (result != LUA_OK) {
            const char* error = lua_tostring(s_LuaState, -1);
            GE_CORE_ERROR("Lua error calling {0}: {1}", functionName, error);
            lua_pop(s_LuaState, 1);
            return false;
        }
        
        return true;
    }

    void ScriptEngine::RegisterEntity(Entity entity) {
        // Push entity userdata to Lua
        // TODO: Implement full entity binding
    }

    void ScriptEngine::RegisterCoreFunctions() {
        // Register logging
        lua_register(s_LuaState, "Log", [](lua_State* L) -> int {
            const char* msg = luaL_checkstring(L, 1);
            GE_INFO("{0}", msg);
            return 0;
        });
        
        lua_register(s_LuaState, "LogWarning", [](lua_State* L) -> int {
            const char* msg = luaL_checkstring(L, 1);
            GE_WARN("{0}", msg);
            return 0;
        });
        
        lua_register(s_LuaState, "LogError", [](lua_State* L) -> int {
            const char* msg = luaL_checkstring(L, 1);
            GE_ERROR("{0}", msg);
            return 0;
        });
        
        // getprops() - introspection utility
        lua_register(s_LuaState, "getprops", LuaGetProps);

        // Override print() to redirect to callback
        lua_pushcfunction(s_LuaState, LuaPrint);
        lua_setglobal(s_LuaState, "print");
    }

    void ScriptEngine::RegisterComponents() {
        // ==================== Transform Component ====================
        luaL_newmetatable(s_LuaState, "Transform");
        
        // GetPosition
        lua_register(s_LuaState, "Transform_GetPosition", [](lua_State* L) -> int {
            TransformComponent* transform = static_cast<TransformComponent*>(lua_touserdata(L, 1));
            if (transform) {
                PushVec3(L, transform->Position);
                return 1;
            }
            return 0;
        });
        
        // SetPosition
        lua_register(s_LuaState, "Transform_SetPosition", [](lua_State* L) -> int {
            TransformComponent* transform = static_cast<TransformComponent*>(lua_touserdata(L, 1));
            if (transform) {
                transform->Position = ToVec3(L, 2);
            }
            return 0;
        });
        
        // GetRotation
        lua_register(s_LuaState, "Transform_GetRotation", [](lua_State* L) -> int {
            TransformComponent* transform = static_cast<TransformComponent*>(lua_touserdata(L, 1));
            if (transform) {
                PushQuat(L, transform->Rotation);
                return 1;
            }
            return 0;
        });
        
        // SetRotation
        lua_register(s_LuaState, "Transform_SetRotation", [](lua_State* L) -> int {
            TransformComponent* transform = static_cast<TransformComponent*>(lua_touserdata(L, 1));
            if (transform) {
                transform->Rotation = ToQuat(L, 2);
            }
            return 0;
        });
        
        // GetScale
        lua_register(s_LuaState, "Transform_GetScale", [](lua_State* L) -> int {
            TransformComponent* transform = static_cast<TransformComponent*>(lua_touserdata(L, 1));
            if (transform) {
                PushVec3(L, transform->Scale);
                return 1;
            }
            return 0;
        });
        
        // SetScale
        lua_register(s_LuaState, "Transform_SetScale", [](lua_State* L) -> int {
            TransformComponent* transform = static_cast<TransformComponent*>(lua_touserdata(L, 1));
            if (transform) {
                transform->Scale = ToVec3(L, 2);
            }
            return 0;
        });
        
        // Translate
        lua_register(s_LuaState, "Transform_Translate", [](lua_State* L) -> int {
            TransformComponent* transform = static_cast<TransformComponent*>(lua_touserdata(L, 1));
            if (transform) {
                glm::vec3 delta = ToVec3(L, 2);
                transform->Position += delta;
            }
            return 0;
        });
        
        // LookAt
        lua_register(s_LuaState, "Transform_LookAt", [](lua_State* L) -> int {
            TransformComponent* transform = static_cast<TransformComponent*>(lua_touserdata(L, 1));
            if (transform) {
                glm::vec3 target = ToVec3(L, 2);
                glm::vec3 direction = glm::normalize(target - transform->Position);
                transform->Rotation = glm::quatLookAt(direction, glm::vec3(0, 1, 0));
            }
            return 0;
        });
        
        // ==================== RigidBody Component ====================
        lua_register(s_LuaState, "RigidBody_ApplyForce", [](lua_State* L) -> int {
            RigidBodyComponent* rb = static_cast<RigidBodyComponent*>(lua_touserdata(L, 1));
            if (rb && rb->GetRigidBody()) {
                glm::vec3 force = ToVec3(L, 2);
                rb->GetRigidBody()->ApplyForce(force);
            }
            return 0;
        });
        
        lua_register(s_LuaState, "RigidBody_ApplyImpulse", [](lua_State* L) -> int {
            RigidBodyComponent* rb = static_cast<RigidBodyComponent*>(lua_touserdata(L, 1));
            if (rb && rb->GetRigidBody()) {
                glm::vec3 impulse = ToVec3(L, 2);
                rb->GetRigidBody()->ApplyImpulse(impulse);
            }
            return 0;
        });
        
        lua_register(s_LuaState, "RigidBody_GetVelocity", [](lua_State* L) -> int {
            RigidBodyComponent* rb = static_cast<RigidBodyComponent*>(lua_touserdata(L, 1));
            if (rb && rb->GetRigidBody()) {
                PushVec3(L, rb->GetRigidBody()->GetLinearVelocity());
                return 1;
            }
            return 0;
        });
        
        lua_register(s_LuaState, "RigidBody_SetVelocity", [](lua_State* L) -> int {
            RigidBodyComponent* rb = static_cast<RigidBodyComponent*>(lua_touserdata(L, 1));
            if (rb && rb->GetRigidBody()) {
                glm::vec3 velocity = ToVec3(L, 2);
                rb->GetRigidBody()->SetLinearVelocity(velocity);
            }
            return 0;
        });
        
        GE_CORE_DEBUG("Lua component bindings registered");
    }

    void ScriptEngine::RegisterMathTypes() {
        // ==================== Vec3 ====================
        lua_register(s_LuaState, "Vec3", [](lua_State* L) -> int {
            float x = static_cast<float>(luaL_optnumber(L, 1, 0.0));
            float y = static_cast<float>(luaL_optnumber(L, 2, 0.0));
            float z = static_cast<float>(luaL_optnumber(L, 3, 0.0));
            PushVec3(L, glm::vec3(x, y, z));
            return 1;
        });
        
        lua_register(s_LuaState, "Vec3_Add", [](lua_State* L) -> int {
            glm::vec3 a = ToVec3(L, 1);
            glm::vec3 b = ToVec3(L, 2);
            PushVec3(L, a + b);
            return 1;
        });
        
        lua_register(s_LuaState, "Vec3_Sub", [](lua_State* L) -> int {
            glm::vec3 a = ToVec3(L, 1);
            glm::vec3 b = ToVec3(L, 2);
            PushVec3(L, a - b);
            return 1;
        });
        
        lua_register(s_LuaState, "Vec3_Mul", [](lua_State* L) -> int {
            glm::vec3 a = ToVec3(L, 1);
            float scalar = static_cast<float>(luaL_checknumber(L, 2));
            PushVec3(L, a * scalar);
            return 1;
        });
        
        lua_register(s_LuaState, "Vec3_Dot", [](lua_State* L) -> int {
            glm::vec3 a = ToVec3(L, 1);
            glm::vec3 b = ToVec3(L, 2);
            lua_pushnumber(L, glm::dot(a, b));
            return 1;
        });
        
        lua_register(s_LuaState, "Vec3_Cross", [](lua_State* L) -> int {
            glm::vec3 a = ToVec3(L, 1);
            glm::vec3 b = ToVec3(L, 2);
            PushVec3(L, glm::cross(a, b));
            return 1;
        });
        
        lua_register(s_LuaState, "Vec3_Normalize", [](lua_State* L) -> int {
            glm::vec3 v = ToVec3(L, 1);
            float len = glm::length(v);
            if (len > 0.0001f) {
                PushVec3(L, v / len);
            } else {
                PushVec3(L, glm::vec3(0));
            }
            return 1;
        });
        
        lua_register(s_LuaState, "Vec3_Length", [](lua_State* L) -> int {
            glm::vec3 v = ToVec3(L, 1);
            lua_pushnumber(L, glm::length(v));
            return 1;
        });
        
        lua_register(s_LuaState, "Vec3_Distance", [](lua_State* L) -> int {
            glm::vec3 a = ToVec3(L, 1);
            glm::vec3 b = ToVec3(L, 2);
            lua_pushnumber(L, glm::distance(a, b));
            return 1;
        });
        
        lua_register(s_LuaState, "Vec3_Lerp", [](lua_State* L) -> int {
            glm::vec3 a = ToVec3(L, 1);
            glm::vec3 b = ToVec3(L, 2);
            float t = static_cast<float>(luaL_checknumber(L, 3));
            PushVec3(L, glm::mix(a, b, t));
            return 1;
        });
        
        // ==================== Quat ====================
        lua_register(s_LuaState, "Quat", [](lua_State* L) -> int {
            // Quat() or Quat(x, y, z, w)
            if (lua_gettop(L) == 0) {
                PushQuat(L, glm::quat(1, 0, 0, 0));
            } else {
                float x = static_cast<float>(luaL_checknumber(L, 1));
                float y = static_cast<float>(luaL_checknumber(L, 2));
                float z = static_cast<float>(luaL_checknumber(L, 3));
                float w = static_cast<float>(luaL_checknumber(L, 4));
                PushQuat(L, glm::quat(w, x, y, z));
            }
            return 1;
        });
        
        lua_register(s_LuaState, "Quat_FromEuler", [](lua_State* L) -> int {
            glm::vec3 euler = ToVec3(L, 1);
            PushQuat(L, glm::quat(glm::radians(euler)));
            return 1;
        });
        
        lua_register(s_LuaState, "Quat_ToEuler", [](lua_State* L) -> int {
            glm::quat q = ToQuat(L, 1);
            PushVec3(L, glm::degrees(glm::eulerAngles(q)));
            return 1;
        });
        
        lua_register(s_LuaState, "Quat_FromAxisAngle", [](lua_State* L) -> int {
            glm::vec3 axis = ToVec3(L, 1);
            float angle = static_cast<float>(luaL_checknumber(L, 2));
            PushQuat(L, glm::angleAxis(glm::radians(angle), glm::normalize(axis)));
            return 1;
        });
        
        lua_register(s_LuaState, "Quat_Slerp", [](lua_State* L) -> int {
            glm::quat a = ToQuat(L, 1);
            glm::quat b = ToQuat(L, 2);
            float t = static_cast<float>(luaL_checknumber(L, 3));
            PushQuat(L, glm::slerp(a, b, t));
            return 1;
        });
        
        lua_register(s_LuaState, "Quat_Mul", [](lua_State* L) -> int {
            glm::quat a = ToQuat(L, 1);
            glm::quat b = ToQuat(L, 2);
            PushQuat(L, a * b);
            return 1;
        });
        
        lua_register(s_LuaState, "Quat_RotateVec3", [](lua_State* L) -> int {
            glm::quat q = ToQuat(L, 1);
            glm::vec3 v = ToVec3(L, 2);
            PushVec3(L, q * v);
            return 1;
        });
        
        // ==================== Math Utilities ====================
        lua_register(s_LuaState, "Math_Clamp", [](lua_State* L) -> int {
            float value = static_cast<float>(luaL_checknumber(L, 1));
            float min = static_cast<float>(luaL_checknumber(L, 2));
            float max = static_cast<float>(luaL_checknumber(L, 3));
            lua_pushnumber(L, glm::clamp(value, min, max));
            return 1;
        });
        
        lua_register(s_LuaState, "Math_Lerp", [](lua_State* L) -> int {
            float a = static_cast<float>(luaL_checknumber(L, 1));
            float b = static_cast<float>(luaL_checknumber(L, 2));
            float t = static_cast<float>(luaL_checknumber(L, 3));
            lua_pushnumber(L, glm::mix(a, b, t));
            return 1;
        });
        
        lua_register(s_LuaState, "Math_Sin", [](lua_State* L) -> int {
            lua_pushnumber(L, std::sin(luaL_checknumber(L, 1)));
            return 1;
        });
        
        lua_register(s_LuaState, "Math_Cos", [](lua_State* L) -> int {
            lua_pushnumber(L, std::cos(luaL_checknumber(L, 1)));
            return 1;
        });
        
        lua_register(s_LuaState, "Math_Abs", [](lua_State* L) -> int {
            lua_pushnumber(L, std::abs(luaL_checknumber(L, 1)));
            return 1;
        });
        
        lua_register(s_LuaState, "Math_Random", [](lua_State* L) -> int {
            float min = static_cast<float>(luaL_optnumber(L, 1, 0.0));
            float max = static_cast<float>(luaL_optnumber(L, 2, 1.0));
            float random = min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
            lua_pushnumber(L, random);
            return 1;
        });
        
        GE_CORE_DEBUG("Lua math bindings registered");
    }

    // Helper function for getprops (introspection)
    static int LuaGetProps(lua_State* L) {
        if (lua_gettop(L) == 0) {
            GE_CORE_INFO("Lua: getprops() - No object provided");
            return 0;
        }
        
        // Get object from stack
        int objIndex = 1;
        std::stringstream ss;
        ss << "Lua: getprops() - Object type: ";
        
        if (lua_istable(L, objIndex)) {
            ss << "table\n";
            lua_pushnil(L);
            while (lua_next(L, objIndex) != 0) {
                const char* key = lua_tostring(L, -2);
                const char* valueType = lua_typename(L, lua_type(L, -1));
                ss << "  " << key << ": " << valueType << "\n";
                lua_pop(L, 1);
            }
        } else if (lua_isuserdata(L, objIndex)) {
            ss << "userdata\n";
        } else {
            ss << lua_typename(L, lua_type(L, objIndex)) << "\n";
        }
        
        GE_CORE_INFO(ss.str());
        return 0;
    }

    void ScriptEngine::RegisterApplicationAPI() {
        // Create application table
        lua_newtable(s_LuaState);
        
        // application.SetActivePath(path, fadeSeconds)
        lua_pushcfunction(s_LuaState, [](lua_State* L) -> int {
            if (lua_gettop(L) < 1) {
                GE_CORE_ERROR("Lua: application.SetActivePath requires at least 1 argument");
                return 0;
            }
            
            // Get render path from userdata or registry
            Ref<RenderPath> path;
            if (lua_isuserdata(L, 1)) {
                // Try to get from userdata
                void* ud = lua_touserdata(L, 1);
                // For now, we'll use a simpler approach with registry IDs
            } else if (lua_isnumber(L, 1)) {
                int id = static_cast<int>(lua_tonumber(L, 1));
                auto it = s_RenderPathRegistry.find(id);
                if (it != s_RenderPathRegistry.end()) {
                    path = it->second;
                }
            }
            
            float fadeSeconds = 0.0f;
            if (lua_gettop(L) > 1 && lua_isnumber(L, 2)) {
                fadeSeconds = static_cast<float>(lua_tonumber(L, 2));
            }
            
            if (path) {
                auto& app = Application::Get();
                app.ActivatePath(path, fadeSeconds);
            } else {
                GE_CORE_WARN("Lua: application.SetActivePath - Invalid render path");
            }
            
            return 0;
        });
        lua_setfield(s_LuaState, -2, "SetActivePath");
        
        // application.GetActivePath()
        lua_pushcfunction(s_LuaState, [](lua_State* L) -> int {
            auto& app = Application::Get();
            auto path = app.GetActiveRenderPath();
            
            if (path) {
                // Store in registry and return ID
                int id = ++s_RenderPathCounter;
                s_RenderPathRegistry[id] = path;
                lua_pushnumber(L, id);
                return 1;
            }
            
            lua_pushnil(L);
            return 1;
        });
        lua_setfield(s_LuaState, -2, "GetActivePath");
        
        // Set as global
        lua_setglobal(s_LuaState, "application");
        
        GE_CORE_DEBUG("Lua application API registered");
        
        // Register RenderPath3D type
        lua_newtable(s_LuaState);
        
        // RenderPath3D.new()
        lua_pushcfunction(s_LuaState, [](lua_State* L) -> int {
            auto path = CreateRef<RenderPath3D>();
            int id = ++s_RenderPathCounter;
            s_RenderPathRegistry[id] = path;
            
            lua_newtable(L);
            lua_pushnumber(L, id);
            lua_setfield(L, -2, "id");
            
            // Add methods
            lua_pushcfunction(L, [](lua_State* L) -> int {
                lua_getfield(L, 1, "id");
                int id = static_cast<int>(lua_tonumber(L, -1));
                lua_pop(L, 1);
                
                auto it = s_RenderPathRegistry.find(id);
                if (it != s_RenderPathRegistry.end()) {
                    auto path3D = std::dynamic_pointer_cast<RenderPath3D>(it->second);
                    if (path3D) {
                        bool enabled = lua_toboolean(L, 2) != 0;
                        path3D->SetFXAAEnabled(enabled);
                    }
                }
                return 0;
            });
            lua_setfield(L, -2, "setFXAAEnabled");
            
            lua_pushcfunction(L, [](lua_State* L) -> int {
                lua_getfield(L, 1, "id");
                int id = static_cast<int>(lua_tonumber(L, -1));
                lua_pop(L, 1);
                
                auto it = s_RenderPathRegistry.find(id);
                if (it != s_RenderPathRegistry.end()) {
                    auto path3D = std::dynamic_pointer_cast<RenderPath3D>(it->second);
                    if (path3D) {
                        bool enabled = lua_toboolean(L, 2) != 0;
                        path3D->SetBloomEnabled(enabled);
                    }
                }
                return 0;
            });
            lua_setfield(L, -2, "setBloomEnabled");
            
            lua_pushcfunction(L, [](lua_State* L) -> int {
                lua_getfield(L, 1, "id");
                int id = static_cast<int>(lua_tonumber(L, -1));
                lua_pop(L, 1);
                
                auto it = s_RenderPathRegistry.find(id);
                if (it != s_RenderPathRegistry.end()) {
                    auto path3D = std::dynamic_pointer_cast<RenderPath3D>(it->second);
                    if (path3D) {
                        bool enabled = lua_toboolean(L, 2) != 0;
                        path3D->SetSSAOEnabled(enabled);
                    }
                }
                return 0;
            });
            lua_setfield(L, -2, "setSSAOEnabled");
            
            return 1;
        });
        lua_setfield(s_LuaState, -2, "new");
        lua_setglobal(s_LuaState, "RenderPath3D");
        
        // Register RenderPath2D type
        lua_newtable(s_LuaState);
        
        // RenderPath2D.new()
        lua_pushcfunction(s_LuaState, [](lua_State* L) -> int {
            auto path = CreateRef<RenderPath2D>();
            int id = ++s_RenderPathCounter;
            s_RenderPathRegistry[id] = path;
            
            lua_newtable(L);
            lua_pushnumber(L, id);
            lua_setfield(L, -2, "id");
            return 1;
        });
        lua_setfield(s_LuaState, -2, "new");
        lua_setglobal(s_LuaState, "RenderPath2D");
    }

    void ScriptEngine::RegisterSceneAPI() {
        // Create scene table
        lua_newtable(s_LuaState);
        
        // scene.GetScene() 
        lua_pushcfunction(s_LuaState, [](lua_State* L) -> int {
            auto scene = SceneHelpers::GetScene();
            if (scene) {
                // Store in registry and return ID
                int id = ++s_SceneCounter;
                s_SceneRegistry[id] = scene;
                
                // Return table with scene ID
                lua_newtable(L);
                lua_pushnumber(L, id);
                lua_setfield(L, -2, "id");
                lua_pushstring(L, scene->GetName().c_str());
                lua_setfield(L, -2, "name");
                return 1;
            }
            lua_pushnil(L);
            return 1;
        });
        lua_setfield(s_LuaState, -2, "GetScene");
        
        // scene.LoadModel(path) or scene.LoadModel(scene, path)
        lua_pushcfunction(s_LuaState, [](lua_State* L) -> int {
            int top = lua_gettop(L);
            Entity entity;
            
            if (top == 1) {
                // LoadModel(path) - load into global scene
                const char* path = luaL_checkstring(L, 1);
                entity = SceneHelpers::LoadModel(path);
            } else if (top == 2) {
                // LoadModel(scene, path)
                if (lua_istable(L, 1)) {
                    lua_getfield(L, 1, "id");
                    if (lua_isnumber(L, -1)) {
                        int sceneId = static_cast<int>(lua_tonumber(L, -1));
                        lua_pop(L, 1);
                        
                        auto it = s_SceneRegistry.find(sceneId);
                        if (it != s_SceneRegistry.end()) {
                            const char* path = luaL_checkstring(L, 2);
                            entity = SceneHelpers::LoadModel(it->second, path);
                        }
                    } else {
                        lua_pop(L, 1);
                    }
                }
            }
            
            if (entity.IsValid()) {
                // Return entity ID (UUID has operator uint64_t())
                lua_pushnumber(L, static_cast<uint64_t>(entity.GetUUID()));
                return 1;
            }
            
            lua_pushnil(L);
            return 1;
        });
        lua_setfield(s_LuaState, -2, "LoadModel");
        
        // Set as global
        lua_setglobal(s_LuaState, "scene");
        
        GE_CORE_DEBUG("Lua scene API registered");
    }

    void ScriptEngine::RegisterAudioAPI() {
        // Create audio table
        lua_newtable(s_LuaState);
        
        // audio.CreateSound(path)
        lua_pushcfunction(s_LuaState, [](lua_State* L) -> int {
            const char* path = luaL_checkstring(L, 1);
            AudioHelpers::Sound sound;
            if (AudioHelpers::CreateSound(path, sound)) {
                // Store in registry and return ID
                int id = ++s_SoundCounter;
                s_SoundRegistry[id] = sound;
                
                lua_newtable(L);
                lua_pushnumber(L, id);
                lua_setfield(L, -2, "id");
                lua_pushstring(L, path);
                lua_setfield(L, -2, "path");
                return 1;
            }
            lua_pushnil(L);
            return 1;
        });
        lua_setfield(s_LuaState, -2, "CreateSound");
        
        // audio.CreateSoundInstance(sound)
        lua_pushcfunction(s_LuaState, [](lua_State* L) -> int {
            if (!lua_istable(L, 1)) {
                GE_CORE_ERROR("Lua: audio.CreateSoundInstance requires sound table");
                lua_pushnil(L);
                return 1;
            }
            
            lua_getfield(L, 1, "id");
            if (!lua_isnumber(L, -1)) {
                lua_pop(L, 1);
                lua_pushnil(L);
                return 1;
            }
            
            int soundId = static_cast<int>(lua_tonumber(L, -1));
            lua_pop(L, 1);
            
            auto soundIt = s_SoundRegistry.find(soundId);
            if (soundIt == s_SoundRegistry.end()) {
                lua_pushnil(L);
                return 1;
            }
            
            AudioHelpers::SoundInstance instance;
            if (AudioHelpers::CreateSoundInstance(soundIt->second, instance)) {
                int id = ++s_SoundInstanceCounter;
                s_SoundInstanceRegistry[id] = instance;
                
                lua_newtable(L);
                lua_pushnumber(L, id);
                lua_setfield(L, -2, "id");
                return 1;
            }
            
            lua_pushnil(L);
            return 1;
        });
        lua_setfield(s_LuaState, -2, "CreateSoundInstance");
        
        // audio.Play(instance)
        lua_pushcfunction(s_LuaState, [](lua_State* L) -> int {
            if (!lua_istable(L, 1)) {
                return 0;
            }
            
            lua_getfield(L, 1, "id");
            if (!lua_isnumber(L, -1)) {
                lua_pop(L, 1);
                return 0;
            }
            
            int instanceId = static_cast<int>(lua_tonumber(L, -1));
            lua_pop(L, 1);
            
            auto it = s_SoundInstanceRegistry.find(instanceId);
            if (it != s_SoundInstanceRegistry.end()) {
                AudioHelpers::Play(it->second);
            }
            return 0;
        });
        lua_setfield(s_LuaState, -2, "Play");
        
        // audio.Stop(instance)
        lua_pushcfunction(s_LuaState, [](lua_State* L) -> int {
            if (!lua_istable(L, 1)) {
                return 0;
            }
            
            lua_getfield(L, 1, "id");
            if (!lua_isnumber(L, -1)) {
                lua_pop(L, 1);
                return 0;
            }
            
            int instanceId = static_cast<int>(lua_tonumber(L, -1));
            lua_pop(L, 1);
            
            auto it = s_SoundInstanceRegistry.find(instanceId);
            if (it != s_SoundInstanceRegistry.end()) {
                AudioHelpers::Stop(it->second);
            }
            return 0;
        });
        lua_setfield(s_LuaState, -2, "Stop");
        
        // audio.SetVolume(volume, instance?)
        lua_pushcfunction(s_LuaState, [](lua_State* L) -> int {
            float volume = static_cast<float>(luaL_checknumber(L, 1));
            AudioHelpers::SoundInstance* instance = nullptr;
            
            if (lua_gettop(L) > 1 && !lua_isnil(L, 2) && lua_istable(L, 2)) {
                lua_getfield(L, 2, "id");
                if (lua_isnumber(L, -1)) {
                    int instanceId = static_cast<int>(lua_tonumber(L, -1));
                    lua_pop(L, 1);
                    
                    auto it = s_SoundInstanceRegistry.find(instanceId);
                    if (it != s_SoundInstanceRegistry.end()) {
                        instance = &it->second;
                    }
                } else {
                    lua_pop(L, 1);
                }
            }
            
            AudioHelpers::SetVolume(volume, instance);
            return 0;
        });
        lua_setfield(s_LuaState, -2, "SetVolume");
        
        // Set as global
        lua_setglobal(s_LuaState, "audio");
        
        GE_CORE_DEBUG("Lua audio API registered");
    }

    void ScriptEngine::RegisterInputAPI() {
        // Create input table
        lua_newtable(s_LuaState);
        
        // input.Press(key)
        lua_pushcfunction(s_LuaState, [](lua_State* L) -> int {
            int key = static_cast<int>(luaL_checknumber(L, 1));
            bool pressed = InputHelpers::Press(static_cast<KeyCode>(key));
            lua_pushboolean(L, pressed);
            return 1;
        });
        lua_setfield(s_LuaState, -2, "Press");
        
        // input.Down(key)
        lua_pushcfunction(s_LuaState, [](lua_State* L) -> int {
            int key = static_cast<int>(luaL_checknumber(L, 1));
            bool down = InputHelpers::Down(static_cast<KeyCode>(key));
            lua_pushboolean(L, down);
            return 1;
        });
        lua_setfield(s_LuaState, -2, "Down");
        
        // Key constants
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_SPACE));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_SPACE");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_LEFT));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_LEFT");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_RIGHT));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_RIGHT");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_UP));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_UP");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_DOWN));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_DOWN");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_ESCAPE));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_ESCAPE");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_ENTER));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_ENTER");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_TAB));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_TAB");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_BACKSPACE));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_BACKSPACE");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_DELETE));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_DELETE");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_HOME));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_HOME");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_END));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_END");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_PAGEUP));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_PAGEUP");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_PAGEDOWN));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_PAGEDOWN");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_F1));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_F1");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_F2));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_F2");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_F3));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_F3");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_F4));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_F4");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_F5));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_F5");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_F6));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_F6");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_F7));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_F7");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_F8));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_F8");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_F9));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_F9");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_F10));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_F10");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_F11));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_F11");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::KEYBOARD_BUTTON_F12));
        lua_setfield(s_LuaState, -2, "KEYBOARD_BUTTON_F12");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::MOUSE_BUTTON_LEFT));
        lua_setfield(s_LuaState, -2, "MOUSE_BUTTON_LEFT");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::MOUSE_BUTTON_RIGHT));
        lua_setfield(s_LuaState, -2, "MOUSE_BUTTON_RIGHT");
        lua_pushnumber(s_LuaState, static_cast<int>(InputHelpers::Keys::MOUSE_BUTTON_MIDDLE));
        lua_setfield(s_LuaState, -2, "MOUSE_BUTTON_MIDDLE");
        
        // Set as global
        lua_setglobal(s_LuaState, "input");
        
        GE_CORE_DEBUG("Lua input API registered");
    }

    bool ScriptEngine::LoadStartupScript() {
        // Look for startup.lua in working directory or Scripts folder
        std::vector<std::string> paths = {
            "startup.lua",
            "Scripts/startup.lua",
            "Assets/Scripts/startup.lua"
        };
        
        for (const auto& path : paths) {
            if (FileSystem::Exists(path)) {
                GE_CORE_INFO("Loading startup script: {0}", path);
                return LoadScript(path);
            }
        }
        
        return true; // Not an error if startup.lua doesn't exist
    }

}