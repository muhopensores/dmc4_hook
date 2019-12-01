#include "hacklib/ConsoleEx.h"


using namespace hl;


static int g_wndClassRefCount = 0;

static const char WNDCLASS_CONSOLEEX[] = "CONSOLEEX";

static const int CELLWIDTH = 8;
static const int CELLHEIGHT = 15;
static const int SPACING = 3;

static const uintptr_t IDC_EDITOUT = 100;
static const uintptr_t IDC_EDITIN = 101;


CONSOLEEX_PARAMETERS ConsoleEx::GetDefaultParameters()
{
    CONSOLEEX_PARAMETERS parameters = {};
    parameters.cellsX = 80;
    parameters.cellsYVisible = 20;
    parameters.cellsYBuffer = 500;
    parameters.bgColor = RGB(0, 0, 0);
    parameters.textColor = RGB(192, 192, 192);
    parameters.closemenu = false;
    return parameters;
}


ConsoleEx::ConsoleEx(HINSTANCE hModule)
{
    if (hModule)
        m_hModule = hModule;
    else
        m_hModule = GetModuleHandle(NULL);

    if (!g_wndClassRefCount++)
    {
        WNDCLASSA wc = {};
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = s_WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = sizeof(this);
        wc.hInstance = m_hModule;
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_GRAYTEXT + 1);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = WNDCLASS_CONSOLEEX;

        RegisterClassA(&wc);
    }
}

ConsoleEx::~ConsoleEx()
{
    close();

    // unregister window class when there will be 0 refs
    if (!--g_wndClassRefCount)
        UnregisterClassA(WNDCLASS_CONSOLEEX, m_hModule);
}


bool ConsoleEx::create(const std::string& windowTitle, CONSOLEEX_PARAMETERS* parameters)
{
    // dont allow displaying twice
    close();

    // overwrite default parameters if custom ones are given
    if (parameters)
        m_parameters = *parameters;

    m_title = windowTitle;

    // promise of return value of initialization
    std::promise<bool> p;
    auto ret = p.get_future();
    m_thread = std::thread(&ConsoleEx::threadProc, this, std::ref(p));

    // wait for initialization to be done and return false on error
    if (!ret.get())
        return false;

    m_isOpen = true;
    return true;
}

void ConsoleEx::registerInputCallback(void (*cbInput)(std::string))
{
    m_cbInput = cbInput;
}


void ConsoleEx::close()
{
    if (m_isOpen)
        SendMessageA(m_hWnd, WM_CLOSE, 0, 0);

    if (m_thread.joinable())
        m_thread.join();
}

bool ConsoleEx::isOpen() const
{
    return m_isOpen;
}

HWND ConsoleEx::getWindowHandle() const
{
    return m_hWnd;
}


void ConsoleEx::vprintf(const char* format, va_list valist)
{
    va_list valist_copy;
    va_copy(valist_copy, valist);

    // get size of formatted string
    int size = vsnprintf(nullptr, 0, format, valist);
    // allocate buffer to hold formatted string + null terminator
    char* pBuffer = new char[size + 1];
    // write formatted string to buffer
    vsnprintf(pBuffer, size + 1, format, valist_copy);

    writeStringToRawBuffer(pBuffer);

    delete[] pBuffer;
    va_end(valist_copy);
}

void ConsoleEx::printf(const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    vprintf(format, vl);
    va_end(vl);
}


// WndProc for CONSOLEEX window class
LRESULT CALLBACK ConsoleEx::s_WndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    // if msg is WM_NCCREATE, associate instance ptr from createparameter to this window
    // note: WM_NCCREATE is not the first msg to be sent to a window
    if (msg == WM_NCCREATE)
    {
        LPCREATESTRUCT cs = reinterpret_cast<LPCREATESTRUCT>(lparam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }
    // get instance ptr associated with this window
    ConsoleEx* pInstance = reinterpret_cast<ConsoleEx*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    // call member function to handle messages indiviually for each instance
    // always check ptr because wndproc is called before association
    if (pInstance)
    {
        return pInstance->wndProc(hWnd, msg, wparam, lparam);
    }
    // else
    return DefWindowProc(hWnd, msg, wparam, lparam);
}


LRESULT ConsoleEx::wndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CLOSE:
        // post WM_DESTORY
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        if (m_hbrBackGround)
            DeleteObject(m_hbrBackGround);

        // post WM_QUIT
        PostQuitMessage(0);
        return 0;
    case WM_CREATE:
    {
        // create brush for edit control backgrounds
        m_hbrBackGround = CreateSolidBrush(m_parameters.bgColor);

        // input edit control
        m_hEditIn = CreateWindowA("EDIT", 0,
                                  // styles for console input edit control
                                  WS_CHILD | WS_VISIBLE | ES_LEFT |
                                      // will make sure EN_MAXTEXT is sent when hitting enter key
                                      ES_MULTILINE | ES_AUTOHSCROLL,
                                  // metrics
                                  0, m_parameters.cellsYVisible * CELLHEIGHT + SPACING, m_parameters.cellsX * CELLWIDTH,
                                  CELLHEIGHT, hWnd, reinterpret_cast<HMENU>(IDC_EDITIN), m_hModule, NULL);

        // output edit control
        m_hEditOut = CreateWindowW(L"EDIT", 0,
                                   // styles for console output edit box
                                   WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE |
                                       // will prevent line endings from jumping to the next line
                                       ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_READONLY |
                                       // raw buffer will be accessable from this program
                                       DS_LOCALEDIT,
                                   // metrics
                                   0, 0, m_parameters.cellsX * CELLWIDTH + GetSystemMetrics(SM_CXVSCROLL),
                                   m_parameters.cellsYVisible * CELLHEIGHT, hWnd, reinterpret_cast<HMENU>(IDC_EDITOUT),
                                   m_hModule, NULL);

        if (!m_hbrBackGround || !m_hEditOut || !m_hEditIn)
        {
            // createwindow of parent will return 0
            return -1;
        }

        // set font in edit controls to system monospaced font
        HGDIOBJ fixedFont = GetStockObject(SYSTEM_FIXED_FONT);
        SendMessageA(m_hEditOut, WM_SETFONT, reinterpret_cast<WPARAM>(fixedFont), MAKELPARAM(FALSE, 0));
        SendMessageA(m_hEditIn, WM_SETFONT, reinterpret_cast<WPARAM>(fixedFont), MAKELPARAM(FALSE, 0));

        // get edit control raw buffer
        m_rawBuffer.setBuffer(reinterpret_cast<HLOCAL>(SendMessageA(m_hEditOut, EM_GETHANDLE, 0, 0)));

        // resize raw edit control buffer
        if (!m_rawBuffer.resize(m_parameters.cellsX, m_parameters.cellsYBuffer))
        {
            // createwindow of parent will return 0
            return -1;
        }

        // set up raw edit control buffer with spaces, CRLF and null termination
        m_rawBuffer.lock();
        m_rawBuffer.clear();
        m_rawBuffer.unlock();

        SendMessageA(m_hEditOut, EM_SETHANDLE, reinterpret_cast<WPARAM>(m_rawBuffer.getBuffer()), 0);

        // scroll to bottom
        SendMessageA(m_hEditOut, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
    }
        return 0;
    case WM_CTLCOLORSTATIC:
        SetBkColor(reinterpret_cast<HDC>(wparam), m_parameters.bgColor);
        SetTextColor(reinterpret_cast<HDC>(wparam), m_parameters.textColor);
        return reinterpret_cast<INT_PTR>(m_hbrBackGround);
    case WM_CTLCOLOREDIT:
        SetBkColor(reinterpret_cast<HDC>(wparam), m_parameters.bgColor);
        SetTextColor(reinterpret_cast<HDC>(wparam), m_parameters.textColor);
        return reinterpret_cast<INT_PTR>(m_hbrBackGround);
    case WM_COMMAND:
    {
        // message when enter was pressed in input edit
        if (LOWORD(wparam) == IDC_EDITIN && HIWORD(wparam) == EN_MAXTEXT)
        {
            // truncate input when it is longer that 256 characters
            // GetWindowTextA nullterminates string correctly
            char inputBuffer[256];
            GetWindowTextA(m_hEditIn, inputBuffer, 256);

            // only call callback when input is not empty
            if (inputBuffer[0] != '\0')
            {
                // clear content of input edit
                SetWindowTextA(m_hEditIn, "");

                // callback for input
                if (m_cbInput)
                {
                    m_cbInput(inputBuffer);
                }
            }
        }
    }
        return 0;
    }

    return DefWindowProcA(hWnd, msg, wparam, lparam);
}

void ConsoleEx::threadProc(std::promise<bool>& p)
{
    // calculate window size that will have the matching client size
    // height: number of output cells + input edit line + spacing between the two
    RECT rect;
    rect.left = CW_USEDEFAULT;
    rect.top = CW_USEDEFAULT;
    rect.right = CW_USEDEFAULT + GetSystemMetrics(SM_CXVSCROLL) + m_parameters.cellsX * CELLWIDTH;
    rect.bottom = CW_USEDEFAULT + (m_parameters.cellsYVisible + 1) * CELLHEIGHT + SPACING;
    AdjustWindowRect(&rect, WS_CAPTION, FALSE);

    // create parent window. WM_CREATE will be executed before returning
    m_hWnd = CreateWindowA(WNDCLASS_CONSOLEEX, m_title.c_str(),
                           // clip children to prevent flicker
                           WS_CAPTION | WS_CLIPCHILDREN | (m_parameters.closemenu ? WS_SYSMENU : 0),
                           // metrics
                           CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, HWND_DESKTOP,
                           NULL, m_hModule,
                           // pass a ptr to this instance as creation parameter
                           this);

    // any errors that could occur at creation will result in m_hWnd to be 0
    if (!m_hWnd)
    {
        // cleanup partially initialized stuff
        m_rawBuffer.free();

        // signal failure and exit thread
        p.set_value(false);
        return;
    }

    ShowWindow(m_hWnd, SW_SHOWDEFAULT);

    // signal finished initialization
    p.set_value(true);

    // process window messages until WM_QUIT
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    m_isOpen = false;
    m_hWnd = NULL;
}


// function splits argument pBuffer into strings that can be drawn to a single console line
// splits occur at line feed
void ConsoleEx::writeStringToRawBuffer(char* strOut)
{
    int len = static_cast<int>(strlen(strOut));

    // convert string to wide chars
    wchar_t* strOutW = new wchar_t[len + 1];
    size_t copied;
    mbstowcs_s(&copied, strOutW, len + 1, strOut, len);

    // remove special characters: horizontal tab ('\t'), vertical tab ('\v'), backspace ('\b'), carriage return ('\r'),
    // form feed ('\f'), alert ('\a')
    for (int i = 0; i < len; i++)
    {
        if (strOutW[i] == L'\t' || strOutW[i] == L'\v' || strOutW[i] == L'\b' || strOutW[i] == L'\r' ||
            strOutW[i] == L'\f' || strOutW[i] == L'\a')
        {
            memmove(&strOutW[i], &strOutW[i + 1], (len - i + 1) * sizeof(wchar_t));
            i--;
            len--;
        }
    }

    // replace LF ('\n') with null terminator ('\0') to split string
    for (int i = 0; i < len; i++)
    {
        if (strOutW[i] == L'\n')
        {
            strOutW[i] = L'\0';
        }
    }

    m_rawBuffer.lock();
    wchar_t* pOut = strOutW;
    for (int i = 0; i < len; i++)
    {
        // if premature null terminator is found, write and scroll
        if (strOutW[i] == L'\0')
        {
            m_rawBuffer.write(pOut);
            m_rawBuffer.scrollUp();
            pOut = &strOutW[i + 1];
        }
        // write last chunk
        if (i == len - 1)
        {
            m_rawBuffer.write(pOut);
        }
    }
    m_rawBuffer.unlock();

    delete[] strOutW;

    // queue redraw
    if (m_hEditOut)
        InvalidateRect(m_hEditOut, NULL, FALSE);
}


// =============================================
// === ConsoleEx::ConsoleBuffer
// =============================================

ConsoleEx::ConsoleBuffer::ConsoleBuffer()
{
    m_bufferHandle = NULL;
    m_buffer = nullptr;
    m_cursorPos = 0;
    m_wid = m_hei = 0;
}
ConsoleEx::ConsoleBuffer::~ConsoleBuffer()
{
    free();
}

void ConsoleEx::ConsoleBuffer::free()
{
    if (m_bufferHandle)
    {
        LocalFree(m_bufferHandle);
        m_bufferHandle = NULL;
    }
}
HLOCAL ConsoleEx::ConsoleBuffer::getBuffer() const
{
    return m_bufferHandle;
}
void ConsoleEx::ConsoleBuffer::setBuffer(HLOCAL buffer)
{
    if (!m_bufferHandle)
    {
        m_bufferHandle = buffer;
    }
}

void ConsoleEx::ConsoleBuffer::lock()
{
    m_writeMutex.lock();
    m_buffer = reinterpret_cast<wchar_t*>(LocalLock(m_bufferHandle));
}
void ConsoleEx::ConsoleBuffer::unlock()
{
    LocalUnlock(m_bufferHandle);
    m_buffer = nullptr;
    m_writeMutex.unlock();
}

bool ConsoleEx::ConsoleBuffer::resize(int width, int height)
{
    if (!m_bufferHandle)
    {
        return false;
    }
    m_wid = width;
    m_hei = height;

    // size: a line has 'width' characters and CRLF ('\r','\n')
    // last line doesnt need CRLF but a null terminator ('\0')
    m_bufferHandle = LocalReAlloc(m_bufferHandle, ((width + 2) * height - 1) * sizeof(wchar_t), LMEM_MOVEABLE);
    if (!m_bufferHandle)
    {
        return false;
    }
    return true;
}
void ConsoleEx::ConsoleBuffer::clear()
{
    if (m_buffer)
    {
        // buffer will contain spaces (' ') in the usable buffer area
        for (int i = 0; i < (m_wid + 2) * m_hei - 2; i++)
        {
            m_buffer[i] = L' ';
        }
        // buffer ends with null terminator ('\0')
        m_buffer[(m_wid + 2) * m_hei - 2] = L'\0';
        // every line but the last will end with CRLF ('\r','\n')
        for (int i = 0; i < m_hei - 1; i++)
        {
            m_buffer[(m_wid + 2) * i + m_wid] = L'\r';
            m_buffer[(m_wid + 2) * i + m_wid + 1] = L'\n';
        }
        // insert cursor ('_') at beginning of last line
        m_cursorPos = 0;
        m_buffer[getBufferOffsetFromCursorPos()] = L'_';

        // buffer looks like this: (m_wid=10,m_hei=3)
        // '          \r\n'
        // '          \r\n'
        // '_         \0'
    }
}

// function expects input string to not contain '\r','\n','\t'
void ConsoleEx::ConsoleBuffer::write(wchar_t* str)
{
    if (m_buffer)
    {
        // ptr to start of snippet that is drawn
        wchar_t* pStr = str;

        while (pStr)
        {
            int len = static_cast<int>(wcslen(pStr));

            int copyLen = len;
            if (copyLen > m_wid - m_cursorPos)
            {
                copyLen = m_wid - m_cursorPos;
            }

            // dont use strcpy to not mess crafted buffer up
            memcpy(&m_buffer[getBufferOffsetFromCursorPos()], pStr, copyLen * sizeof(wchar_t));

            // adjust cursor position
            m_cursorPos += copyLen;
            // draw cursor when it is visible
            if (m_cursorPos < m_wid)
            {
                m_buffer[getBufferOffsetFromCursorPos()] = L'_';
            }

            // if string was not fully outputed, adjust ptr and continue
            if (copyLen != len)
            {
                scrollUp();
                pStr = &pStr[copyLen];
            }
            else
            {
                pStr = nullptr;
            }
        }
    }
}
void ConsoleEx::ConsoleBuffer::scrollUp()
{
    if (m_buffer)
    {
        // erase cursor when it is visible
        if (m_cursorPos < m_wid)
        {
            m_buffer[getBufferOffsetFromCursorPos()] = L' ';
        }

        // scroll all content of buffer up a line
        memmove(m_buffer, &m_buffer[m_wid + 2], (((m_wid + 2) * (m_hei - 1)) - 2) * sizeof(wchar_t));
        // add CRLF to line before last line
        m_buffer[(m_wid + 2) * (m_hei - 2) + m_wid] = L'\r';
        m_buffer[(m_wid + 2) * (m_hei - 2) + m_wid + 1] = L'\n';
        // erase new line
        for (int i = 0; i < m_wid; i++)
        {
            m_buffer[((m_wid + 2) * (m_hei - 1)) + i] = L' ';
        }

        // set cursor
        m_cursorPos = 0;
        m_buffer[getBufferOffsetFromCursorPos()] = L'_';
    }
}


int ConsoleEx::ConsoleBuffer::getBufferOffsetFromCursorPos()
{
    return (m_wid + 2) * (m_hei - 1) + m_cursorPos;
}
