#include "CommandLineArgs.hpp"
#include "processenv.h"
#include <climits>
#include <cstring>

static constexpr size_t MAX_CSTR_ARGS = 4096;

static char g_command_line_buffer[MAX_CSTR_ARGS] = {0};

static const char* command_line() {
    if (!g_command_line_buffer[0]) {
        strncpy_s(g_command_line_buffer, MAX_CSTR_ARGS, GetCommandLineA(), _TRUNCATE);
    }
    return g_command_line_buffer;
}

static size_t next_token(const char* line, size_t cursor, char* token, size_t token_size) {
    while (line[cursor] == ' ' || line[cursor] == '\t') {
        ++cursor;
    }
    if (line[cursor] == '\0') {
        return std::string::npos;
    }

    size_t out  = 0;
    bool quoted = false;

    if (line[cursor] == '"') {
        quoted = true;
        ++cursor;
        while (line[cursor] != '\0' && line[cursor] != '"') {
            if (out + 1 < token_size) {
                token[out++] = line[cursor];
            }
            ++cursor;
        }
        if (line[cursor] == '"') {
            ++cursor;
        }
    } else {
        while (line[cursor] != '\0' && line[cursor] != ' ' && line[cursor] != '\t') {
            if (out + 1 < token_size) {
                token[out++] = line[cursor];
            }
            ++cursor;
        }
    }

    token[out] = '\0';
    return cursor;
}

// Walks tokens until `arg_name` matches; returns the position just after the
// match so the caller can read the following token (or know there is none).
static size_t find_argument_token(const char* arg_name, char* token, size_t token_size) {
    const char* line = command_line();
    size_t cursor    = 0;
    size_t next = 0;
    while ((next = next_token(line, cursor, token, token_size)) != std::string::npos) {
        cursor = next;
        if (strcmp(token, arg_name) == 0) {
            return cursor;
        }
    }
    return std::string::npos;
}

namespace utility {

bool get_argument(const char* arg_name, char* value, size_t value_size) {
    char token[MAX_CSTR_ARGS];
    const size_t match_end = find_argument_token(arg_name, token, sizeof(token));
    if (match_end == std::string::npos) {
        return false;
    }

    if (next_token(command_line(), match_end, token, sizeof(token)) == std::string::npos) {
        return false;
    }

    strncpy_s(value, value_size, token, _TRUNCATE);
    return true;
}

bool get_argument(const char* arg_name, char* value) {
    return get_argument(arg_name, value, MAX_PATH);
}

int get_argument(const char* arg_name) {
    char token[MAX_CSTR_ARGS];
    const size_t match_end = find_argument_token(arg_name, token, sizeof(token));
    if (match_end == std::string::npos) {
        return -1;
    }
    if (next_token(command_line(), match_end, token, sizeof(token)) == std::string::npos) {
        return -1;
    }
    return atoi(token);
}

bool check_argument(const char* arg_name) {
    char token[MAX_CSTR_ARGS];
    return find_argument_token(arg_name, token, sizeof(token)) != std::string::npos;
}
} // namespace utility
