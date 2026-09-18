#pragma once
// Test-only recording of CPU draw commands, never a substitute for GPU acceptance.
#include "glm/glm.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace showcase_test {
struct Color { float r=1,g=1,b=1,a=1; };
struct Vertex { float x=0,y=0,z=0; Color color; };
struct Draw { int mode=0; float lineWidth=1; std::vector<Vertex> vertices; };
using Matrix = std::array<float,16>;
inline Matrix identity() { return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}; }
inline Matrix multiply(const Matrix& a,const Matrix& b) {
    Matrix out{};
    for(int i=0;i<4;++i) for(int j=0;j<4;++j) for(int k=0;k<4;++k) out[i*4+j]+=a[i*4+k]*b[k*4+j];
    return out;
}
struct Style { Color color; float lineWidth=1; bool filled=true; int blend=0; };
struct View { Matrix matrix=identity(); std::array<float,4> viewport{0,0,640,360}; };
struct Recorder {
    Style style; View view; std::vector<Style> styles; std::vector<View> views;
    std::vector<Matrix> matrices; std::vector<Draw> draws; bool depth=false; bool cull=false;
    void reset() { *this=Recorder{}; }
    void resetDraws() { draws.clear(); }
    void push(int mode,const std::vector<glm::vec3>& vs,const std::vector<Color>& cs,const std::vector<unsigned>& indices) {
        Draw d; d.mode=mode; d.lineWidth=style.lineWidth;
        auto emit=[&](std::size_t i) {
            if(i>=vs.size()) throw std::runtime_error("mesh index outside vertex array");
            const auto& v=vs[i]; const auto& m=view.matrix;
            Color c=cs.empty()?style.color:cs.at(i);
            Vertex out{m[0]*v.x+m[1]*v.y+m[2]*v.z+m[3],m[4]*v.x+m[5]*v.y+m[6]*v.z+m[7],m[8]*v.x+m[9]*v.y+m[10]*v.z+m[11],c};
            for(float n : {out.x,out.y,out.z,c.r,c.g,c.b,c.a}) if(!std::isfinite(n)) throw std::runtime_error("non-finite mesh position/color");
            if(std::abs(out.x)>1e8f||std::abs(out.y)>1e8f||std::abs(out.z)>1e8f) throw std::runtime_error("unbounded mesh position");
            if(c.a < -0.001f || c.a>1.001f) throw std::runtime_error("mesh alpha outside normalized range");
            d.vertices.push_back(out);
        };
        if(indices.empty()) for(std::size_t i=0;i<vs.size();++i) emit(i); else for(auto i:indices) emit(i);
        if(!d.vertices.empty()) draws.push_back(std::move(d));
    }
};
inline Recorder recorder;
inline std::uint64_t signature() {
    std::uint64_t h=1469598103934665603ull;
    auto mix=[&](float f) { std::uint32_t bits; std::memcpy(&bits,&f,sizeof bits); h^=bits; h*=1099511628211ull; };
    for(const auto& d:recorder.draws) {mix(float(d.mode));mix(d.lineWidth);for(const auto& v:d.vertices)for(float f:{v.x,v.y,v.z,v.color.r,v.color.g,v.color.b,v.color.a})mix(f);}
    return h;
}
inline std::size_t vertexCount() {std::size_t n=0;for(const auto& d:recorder.draws)n+=d.vertices.size();return n;}
inline double alphaSum() {double a=0;for(const auto& d:recorder.draws)for(const auto& v:d.vertices)a+=v.color.a;return a;}
}
