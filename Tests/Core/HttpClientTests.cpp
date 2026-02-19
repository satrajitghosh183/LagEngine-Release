/**
 * @file HttpClientTests.cpp
 * @brief Unit tests for HttpClient URL parsing
 */

#include <gtest/gtest.h>
#include "Core/HttpClient.hpp"

using namespace GameEngine;

// We can only test URL parsing reliably without a network
// The actual HTTP methods would need a running server

TEST(HttpClient, GetWithInvalidHost) {
    auto response = HttpClient::Get("http://invalid-host-that-does-not-exist.local/test", 1000);
    EXPECT_FALSE(response.Success);
    EXPECT_FALSE(response.Error.empty());
}

TEST(HttpClient, PostWithInvalidHost) {
    auto response = HttpClient::Post("http://invalid-host-that-does-not-exist.local/test", "{}", 1000);
    EXPECT_FALSE(response.Success);
    EXPECT_FALSE(response.Error.empty());
}

TEST(HttpClient, IsServerAvailableWithInvalidHost) {
    bool available = HttpClient::IsServerAvailable("invalid-host-that-does-not-exist.local", 99999);
    EXPECT_FALSE(available);
}

TEST(HttpClient, ResponseDefaults) {
    HttpClient::Response resp;
    EXPECT_EQ(resp.StatusCode, 0);
    EXPECT_TRUE(resp.Body.empty());
    EXPECT_FALSE(resp.Success);
    EXPECT_TRUE(resp.Error.empty());
}
