#include "GerstnerOceanLayer.h"
#include "Projection.h"
#include "ofGraphics.h"
#include <algorithm>
#include <array>
#include <cmath>

void GerstnerOceanLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) return;
    const auto& d = config["defaults"];
    speed_ = static_cast<float>(Model::bounded(d.value("speed", speed_), 0, 4));
    bpmSync_ = d.value("bpmSync", bpmSync_);
    bpmMultiplier_ = static_cast<float>(Model::bounded(d.value("bpmMultiplier", bpmMultiplier_), 0.25, 8));
    scale_ = static_cast<float>(Model::bounded(d.value("scale", scale_), 0.3, 1.6));
    yawDeg_ = static_cast<float>(Model::bounded(d.value("yawDeg", yawDeg_), -180, 180));
    tiltDeg_ = static_cast<float>(Model::bounded(d.value("tiltDeg", tiltDeg_), 10, 75));
    spinRate_ = static_cast<float>(Model::bounded(d.value("spinRate", spinRate_), -30, 30));
    seed_ = static_cast<float>(Model::bounded(d.value("seed", seed_), 0, 65535));
    reseed_ = d.value("reseed", reseed_);
    waveHeight_ = static_cast<float>(Model::bounded(d.value("waveHeight", waveHeight_), 0.01, 0.32));
    wavelength_ = static_cast<float>(Model::bounded(d.value("wavelength", wavelength_), 0.7, 3));
    steepness_ = static_cast<float>(Model::bounded(d.value("steepness", steepness_), 0, 1));
    directionDeg_ = static_cast<float>(Model::bounded(d.value("directionDeg", directionDeg_), -180, 180));
    wireOpacity_ = static_cast<float>(Model::bounded(d.value("wireOpacity", wireOpacity_), 0, 1));
    colorR_ = static_cast<float>(Model::bounded(d.value("colorR", colorR_), 0, 1));
    colorG_ = static_cast<float>(Model::bounded(d.value("colorG", colorG_), 0, 1));
    colorB_ = static_cast<float>(Model::bounded(d.value("colorB", colorB_), 0, 1));
    highlightR_ = static_cast<float>(Model::bounded(d.value("highlightR", highlightR_), 0, 1));
    highlightG_ = static_cast<float>(Model::bounded(d.value("highlightG", highlightG_), 0, 1));
    highlightB_ = static_cast<float>(Model::bounded(d.value("highlightB", highlightB_), 0, 1));
}
void GerstnerOceanLayer::bindParameters(synaptome::element::ParameterBinder& binder) {
    binder.bind("speed", speed_);
    binder.bind("bpmSync", bpmSync_);
    binder.bind("bpmMultiplier", bpmMultiplier_);
    binder.bind("scale", scale_);
    binder.bind("yawDeg", yawDeg_);
    binder.bind("tiltDeg", tiltDeg_);
    binder.bind("spinRate", spinRate_);
    binder.bind("seed", seed_);
    binder.bind("reseed", reseed_);
    binder.bind("waveHeight", waveHeight_);
    binder.bind("wavelength", wavelength_);
    binder.bind("steepness", steepness_);
    binder.bind("directionDeg", directionDeg_);
    binder.bind("wireOpacity", wireOpacity_);
    binder.bind("colorR", colorR_);
    binder.bind("colorG", colorG_);
    binder.bind("colorB", colorB_);
    binder.bind("highlightR", highlightR_);
    binder.bind("highlightG", highlightG_);
    binder.bind("highlightB", highlightB_);
}
GerstnerOceanLayer::Model::Controls GerstnerOceanLayer::controls() const {
    return { waveHeight_, wavelength_, steepness_, directionDeg_ };
}
void GerstnerOceanLayer::setup(ParameterRegistry& registry) {
    (void)registry;
    lastSeed_ = static_cast<std::uint32_t>(Model::bounded(seed_, 0, 65535));
    model_.reset(lastSeed_, controls());
    mesh_.setMode(OF_PRIMITIVE_TRIANGLES);
    edges_.setMode(OF_PRIMITIVE_LINES);
}
void GerstnerOceanLayer::update(const LayerUpdateParams& p) {
    if (!enabled_) return;
    const auto seed = static_cast<std::uint32_t>(Model::bounded(seed_, 0, 65535));
    if (reseed_ || seed != lastSeed_) {
        model_.reset(seed, controls()); lastSeed_ = seed; reseed_ = false;
    }
    double dt = Model::bounded(p.dt, 0, .1) * Model::bounded(p.speed, 0, 8) *
                Model::bounded(speed_, 0, 4);
    if (bpmSync_) dt *= Model::bounded(p.bpm, 0, 400)/120.0 * Model::bounded(bpmMultiplier_, .25, 8);
    dt = std::min(dt, .2);
    model_.update(dt, controls());
    spinAngle_ = std::fmod(spinAngle_ + dt * Model::bounded(spinRate_, -30, 30), 360.0);
}

void GerstnerOceanLayer::draw(const LayerDrawParams& p) {
    if(!enabled_ || p.slotOpacity<=0 || p.viewport.x<=0 || p.viewport.y<=0) return;
    const ShowProjection project(p.viewport.x,p.viewport.y,Model::bounded(scale_,.3,1.6),
                                 Model::bounded(yawDeg_,-180,180)+spinAngle_,
                                 Model::bounded(tiltDeg_,10,75));
    std::array<ShowFacet,Model::columns*Model::rows*2> facets;
    const auto worldPoint=[&](int index) {
        const auto& q=model_.surface()[index];
        return ShowPoint{q.x,q.y,q.z};
    };
    std::size_t next=0;
    for(int y=0;y<Model::rows;++y) for(int x=0;x<Model::columns;++x) {
        const int aIndex=y*(Model::columns+1)+x;
        const int ids[2][3]={{aIndex,aIndex+Model::columns+1,aIndex+1},
                              {aIndex+1,aIndex+Model::columns+1,aIndex+Model::columns+2}};
        for(const auto& triangle : ids) {
            const auto a=worldPoint(triangle[0]),b=worldPoint(triangle[1]),c=worldPoint(triangle[2]);
            const auto n=showNormal(a,b,c);
            auto& facet=facets[next++];
            facet.p[0]=project(a);facet.p[1]=project(b);facet.p[2]=project(c);
            facet.depth=(facet.p[0].z+facet.p[1].z+facet.p[2].z)/3;
            // Lighting is derived from the actual deformed triangle normal.
            facet.light=showClamp(.32+.68*std::max(0.0,n.x*.35+n.y*.85+n.z*.4));
            facet.mix=showClamp(.45+((a.y+b.y+c.y)/3)/.64+(1-n.y)*.45);
        }
    }
    std::sort(facets.begin(),facets.end(),[](const ShowFacet& a,const ShowFacet& b) {
        return a.depth<b.depth;
    });
    mesh_.clear();mesh_.setMode(OF_PRIMITIVE_TRIANGLES);
    edges_.clear();edges_.setMode(OF_PRIMITIVE_LINES);
    const float alpha=showClamp(p.slotOpacity);
    const float wire=showClamp(wireOpacity_)*alpha;
    for(const auto& facet : facets) {
        const auto color=showPalette(facet.mix,facet.light,alpha,
                                    colorR_,colorG_,colorB_,highlightR_,highlightG_,highlightB_);
        for(const auto& point : facet.p) showVertex(mesh_,point,color);
        if(wire>0) {
            const auto edge=showPalette(facet.mix,.75f,wire,
                                       colorR_,colorG_,colorB_,highlightR_,highlightG_,highlightB_);
            for(int i=0;i<3;++i) {
                showVertex(edges_,facet.p[i],edge);showVertex(edges_,facet.p[(i+1)%3],edge);
            }
        }
    }
    ShowDrawScope scope(static_cast<float>(p.viewport.x),static_cast<float>(p.viewport.y));
    mesh_.draw();
    if(wire>0) edges_.draw();
}
