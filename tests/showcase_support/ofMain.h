#pragma once
#include "glm/glm.hpp"
#include "ofGraphics.h"
#include "ofUtils.h"
#include "ofFileUtils.h"
#include "ofEvents.h"
#include "ofLog.h"
#include "ofCamera.h"
#include "ofMesh.h"
inline float ofDegToRad(float degrees){return degrees*.01745329251994329577f;}

using GLenum=unsigned int; using GLboolean=unsigned char;
inline constexpr GLenum GL_DEPTH_TEST=0x0B71, GL_CULL_FACE=0x0B44;
inline GLboolean glIsEnabled(GLenum flag){return flag==GL_DEPTH_TEST?showcase_test::recorder.depth:showcase_test::recorder.cull;}
inline void glEnable(GLenum flag){if(flag==GL_DEPTH_TEST)showcase_test::recorder.depth=true;else showcase_test::recorder.cull=true;}
inline void glDisable(GLenum flag){if(flag==GL_DEPTH_TEST)showcase_test::recorder.depth=false;else showcase_test::recorder.cull=false;}
