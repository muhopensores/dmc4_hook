#ifndef HACKLIB_LOGGING_H
#define HACKLIB_LOGGING_H

#include <functional>
#include <string>


#ifdef _DEBUG
#define HL_LOG_DBG(format, ...) hl::LogDebug(__FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define HL_LOG_ERR(format, ...) hl::LogError(__FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#else
#define HL_LOG_DBG(format, ...)
#define HL_LOG_ERR(format, ...) hl::LogError(format, ##__VA_ARGS__);
#endif
#define HL_LOG_RAW(format, ...) hl::LogRaw(format, ##__VA_ARGS__);


namespace hl
{
struct LogConfig
{
    // To turn off logging to file.
    bool logToFile = true;
    // Whether or not the time is prepended.
    bool logTime = true;
    // Path of log file. Leave empty for "<moduleName>_log.txt"
    std::string fileName;
    // Log function.
    std::function<void(const std::string&)> logFunc;
};

void ConfigLog(const LogConfig& config);

void LogDebug(const char* file, const char* func, int line, const char* format, ...);
void LogError(const char* file, const char* func, int line, const char* format, ...);
void LogError(const char* format, ...);
void LogRaw(const char* format, ...);
}

#endif
