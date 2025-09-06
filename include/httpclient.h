#pragma once

#include <string>
#include <functional>
#include <future>
#include <mutex>
#include <memory>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

struct AsyncRequestContext {
    std::function<void(const std::string&)> callback;
    std::string response;
    HINTERNET hConnect = nullptr;
    HINTERNET hRequest = nullptr;
    std::unique_ptr<char[]> buffer;
};

namespace HTTPClient {
    void Initialize();
    void Shutdown();
    void GetRequestAsync(const std::wstring& wUrl, std::function<void(const std::string&)> callback);
    std::future<std::string> GetRequestAsync(const std::wstring& wUrl);
    std::string GetRequest(const std::wstring& wUrl);
    std::wstring StringToWString(const std::string& str);
}