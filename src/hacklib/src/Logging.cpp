#include "hacklib/Logging.h"
#include "hacklib/Main.h"
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <fstream>


static hl::LogConfig g_cfg;


class FormatStr
{
public:
    FormatStr(const char* format, va_list vl)
    {
        va_list vl_copy;
        va_copy(vl_copy, vl);

        int size = vsnprintf(nullptr, 0, format, vl);
        m_str = new char[size + 1];
        vsnprintf(m_str, size + 1, format, vl_copy);

        va_end(vl_copy);
    }
    ~FormatStr() { delete[] m_str; }
    const char* str() const { return m_str; }

private:
    char* m_str;
};


static std::string GetTimeStr()
{
    char strtime[256];
    auto timept = std::chrono::system_clock::now();
    auto timetp = std::chrono::system_clock::to_time_t(timept);
    strftime(strtime, sizeof(strtime), "%X", std::localtime(&timetp));

    return std::string(strtime);
}

static std::string GetCodeStr(const char* file, const char* func, int line)
{
    std::string fileStr = file;
    fileStr = fileStr.substr(fileStr.find_last_of('\\') + 1);
    fileStr = fileStr.substr(fileStr.find_last_of('/') + 1);

    std::string str;
    str += '[';
    str += fileStr;
    str += ':';
    str += std::to_string(line);
    str += '|';
    str += func;
    str += ']';
    return str;
}

static void LogString(const std::string& str)
{
    std::string logStr = str;
    if (g_cfg.logTime)
    {
        logStr = GetTimeStr() + " " + str;
    }

    if (g_cfg.logFunc)
    {
        g_cfg.logFunc(logStr);
    }

    if (g_cfg.logToFile)
    {
        std::ofstream logfile(g_cfg.fileName, std::ios::out | std::ios::app);
		if (logfile.fail())
		{
            MessageBox(NULL, "ofstream fail", "error", MB_ICONERROR);
			LPCSTR folder = ".\\dmc";
			CreateDirectory(folder, NULL);
			std::ofstream logfile(g_cfg.fileName, std::ios::out | std::ios::app);
		}
        logfile << logStr;
    }
}


void hl::ConfigLog(const LogConfig& config)
{
    g_cfg = config;

    if (g_cfg.fileName == "")
    {
        g_cfg.fileName = hl::GetCurrentModulePath() + "_log.txt";
    }
}


void hl::LogDebug(const char* file, const char* func, int line, const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    FormatStr str(format, vl);

    LogString(GetCodeStr(file, func, line) + " " + str.str());

    va_end(vl);
}

void hl::LogError(const char* file, const char* func, int line, const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    FormatStr str(format, vl);

    LogString(GetCodeStr(file, func, line) + " ERROR: " + str.str());

    va_end(vl);
}

void hl::LogError(const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    FormatStr str(format, vl);

    LogString(std::string("ERROR: ") + str.str());

    va_end(vl);
}

void hl::LogRaw(const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    FormatStr str(format, vl);

    LogString(str.str());

    va_end(vl);
}
