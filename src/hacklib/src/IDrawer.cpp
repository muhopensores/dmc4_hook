#include "hacklib/IDrawer.h"
#include "glm/gtc/matrix_transform.hpp"
#include <GL/gl.h>
#include <stdexcept>


using namespace hl;


Color::Color() : Color(0, 0, 0, 0) {}
Color::Color(uint8_t red, uint8_t green, uint8_t blue) : Color(255, red, green, blue) {}
Color::Color(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue)
{
    m_data = (alpha << 24) | (red << 16) | (green << 8) | blue;
}
Color::Color(uint32_t combined)
{
    m_data = combined;
}

void Color::glSet() const
{
    glColor4ub((m_data >> 16) & 0xff, (m_data >> 8) & 0xff, m_data & 0xff, (m_data >> 24) & 0xff);
}


void IDrawer::clearRessources() {}


void IDrawer::setContext(hl::GraphicsContext context)
{
    clearRessources();
    m_context = context;
}

hl::GraphicsContext IDrawer::getContext() const
{
    return m_context;
}


void IDrawer::update(const hl::Mat4x4& viewMatrix, const hl::Mat4x4& projectionMatrix)
{
    updateDimensions();

    m_viewMatrix = viewMatrix;
    m_projMatrix = projectionMatrix;
}


float IDrawer::getWidth() const
{
    return m_width;
}

float IDrawer::getHeight() const
{
    return m_height;
}


void IDrawer::project(const hl::Vec3& worldPos, hl::Vec3& screenPos, const hl::Mat4x4* worldMatrix) const
{
    hl::Mat4x4 modelMatrix = m_viewMatrix;
    if (worldMatrix)
    {
        modelMatrix = modelMatrix * *worldMatrix;
    }

    glm::vec4 viewport(0.0f, 0.0f, m_width, m_height);
    screenPos = glm::project(worldPos, modelMatrix, m_projMatrix, viewport);

    // Invert Y axis, because the drawer interface assumes a top to bottom (d3d-like) viewport.
    screenPos[1] = m_height - screenPos[1];
}

bool IDrawer::isInfrontCam(const hl::Vec3& screenPos) const
{
    return screenPos.z <= 1;
}

bool IDrawer::isOnScreen(const hl::Vec3& screenPos, float offScreenTolerance) const
{
    return (screenPos.x + offScreenTolerance >= 0) && (screenPos.x - offScreenTolerance <= m_width) &&
           (screenPos.y + offScreenTolerance >= 0) && (screenPos.y - offScreenTolerance <= m_height);
}


void IDrawer::drawLine(float x1, float y1, float x2, float y2, hl::Color color) const
{
    throw std::runtime_error("not implemented");
}

void IDrawer::drawLineProjected(hl::Vec3 pos1, hl::Vec3 pos2, hl::Color color) const
{
    project(pos1, pos1);
    project(pos2, pos2);

    if (isInfrontCam(pos1) && isInfrontCam(pos2))
        drawLine(pos1.x, pos1.y, pos2.x, pos2.y, color);
}

void IDrawer::drawRect(float x, float y, float w, float h, hl::Color color) const
{
    throw std::runtime_error("not implemented");
}

void IDrawer::drawRectFilled(float x, float y, float w, float h, hl::Color color) const
{
    throw std::runtime_error("not implemented");
}

void IDrawer::drawCircle(float mx, float my, float r, hl::Color color) const
{
    throw std::runtime_error("not implemented");
}

void IDrawer::drawCircleFilled(float mx, float my, float r, hl::Color color) const
{
    throw std::runtime_error("not implemented");
}


void IDrawer::onLostDevice() {}

void IDrawer::onResetDevice() {}
