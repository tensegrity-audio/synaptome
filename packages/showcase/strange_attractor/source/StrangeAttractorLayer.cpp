#include "StrangeAttractorLayer.h"
#include "Projection.h"
#include "ofGraphics.h"
#include <algorithm>
#include <array>
#include <cmath>

void StrangeAttractorLayer::configure(const ofJson& config) {
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
    sigma_ = static_cast<float>(Model::bounded(d.value("sigma", sigma_), 6, 16));
    rho_ = static_cast<float>(Model::bounded(d.value("rho", rho_), 24, 40));
    beta_ = static_cast<float>(Model::bounded(d.value("beta", beta_), 2, 3.5));
    trailSeconds_ = static_cast<float>(Model::bounded(d.value("trailSeconds", trailSeconds_), 0.2, 6));
    ribbonWidth_ = static_cast<float>(Model::bounded(d.value("ribbonWidth", ribbonWidth_), 0.001, 0.025));
    colorR_ = static_cast<float>(Model::bounded(d.value("colorR", colorR_), 0, 1));
    colorG_ = static_cast<float>(Model::bounded(d.value("colorG", colorG_), 0, 1));
    colorB_ = static_cast<float>(Model::bounded(d.value("colorB", colorB_), 0, 1));
    highlightR_ = static_cast<float>(Model::bounded(d.value("highlightR", highlightR_), 0, 1));
    highlightG_ = static_cast<float>(Model::bounded(d.value("highlightG", highlightG_), 0, 1));
    highlightB_ = static_cast<float>(Model::bounded(d.value("highlightB", highlightB_), 0, 1));
}
void StrangeAttractorLayer::bindParameters(synaptome::element::ParameterBinder& binder) {
    binder.bind("speed", speed_);
    binder.bind("bpmSync", bpmSync_);
    binder.bind("bpmMultiplier", bpmMultiplier_);
    binder.bind("scale", scale_);
    binder.bind("yawDeg", yawDeg_);
    binder.bind("tiltDeg", tiltDeg_);
    binder.bind("spinRate", spinRate_);
    binder.bind("seed", seed_);
    binder.bind("reseed", reseed_);
    binder.bind("sigma", sigma_);
    binder.bind("rho", rho_);
    binder.bind("beta", beta_);
    binder.bind("trailSeconds", trailSeconds_);
    binder.bind("ribbonWidth", ribbonWidth_);
    binder.bind("colorR", colorR_);
    binder.bind("colorG", colorG_);
    binder.bind("colorB", colorB_);
    binder.bind("highlightR", highlightR_);
    binder.bind("highlightG", highlightG_);
    binder.bind("highlightB", highlightB_);
}
StrangeAttractorLayer::Model::Controls StrangeAttractorLayer::controls() const {
    return { sigma_, rho_, beta_ };
}
void StrangeAttractorLayer::setup(ParameterRegistry& registry) {
    (void)registry;
    lastSeed_ = static_cast<std::uint32_t>(Model::bounded(seed_, 0, 65535));
    model_.reset(lastSeed_, controls());
    mesh_.setMode(OF_PRIMITIVE_TRIANGLES);
    edges_.setMode(OF_PRIMITIVE_LINES);
}
void StrangeAttractorLayer::update(const LayerUpdateParams& p) {
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

void StrangeAttractorLayer::draw(const LayerDrawParams& p) {
    if(!enabled_ || p.slotOpacity<=0 || p.viewport.x<=0 || p.viewport.y<=0) return;
    const ShowProjection project(p.viewport.x,p.viewport.y,Model::bounded(scale_,.3,1.6),
                                 Model::bounded(yawDeg_,-180,180)+spinAngle_,
                                 Model::bounded(tiltDeg_,10,75));
    mesh_.clear();mesh_.setMode(OF_PRIMITIVE_TRIANGLES);
    const std::size_t count=static_cast<std::size_t>(Model::bounded(trailSeconds_,.2,6)/Model::stepSeconds);
    const std::size_t start=Model::historyCount-std::min(count,Model::historyCount);
    const double halfWidth=Model::bounded(ribbonWidth_,.001,.025)*project.radius*.5;
    for(std::size_t trail=0;trail<Model::trajectoryCount;++trail) {
        for(std::size_t age=start;age+1<Model::historyCount;++age) {
            const auto& a=model_.point(trail,age);const auto& b=model_.point(trail,age+1);
            const auto pa=project({a.x/25,(a.z-27)/25,a.y/25});
            const auto pb=project({b.x/25,(b.z-27)/25,b.y/25});
            const double dx=pb.x-pa.x,dy=pb.y-pa.y;
            const double length=std::sqrt(dx*dx+dy*dy);
            if(length<1e-6) continue;
            const double nx=-dy/length*halfWidth,ny=dx/length*halfWidth;
            const double velocity=std::sqrt((b.x-a.x)*(b.x-a.x)+(b.y-a.y)*(b.y-a.y)+
                                             (b.z-a.z)*(b.z-a.z))/Model::stepSeconds;
            const float ageFade=showClamp(static_cast<double>(age-start+1)/std::max<std::size_t>(1,count));
            const float light=showClamp(.65+.25*(pa.z+1),.35,1);
            const auto color=showPalette(showClamp(velocity/220),light,
                showClamp(p.slotOpacity)*(.12f+.88f*ageFade),
                colorR_,colorG_,colorB_,highlightR_,highlightG_,highlightB_);
            const ShowPoint quad[4]={{pa.x+nx,pa.y+ny,0},{pa.x-nx,pa.y-ny,0},
                                     {pb.x-nx,pb.y-ny,0},{pb.x+nx,pb.y+ny,0}};
            for(int index : {0,1,2,0,2,3}) showVertex(mesh_,quad[index],color);
        }
    }
    ShowDrawScope scope(static_cast<float>(p.viewport.x),static_cast<float>(p.viewport.y));
    mesh_.draw();
}
