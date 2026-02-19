#pragma once

#include <string>

namespace GameEngine {

    class HttpClient {
    public:
        struct Response {
            int StatusCode = 0;
            std::string Body;
            bool Success = false;
            std::string Error;
        };

        static Response Post(const std::string& url, const std::string& jsonBody, int timeoutMs = 120000);
        static Response Get(const std::string& url, int timeoutMs = 10000);
        static bool IsServerAvailable(const std::string& host, int port);

    private:
        struct URLParts {
            std::string Host;
            std::string Path;
            int Port = 80;
            bool IsHTTPS = false;
        };
        static URLParts ParseURL(const std::string& url);
    };

}
