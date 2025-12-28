#pragma once

#include <string>
#include <string_view>
#include <windows.h>

namespace GW2_SCT::Utf {

inline bool WideToUtf8(std::wstring_view input, std::string& output) {
    if (input.empty()) {
        output.clear();
        return true;
    }

    int requiredSize = WideCharToMultiByte(
        CP_UTF8, 0, input.data(), static_cast<int>(input.size()),
        nullptr, 0, nullptr, nullptr);

    if (requiredSize <= 0) {
        output.clear();
        return false;
    }

    output.resize(requiredSize);
    int converted = WideCharToMultiByte(
        CP_UTF8, 0, input.data(), static_cast<int>(input.size()),
        output.data(), requiredSize, nullptr, nullptr);

    if (converted <= 0) {
        output.clear();
        return false;
    }

    return true;
}

inline std::string WideToUtf8(std::wstring_view input) {
    std::string output;
    WideToUtf8(input, output);
    return output;
}

inline bool Utf8ToWide(std::string_view input, std::wstring& output) {
    if (input.empty()) {
        output.clear();
        return true;
    }

    int requiredSize = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()),
        nullptr, 0);

    if (requiredSize <= 0) {
        output.clear();
        return false;
    }

    output.resize(requiredSize);
    int converted = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()),
        output.data(), requiredSize);

    if (converted <= 0) {
        output.clear();
        return false;
    }

    return true;
}

inline std::wstring Utf8ToWide(std::string_view input) {
    std::wstring output;
    Utf8ToWide(input, output);
    return output;
}

} // namespace GW2_SCT::Utf

