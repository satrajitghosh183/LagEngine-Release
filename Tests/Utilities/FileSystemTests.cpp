/**
 * @file FileSystemTests.cpp
 * @brief Unit tests for FileSystem utilities
 */

#include <gtest/gtest.h>
#include "Platform/FileSystem.hpp"
#include <fstream>
#include <filesystem>

using namespace GameEngine;

class FileSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_TestDir = "test_filesystem_dir";
        m_TestFile = "test_file.txt";
        m_TestContent = "Hello, World!\nThis is a test file.";
    }

    void TearDown() override {
        // Cleanup
        if (std::filesystem::exists(m_TestFile)) {
            std::filesystem::remove(m_TestFile);
        }
        if (std::filesystem::exists(m_TestDir)) {
            std::filesystem::remove_all(m_TestDir);
        }
    }

    std::string m_TestDir;
    std::string m_TestFile;
    std::string m_TestContent;
};

// ==================== File Existence ====================

TEST_F(FileSystemTest, FileExists_ReturnsFalse_WhenNotExists) {
    EXPECT_FALSE(FileSystem::Exists("nonexistent_file_xyz.txt"));
}

TEST_F(FileSystemTest, FileExists_ReturnsTrue_WhenExists) {
    // Create test file
    std::ofstream file(m_TestFile);
    file << m_TestContent;
    file.close();

    EXPECT_TRUE(FileSystem::Exists(m_TestFile));
}

// ==================== File Reading ====================

TEST_F(FileSystemTest, ReadFileAsString) {
    // Create test file
    std::ofstream file(m_TestFile);
    file << m_TestContent;
    file.close();

    std::string content = FileSystem::ReadFileAsString(m_TestFile);
    EXPECT_EQ(content, m_TestContent);
}

TEST_F(FileSystemTest, ReadFileAsBinary) {
    // Create test file
    std::ofstream file(m_TestFile, std::ios::binary);
    file << m_TestContent;
    file.close();

    std::vector<uint8_t> bytes = FileSystem::ReadFileAsBinary(m_TestFile);
    EXPECT_EQ(bytes.size(), m_TestContent.size());
}

TEST_F(FileSystemTest, ReadNonexistentFile) {
    std::string content = FileSystem::ReadFileAsString("nonexistent.txt");
    EXPECT_TRUE(content.empty());
}

// ==================== File Writing ====================

TEST_F(FileSystemTest, WriteStringToFile) {
    bool success = FileSystem::WriteStringToFile(m_TestFile, m_TestContent);
    EXPECT_TRUE(success);

    // Verify content
    std::string readBack = FileSystem::ReadFileAsString(m_TestFile);
    EXPECT_EQ(readBack, m_TestContent);
}

TEST_F(FileSystemTest, WriteBinaryToFile) {
    std::vector<uint8_t> bytes(m_TestContent.begin(), m_TestContent.end());
    bool success = FileSystem::WriteBinaryToFile(m_TestFile, bytes);
    EXPECT_TRUE(success);

    // Verify
    std::vector<uint8_t> readBack = FileSystem::ReadFileAsBinary(m_TestFile);
    EXPECT_EQ(readBack, bytes);
}

// ==================== Directory Operations ====================

TEST_F(FileSystemTest, CreateDirectoryTest) {
    bool success = FileSystem::CreateDirectory(m_TestDir);
    EXPECT_TRUE(success);
    EXPECT_TRUE(std::filesystem::is_directory(m_TestDir));
}

TEST_F(FileSystemTest, IsDirectory) {
    FileSystem::CreateDirectory(m_TestDir);
    EXPECT_TRUE(FileSystem::IsDirectory(m_TestDir));

    FileSystem::WriteStringToFile(m_TestFile, "test");
    EXPECT_FALSE(FileSystem::IsDirectory(m_TestFile));
}

// ==================== Path Operations ====================

TEST_F(FileSystemTest, GetExtension) {
    EXPECT_EQ(FileSystem::GetExtension("file.txt"), ".txt");
    EXPECT_EQ(FileSystem::GetExtension("file.tar.gz"), ".gz");
    EXPECT_EQ(FileSystem::GetExtension("noextension"), "");
}

TEST_F(FileSystemTest, GetFilename) {
    EXPECT_EQ(FileSystem::GetFilename("/path/to/file.txt"), "file.txt");
    EXPECT_EQ(FileSystem::GetFilename("file.txt"), "file.txt");
}

TEST_F(FileSystemTest, GetFilenameWithoutExtension) {
    EXPECT_EQ(FileSystem::GetFilenameWithoutExtension("/path/to/file.txt"), "file");
    EXPECT_EQ(FileSystem::GetFilenameWithoutExtension("file.txt"), "file");
}

TEST_F(FileSystemTest, GetDirectory) {
    std::string dir = FileSystem::GetDirectory("/path/to/file.txt");
    EXPECT_EQ(dir, "/path/to");
}

TEST_F(FileSystemTest, JoinPath) {
    std::string joined = FileSystem::Join("path/to", "file.txt");
    EXPECT_TRUE(joined == "path/to/file.txt" || joined == "path/to\\file.txt");
}

// ==================== File Size ====================

TEST_F(FileSystemTest, GetFileSize) {
    FileSystem::WriteStringToFile(m_TestFile, m_TestContent);

    size_t size = FileSystem::GetFileSize(m_TestFile);
    EXPECT_EQ(size, m_TestContent.size());
}

TEST_F(FileSystemTest, GetFileSizeNonexistent) {
    size_t size = FileSystem::GetFileSize("nonexistent.txt");
    EXPECT_EQ(size, 0);
}

// ==================== Directory Listing ====================

TEST_F(FileSystemTest, GetFilesInDirectory) {
    FileSystem::CreateDirectory(m_TestDir);
    FileSystem::WriteStringToFile(m_TestDir + "/file1.txt", "content1");
    FileSystem::WriteStringToFile(m_TestDir + "/file2.txt", "content2");

    std::vector<std::string> files = FileSystem::GetFilesInDirectory(m_TestDir);
    EXPECT_EQ(files.size(), 2u);
}

TEST_F(FileSystemTest, GetFilesWithExtension) {
    FileSystem::CreateDirectory(m_TestDir);
    FileSystem::WriteStringToFile(m_TestDir + "/file1.txt", "content1");
    FileSystem::WriteStringToFile(m_TestDir + "/file2.txt", "content2");
    FileSystem::WriteStringToFile(m_TestDir + "/data.json", "{}");

    std::vector<std::string> txtFiles = FileSystem::GetFilesWithExtension(m_TestDir, ".txt");
    EXPECT_EQ(txtFiles.size(), 2u);

    std::vector<std::string> jsonFiles = FileSystem::GetFilesWithExtension(m_TestDir, ".json");
    EXPECT_EQ(jsonFiles.size(), 1u);
}

// ==================== File Deletion ====================

TEST_F(FileSystemTest, DeleteFileTest) {
    FileSystem::WriteStringToFile(m_TestFile, m_TestContent);
    EXPECT_TRUE(FileSystem::Exists(m_TestFile));

    bool success = FileSystem::DeleteFile(m_TestFile);
    EXPECT_TRUE(success);
    EXPECT_FALSE(FileSystem::Exists(m_TestFile));
}

TEST_F(FileSystemTest, DeleteNonexistent) {
    bool success = FileSystem::DeleteFile("nonexistent.txt");
    EXPECT_FALSE(success);
}
