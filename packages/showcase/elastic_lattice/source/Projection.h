#pragma once
#include "ofMain.h"
#include <algorithm>
#include <cmath>

// Package-local projection support. Fixed world-to-screen orthographic mapping
// deliberately avoids changing or retaining the host camera. Facets are sorted
// back-to-front, so their depth remains meaningful without a host depth buffer.
namespace {
struct ShowPoint { double x, y, z; };
struct ShowProjection {
    double width, height, radius, cy, sy, ct, st;
    ShowProjection(double w, double h, double scale, double yaw, double tilt)
        : width(w),height(h),radius(std::min(w,h)*.34*scale),
          cy(std::cos(yaw*.017453292519943295)),sy(std::sin(yaw*.017453292519943295)),
          ct(std::cos(tilt*.017453292519943295)),st(std::sin(tilt*.017453292519943295)) {}
    ShowPoint operator()(const ShowPoint& p) const {
        const double x=p.x*cy+p.z*sy, z=-p.x*sy+p.z*cy;
        return {width*.5+x*radius,height*.5-(p.y*ct-z*st)*radius,p.y*st+z*ct};
    }
};
struct ShowFacet {
    ShowPoint p[3];
    double depth;
    float mix, light;
};
inline float showClamp(double x, double low=0, double high=1) {
    return static_cast<float>(std::isfinite(x)?std::max(low,std::min(high,x)):low);
}
inline ShowPoint showNormal(const ShowPoint& a,const ShowPoint& b,const ShowPoint& c) {
    const ShowPoint u{b.x-a.x,b.y-a.y,b.z-a.z},v{c.x-a.x,c.y-a.y,c.z-a.z};
    ShowPoint n{u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};
    const double length=std::sqrt(n.x*n.x+n.y*n.y+n.z*n.z);
    if(length<1e-12) return {0,1,0};
    n.x/=length;n.y/=length;n.z/=length;
    if(n.y<0) {n.x=-n.x;n.y=-n.y;n.z=-n.z;}
    return n;
}
inline ofFloatColor showPalette(float t,float light,float alpha,
                         float r,float g,float b,float hr,float hg,float hb) {
    t=showClamp(t);
    return ofFloatColor(showClamp((r+(hr-r)*t)*light),showClamp((g+(hg-g)*t)*light),
                        showClamp((b+(hb-b)*t)*light),showClamp(alpha));
}
inline void showVertex(ofMesh& mesh,const ShowPoint& p,const ofFloatColor& color) {
    mesh.addVertex(glm::vec3(static_cast<float>(p.x),static_cast<float>(p.y),0));
    mesh.addColor(color);
}
class ShowDrawScope {
public:
    ShowDrawScope(float width,float height)
        : depth_(glIsEnabled(GL_DEPTH_TEST)),cull_(glIsEnabled(GL_CULL_FACE)) {
        ofPushStyle();ofPushView();ofPushMatrix();
        ofViewport(0,0,width,height);
        ofSetupScreenOrtho(width,height,-1,1);
        glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
        ofEnableAlphaBlending();ofFill();ofSetColor(255);ofSetLineWidth(1);
    }
    ~ShowDrawScope() {
        ofPopMatrix();ofPopView();ofPopStyle();
        if(depth_) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if(cull_) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    }
    ShowDrawScope(const ShowDrawScope&)=delete;
    ShowDrawScope& operator=(const ShowDrawScope&)=delete;
private:
    GLboolean depth_,cull_;
};
}
