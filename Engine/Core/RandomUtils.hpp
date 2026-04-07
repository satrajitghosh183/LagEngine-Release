#pragma once

#include <random>
#include <glm/glm.hpp>

namespace GameEngine {
namespace Random {

    /**
     * @brief Random number generator
     */
    class RandomGenerator {
    public:
        RandomGenerator();
        explicit RandomGenerator(uint32_t seed);
        
        /**
         * @brief Set seed
         */
        void SetSeed(uint32_t seed);
        
        /**
         * @brief Random float [0, 1)
         */
        float Float();
        
        /**
         * @brief Random float [min, max)
         */
        float Range(float min, float max);
        
        /**
         * @brief Random integer [min, max]
         */
        int Int(int min, int max);
        
        /**
         * @brief Random boolean
         */
        bool Bool();
        
        /**
         * @brief Random point in unit circle
         */
        glm::vec2 InsideUnitCircle();
        
        /**
         * @brief Random point on unit circle
         */
        glm::vec2 OnUnitCircle();
        
        /**
         * @brief Random point in unit sphere
         */
        glm::vec3 InsideUnitSphere();
        
        /**
         * @brief Random point on unit sphere
         */
        glm::vec3 OnUnitSphere();
        
        /**
         * @brief Random RGB color
         */
        glm::vec3 Color();
        
        /**
         * @brief Random RGBA color
         */
        glm::vec4 ColorWithAlpha();
        
        /**
         * @brief Random rotation (quaternion)
         */
        glm::quat Rotation();
        
    private:
        std::mt19937 m_Engine;
        std::uniform_real_distribution<float> m_Distribution;
    };

    /**
     * @brief Global random generator
     */
    extern RandomGenerator Global;

    /**
     * @brief Quick random float [0, 1)
     */
    inline float Float() { return Global.Float(); }
    
    /**
     * @brief Quick random float [min, max)
     */
    inline float Range(float min, float max) { return Global.Range(min, max); }
    
    /**
     * @brief Quick random integer [min, max]
     */
    inline int Int(int min, int max) { return Global.Int(min, max); }
    
    /**
     * @brief Quick random boolean
     */
    inline bool Bool() { return Global.Bool(); }

}}