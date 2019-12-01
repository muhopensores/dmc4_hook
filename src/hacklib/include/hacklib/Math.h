#ifndef HACKLIB_MATH_H
#define HACKLIB_MATH_H

#ifdef _WIN32
#include "d3dx9.h"
#endif

#define GLM_FORCE_NO_CTOR_INIT
#include "glm/glm.hpp"


namespace hl
{
typedef glm::vec2 Vec2;
typedef glm::vec3 Vec3;
typedef glm::mat4x4 Mat4x4;
}

#endif
