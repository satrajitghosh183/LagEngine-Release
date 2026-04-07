#pragma once

#include <memory>
#include <string>
#include <functional>

// Platform detection
#ifdef _WIN32
    #ifdef _WIN64
        #define GE_PLATFORM_WINDOWS
    #else
        #error "32-bit Windows is not supported!"
    #endif
#elif defined(__APPLE__) || defined(__MACH__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC == 1
        #define GE_PLATFORM_MACOS
    #else
        #error "Unsupported Apple platform!"
    #endif
#elif defined(__linux__)
    #define GE_PLATFORM_LINUX
#else
    #error "Unknown platform!"
#endif

// Debug break
#ifdef GE_DEBUG
    #if defined(GE_PLATFORM_WINDOWS)
        #define GE_DEBUGBREAK() __debugbreak()
    #elif defined(GE_PLATFORM_LINUX) || defined(GE_PLATFORM_MACOS)
        #include <signal.h>
        #define GE_DEBUGBREAK() raise(SIGTRAP)
    #else
        #define GE_DEBUGBREAK()
    #endif
    #define GE_ENABLE_ASSERTS
#else
    #define GE_DEBUGBREAK()
#endif

// Assertions
#ifdef GE_ENABLE_ASSERTS
    #define GE_ASSERT(x, ...) { if(!(x)) { GE_ERROR("Assertion Failed: {0}", __VA_ARGS__); GE_DEBUGBREAK(); } }
    #define GE_CORE_ASSERT(x, ...) { if(!(x)) { GE_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); GE_DEBUGBREAK(); } }
#else
    #define GE_ASSERT(x, ...)
    #define GE_CORE_ASSERT(x, ...)
#endif

// Bit manipulation
#define BIT(x) (1 << x)

// Bind event function
#define GE_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

// Smart pointers
namespace GameEngine {
    template<typename T>
    using Scope = std::unique_ptr<T>;
    
    template<typename T, typename ... Args>
    constexpr Scope<T> CreateScope(Args&& ... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
    
    template<typename T>
    using Ref = std::shared_ptr<T>;
    
    template<typename T, typename ... Args>
    constexpr Ref<T> CreateRef(Args&& ... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
}