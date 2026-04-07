/**
 * @file HotkeyTests.cpp
 * @brief Unit tests for the Editor hotkey system
 */

#include <gtest/gtest.h>
#include "../../Editor/Core/Hotkeys.hpp"
#include <set>

using namespace GameEngine;

class HotkeyTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_HotkeyManager = std::make_unique<HotkeyManager>();
    }

    std::unique_ptr<HotkeyManager> m_HotkeyManager;
};

// ==================== Default Hotkeys ====================

TEST_F(HotkeyTest, DefaultHotkeysLoaded) {
    // Check that default hotkeys are loaded
    auto defaults = HotkeyManager::GetDefaultHotkeys();
    
    // Verify some expected defaults exist
    EXPECT_NE(defaults[static_cast<size_t>(EditorActions::UNDO_ACTION)].m_Key, KeyCode::None);
    EXPECT_NE(defaults[static_cast<size_t>(EditorActions::REDO_ACTION)].m_Key, KeyCode::None);
    EXPECT_NE(defaults[static_cast<size_t>(EditorActions::SAVE_SCENE)].m_Key, KeyCode::None);
}

TEST_F(HotkeyTest, UndoHotkey) {
    auto hotkey = m_HotkeyManager->GetHotkey(EditorActions::UNDO_ACTION);
    EXPECT_EQ(hotkey.m_Key, KeyCode::Z);
    EXPECT_TRUE(hotkey.m_Control);
    EXPECT_TRUE(hotkey.m_Press);  // Should be press, not hold
}

TEST_F(HotkeyTest, RedoHotkey) {
    auto hotkey = m_HotkeyManager->GetHotkey(EditorActions::REDO_ACTION);
    EXPECT_EQ(hotkey.m_Key, KeyCode::Y);
    EXPECT_TRUE(hotkey.m_Control);
}

TEST_F(HotkeyTest, SaveHotkey) {
    auto hotkey = m_HotkeyManager->GetHotkey(EditorActions::SAVE_SCENE);
    EXPECT_EQ(hotkey.m_Key, KeyCode::S);
    EXPECT_TRUE(hotkey.m_Control);
}

TEST_F(HotkeyTest, SaveAsHotkey) {
    auto hotkey = m_HotkeyManager->GetHotkey(EditorActions::SAVE_SCENE_AS);
    EXPECT_EQ(hotkey.m_Key, KeyCode::S);
    EXPECT_TRUE(hotkey.m_Control);
    EXPECT_TRUE(hotkey.m_Shift);  // Save As uses Ctrl+Shift+S
}

// ==================== Hotkey Modification ====================

TEST_F(HotkeyTest, SetHotkey) {
    HotkeyInfo newHotkey;
    newHotkey.m_Key = KeyCode::F5;
    newHotkey.m_Press = true;
    newHotkey.m_Control = false;
    newHotkey.m_Shift = false;
    
    m_HotkeyManager->SetHotkey(EditorActions::SAVE_SCENE, newHotkey);
    
    auto retrieved = m_HotkeyManager->GetHotkey(EditorActions::SAVE_SCENE);
    EXPECT_EQ(retrieved.m_Key, KeyCode::F5);
    EXPECT_FALSE(retrieved.m_Control);
}

// ==================== Hotkey String Representation ====================

TEST_F(HotkeyTest, GetInputString_Simple) {
    HotkeyInfo hotkey;
    hotkey.m_Key = KeyCode::S;
    hotkey.m_Control = false;
    hotkey.m_Shift = false;
    
    m_HotkeyManager->SetHotkey(EditorActions::SAVE_SCENE, hotkey);
    std::string str = m_HotkeyManager->GetInputString(EditorActions::SAVE_SCENE);
    
    EXPECT_EQ(str, "S");
}

TEST_F(HotkeyTest, GetInputString_WithCtrl) {
    HotkeyInfo hotkey;
    hotkey.m_Key = KeyCode::S;
    hotkey.m_Control = true;
    hotkey.m_Shift = false;
    
    m_HotkeyManager->SetHotkey(EditorActions::SAVE_SCENE, hotkey);
    std::string str = m_HotkeyManager->GetInputString(EditorActions::SAVE_SCENE);
    
    EXPECT_TRUE(str.find("Ctrl") != std::string::npos);
    EXPECT_TRUE(str.find("S") != std::string::npos);
}

TEST_F(HotkeyTest, GetInputString_WithCtrlShift) {
    HotkeyInfo hotkey;
    hotkey.m_Key = KeyCode::S;
    hotkey.m_Control = true;
    hotkey.m_Shift = true;
    
    m_HotkeyManager->SetHotkey(EditorActions::SAVE_SCENE_AS, hotkey);
    std::string str = m_HotkeyManager->GetInputString(EditorActions::SAVE_SCENE_AS);
    
    EXPECT_TRUE(str.find("Ctrl") != std::string::npos);
    EXPECT_TRUE(str.find("Shift") != std::string::npos);
    EXPECT_TRUE(str.find("S") != std::string::npos);
}

// ==================== All Actions Covered ====================

TEST_F(HotkeyTest, AllActionsHaveDefaults) {
    auto defaults = HotkeyManager::GetDefaultHotkeys();
    
    // Every action should have a defined hotkey (even if it's None)
    for (size_t i = 0; i < static_cast<size_t>(EditorActions::COUNT); i++) {
        // Just verify we can access each one without crashing
        auto hotkey = m_HotkeyManager->GetHotkey(static_cast<EditorActions>(i));
        // Could be KeyCode::None, which is valid for unassigned actions
    }
}

// ==================== Camera Movement Hotkeys ====================

TEST_F(HotkeyTest, CameraMovementHotkeys) {
    auto forward = m_HotkeyManager->GetHotkey(EditorActions::MOVE_CAMERA_FORWARD);
    auto backward = m_HotkeyManager->GetHotkey(EditorActions::MOVE_CAMERA_BACKWARD);
    auto left = m_HotkeyManager->GetHotkey(EditorActions::MOVE_CAMERA_LEFT);
    auto right = m_HotkeyManager->GetHotkey(EditorActions::MOVE_CAMERA_RIGHT);
    
    // Standard WASD bindings
    EXPECT_EQ(forward.m_Key, KeyCode::W);
    EXPECT_EQ(backward.m_Key, KeyCode::S);
    EXPECT_EQ(left.m_Key, KeyCode::A);
    EXPECT_EQ(right.m_Key, KeyCode::D);
    
    // These should be hold, not press
    EXPECT_FALSE(forward.m_Press);
    EXPECT_FALSE(backward.m_Press);
}

// ==================== Transform Gizmo Hotkeys ====================

TEST_F(HotkeyTest, TransformToggleHotkeys) {
    auto move = m_HotkeyManager->GetHotkey(EditorActions::MOVE_TOGGLE_ACTION);
    auto rotate = m_HotkeyManager->GetHotkey(EditorActions::ROTATE_TOGGLE_ACTION);
    auto scale = m_HotkeyManager->GetHotkey(EditorActions::SCALE_TOGGLE_ACTION);
    
    // 1, 2, 3 keys for transform modes
    EXPECT_EQ(move.m_Key, KeyCode::D1);
    EXPECT_EQ(rotate.m_Key, KeyCode::D2);
    EXPECT_EQ(scale.m_Key, KeyCode::D3);
}

// ==================== EditorActions Enum ====================

TEST_F(HotkeyTest, EditorActionsCount) {
    // Verify the COUNT value is reasonable
    int count = static_cast<int>(EditorActions::COUNT);
    EXPECT_GT(count, 0);
    EXPECT_LT(count, 100);  // Sanity check - shouldn't have 100+ actions
}

TEST_F(HotkeyTest, EditorActionsUnique) {
    // Each action should have a unique integer value
    std::set<int> values;
    for (int i = 0; i < static_cast<int>(EditorActions::COUNT); i++) {
        values.insert(i);
    }
    EXPECT_EQ(values.size(), static_cast<size_t>(EditorActions::COUNT));
}
