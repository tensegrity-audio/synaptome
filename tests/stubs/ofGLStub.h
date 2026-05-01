#pragma once

#ifdef OF_SDK_AVAILABLE
#include <gl/ofGLUtils.h>
#else

using GLenum = unsigned int;
using GLboolean = unsigned char;

constexpr GLenum GL_DEPTH_TEST = 0x0B71;
constexpr GLenum GL_SCISSOR_TEST = 0x0C11;
constexpr GLenum GL_VERTEX_SHADER = 0x8B31;
constexpr GLenum GL_FRAGMENT_SHADER = 0x8B30;
constexpr GLenum GL_TEXTURE_2D = 0x0DE1;
constexpr GLenum GL_RGBA = 0x1908;
constexpr GLenum GL_LINEAR = 0x2601;
constexpr GLenum GL_CLAMP_TO_EDGE = 0x812F;

constexpr GLboolean GL_TRUE = 1;
constexpr GLboolean GL_FALSE = 0;

inline GLboolean glIsEnabled(GLenum) { return GL_FALSE; }
inline void glDisable(GLenum) {}
inline void glEnable(GLenum) {}

#endif
