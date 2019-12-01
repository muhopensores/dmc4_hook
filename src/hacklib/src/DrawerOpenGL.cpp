#define _USE_MATH_DEFINES

#include "hacklib/DrawerOpenGL.h"
#include <GL/gl.h>
#include <cmath>


using namespace hl;


void DrawerOpenGL::drawLine(float x1, float y1, float x2, float y2, hl::Color color) const
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, m_width, m_height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glLineWidth(1.0);
    color.glSet();
    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

void DrawerOpenGL::drawRect(float x, float y, float w, float h, hl::Color color) const
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, m_width, m_height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_LINE_LOOP);
    color.glSet();
    glVertex2f(x, y);
    glVertex2f(x, y + h);
    glVertex2f(x + w, y + h);
    glVertex2f(x + w, y);
    glEnd();
}

void DrawerOpenGL::drawRectFilled(float x, float y, float w, float h, hl::Color color) const
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, m_width, m_height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_QUADS);
    color.glSet();
    glVertex2f(x, y);
    glVertex2f(x, y + h);
    glVertex2f(x + w, y + h);
    glVertex2f(x + w, y);
    glEnd();
}

void DrawerOpenGL::drawCircle(float mx, float my, float r, hl::Color color) const
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, m_width, m_height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_LINE_LOOP);
    color.glSet();
    for (int i = 0; i < CIRCLE_RESOLUTION; i++)
    {
        glVertex2f(mx + r * cos(static_cast<float>(M_PI) * (i / (CIRCLE_RESOLUTION / 2.0f))),
                   my + r * sin(static_cast<float>(M_PI) * (i / (CIRCLE_RESOLUTION / 2.0f))));
    }
    glEnd();
}

void DrawerOpenGL::drawCircleFilled(float mx, float my, float r, hl::Color color) const
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, m_width, m_height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_TRIANGLE_FAN);
    color.glSet();
    for (int i = 0; i < CIRCLE_RESOLUTION + 1; i++)
    {
        glVertex2f(mx + r * cos(static_cast<float>(M_PI) * (i / (CIRCLE_RESOLUTION / 2.0f))),
                   my + r * sin(static_cast<float>(M_PI) * (i / (CIRCLE_RESOLUTION / 2.0f))));
    }
    glEnd();
}


void DrawerOpenGL::updateDimensions()
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    m_width = static_cast<float>(viewport[2]);
    m_height = static_cast<float>(viewport[3]);
}
