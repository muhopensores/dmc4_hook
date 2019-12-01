#ifndef HACKLIB_CONSOLEEX_H
#define HACKLIB_CONSOLEEX_H

#include <Windows.h>
#include <functional>
#include <future>
#include <string>
#include <thread>


namespace hl
{
struct CONSOLEEX_PARAMETERS
{
    // width of console buffer in characters
    int cellsX;
    // height of visible console buffer in characters
    int cellsYVisible;
    // height of total console buffer in characters
    int cellsYBuffer;
    // background color for output and input child windows
    COLORREF bgColor;
    // text color for output and input child windows
    COLORREF textColor;
    // specifies if the console can be closed via windows shell
    bool closemenu;
};


/*
 * A console with simultaneous input and output for Windows.
 */
class ConsoleEx
{
    ConsoleEx(const ConsoleEx&) = delete;
    ConsoleEx& operator=(const ConsoleEx&) = delete;

public:
    // fetches default initialisation of parameter struct
    static CONSOLEEX_PARAMETERS GetDefaultParameters();


public:
    ConsoleEx(HINSTANCE hModule = NULL);
    ~ConsoleEx();

    // opens console and starts handling of all events
    // waits for private thread to run (eg. will deadlock when called in DllMain)
    bool create(const std::string& windowTitle, CONSOLEEX_PARAMETERS* parameters = nullptr);

    // registers a callback that will be called on input
    void registerInputCallback(void (*cbInput)(std::string));
    template <class F>
    void registerInputCallback(F cbInput)
    {
        m_cbInput = cbInput;
    }

    void close();
    bool isOpen() const;
    HWND getWindowHandle() const;

    // member functions to send output to console
    void vprintf(const char* format, va_list valist);
    void printf(const char* format, ...);


private:
    static LRESULT CALLBACK s_WndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);


private:
    // member functions to process window and thread callbacks
    LRESULT wndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
    void threadProc(std::promise<bool>& p);

    // writes arbitrary string to ConsoleBuffer
    void writeStringToRawBuffer(char* strOut);


    std::thread m_thread;
    HINSTANCE m_hModule = NULL;
    HWND m_hWnd = NULL;
    HWND m_hEditOut = NULL;
    HWND m_hEditIn = NULL;
    HBRUSH m_hbrBackGround = NULL;
    std::string m_title;
    CONSOLEEX_PARAMETERS m_parameters = GetDefaultParameters();
    std::function<void(std::string)> m_cbInput;
    bool m_isOpen = false;


    // abstraction of dirty raw buffer modifications
    class ConsoleBuffer
    {
        ConsoleBuffer(const ConsoleBuffer&) = delete;
        ConsoleBuffer& operator=(const ConsoleBuffer&) = delete;

    public:
        ConsoleBuffer();
        ~ConsoleBuffer();

        void free();
        HLOCAL getBuffer() const;
        void setBuffer(HLOCAL buffer);

        void lock();
        void unlock();

        bool resize(int width, int height);
        void clear();

        // function expects input string to not contain special characters like '\r','\n','\t', etc
        void write(wchar_t* str);
        void scrollUp();

    private:
        int getBufferOffsetFromCursorPos();

        mutable std::mutex m_writeMutex;
        HLOCAL m_bufferHandle;
        wchar_t* m_buffer;
        int m_cursorPos;
        int m_wid, m_hei;

    } m_rawBuffer;
};
}

#endif
