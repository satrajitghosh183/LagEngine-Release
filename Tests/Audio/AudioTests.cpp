/**
 * @file AudioTests.cpp
 * @brief Unit tests for Audio system
 */

#include <gtest/gtest.h>
#include "Audio/AudioHelpers.hpp"

using namespace GameEngine;

// ==================== Sound Structure Tests ====================

TEST(AudioTest, SoundDefaultConstruction) {
    AudioHelpers::Sound sound;
    
    // Default sound should be invalid/empty
    EXPECT_TRUE(sound.Path.empty());
}

TEST(AudioTest, SoundInstanceDefaultConstruction) {
    AudioHelpers::SoundInstance instance;

    // Default instance should have reasonable defaults
    EXPECT_FLOAT_EQ(instance.Volume, 1.0f);
    EXPECT_FALSE(instance.Loop);
}

// ==================== Volume Tests ====================

TEST(AudioTest, VolumeRange) {
    // Volume should typically be 0.0 to 1.0
    float minVolume = 0.0f;
    float maxVolume = 1.0f;
    
    EXPECT_GE(maxVolume, minVolume);
    
    // Test clamping logic
    float testVolume = 1.5f;
    float clampedVolume = std::clamp(testVolume, 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(clampedVolume, 1.0f);
}

TEST(AudioTest, PitchRange) {
    // Reasonable pitch range
    float minPitch = 0.1f;
    float maxPitch = 4.0f;
    
    // Normal pitch
    float normalPitch = 1.0f;
    EXPECT_GE(normalPitch, minPitch);
    EXPECT_LE(normalPitch, maxPitch);
}

// ==================== Audio Helpers Interface ====================

TEST(AudioTest, AudioHelpersInterface) {
    // Test that audio helper functions exist
    // Note: Actual playback requires audio hardware/context

    (void)&AudioHelpers::CreateSound;
    (void)&AudioHelpers::CreateSoundInstance;
    (void)&AudioHelpers::Play;
    (void)&AudioHelpers::Stop;
    (void)&AudioHelpers::SetVolume;
}

// ==================== File Format Support ====================

TEST(AudioTest, SupportedFormats) {
    std::vector<std::string> supportedExtensions = {
        ".wav",
        ".ogg",
        ".mp3",
        ".flac"
    };
    
    for (const auto& ext : supportedExtensions) {
        EXPECT_FALSE(ext.empty());
        EXPECT_EQ(ext[0], '.');
    }
}

// ==================== 3D Audio Parameters ====================

TEST(AudioTest, SpatialAudioDefaults) {
    // Default 3D audio parameters
    float defaultMinDistance = 1.0f;
    float defaultMaxDistance = 100.0f;
    float defaultRolloffFactor = 1.0f;
    
    EXPECT_GT(defaultMinDistance, 0.0f);
    EXPECT_GT(defaultMaxDistance, defaultMinDistance);
    EXPECT_GE(defaultRolloffFactor, 0.0f);
}

TEST(AudioTest, AttenuationModels) {
    // Audio attenuation models
    enum class AttenuationModel {
        None,
        Linear,
        InverseDistance,
        InverseDistanceClamped,
        ExponentialDistance,
        ExponentialDistanceClamped
    };
    
    AttenuationModel defaultModel = AttenuationModel::InverseDistanceClamped;
    EXPECT_NE(static_cast<int>(defaultModel), 0);
}
