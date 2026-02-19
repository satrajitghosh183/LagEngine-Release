#pragma once
#include <random>
#include <cstdint>

namespace RNG {

    // Base class for random number generators.
    class NumberGenerator {
    protected:
        std::random_device rd;
        std::mt19937 gen;

        NumberGenerator() : gen(rd()) {}
    };

    // Generator for real (floating-point) numbers.
    template<typename T>
    class RealNumberGenerator : public NumberGenerator {
    private:
        std::uniform_real_distribution<T> dis;
    public:
        RealNumberGenerator()
            : NumberGenerator(), dis(0.0, 1.0) {}

        // Copy constructor (reinitializes the generator since std::random_device is non-copyable).
        RealNumberGenerator(const RealNumberGenerator<T>& other)
            : NumberGenerator(), dis(other.dis) {}

        // Returns a random value in the range [0, 1).
        T get() {
            return dis(gen);
        }

        // Returns a random value in the range [0, max).
        T getUnder(T max) {
            return get() * max;
        }

        // Returns a random value in the range [min, max).
        T getRange(T min, T max) {
            return min + get() * (max - min);
        }

        // Returns a random value in the range [-width/2, width/2).
        T getRange(T width) {
            return getRange(-width * static_cast<T>(0.5), width * static_cast<T>(0.5));
        }
    };

    // Singleton-style accessor for floating-point random numbers.
    template<typename T>
    class RNG {
    private:
        static RealNumberGenerator<T> gen;
    public:
        // Returns a random value in [0, 1).
        static T get() {
            return gen.get();
        }
        // Returns a random value in [0, max).
        static T getUnder(T max) {
            return gen.getUnder(max);
        }
        // Returns a random value in [min, max).
        static T getRange(T min, T max) {
            return gen.getRange(min, max);
        }
        // Returns a random value in [-width/2, width/2).
        static T getRange(T width) {
            return gen.getRange(width);
        }
        // Returns true with probability equal to threshold.
        static bool proba(T threshold) {
            return get() < threshold;
        }
    };

    // Define static member.
    template<typename T>
    RealNumberGenerator<T> RNG<T>::gen = RealNumberGenerator<T>();

    // Generator for integer numbers.
    template<typename T>
    class IntegerNumberGenerator : public NumberGenerator {
    public:
        IntegerNumberGenerator() : NumberGenerator() {}

        IntegerNumberGenerator(const IntegerNumberGenerator<T>& other)
            : NumberGenerator() {}

        // Returns a random integer in the range [0, max].
        T getUnder(T max) {
            std::uniform_int_distribution<T> dist(0, max);
            return dist(gen);
        }

        // Returns a random integer in the range [min, max].
        T getRange(T min, T max) {
            std::uniform_int_distribution<T> dist(min, max);
            return dist(gen);
        }
    };

    // Singleton-style accessor for integer random numbers.
    template<typename T>
    class RNGi {
    private:
        static IntegerNumberGenerator<T> gen;
    public:
        // Returns a random integer in the range [0, max].
        static T getUnder(T max) {
            return gen.getUnder(max);
        }
        // Returns a random integer in the range [min, max].
        static T getRange(T min, T max) {
            return gen.getRange(min, max);
        }
    };

    // Define static member for integer RNG.
    template<typename T>
    IntegerNumberGenerator<T> RNGi<T>::gen;

    // Type aliases for convenience.
    using RNGf   = RNG<float>;
    using RNGi32 = RNGi<int32_t>;
    using RNGi64 = RNGi<int64_t>;
    using RNGu32 = RNGi<uint32_t>;
    using RNGu64 = RNGi<uint64_t>;

} // namespace RNG
