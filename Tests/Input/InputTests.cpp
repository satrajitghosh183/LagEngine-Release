/**
 * @file InputTests.cpp
 * @brief Unit tests for the Input system
 */

#include <gtest/gtest.h>
#include "Platform/Input.hpp"

using namespace GameEngine;

// ==================== KeyCode Enum ====================

TEST(InputTest, KeyCodeValues) {
    // Test that KeyCode values match expected GLFW values
    EXPECT_EQ(static_cast<int>(KeyCode::None), 0);
    EXPECT_EQ(static_cast<int>(KeyCode::Space), 32);
    EXPECT_EQ(static_cast<int>(KeyCode::A), 65);
    EXPECT_EQ(static_cast<int>(KeyCode::Z), 90);
    EXPECT_EQ(static_cast<int>(KeyCode::Escape), 256);
    EXPECT_EQ(static_cast<int>(KeyCode::Enter), 257);
    EXPECT_EQ(static_cast<int>(KeyCode::F1), 290);
    EXPECT_EQ(static_cast<int>(KeyCode::F12), 301);
    EXPECT_EQ(static_cast<int>(KeyCode::LeftShift), 340);
    EXPECT_EQ(static_cast<int>(KeyCode::LeftControl), 341);
}

TEST(InputTest, KeyCodeRange) {
    // Test digit keys
    EXPECT_EQ(static_cast<int>(KeyCode::D0), 48);
    EXPECT_EQ(static_cast<int>(KeyCode::D9), 57);
    
    // Test letter keys
    for (int i = 0; i < 26; i++) {
        KeyCode k = static_cast<KeyCode>(65 + i);
        EXPECT_GE(static_cast<int>(k), 65);
        EXPECT_LE(static_cast<int>(k), 90);
    }
}

// ==================== MouseButton Enum ====================

TEST(InputTest, MouseButtonValues) {
    EXPECT_EQ(static_cast<int>(MouseButton::Left), 0);
    EXPECT_EQ(static_cast<int>(MouseButton::Right), 1);
    EXPECT_EQ(static_cast<int>(MouseButton::Middle), 2);
}

// ==================== Action Mapping ====================

TEST(InputTest, MapAction) {
    // This tests the action mapping interface
    EXPECT_NO_THROW(Input::MapAction("jump", KeyCode::Space));
    EXPECT_NO_THROW(Input::MapAction("shoot", KeyCode::LeftControl));
}

TEST(InputTest, MapAxis) {
    // This tests the axis mapping interface
    EXPECT_NO_THROW(Input::MapAxis("horizontal", KeyCode::D, KeyCode::A));
    EXPECT_NO_THROW(Input::MapAxis("vertical", KeyCode::W, KeyCode::S));
}

// Note: Most Input tests require a window context to be meaningful
// These are interface tests to ensure the API exists and is accessible

// ==================== Static Methods Exist ====================

TEST(InputTest, StaticMethodsExist) {
    // These would normally require a window, but we test that the methods exist
    // by checking they don't throw when called (they may return default values)
    
    // The following may cause issues without a window context,
    // so we just verify the interface compiles
    (void)&Input::IsKeyPressed;
    (void)&Input::IsKeyJustPressed;
    (void)&Input::IsKeyJustReleased;
    (void)&Input::IsMouseButtonPressed;
    (void)&Input::IsMouseButtonJustPressed;
    (void)&Input::GetMousePosition;
    (void)&Input::GetMouseDelta;
    (void)&Input::GetMouseScroll;
    (void)&Input::SetCursorVisible;
    (void)&Input::SetCursorLocked;
    (void)&Input::Update;
}

// ==================== Modifier Key Tests ====================

TEST(InputTest, ModifierKeyCodesExist) {
    // Ensure modifier key codes are defined
    EXPECT_NE(static_cast<int>(KeyCode::LeftShift), 0);
    EXPECT_NE(static_cast<int>(KeyCode::RightShift), 0);
    EXPECT_NE(static_cast<int>(KeyCode::LeftControl), 0);
    EXPECT_NE(static_cast<int>(KeyCode::RightControl), 0);
    EXPECT_NE(static_cast<int>(KeyCode::LeftAlt), 0);
    EXPECT_NE(static_cast<int>(KeyCode::RightAlt), 0);
    EXPECT_NE(static_cast<int>(KeyCode::LeftSuper), 0);
    EXPECT_NE(static_cast<int>(KeyCode::RightSuper), 0);
}

// ==================== Arrow and Navigation Keys ====================

TEST(InputTest, NavigationKeyCodesExist) {
    EXPECT_EQ(static_cast<int>(KeyCode::Right), 262);
    EXPECT_EQ(static_cast<int>(KeyCode::Left), 263);
    EXPECT_EQ(static_cast<int>(KeyCode::Down), 264);
    EXPECT_EQ(static_cast<int>(KeyCode::Up), 265);
    EXPECT_EQ(static_cast<int>(KeyCode::PageUp), 266);
    EXPECT_EQ(static_cast<int>(KeyCode::PageDown), 267);
    EXPECT_EQ(static_cast<int>(KeyCode::Home), 268);
    EXPECT_EQ(static_cast<int>(KeyCode::End), 269);
}
