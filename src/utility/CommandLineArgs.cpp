#include "CommandLineArgs.hpp"
#include "processenv.h"
#include <climits>

#define MAX_CSTR_ARGS 1024

static char g_command_line_buffer[MAX_CSTR_ARGS];

namespace utility {

bool get_argument(const char* arg_name, char* value) {

    if(!g_command_line_buffer[0]) {
        strcpy_s(g_command_line_buffer, MAX_CSTR_ARGS, GetCommandLineA());
    }

    char* args = g_command_line_buffer;

    char buffer[512];
    strcpy_s(buffer, sizeof(buffer), args);

    char* token;
    char* saveptr;

    token = strtok(buffer, " ");

    while (token != NULL) {

        if (strcmp(token, arg_name) == 0) {

            token = strtok(NULL, " ");
            strcpy(value, token);

            return true;
        }

        token = strtok(NULL, " ");
    }

    return false;
}

int get_argument(const char* arg_name) {

    if(!g_command_line_buffer[0]) {
        strcpy_s(g_command_line_buffer, MAX_CSTR_ARGS, GetCommandLineA());
    }

    char* args = g_command_line_buffer;

    char buffer[512];
    strcpy_s(buffer, sizeof(buffer), args);

    char* token;
    char* saveptr;

    token = strtok(buffer, " ");

    while (token != NULL) {

        if (strcmp(token, arg_name) == 0) {

            token = strtok(NULL, " ");
            return atoi(token);
        }

        token = strtok(NULL, " ");
    }

    return -1;
}

bool check_argument(const char* arg_name) {

    if(!g_command_line_buffer[0]) {
        strcpy_s(g_command_line_buffer, MAX_CSTR_ARGS, GetCommandLineA());
    }

    const char* args = g_command_line_buffer;

    char buffer[512];
    strcpy_s(buffer, sizeof(buffer), args);

    char* token;
    char* saveptr;

    token = strtok(buffer, " ");

    while (token != NULL) {

        if (strcmp(token, arg_name) == 0) {
            return true;
        }

        token = strtok(NULL, " ");
    }

    return false;
}
} // namespace utility
