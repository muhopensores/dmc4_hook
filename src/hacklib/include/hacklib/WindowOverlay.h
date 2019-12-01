#ifndef HACKLIB_WINDOWOVERLAY_H
#define HACKLIB_WINDOWOVERLAY_H

#include "GfxOverlay.h"


namespace hl
{
/**
 * \brief Wrapper for GfxOverlay that follows the main window of the current process.
 */
class WindowOverlay : public GfxOverlay
{
public:
    /// Returns the window that will be covered by the overlay.
    static WindowHandle GetTargetWindow();

public:
    WindowOverlay(ModuleHandle hModule = NULL);

    /// Create the overlay.
    /// The overlay dissapears if the target window loses focus. Set
    /// the parameter to true to prevent this.
    /// \return An hl::GfxOverlay::Error instance.
    GfxOverlay::Error create();

    /// When the host window is resized, the device must be reset.
    /// This function registers hooks before and after the Reset operation.
    /// For example see hl::Drawer::OnDeviceLost and OnDeviceReset.
    /// \param cbPreReset The function to call before resetting.
    /// \param cbPostReset The function to call after resetting.
    template <class F1, class F2>
    void registerResetHandlers(F1 cbPreReset, F2 cbPostReset)
    {
        m_cbPreReset = cbPreReset;
        m_cbPostReset = cbPostReset;
    }

private:
    virtual void cbWindowLoop();

private:
    WindowHandle m_targetWindow = 0;
    bool m_isTargetForeground = false;
    std::function<void()> m_cbPreReset;
    std::function<void()> m_cbPostReset;
};
}

#endif
