#include <algorithm>

#include <Windows.h>

#include "Memory.hpp"
#include "Pattern.hpp"

using namespace std;

namespace utility {
    static uint8_t to_byte(char digit) {
        if (digit >= '0' && digit <= '9') {
            return (digit - '0');
        }

        if (digit >= 'a' && digit <= 'f') {
            return (digit - 'a' + 10);
        }

        if (digit >= 'A' && digit <= 'F') {
            return (digit - 'A' + 10);
        }

        return 0;
    }

    Pattern::Pattern(const string& pattern)
        : m_pattern{move(build_pattern(pattern))}
    {
    }

    optional<uintptr_t> Pattern::find(uintptr_t start, size_t length) {
        auto pattern_length = m_pattern.size();
        auto end = start + length - pattern_length;

        for (auto i = start; i <= end; ++i) {
            auto j = i;
            auto failed_to_match = false;

            // Make sure the address is readable.
            if (IsBadReadPtr((const void*)i, pattern_length) != FALSE) {
            //if (!isGoodReadPtr(i, patternLength)) {
                i += pattern_length - 1;
                continue;
            }

            for (auto& k : m_pattern) {
                if (k != -1 && k != *(uint8_t*)j) {
                    failed_to_match = true;
                    break;
                }

                ++j;
            }

            if (!failed_to_match) {
                return i;
            }
        }

        return {};
    }

    vector<int16_t> build_pattern(string pattern_str) {
        // Remove spaces from the pattern string.
        pattern_str.erase(remove_if(begin(pattern_str), end(pattern_str), isspace), end(pattern_str));

        auto length = pattern_str.length();
        vector<int16_t> pattern{};

        for (size_t i = 0; i < length;) {
            auto p1 = pattern_str[i];

            if (p1 != '?') {
                // Bytes require 2 hex characters to encode, make sure we don't read
                // past the end of the pattern string attempting to read the next char.
                if (i + 1 >= length) {
                    break;
                }

                auto p2 = pattern_str[i + 1];
                auto value = to_byte(p1) << 4 | to_byte(p2);

                pattern.emplace_back(value);

                i += 2;
            }
            else {
                // Wildcard's (?'s) get encoded as a -1.
                pattern.emplace_back(-1);
                i += 1;
            }
        }

        return pattern;
    }
}
