#pragma once
#include "ofMesh.h"
#include "ofUtils.h"
#include <string>
struct ofColor { unsigned char r=255,g=255,b=255,a=255; ofColor()=default;explicit ofColor(unsigned char v,unsigned char alpha=255):r(v),g(v),b(v),a(alpha){} ofColor(unsigned char red,unsigned char green,unsigned char blue,unsigned char alpha=255):r(red),g(green),b(blue),a(alpha){} };
using ofStyle=showcase_test::Style;
inline ofStyle ofGetStyle(){return showcase_test::recorder.style;}
inline void ofPushStyle(){auto& r=showcase_test::recorder;r.styles.push_back(r.style);}
inline void ofPopStyle(){auto& r=showcase_test::recorder;if(r.styles.empty())throw std::runtime_error("style stack underflow");r.style=r.styles.back();r.styles.pop_back();}
inline void ofPushView(){auto& r=showcase_test::recorder;r.views.push_back(r.view);}
inline void ofPopView(){auto& r=showcase_test::recorder;if(r.views.empty())throw std::runtime_error("view stack underflow");r.view=r.views.back();r.views.pop_back();}
inline void ofPushMatrix(){auto& r=showcase_test::recorder;r.matrices.push_back(r.view.matrix);}
inline void ofPopMatrix(){auto& r=showcase_test::recorder;if(r.matrices.empty())throw std::runtime_error("matrix stack underflow");r.view.matrix=r.matrices.back();r.matrices.pop_back();}
inline void ofTranslate(float x,float y,float z=0){auto t=showcase_test::identity();t[3]=x;t[7]=y;t[11]=z;auto& m=showcase_test::recorder.view.matrix;m=showcase_test::multiply(m,t);}
inline void ofScale(float x,float y,float z=1){auto t=showcase_test::identity();t[0]=x;t[5]=y;t[10]=z;auto& m=showcase_test::recorder.view.matrix;m=showcase_test::multiply(m,t);}
inline void ofRotateDeg(float degrees){auto t=showcase_test::identity();float a=degrees*.0174532925199433f;t[0]=std::cos(a);t[1]=-std::sin(a);t[4]=std::sin(a);t[5]=std::cos(a);auto& m=showcase_test::recorder.view.matrix;m=showcase_test::multiply(m,t);}
inline void ofSetColor(int red,int green,int blue,int alpha=255){showcase_test::recorder.style.color={red/255.f,green/255.f,blue/255.f,alpha/255.f};}
inline void ofSetColor(int gray){ofSetColor(gray,gray,gray);}
inline void ofSetColor(const ofColor& c){ofSetColor(c.r,c.g,c.b,c.a);}
inline void ofSetColor(const ofFloatColor& c){showcase_test::recorder.style.color={c.r,c.g,c.b,c.a};}
inline void ofNoFill(){showcase_test::recorder.style.filled=false;}
inline void ofFill(){showcase_test::recorder.style.filled=true;}
inline void ofSetLineWidth(float w){if(!std::isfinite(w)||w<0)throw std::runtime_error("invalid line width");showcase_test::recorder.style.lineWidth=w;}
inline void ofViewport(float x,float y,float w,float h){showcase_test::recorder.view.viewport={x,y,w,h};}
inline void ofSetupScreenOrtho(float,float,float=-1,float=1){showcase_test::recorder.view.matrix=showcase_test::identity();}
inline void ofDrawRectangle(float x,float y,float w,float h){showcase_test::recorder.push(OF_PRIMITIVE_TRIANGLES,{{x,y,0},{x+w,y,0},{x+w,y+h,0},{x,y,0},{x+w,y+h,0},{x,y+h,0}},{},{});}
inline void ofDrawLine(float x,float y,float xx,float yy){showcase_test::recorder.push(OF_PRIMITIVE_LINES,{{x,y,0},{xx,yy,0}},{},{});}
inline void ofDrawCircle(float x,float y,float radius){std::vector<glm::vec3> v;for(int i=0;i<16;++i){float a=i*6.2831853f/16,b=(i+1)*6.2831853f/16;v.insert(v.end(),{{x,y,0},{x+radius*std::cos(a),y+radius*std::sin(a),0},{x+radius*std::cos(b),y+radius*std::sin(b),0}});}showcase_test::recorder.push(OF_PRIMITIVE_TRIANGLES,v,{},{});}
enum ofBlendMode{OF_BLENDMODE_ALPHA=0,OF_BLENDMODE_ADD=1};
inline void ofEnableBlendMode(ofBlendMode m){showcase_test::recorder.style.blend=m;}
inline void ofEnableAlphaBlending(){ofEnableBlendMode(OF_BLENDMODE_ALPHA);}
inline void ofDisableBlendMode(){showcase_test::recorder.style.blend=-1;}
inline void ofDisableDepthTest(){showcase_test::recorder.depth=false;}
inline void ofEnableDepthTest(){showcase_test::recorder.depth=true;}
inline float ofGetWidth(){return 640;} inline float ofGetHeight(){return 360;}
