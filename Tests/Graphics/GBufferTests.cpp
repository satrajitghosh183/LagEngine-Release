/**
 * @file GBufferTests.cpp
 * @brief Unit tests for GBuffer configuration
 *
 * Note: Full GBuffer functionality requires an OpenGL context.
 * These tests verify configuration and state management only.
 */

#include <gtest/gtest.h>
#include "Graphics/GBuffer.hpp"

using namespace GameEngine;

TEST(GBuffer, DefaultDimensions) {
    // GBuffer should store dimensions
    GBuffer gbuffer(1920, 1080);
    EXPECT_EQ(gbuffer.GetWidth(), 1920);
    EXPECT_EQ(gbuffer.GetHeight(), 1080);
}

TEST(GBuffer, ResizeUpdatesDimensions) {
    GBuffer gbuffer(800, 600);
    gbuffer.Resize(1920, 1080);
    EXPECT_EQ(gbuffer.GetWidth(), 1920);
    EXPECT_EQ(gbuffer.GetHeight(), 1080);
}
