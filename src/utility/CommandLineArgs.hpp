#pragma once

namespace utility {
    int get_argument(const char* arg_name);
    bool get_argument(const char* arg_name, char* value);
    bool get_argument(const char* arg_name, char* value, size_t value_size);
    bool check_argument(const char* arg_name);
}
