#include "HttpClient.hpp"
#include "Logger.hpp"

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#endif

namespace GameEngine {

    HttpClient::URLParts HttpClient::ParseURL(const std::string& url) {
        URLParts parts;
        std::string remaining = url;

        if (remaining.substr(0, 8) == "https://") {
            parts.IsHTTPS = true; parts.Port = 443; remaining = remaining.substr(8);
        } else if (remaining.substr(0, 7) == "http://") {
            parts.IsHTTPS = false; parts.Port = 80; remaining = remaining.substr(7);
        }

        auto pathPos = remaining.find('/');
        std::string hostPort = (pathPos != std::string::npos) ? remaining.substr(0, pathPos) : remaining;
        parts.Path = (pathPos != std::string::npos) ? remaining.substr(pathPos) : "/";

        auto colonPos = hostPort.find(':');
        if (colonPos != std::string::npos) {
            parts.Host = hostPort.substr(0, colonPos);
            parts.Port = std::stoi(hostPort.substr(colonPos + 1));
        } else {
            parts.Host = hostPort;
        }

        return parts;
    }

#ifdef _WIN32
    static bool IsLocalHost(const std::string& host) {
        return host == "localhost" || host == "127.0.0.1" || host == "::1" ||
               host == "0.0.0.0" || host.find("192.168.") == 0 || host.find("10.") == 0;
    }

    HttpClient::Response HttpClient::Post(const std::string& url, const std::string& jsonBody, int timeoutMs) {
        Response response;
        auto parts = ParseURL(url);

        std::wstring wHost(parts.Host.begin(), parts.Host.end());
        std::wstring wPath(parts.Path.begin(), parts.Path.end());

        // Use NO_PROXY for local connections to avoid proxy interference
        DWORD accessType = IsLocalHost(parts.Host) ?
            WINHTTP_ACCESS_TYPE_NO_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;

        HINTERNET hSession = WinHttpOpen(L"GameEngine/1.0", accessType,
                                         WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) { response.Error = "WinHttpOpen failed"; return response; }

        WinHttpSetTimeouts(hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

        HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), parts.Port, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); response.Error = "WinHttpConnect failed"; return response; }

        DWORD flags = parts.IsHTTPS ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wPath.c_str(), nullptr,
                                                WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); response.Error = "WinHttpOpenRequest failed"; return response; }

        LPCWSTR contentType = L"Content-Type: application/json";
        BOOL sent = WinHttpSendRequest(hRequest, contentType, -1,
                                       (LPVOID)jsonBody.c_str(), (DWORD)jsonBody.size(),
                                       (DWORD)jsonBody.size(), 0);
        if (!sent) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); response.Error = "WinHttpSendRequest failed"; return response; }

        if (!WinHttpReceiveResponse(hRequest, nullptr)) {
            WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
            response.Error = "WinHttpReceiveResponse failed"; return response;
        }

        DWORD statusCode = 0, statusCodeSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                           WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);
        response.StatusCode = static_cast<int>(statusCode);

        std::string body;
        DWORD bytesAvailable = 0;
        do {
            WinHttpQueryDataAvailable(hRequest, &bytesAvailable);
            if (bytesAvailable > 0) {
                std::vector<char> buffer(bytesAvailable + 1);
                DWORD bytesRead = 0;
                WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead);
                body.append(buffer.data(), bytesRead);
            }
        } while (bytesAvailable > 0);

        response.Body = body;
        response.Success = (statusCode >= 200 && statusCode < 300);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response;
    }

    HttpClient::Response HttpClient::Get(const std::string& url, int timeoutMs) {
        Response response;
        auto parts = ParseURL(url);

        std::wstring wHost(parts.Host.begin(), parts.Host.end());
        std::wstring wPath(parts.Path.begin(), parts.Path.end());

        // Use NO_PROXY for local connections to avoid proxy interference
        DWORD accessType = IsLocalHost(parts.Host) ?
            WINHTTP_ACCESS_TYPE_NO_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;

        HINTERNET hSession = WinHttpOpen(L"GameEngine/1.0", accessType,
                                         WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) { response.Error = "WinHttpOpen failed"; return response; }

        WinHttpSetTimeouts(hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

        HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), parts.Port, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); response.Error = "WinHttpConnect failed"; return response; }

        DWORD flags = parts.IsHTTPS ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wPath.c_str(), nullptr,
                                                WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); response.Error = "WinHttpOpenRequest failed"; return response; }

        if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
            WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
            response.Error = "WinHttpSendRequest failed"; return response;
        }

        if (!WinHttpReceiveResponse(hRequest, nullptr)) {
            WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
            response.Error = "WinHttpReceiveResponse failed"; return response;
        }

        DWORD statusCode = 0, statusCodeSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                           WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);
        response.StatusCode = static_cast<int>(statusCode);

        std::string body;
        DWORD bytesAvailable = 0;
        do {
            WinHttpQueryDataAvailable(hRequest, &bytesAvailable);
            if (bytesAvailable > 0) {
                std::vector<char> buffer(bytesAvailable + 1);
                DWORD bytesRead = 0;
                WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead);
                body.append(buffer.data(), bytesRead);
            }
        } while (bytesAvailable > 0);

        response.Body = body;
        response.Success = (statusCode >= 200 && statusCode < 300);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response;
    }

    bool HttpClient::IsServerAvailable(const std::string& host, int port) {
        auto resp = Get("http://" + host + ":" + std::to_string(port) + "/api/tags", 3000);
        return resp.Success;
    }

#elif defined(__linux__) || defined(__APPLE__)
    // Linux/macOS implementation using BSD sockets

    static int CreateSocket(const std::string& host, int port, int timeoutMs) {
        struct addrinfo hints{}, *result, *rp;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        
        std::string portStr = std::to_string(port);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
            return -1;
        }
        
        int sock = -1;
        for (rp = result; rp != nullptr; rp = rp->ai_next) {
            sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (sock == -1) continue;
            
            // Set non-blocking for connect timeout
            int flags = fcntl(sock, F_GETFL, 0);
            fcntl(sock, F_SETFL, flags | O_NONBLOCK);
            
            int connectResult = connect(sock, rp->ai_addr, rp->ai_addrlen);
            if (connectResult == 0 || errno == EINPROGRESS) {
                struct pollfd pfd;
                pfd.fd = sock;
                pfd.events = POLLOUT;
                
                if (poll(&pfd, 1, timeoutMs) > 0 && (pfd.revents & POLLOUT)) {
                    // Connection successful, set back to blocking
                    fcntl(sock, F_SETFL, flags);
                    break;
                }
            }
            
            close(sock);
            sock = -1;
        }
        
        freeaddrinfo(result);
        return sock;
    }

    static std::string ReadResponse(int sock, int timeoutMs) {
        std::string response;
        char buffer[4096];
        
        struct pollfd pfd;
        pfd.fd = sock;
        pfd.events = POLLIN;
        
        while (poll(&pfd, 1, timeoutMs) > 0 && (pfd.revents & POLLIN)) {
            ssize_t bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead <= 0) break;
            buffer[bytesRead] = '\0';
            response += buffer;
            
            // Check if we've received the full response (Content-Length or chunked)
            // Simple heuristic: if body ends with } or ], assume JSON complete
            if (response.find("\r\n\r\n") != std::string::npos) {
                size_t bodyStart = response.find("\r\n\r\n") + 4;
                std::string body = response.substr(bodyStart);
                // For JSON responses, check if balanced
                if (!body.empty() && (body.back() == '}' || body.back() == ']' || body.back() == '\n')) {
                    // Give a short poll to see if more data coming
                    if (poll(&pfd, 1, 100) <= 0) break;
                }
            }
        }
        
        return response;
    }

    static int ParseStatusCode(const std::string& response) {
        // Parse "HTTP/1.1 200 OK"
        size_t pos = response.find(' ');
        if (pos == std::string::npos) return 0;
        pos++;
        size_t end = response.find(' ', pos);
        if (end == std::string::npos) end = response.find('\r', pos);
        if (end == std::string::npos) return 0;
        try {
            return std::stoi(response.substr(pos, end - pos));
        } catch (...) {
            return 0;
        }
    }

    static std::string ExtractBody(const std::string& response) {
        size_t bodyStart = response.find("\r\n\r\n");
        if (bodyStart == std::string::npos) return "";
        return response.substr(bodyStart + 4);
    }

    HttpClient::Response HttpClient::Post(const std::string& url, const std::string& jsonBody, int timeoutMs) {
        Response response;
        auto parts = ParseURL(url);
        
        int sock = CreateSocket(parts.Host, parts.Port, timeoutMs);
        if (sock < 0) {
            response.Error = "Failed to connect to " + parts.Host + ":" + std::to_string(parts.Port);
            return response;
        }
        
        // Build HTTP request
        std::ostringstream request;
        request << "POST " << parts.Path << " HTTP/1.1\r\n";
        request << "Host: " << parts.Host << ":" << parts.Port << "\r\n";
        request << "Content-Type: application/json\r\n";
        request << "Content-Length: " << jsonBody.size() << "\r\n";
        request << "Connection: close\r\n";
        request << "\r\n";
        request << jsonBody;
        
        std::string requestStr = request.str();
        if (send(sock, requestStr.c_str(), requestStr.size(), 0) < 0) {
            close(sock);
            response.Error = "Failed to send request";
            return response;
        }
        
        std::string rawResponse = ReadResponse(sock, timeoutMs);
        close(sock);
        
        if (rawResponse.empty()) {
            response.Error = "No response received";
            return response;
        }
        
        response.StatusCode = ParseStatusCode(rawResponse);
        response.Body = ExtractBody(rawResponse);
        response.Success = (response.StatusCode >= 200 && response.StatusCode < 300);
        
        if (!response.Success && response.Error.empty()) {
            response.Error = "HTTP " + std::to_string(response.StatusCode);
        }
        
        return response;
    }

    HttpClient::Response HttpClient::Get(const std::string& url, int timeoutMs) {
        Response response;
        auto parts = ParseURL(url);
        
        int sock = CreateSocket(parts.Host, parts.Port, timeoutMs);
        if (sock < 0) {
            response.Error = "Failed to connect to " + parts.Host + ":" + std::to_string(parts.Port);
            return response;
        }
        
        // Build HTTP request
        std::ostringstream request;
        request << "GET " << parts.Path << " HTTP/1.1\r\n";
        request << "Host: " << parts.Host << ":" << parts.Port << "\r\n";
        request << "Connection: close\r\n";
        request << "\r\n";
        
        std::string requestStr = request.str();
        if (send(sock, requestStr.c_str(), requestStr.size(), 0) < 0) {
            close(sock);
            response.Error = "Failed to send request";
            return response;
        }
        
        std::string rawResponse = ReadResponse(sock, timeoutMs);
        close(sock);
        
        if (rawResponse.empty()) {
            response.Error = "No response received";
            return response;
        }
        
        response.StatusCode = ParseStatusCode(rawResponse);
        response.Body = ExtractBody(rawResponse);
        response.Success = (response.StatusCode >= 200 && response.StatusCode < 300);
        
        if (!response.Success && response.Error.empty()) {
            response.Error = "HTTP " + std::to_string(response.StatusCode);
        }
        
        return response;
    }

    bool HttpClient::IsServerAvailable(const std::string& host, int port) {
        int sock = CreateSocket(host, port, 3000);
        if (sock < 0) return false;
        close(sock);
        return true;
    }

#else
    // Unsupported platform stubs
    HttpClient::Response HttpClient::Post(const std::string&, const std::string&, int) {
        return {0, "", false, "HTTP client not available on this platform"};
    }
    HttpClient::Response HttpClient::Get(const std::string&, int) {
        return {0, "", false, "HTTP client not available on this platform"};
    }
    bool HttpClient::IsServerAvailable(const std::string&, int) { return false; }
#endif

}
