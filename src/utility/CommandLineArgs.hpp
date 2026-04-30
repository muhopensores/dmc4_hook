#pragma once

namespace utility {
    int get_argument(const char* arg_name);
    bool get_argument(const char* arg_name, char* value);
    bool check_argument(const char* arg_name);
}
