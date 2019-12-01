#include "hacklib/GfxOverlay.h"


using namespace hl;


GfxOverlay::GfxOverlay(ModuleHandle hModule) : m_hModule(hModule)
{
    impl_construct();
}

GfxOverlay::~GfxOverlay()
{
    close();

    impl_destruct();
}


GfxOverlay::Error GfxOverlay::create(int posX, int posY, int width, int height)
{
    // dont allow displaying twice
    close();

    m_posX = posX;
    m_posY = posY;
    m_width = width;
    m_height = height;

    // promise of return value of initialization
    std::promise<Error> p;
    auto ret = p.get_future();
    m_thread = std::thread(&GfxOverlay::impl_windowThread, this, std::ref(p));

    // wait for initialization to be done and return false on error
    auto err = ret.get();
    if (err == Error::Okay)
        m_isOpen = true;

    return err;
}


void GfxOverlay::close()
{
    if (m_isOpen)
    {
        impl_close();
    }

    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void GfxOverlay::setTargetRefreshRate(int rate)
{
    m_frameTime = std::chrono::nanoseconds(1000000000ull / rate);
}

bool GfxOverlay::isOpen() const
{
    return m_isOpen;
}


int GfxOverlay::getPosX() const
{
    return m_posX;
}
int GfxOverlay::getPosY() const
{
    return m_posY;
}
int GfxOverlay::getWidth() const
{
    return m_width;
}
int GfxOverlay::getHeight() const
{
    return m_height;
}


void GfxOverlay::cbWindowLoop()
{
    // Empty default implementation.
}
