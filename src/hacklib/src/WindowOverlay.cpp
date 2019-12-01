#include "hacklib/WindowOverlay.h"


using namespace hl;


WindowOverlay::WindowOverlay(ModuleHandle hModule) : GfxOverlay(hModule) {}


GfxOverlay::Error WindowOverlay::create()
{
    // get window that belongs to current process
    m_targetWindow = GetTargetWindow();
    if (!m_targetWindow)
        return Error::Other;

    // adjust metrics
    cbWindowLoop();
    m_isTargetForeground = true;

    return GfxOverlay::create(m_posX, m_posY, m_width, m_height);
}
