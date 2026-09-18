#pragma once

#ifdef OF_SDK_AVAILABLE
#include <graphics/ofMesh.h>
#else

#include "glm/glm.hpp"
#include "ofFloatColor.h"
#include <cstddef>
#include <vector>

using ofIndexType = unsigned int;
inline constexpr int OF_PRIMITIVE_LINE_STRIP = 1;
inline constexpr int OF_PRIMITIVE_TRIANGLES = 2;
inline constexpr int OF_PRIMITIVE_LINES = 3;

class ofMesh {
public:
    void clear() {
        vertices_.clear();
        colors_.clear();
        indices_.clear();
    }
    void setMode(int mode) { mode_ = mode; }
    void addVertex(const glm::vec3& vertex) {
        vertices_.push_back(vertex);
    }
    void setVertex(std::size_t index, const glm::vec3& vertex) {
        if (index < vertices_.size()) {
            vertices_[index] = vertex;
        }
    }
    void addColor(const ofFloatColor& color) { colors_.push_back(color); }
    void addIndex(unsigned int index) { indices_.push_back(index); }
    void draw() const {}

private:
    int mode_ = 0;
    std::vector<glm::vec3> vertices_;
    std::vector<ofFloatColor> colors_;
    std::vector<unsigned int> indices_;
};

using ofVboMesh = ofMesh;

#endif
