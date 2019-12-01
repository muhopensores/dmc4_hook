#include "hacklib/GfxOverlay_UNIX.h"
#include "hacklib/GfxOverlay.h"
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/shape.h>
#include <string.h>


using namespace hl;


GLXFBConfig GfxOverlayImpl::getFBConfig(int depthBits) const
{
    // Look for a config with R8G8B8A8 and alpha mask.

    GLXFBConfig chosenConfig = 0;

    int fbAttributes[] = { GLX_RENDER_TYPE,
                           GLX_RGBA_BIT,
                           GLX_DRAWABLE_TYPE,
                           GLX_WINDOW_BIT,
                           GLX_DOUBLEBUFFER,
                           True,
                           GLX_RED_SIZE,
                           8,
                           GLX_GREEN_SIZE,
                           8,
                           GLX_BLUE_SIZE,
                           8,
                           GLX_ALPHA_SIZE,
                           8,
                           GLX_DEPTH_SIZE,
                           depthBits,
                           None };

    int numfbconfigs = 0;
    auto fbconfigs = glXChooseFBConfig(display, screen, fbAttributes, &numfbconfigs);
    for (int i = 0; i < numfbconfigs; i++)
    {
        auto visual = glXGetVisualFromFBConfig(display, fbconfigs[i]);
        if (!visual)
        {
            continue;
        }

        auto pictFormat = XRenderFindVisualFormat(display, visual->visual);
        if (!pictFormat)
        {
            continue;
        }

        if (pictFormat->direct.alphaMask > 0)
        {
            chosenConfig = fbconfigs[i];
            break;
        }
    }

    return chosenConfig;
}


bool GfxOverlay::IsCompositionEnabled()
{
    XInitThreads();
    auto display = XOpenDisplay(NULL);
    if (!display)
    {
        throw std::runtime_error("Could not connect to X Server");
    }

    int screen = DefaultScreen(display);
    auto atomName = "_NET_WM_CM_S" + std::to_string(screen);

    auto atom = XInternAtom(display, atomName.c_str(), False);
    bool enabled = XGetSelectionOwner(display, atom) != None;

    XCloseDisplay(display);
    return enabled;
}


GraphicsContext GfxOverlay::getContext() const
{
    return m_impl->context;
}


void GfxOverlay::resetContext()
{
    glXMakeCurrent(m_impl->display, m_impl->hWndGL, m_impl->context);

    glDrawBuffer(GL_BACK);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
}

void GfxOverlay::beginDraw()
{
    glXMakeCurrent(m_impl->display, m_impl->hWndGL, m_impl->context);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GfxOverlay::swapBuffers()
{
    glXSwapBuffers(m_impl->display, m_impl->hWndGL);
}


void GfxOverlay::impl_construct()
{
    m_impl = new GfxOverlayImpl;

    XInitThreads();
    m_impl->display = XOpenDisplay(NULL);
    if (!m_impl->display)
    {
        throw std::runtime_error("Could not connect to X Server");
    }
}

void GfxOverlay::impl_destruct()
{
    XCloseDisplay(m_impl->display);

    delete m_impl;
}

void GfxOverlay::impl_close()
{
    auto display = m_impl->display;

    XEvent event = {};
    event.xclient.type = ClientMessage;
    event.xclient.window = m_hWnd;
    event.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", True);
    event.xclient.format = 32;
    event.xclient.data.l[0] = XInternAtom(display, "WM_DELETE_WINDOW", False);
    event.xclient.data.l[1] = CurrentTime;
    XSendEvent(display, m_hWnd, False, NoEventMask, &event);
}

void GfxOverlay::impl_windowThread(std::promise<Error>& p)
{
    auto display = m_impl->display;

    m_impl->screen = DefaultScreen(display);
    auto root = RootWindow(display, m_impl->screen);

    // Try to get a 24-bit depth buffer.
    GLXFBConfig fbconfig = m_impl->getFBConfig(24);
    if (!fbconfig)
    {
        fbconfig = m_impl->getFBConfig(16);
        if (!fbconfig)
        {
            p.set_value(Error::Other);
            return;
        }
    }
    auto visual = glXGetVisualFromFBConfig(display, fbconfig);

    auto cmap = XCreateColormap(display, root, visual->visual, AllocNone);
    if (!cmap)
    {
        p.set_value(Error::Other);
        return;
    }

    XSetWindowAttributes attributes = {};
    attributes.colormap = cmap;
    attributes.background_pixmap = None;
    attributes.border_pixmap = None;
    attributes.border_pixel = 0;
    attributes.event_mask = StructureNotifyMask | EnterWindowMask | LeaveWindowMask | ExposureMask | ButtonPressMask |
                            ButtonReleaseMask | OwnerGrabButtonMask | KeyPressMask | KeyReleaseMask;

    m_hWnd = XCreateWindow(display, root, m_posX, m_posY, m_width, m_height, 0, visual->depth, InputOutput,
                           visual->visual, CWColormap | CWBorderPixel | CWEventMask, &attributes);
    if (!m_hWnd)
    {
        p.set_value(Error::Window);
        return;
    }

    int glXattr[] = { None };
    m_impl->hWndGL = glXCreateWindow(display, fbconfig, m_hWnd, glXattr);
    if (!m_impl->hWndGL)
    {
        p.set_value(Error::Window);
        return;
    }

    static const char windowName[] = "Hacklib Overlay";
    XTextProperty nameProp;
    nameProp.value = (unsigned char*)windowName;
    nameProp.encoding = XA_STRING;
    nameProp.format = 8;
    nameProp.nitems = strlen(windowName);

    XSizeHints sizeHints;
    sizeHints.x = m_posX;
    sizeHints.y = m_posY;
    sizeHints.width = m_width;
    sizeHints.height = m_height;
    sizeHints.flags = USPosition | USSize;

    XWMHints* wmHints = XAllocWMHints();
    wmHints->initial_state = NormalState;
    wmHints->flags = StateHint;

    XSetWMProperties(display, m_hWnd, &nameProp, &nameProp, NULL, 0, &sizeHints, wmHints, NULL);

    XFree(wmHints);

    XMapWindow(display, m_hWnd);

    XEvent event;
    XIfEvent(display, &event,
             [](Display* display, XEvent* event, XPointer arg) -> Bool {
                 return display && event && arg && (event->type == MapNotify) && (event->xmap.window == *(Window*)arg);
             },
             (XPointer)&m_hWnd);

    auto deleteAtom = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, m_hWnd, &deleteAtom, 1);


    // Remove title bar and window border. Also any other decoration like shadows.
    auto windowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    auto typeDock = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(display, m_hWnd, windowType, XA_ATOM, 32, PropModeReplace, (unsigned char*)&typeDock, 1);

    // Set always-on-top.
    auto windowState = XInternAtom(display, "_NET_WM_STATE", False);
    auto stateAbove = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    XClientMessageEvent topEvent = {};
    topEvent.type = ClientMessage;
    topEvent.window = m_hWnd;
    topEvent.message_type = windowState;
    topEvent.format = 32;
    topEvent.data.l[0] = 1; // Add property.
    topEvent.data.l[1] = stateAbove;
    topEvent.data.l[2] = 0;
    topEvent.data.l[3] = 0;
    topEvent.data.l[4] = 0;
    XSendEvent(display, root, False, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent*)&topEvent);

    // Make the window click-through.
    auto region = XFixesCreateRegion(display, NULL, 0);
    XFixesSetWindowShapeRegion(display, m_hWnd, ShapeBounding, 0, 0, 0);
    XFixesSetWindowShapeRegion(display, m_hWnd, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(display, region);

    m_impl->context = glXCreateNewContext(display, fbconfig, GLX_RGBA_TYPE, 0, True);
    if (!m_impl->context)
    {
        p.set_value(Error::Context);
        return;
    }

    resetContext();
    glXMakeCurrent(display, 0, 0);

    p.set_value(Error::Okay);


    // Window message loop.
    typedef std::chrono::high_resolution_clock Clock;
    auto timestamp = Clock::now();
    bool running = true;
    while (running)
    {
        while (XPending(display))
        {
            XEvent event;
            XNextEvent(display, &event);
            if (event.type == ClientMessage && (Atom)event.xclient.data.l[0] == deleteAtom)
            {
                running = false;
            }
        }

        cbWindowLoop();

        auto until = timestamp + std::chrono::milliseconds(25);
        timestamp = Clock::now();
        std::this_thread::sleep_until(until);
    }

    // Easiest way to free all ressources.
    XCloseDisplay(m_impl->display);
    m_impl->display = XOpenDisplay(NULL);

    m_isOpen = false;
}
