#include "hacklib/WindowOverlay.h"


using namespace hl;


static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    /* criteria: owned by current process, visible, not a child window and client size > 0
        - owned by current process
        - visible
        - not a child window
        - created by main module (exe)
        - client size > 0
    */
    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    if (pid == GetCurrentProcessId())
    {
        LONG_PTR styles = GetWindowLongPtr(hWnd, GWL_STYLE);
        HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
        if ((styles & WS_VISIBLE) && !(styles & WS_CHILDWINDOW) && hinst == GetModuleHandle(NULL))
        {
            RECT rect;
            GetClientRect(hWnd, &rect);
            if (rect.right != 0 && rect.bottom != 0)
            {
                *(HWND*)lParam = hWnd;
            }
        }
    }

    return TRUE;
}

WindowHandle WindowOverlay::GetTargetWindow()
{
    HWND hWnd = NULL;
    EnumWindows(EnumWindowsProc, (LPARAM)&hWnd);
    return hWnd;
}


void WindowOverlay::cbWindowLoop()
{
    // this method makes sure that the overlay is always matching the target window

    bool changeFound = false;
    UINT setWndPosFlags = SWP_NOACTIVATE;

    // window position changes
    POINT pt = {};
    if (!ClientToScreen(m_targetWindow, &pt) || (m_posX == pt.x && m_posY == pt.y))
    {
        setWndPosFlags |= SWP_NOMOVE;
    }
    else
    {
        m_posX = pt.x;
        m_posY = pt.y;
        changeFound = true;
    }

    // window size changes
    RECT rect = {};
    if (!GetClientRect(m_targetWindow, &rect) || (m_width == rect.right && m_height == rect.bottom))
    {
        setWndPosFlags |= SWP_NOSIZE;
    }
    else
    {
        m_width = rect.right;
        m_height = rect.bottom;
        changeFound = true;
    }

    // window visibility changes
    HWND insertAfter = NULL;
    HWND foregroundWindow = GetForegroundWindow();
    bool isForeground = m_targetWindow == foregroundWindow;
    if (m_isTargetForeground == isForeground)
    {
        setWndPosFlags |= SWP_NOZORDER;
    }
    else
    {
        m_isTargetForeground = isForeground;
        changeFound = true;

        if (m_isTargetForeground)
        {
            // Set to topmost so the target can keep being the foreground window.
            insertAfter = HWND_TOPMOST;
        }
        else
        {
            // Get the window above the target so we can insert after that one.
            insertAfter = GetWindow(m_targetWindow, GW_HWNDPREV);
            if (!insertAfter)
            {
                // Nothing above the target. Just make the overlay no longer topmost.
                insertAfter = HWND_NOTOPMOST;
            }
        }
    }

    if (changeFound && m_hWnd)
    {
        SetWindowPos(m_hWnd, insertAfter, m_posX, m_posY, m_width, m_height, setWndPosFlags);
    }

    // if size has changed, reset the overlay
    if (getContext() && !(setWndPosFlags & SWP_NOSIZE))
    {
        if (m_cbPreReset)
            m_cbPreReset();

        resetContext();

        if (m_cbPostReset)
            m_cbPostReset();
    }
}
