#include "GeodesicLayer.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace {
    const ofColor kGeodesicLineColor(255, 140, 0);
}

void GeodesicLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "layer.geodesic" : registryPrefix();

    ParameterRegistry::Descriptor visMeta;
    visMeta.label = "Sphere Visible";
    visMeta.group = "Visibility";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, visMeta);

    ParameterRegistry::Descriptor spinMeta;
    spinMeta.label = "Sphere Spin";
    spinMeta.group = "Sphere";
    spinMeta.range.min = -360.0f;
    spinMeta.range.max = 360.0f;
    spinMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".spin", &paramSpinDeg_, paramSpinDeg_, spinMeta);

    ParameterRegistry::Descriptor hoverMeta;
    hoverMeta.label = "Sphere Hover";
    hoverMeta.group = "Sphere";
    hoverMeta.range.min = 0.0f;
    hoverMeta.range.max = 200.0f;
    hoverMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".hover", &paramHoverAmp_, paramHoverAmp_, hoverMeta);

    ParameterRegistry::Descriptor heightMeta;
    heightMeta.label = "Sphere Base Height";
    heightMeta.group = "Sphere";
    heightMeta.range.min = -400.0f;
    heightMeta.range.max = 400.0f;
    heightMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".baseHeight", &paramBaseHeight_, paramBaseHeight_, heightMeta);

    ParameterRegistry::Descriptor orbitRadiusMeta;
    orbitRadiusMeta.label = "Sphere Orbit Radius";
    orbitRadiusMeta.group = "Sphere";
    orbitRadiusMeta.range.min = 0.0f;
    orbitRadiusMeta.range.max = 600.0f;
    orbitRadiusMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".orbitRadius", &paramOrbitRadius_, paramOrbitRadius_, orbitRadiusMeta);

    ParameterRegistry::Descriptor orbitSpeedMeta;
    orbitSpeedMeta.label = "Sphere Orbit Speed";
    orbitSpeedMeta.group = "Sphere";
    orbitSpeedMeta.range.min = -360.0f;
    orbitSpeedMeta.range.max = 360.0f;
    orbitSpeedMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".orbitSpeed", &paramOrbitSpeedDeg_, paramOrbitSpeedDeg_, orbitSpeedMeta);

    ParameterRegistry::Descriptor rotateMeta;
    rotateMeta.label = "Sphere Rotate X";
    rotateMeta.group = "Sphere";
    rotateMeta.range.min = -180.0f;
    rotateMeta.range.max = 180.0f;
    rotateMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".rotateX", &paramRotateXDeg_, paramRotateXDeg_, rotateMeta);
    rotateMeta.label = "Sphere Rotate Y";
    registry.addFloat(prefix + ".rotateY", &paramRotateYDeg_, paramRotateYDeg_, rotateMeta);
    rotateMeta.label = "Sphere Rotate Z";
    registry.addFloat(prefix + ".rotateZ", &paramRotateZDeg_, paramRotateZDeg_, rotateMeta);

    ParameterRegistry::Descriptor radiusMeta;
    radiusMeta.label = "Sphere Radius";
    radiusMeta.group = "Sphere";
    radiusMeta.range.min = 60.0f;
    radiusMeta.range.max = 400.0f;
    radiusMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".radius", &paramRadius_, paramRadius_, radiusMeta);

    ParameterRegistry::Descriptor deformMeta;
    deformMeta.label = "Sphere Deform";
    deformMeta.group = "Sphere";
    deformMeta.description = "Enable irregular Perlin noise deformation.";
    registry.addBool(prefix + ".deform", &paramDeform_, paramDeform_, deformMeta);

    ParameterRegistry::Descriptor deformAmountMeta;
    deformAmountMeta.label = "Sphere Deform Amount";
    deformAmountMeta.group = "Sphere";
    deformAmountMeta.range.min = 0.0f;
    deformAmountMeta.range.max = 160.0f;
    deformAmountMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".deformAmount", &paramDeformAmount_, paramDeformAmount_, deformAmountMeta);

    ParameterRegistry::Descriptor deformScaleMeta;
    deformScaleMeta.label = "Sphere Deform Scale";
    deformScaleMeta.group = "Sphere";
    deformScaleMeta.range.min = 0.1f;
    deformScaleMeta.range.max = 8.0f;
    deformScaleMeta.range.step = 0.05f;
    registry.addFloat(prefix + ".deformScale", &paramDeformScale_, paramDeformScale_, deformScaleMeta);

    ParameterRegistry::Descriptor deformSpeedMeta;
    deformSpeedMeta.label = "Sphere Deform Speed";
    deformSpeedMeta.group = "Sphere";
    deformSpeedMeta.range.min = 0.0f;
    deformSpeedMeta.range.max = 4.0f;
    deformSpeedMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".deformSpeed", &paramDeformSpeed_, paramDeformSpeed_, deformSpeedMeta);

    ParameterRegistry::Descriptor lineMeta;
    lineMeta.label = "Sphere Line Opacity";
    lineMeta.group = "Sphere";
    lineMeta.range.min = 0.0f;
    lineMeta.range.max = 1.0f;
    lineMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".lineOpacity", &paramLineOpacity_, paramLineOpacity_, lineMeta);

    ParameterRegistry::Descriptor faceMeta;
    faceMeta.label = "Sphere Face Opacity";
    faceMeta.group = "Sphere";
    faceMeta.range.min = 0.0f;
    faceMeta.range.max = 1.0f;
    faceMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".faceOpacity", &paramFaceOpacity_, paramFaceOpacity_, faceMeta);

    rebuildGeodesic();
}

void GeodesicLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;

    float clampedSpin = ofClamp(paramSpinDeg_, -360.0f, 360.0f);
    if (paramSpinDeg_ != clampedSpin) paramSpinDeg_ = clampedSpin;
    spinDeg_ = clampedSpin;

    float clampedHover = ofClamp(paramHoverAmp_, 0.0f, 200.0f);
    if (paramHoverAmp_ != clampedHover) paramHoverAmp_ = clampedHover;
    hoverAmp_ = clampedHover;

    float clampedBaseHeight = ofClamp(paramBaseHeight_, -400.0f, 400.0f);
    if (paramBaseHeight_ != clampedBaseHeight) paramBaseHeight_ = clampedBaseHeight;
    baseHeight_ = clampedBaseHeight;

    float clampedOrbitRadius = ofClamp(paramOrbitRadius_, 0.0f, 600.0f);
    if (paramOrbitRadius_ != clampedOrbitRadius) paramOrbitRadius_ = clampedOrbitRadius;
    orbitRadius_ = clampedOrbitRadius;

    float clampedOrbitSpeed = ofClamp(paramOrbitSpeedDeg_, -360.0f, 360.0f);
    if (paramOrbitSpeedDeg_ != clampedOrbitSpeed) paramOrbitSpeedDeg_ = clampedOrbitSpeed;
    orbitSpeedDeg_ = clampedOrbitSpeed;

    float clampedRotateX = ofClamp(paramRotateXDeg_, -180.0f, 180.0f);
    if (paramRotateXDeg_ != clampedRotateX) paramRotateXDeg_ = clampedRotateX;
    rotateXDeg_ = clampedRotateX;

    float clampedRotateY = ofClamp(paramRotateYDeg_, -180.0f, 180.0f);
    if (paramRotateYDeg_ != clampedRotateY) paramRotateYDeg_ = clampedRotateY;
    rotateYDeg_ = clampedRotateY;

    float clampedRotateZ = ofClamp(paramRotateZDeg_, -180.0f, 180.0f);
    if (paramRotateZDeg_ != clampedRotateZ) paramRotateZDeg_ = clampedRotateZ;
    rotateZDeg_ = clampedRotateZ;

    float clampedRadius = ofClamp(paramRadius_, 60.0f, 400.0f);
    if (paramRadius_ != clampedRadius) paramRadius_ = clampedRadius;
    if (std::abs(clampedRadius - radius_) > 0.5f) {
        radius_ = clampedRadius;
        rebuildGeodesic();
    }

    deform_ = paramDeform_;

    float clampedDeformAmount = ofClamp(paramDeformAmount_, 0.0f, 160.0f);
    if (paramDeformAmount_ != clampedDeformAmount) paramDeformAmount_ = clampedDeformAmount;
    deformAmount_ = clampedDeformAmount;

    float clampedDeformScale = ofClamp(paramDeformScale_, 0.1f, 8.0f);
    if (paramDeformScale_ != clampedDeformScale) paramDeformScale_ = clampedDeformScale;
    deformScale_ = clampedDeformScale;

    float clampedDeformSpeed = ofClamp(paramDeformSpeed_, 0.0f, 4.0f);
    if (paramDeformSpeed_ != clampedDeformSpeed) paramDeformSpeed_ = clampedDeformSpeed;
    deformSpeed_ = clampedDeformSpeed;

    if (deform_ && deformAmount_ > 0.01f) {
        applyDeformation(params.time);
    } else if (meshDeformed_) {
        restoreBaseMesh();
    }

    float clampedLine = ofClamp(paramLineOpacity_, 0.0f, 1.0f);
    if (paramLineOpacity_ != clampedLine) paramLineOpacity_ = clampedLine;
    lineOpacity_ = clampedLine;

    float clampedFace = ofClamp(paramFaceOpacity_, 0.0f, 1.0f);
    if (paramFaceOpacity_ != clampedFace) paramFaceOpacity_ = clampedFace;
    faceOpacity_ = clampedFace;
}

void GeodesicLayer::draw(const LayerDrawParams& params) {
    if (!enabled_) return;
    if (params.slotOpacity <= 0.0f) return;

    params.camera.begin();
    ofEnableDepthTest();

    ofPushMatrix();
    const float hoverY = sinf(params.time * 0.8f) * hoverAmp_;
    const float orbitAngle = std::fmod(params.time * orbitSpeedDeg_, 360.0f);
    ofRotateYDeg(orbitAngle);
    ofTranslate(orbitRadius_, baseHeight_ + hoverY, 0);
    const float spin = std::fmod(params.time * spinDeg_, 360.0f);
    ofRotateYDeg(spin);
    ofRotateXDeg(rotateXDeg_);
    ofRotateYDeg(rotateYDeg_);
    ofRotateZDeg(rotateZDeg_);

    const float alphaScale = ofClamp(params.slotOpacity, 0.0f, 1.0f);
    const int baseAlpha = static_cast<int>(alphaScale * 255.0f);

    if (faceOpacity_ > 0.0f) {
        ofPushStyle();
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        ofSetColor(kGeodesicLineColor.r, kGeodesicLineColor.g, kGeodesicLineColor.b,
                   static_cast<int>(faceOpacity_ * baseAlpha));
        mesh_.draw();
        ofDisableBlendMode();
        ofPopStyle();
    }

    if (lineOpacity_ > 0.0f) {
        ofSetColor(kGeodesicLineColor.r, kGeodesicLineColor.g, kGeodesicLineColor.b,
                   static_cast<int>(lineOpacity_ * baseAlpha));
#ifndef TARGET_OPENGLES
        glLineWidth(1.5f);
#endif
        mesh_.drawWireframe();
    }

    ofPopMatrix();
    ofDisableDepthTest();
    params.camera.end();
}

void GeodesicLayer::incrementSubdivision() {
    if (subdivisions_ >= 4) return;
    subdivisions_ += 1;
    rebuildGeodesic();
}

void GeodesicLayer::decrementSubdivision() {
    if (subdivisions_ <= 1) return;
    subdivisions_ -= 1;
    rebuildGeodesic();
}

void GeodesicLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

void GeodesicLayer::rebuildGeodesic() {
    ico_ = ofIcoSpherePrimitive(radius_, subdivisions_);
    baseMesh_ = ico_.getMesh();
    restoreBaseMesh();
}

void GeodesicLayer::applyDeformation(float time) {
    if (baseMesh_.getNumVertices() == 0) {
        return;
    }

    mesh_ = baseMesh_;
    const auto& baseVertices = baseMesh_.getVertices();
    const float scaledTime = time * deformSpeed_;
    const float minRadius = std::max(1.0f, radius_ * 0.2f);
    const auto vertexCount = static_cast<ofIndexType>(baseVertices.size());

    for (ofIndexType i = 0; i < vertexCount; ++i) {
        const glm::vec3 baseVertex = baseVertices[static_cast<std::size_t>(i)];
        const float baseLength = glm::length(baseVertex);
        if (baseLength <= 0.0001f) {
            continue;
        }

        const glm::vec3 direction = baseVertex / baseLength;
        const float coarse = ofNoise(direction.x * deformScale_ + 17.31f,
                                     direction.y * deformScale_ - 23.79f,
                                     direction.z * deformScale_ + scaledTime);
        const float detail = ofNoise(direction.x * deformScale_ * 2.37f - 41.13f,
                                     direction.y * deformScale_ * 2.37f + scaledTime * 1.67f,
                                     direction.z * deformScale_ * 2.37f + 9.47f);
        const float centeredNoise = ((coarse * 0.72f + detail * 0.28f) - 0.5f) * 2.0f;
        const float displacedRadius = std::max(minRadius, baseLength + centeredNoise * deformAmount_);
        mesh_.setVertex(i, direction * displacedRadius);
    }

    meshDeformed_ = true;
}

void GeodesicLayer::restoreBaseMesh() {
    mesh_ = baseMesh_;
    meshDeformed_ = false;
}
