#ifndef HACKLIB_DRAWEROPENGL_H
#define HACKLIB_DRAWEROPENGL_H

#include "hacklib/IDrawer.h"


namespace hl
{
/**
 * \brief Drawer implementation for OpenGL.
 */
class DrawerOpenGL : public hl::IDrawer
{
public:
    void drawLine(float x1, float y1, float x2, float y2, hl::Color color) const override;
    void drawRect(float x, float y, float w, float h, hl::Color color) const override;
    void drawRectFilled(float x, float y, float w, float h, hl::Color color) const override;
    void drawCircle(float mx, float my, float r, hl::Color color) const override;
    void drawCircleFilled(float mx, float my, float r, hl::Color color) const override;

protected:
    void updateDimensions() override;
};
}

#endif
