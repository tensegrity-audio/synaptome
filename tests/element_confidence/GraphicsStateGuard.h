#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "ofMain.h"

namespace synaptome::tests::element_confidence {

inline bool matrixEqual(
    const glm::mat4& left,
    const glm::mat4& right,
    float epsilon = 0.00001f) {
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            if (std::fabs(left[column][row] - right[column][row]) >
                epsilon) {
                return false;
            }
        }
    }
    return true;
}

inline bool colorEqual(
    const ofFloatColor& left,
    const ofFloatColor& right,
    float epsilon = 0.00001f) {
    return std::fabs(left.r - right.r) <= epsilon &&
        std::fabs(left.g - right.g) <= epsilon &&
        std::fabs(left.b - right.b) <= epsilon &&
        std::fabs(left.a - right.a) <= epsilon;
}

inline bool styleEqual(const ofStyle& left, const ofStyle& right) {
    return colorEqual(left.color, right.color) &&
        colorEqual(left.bgColor, right.bgColor) &&
        left.polyMode == right.polyMode &&
        left.rectMode == right.rectMode &&
        left.bFill == right.bFill &&
        left.drawBitmapMode == right.drawBitmapMode &&
        left.blendingMode == right.blendingMode &&
        left.smoothing == right.smoothing &&
        left.circleResolution == right.circleResolution &&
        left.sphereResolution == right.sphereResolution &&
        left.curveResolution == right.curveResolution &&
        std::fabs(left.lineWidth - right.lineWidth) <= 0.00001f &&
        std::fabs(left.pointSize - right.pointSize) <= 0.00001f;
}

struct TextureBindings {
    GLint texture2d = 0;
#ifdef GL_TEXTURE_BINDING_RECTANGLE
    GLint rectangle = 0;
#endif
    GLint cubeMap = 0;
};

struct GraphicsStateSnapshot {
    GLint drawFramebuffer = 0;
    GLint readFramebuffer = 0;
    GLint viewport[4] = {};
    glm::mat4 modelView;
    glm::mat4 projection;
    glm::mat4 texture;
    ofStyle style;
    GLboolean blend = GL_FALSE;
    GLint blendSrcRgb = 0;
    GLint blendDstRgb = 0;
    GLint blendSrcAlpha = 0;
    GLint blendDstAlpha = 0;
    GLint blendEquationRgb = 0;
    GLint blendEquationAlpha = 0;
    GLboolean depth = GL_FALSE;
    GLint depthFunction = 0;
    GLboolean depthMask = GL_FALSE;
    GLboolean scissor = GL_FALSE;
    GLint scissorBox[4] = {};
    GLint program = 0;
    GLint activeTexture = 0;
    std::vector<TextureBindings> textures;

    static GraphicsStateSnapshot capture() {
        GraphicsStateSnapshot state;
        glGetIntegerv(
            GL_DRAW_FRAMEBUFFER_BINDING,
            &state.drawFramebuffer);
        glGetIntegerv(
            GL_READ_FRAMEBUFFER_BINDING,
            &state.readFramebuffer);
        glGetIntegerv(GL_VIEWPORT, state.viewport);
        state.modelView = ofGetCurrentMatrix(OF_MATRIX_MODELVIEW);
        state.projection = ofGetCurrentMatrix(OF_MATRIX_PROJECTION);
        state.texture = ofGetCurrentMatrix(OF_MATRIX_TEXTURE);
        state.style = ofGetStyle();
        state.blend = glIsEnabled(GL_BLEND);
        glGetIntegerv(GL_BLEND_SRC_RGB, &state.blendSrcRgb);
        glGetIntegerv(GL_BLEND_DST_RGB, &state.blendDstRgb);
        glGetIntegerv(
            GL_BLEND_SRC_ALPHA,
            &state.blendSrcAlpha);
        glGetIntegerv(
            GL_BLEND_DST_ALPHA,
            &state.blendDstAlpha);
        glGetIntegerv(
            GL_BLEND_EQUATION_RGB,
            &state.blendEquationRgb);
        glGetIntegerv(
            GL_BLEND_EQUATION_ALPHA,
            &state.blendEquationAlpha);
        state.depth = glIsEnabled(GL_DEPTH_TEST);
        glGetIntegerv(GL_DEPTH_FUNC, &state.depthFunction);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &state.depthMask);
        state.scissor = glIsEnabled(GL_SCISSOR_TEST);
        glGetIntegerv(GL_SCISSOR_BOX, state.scissorBox);
        glGetIntegerv(GL_CURRENT_PROGRAM, &state.program);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &state.activeTexture);
        GLint textureUnitCount = 0;
        glGetIntegerv(
            GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
            &textureUnitCount);
        state.textures.resize(
            static_cast<std::size_t>(std::max(0, textureUnitCount)));
        for (GLint unit = 0; unit < textureUnitCount; ++unit) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glGetIntegerv(
                GL_TEXTURE_BINDING_2D,
                &state.textures[unit].texture2d);
#ifdef GL_TEXTURE_BINDING_RECTANGLE
            glGetIntegerv(
                GL_TEXTURE_BINDING_RECTANGLE,
                &state.textures[unit].rectangle);
#endif
            glGetIntegerv(
                GL_TEXTURE_BINDING_CUBE_MAP,
                &state.textures[unit].cubeMap);
        }
        glActiveTexture(state.activeTexture);
        return state;
    }

    void restore() const {
        ofSetStyle(style);
        ofSetMatrixMode(OF_MATRIX_MODELVIEW);
        ofLoadMatrix(modelView);
        ofSetMatrixMode(OF_MATRIX_PROJECTION);
        ofLoadMatrix(projection);
        ofSetMatrixMode(OF_MATRIX_TEXTURE);
        ofLoadMatrix(texture);
        ofSetMatrixMode(OF_MATRIX_MODELVIEW);

        if (blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        glBlendFuncSeparate(
            blendSrcRgb,
            blendDstRgb,
            blendSrcAlpha,
            blendDstAlpha);
        glBlendEquationSeparate(
            blendEquationRgb,
            blendEquationAlpha);
        if (depth) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
        glDepthFunc(depthFunction);
        glDepthMask(depthMask);
        if (scissor) glEnable(GL_SCISSOR_TEST);
        else glDisable(GL_SCISSOR_TEST);
        glScissor(
            scissorBox[0],
            scissorBox[1],
            scissorBox[2],
            scissorBox[3]);
        glUseProgram(program);
        for (std::size_t unit = 0; unit < textures.size(); ++unit) {
            glActiveTexture(
                GL_TEXTURE0 + static_cast<GLenum>(unit));
            glBindTexture(
                GL_TEXTURE_2D,
                textures[unit].texture2d);
#ifdef GL_TEXTURE_BINDING_RECTANGLE
            glBindTexture(
                GL_TEXTURE_RECTANGLE,
                textures[unit].rectangle);
#endif
            glBindTexture(
                GL_TEXTURE_CUBE_MAP,
                textures[unit].cubeMap);
        }
        glActiveTexture(activeTexture);
        glBindFramebuffer(
            GL_DRAW_FRAMEBUFFER,
            drawFramebuffer);
        glBindFramebuffer(
            GL_READ_FRAMEBUFFER,
            readFramebuffer);
        glViewport(
            viewport[0],
            viewport[1],
            viewport[2],
            viewport[3]);
    }

    std::vector<std::string> differences(
        const GraphicsStateSnapshot& other) const {
        std::vector<std::string> result;
        if (drawFramebuffer != other.drawFramebuffer ||
            readFramebuffer != other.readFramebuffer) {
            result.push_back("framebuffer");
        }
        if (!std::equal(
                std::begin(viewport),
                std::end(viewport),
                std::begin(other.viewport))) {
            result.push_back("viewport");
        }
        if (!matrixEqual(modelView, other.modelView) ||
            !matrixEqual(projection, other.projection) ||
            !matrixEqual(texture, other.texture)) {
            result.push_back("matrices");
        }
        if (!styleEqual(style, other.style)) {
            result.push_back("style/color");
        }
        if (blend != other.blend ||
            blendSrcRgb != other.blendSrcRgb ||
            blendDstRgb != other.blendDstRgb ||
            blendSrcAlpha != other.blendSrcAlpha ||
            blendDstAlpha != other.blendDstAlpha ||
            blendEquationRgb != other.blendEquationRgb ||
            blendEquationAlpha != other.blendEquationAlpha) {
            result.push_back("blend");
        }
        if (depth != other.depth ||
            depthFunction != other.depthFunction ||
            depthMask != other.depthMask) {
            result.push_back("depth");
        }
        if (scissor != other.scissor ||
            !std::equal(
                std::begin(scissorBox),
                std::end(scissorBox),
                std::begin(other.scissorBox))) {
            result.push_back("scissor");
        }
        if (program != other.program) {
            result.push_back("shader/program");
        }
        if (activeTexture != other.activeTexture) {
            result.push_back("active-texture-unit");
        }
        if (textures.size() != other.textures.size()) {
            result.push_back("bound-textures");
        } else {
            for (std::size_t index = 0; index < textures.size(); ++index) {
                if (textures[index].texture2d !=
                        other.textures[index].texture2d ||
#ifdef GL_TEXTURE_BINDING_RECTANGLE
                    textures[index].rectangle !=
                        other.textures[index].rectangle ||
#endif
                    textures[index].cubeMap !=
                        other.textures[index].cubeMap) {
                    result.push_back("bound-textures");
                    break;
                }
            }
        }
        return result;
    }
};

class GraphicsStateGuard {
public:
    GraphicsStateGuard()
        : before_(GraphicsStateSnapshot::capture()) {}

    std::vector<std::string> restoreAndVerify() const {
        before_.restore();
        const auto restored = GraphicsStateSnapshot::capture();
        return before_.differences(restored);
    }

private:
    GraphicsStateSnapshot before_;
};

} // namespace synaptome::tests::element_confidence
