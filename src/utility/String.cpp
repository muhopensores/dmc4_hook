#include <cstdarg>

#include <Windows.h>

#include "String.hpp"

using namespace std;

namespace utility {
    string narrow(wstring_view str) {
        auto length = WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.length(), nullptr, 0, nullptr, nullptr);
        string narrow_str{};

        narrow_str.resize(length);
        WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.length(), (LPSTR)narrow_str.c_str(), length, nullptr, nullptr);

        return narrow_str;
    }

    wstring widen(string_view str) {
        auto length = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.length(), nullptr, 0);
        wstring wide_str{};

        wide_str.resize(length);
        MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.length(), (LPWSTR)wide_str.c_str(), length);

        return wide_str;
    }

    string format_string(const char* format, va_list args) {
        va_list args_copy{};

        va_copy(args_copy, args);

        auto len = vsnprintf(nullptr, 0, format, args_copy);

        va_end(args_copy);

        if (len <= 0) {
            return {};
        }

        string buffer{};

        buffer.resize(len + 1, 0);
        vsnprintf(buffer.data(), buffer.size(), format, args);
        buffer.resize(buffer.size() - 1); // Removes the extra 0 vsnprintf adds.

        return buffer;
    }

    string to_lower(string str) {
        transform(str.begin(), str.end(), str.begin(), tolower);

        return str;
    };
}
