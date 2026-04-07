#pragma once
#include <random>
#include <cstdint>

// Random number generation utilities
namespace engine::core::rng {

    // Base class for RNG generators
    class NumberGenerator {
    protected:
        std::random_device rd;
        std::mt19937 gen;
        NumberGenerator() : gen(rd()) {}
    };

    // Floating-point RNG
    template<typename T>
    class RealNumberGenerator : public NumberGenerator {
    private:
        std::uniform_real_distribution<T> dis;
    public:
        RealNumberGenerator() : NumberGenerator(), dis(0.0, 1.0) {}
        RealNumberGenerator(const RealNumberGenerator<T>& other) : NumberGenerator(), dis(other.dis) {}

        T get() { return dis(gen); }
        T getUnder(T max) { return get() * max; }
        T getRange(T min, T max) { return min + get() * (max - min); }
        T getRange(T width) { return getRange(-width * static_cast<T>(0.5), width * static_cast<T>(0.5)); }
    };

    // Singleton-like access to real RNG
    template<typename T>
    class RNG {
    private:
        static RealNumberGenerator<T> gen;
    public:
        static T get() { return gen.get(); }
        static T getUnder(T max) { return gen.getUnder(max); }
        static T getRange(T min, T max) { return gen.getRange(min, max); }
        static T getRange(T width) { return gen.getRange(width); }
        static bool proba(T threshold) { return get() < threshold; }
    };

    template<typename T>
    RealNumberGenerator<T> RNG<T>::gen;

    // Integer RNG
    template<typename T>
    class IntegerNumberGenerator : public NumberGenerator {
    public:
        IntegerNumberGenerator() : NumberGenerator() {}
        IntegerNumberGenerator(const IntegerNumberGenerator<T>& other) : NumberGenerator() {}

        T getUnder(T max) {
            std::uniform_int_distribution<T> dist(0, max);
            return dist(gen);
        }
        T getRange(T min, T max) {
            std::uniform_int_distribution<T> dist(min, max);
            return dist(gen);
        }
    };

    // Singleton-style int RNG wrapper
    template<typename T>
    class RNGi {
    private:
        static IntegerNumberGenerator<T> gen;
    public:
        static T getUnder(T max) { return gen.getUnder(max); }
        static T getRange(T min, T max) { return gen.getRange(min, max); }
    };

    template<typename T>
    IntegerNumberGenerator<T> RNGi<T>::gen;

    // Convenience aliases
    using RNGf   = RNG<float>;
    using RNGi32 = RNGi<int32_t>;
    using RNGi64 = RNGi<int64_t>;
    using RNGu32 = RNGi<uint32_t>;
    using RNGu64 = RNGi<uint64_t>;

} // namespace engine::core::rng

