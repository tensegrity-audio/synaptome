#pragma once

#ifdef OF_SDK_AVAILABLE
#include <gl/ofShader.h>
#else

#include <string>

#include "ofGLStub.h"
#include "ofTexture.h"

class ofShader {
public:
    void setupShaderFromSource(GLenum, const std::string&) {}

    void bindDefaults() {}
    void linkProgram() {}

    void begin() const {}
    void end() const {}

    void setUniformTexture(const std::string&, const ofTexture&, int) const {}
    void setUniform2f(const std::string&, float, float) const {}
    void setUniform3f(const std::string&, float, float, float) const {}
    void setUniform1f(const std::string&, float) const {}
    void setUniform1i(const std::string&, int) const {}
    void setUniform1iv(const std::string&, const int*, int) const {}
    void setUniform1fv(const std::string&, const float*, int) const {}
    void setUniform4fv(const std::string&, const float*, int) const {}
};

#endif
