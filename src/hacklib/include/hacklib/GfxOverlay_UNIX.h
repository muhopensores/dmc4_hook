#ifndef HACKLIB_GFXOVERLAY_UNIX_H
#define HACKLIB_GFXOVERLAY_UNIX_H

#include <GL/glx.h>


namespace hl {


class GfxOverlayImpl
{
public:
    GLXFBConfig getFBConfig(int depthBits) const;

public:
    Display *display = nullptr;
    int screen = 0;
    GLXWindow hWndGL = 0;
    GLXContext context = 0;

};

}

#endif
