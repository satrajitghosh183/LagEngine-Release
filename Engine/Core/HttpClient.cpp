#include "HttpClient.hpp"
#include "Logger.hpp"

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
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
    HttpClient::Response HttpClient::Post(const std::string& url, const std::string& jsonBody, int timeoutMs) {
        Response response;
        auto parts = ParseURL(url);

        std::wstring wHost(parts.Host.begin(), parts.Host.end());
        std::wstring wPath(parts.Path.begin(), parts.Path.end());

        HINTERNET hSession = WinHttpOpen(L"GameEngine/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
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

        HINTERNET hSession = WinHttpOpen(L"GameEngine/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
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

#else
    // Non-Windows stubs
    HttpClient::Response HttpClient::Post(const std::string&, const std::string&, int) {
        return {0, "", false, "HTTP client not available on this platform"};
    }
    HttpClient::Response HttpClient::Get(const std::string&, int) {
        return {0, "", false, "HTTP client not available on this platform"};
    }
    bool HttpClient::IsServerAvailable(const std::string&, int) { return false; }
#endif

}
