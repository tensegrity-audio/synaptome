#pragma once
#include "GraphicsRecorder.h"
using ofIndexType = unsigned int;
inline constexpr int OF_PRIMITIVE_LINE_STRIP=1;
inline constexpr int OF_PRIMITIVE_TRIANGLES=2;
inline constexpr int OF_PRIMITIVE_LINES=3;
inline constexpr int OF_PRIMITIVE_TRIANGLE_STRIP=4;
inline constexpr int OF_PRIMITIVE_POINTS=5;
inline constexpr int OF_PRIMITIVE_LINE_LOOP=6;
inline constexpr int OF_PRIMITIVE_TRIANGLE_FAN=7;
struct ofFloatColor {
    float r=1,g=1,b=1,a=1;
    ofFloatColor()=default;
    ofFloatColor(float gray,float alpha=1):r(gray),g(gray),b(gray),a(alpha){}
    ofFloatColor(float red,float green,float blue,float alpha=1):r(red),g(green),b(blue),a(alpha){}
};
class ofMesh {
public:
    void clear(){vertices_.clear();colors_.clear();indices_.clear();}
    void setMode(int m){mode_=m;}
    void addVertex(const glm::vec3& v){vertices_.push_back(v);}
    void addColor(const ofFloatColor& c){colors_.push_back({c.r,c.g,c.b,c.a});}
    void addIndex(unsigned i){indices_.push_back(i);}
    void setVertex(std::size_t i,const glm::vec3& v){vertices_.at(i)=v;}
    void setColor(std::size_t i,const ofFloatColor& c){colors_.at(i)={c.r,c.g,c.b,c.a};}
    std::size_t getNumVertices()const{return vertices_.size();}
    std::vector<glm::vec3>& getVertices(){return vertices_;}
    const std::vector<glm::vec3>& getVertices()const{return vertices_;}
    void draw()const{showcase_test::recorder.push(mode_,vertices_,colors_,indices_);}
    void drawWireframe()const{draw();}
private:
    int mode_=OF_PRIMITIVE_TRIANGLES;
    std::vector<glm::vec3> vertices_; std::vector<showcase_test::Color> colors_; std::vector<unsigned> indices_;
};
using ofVboMesh=ofMesh;
