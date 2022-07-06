#pragma once

#include <cstdint>

namespace utility {
    bool is_good_ptr(uintptr_t ptr, size_t len, uint32_t access);
    bool is_good_read_ptr(uintptr_t ptr, size_t len);
    bool is_good_write_ptr(uintptr_t ptr, size_t len);
    bool is_good_code_ptr(uintptr_t ptr, size_t len);
}
