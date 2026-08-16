#include "GridLayer.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include <cmath>

namespace {
    constexpr float kSegmentStep = 20.0f;
    const ofColor kGridLineColor(0, 255, 160);
}

void GridLayer::bindParameters(
    synaptome::element::ParameterBinder& binder) {
    binder.bind("segments", paramSegments_);
    binder.bind("waveAmount", paramWaveAmount_);
    binder.bind("waveFrequency", paramWaveFrequency_);
    binder.bind("waveSpeed", paramWaveSpeed_);
    binder.bind("bendAmount", paramBendAmount_);
    binder.bind("bendFrequency", paramBendFrequency_);
    binder.bind("bendSpeed", paramBendSpeed_);
    binder.bind("deformAmount", paramDeformAmount_);
    binder.bind("deformScale", paramDeformScale_);
    binder.bind("deformSpeed", paramDeformSpeed_);
    binder.bind("twistAmount", paramTwistAmount_);
    binder.bind("twistSpeed", paramTwistSpeed_);
    binder.bind("bulgeAmount", paramBulgeAmount_);
    binder.bind("bulgeRadius", paramBulgeRadius_);
    binder.bind("bulgeSpeed", paramBulgeSpeed_);
    binder.bind("faceOpacity", paramFaceOpacity_);
    binder.bind("visible", paramEnabled_);
    binder.bind("wave", paramWave_);
    binder.bind("bend", paramBend_);
    binder.bind("deform", paramDeform_);
    binder.bind("twist", paramTwist_);
    binder.bind("bulge", paramBulge_);
}

void GridLayer::setup(ParameterRegistry& registry) {
    (void)registry;
}

void GridLayer::update(const LayerUpdateParams& params) {
    (void)params;

    enabled_ = paramEnabled_;

    float quantized = std::round(paramSegments_ / kSegmentStep) * kSegmentStep;
    quantized = ofClamp(quantized, 20.0f, 200.0f);
    int segTarget = static_cast<int>(quantized);
    if (segments_ != segTarget) {
        segments_ = segTarget;
        ensureVertexCapacity();
    }
    paramSegments_ = quantized;

    wave_ = paramWave_;

    float clampedWaveAmount = ofClamp(paramWaveAmount_, 0.0f, 120.0f);
    if (paramWaveAmount_ != clampedWaveAmount) paramWaveAmount_ = clampedWaveAmount;
    waveAmount_ = clampedWaveAmount;

    float clampedWaveFrequency = ofClamp(paramWaveFrequency_, 0.1f, 12.0f);
    if (paramWaveFrequency_ != clampedWaveFrequency) paramWaveFrequency_ = clampedWaveFrequency;
    waveFrequency_ = clampedWaveFrequency;

    float clampedWaveSpeed = ofClamp(paramWaveSpeed_, 0.0f, 8.0f);
    if (paramWaveSpeed_ != clampedWaveSpeed) paramWaveSpeed_ = clampedWaveSpeed;
    waveSpeed_ = clampedWaveSpeed;

    bend_ = paramBend_;

    float clampedBendAmount = ofClamp(paramBendAmount_, 0.0f, 120.0f);
    if (paramBendAmount_ != clampedBendAmount) paramBendAmount_ = clampedBendAmount;
    bendAmount_ = clampedBendAmount;

    float clampedBendFrequency = ofClamp(paramBendFrequency_, 0.0f, 0.05f);
    if (paramBendFrequency_ != clampedBendFrequency) paramBendFrequency_ = clampedBendFrequency;
    bendFrequency_ = clampedBendFrequency;

    float clampedBendSpeed = ofClamp(paramBendSpeed_, 0.0f, 8.0f);
    if (paramBendSpeed_ != clampedBendSpeed) paramBendSpeed_ = clampedBendSpeed;
    bendSpeed_ = clampedBendSpeed;

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

    twist_ = paramTwist_;

    float clampedTwistAmount = ofClamp(paramTwistAmount_, -180.0f, 180.0f);
    if (paramTwistAmount_ != clampedTwistAmount) paramTwistAmount_ = clampedTwistAmount;
    twistAmount_ = clampedTwistAmount;

    float clampedTwistSpeed = ofClamp(paramTwistSpeed_, 0.0f, 4.0f);
    if (paramTwistSpeed_ != clampedTwistSpeed) paramTwistSpeed_ = clampedTwistSpeed;
    twistSpeed_ = clampedTwistSpeed;

    bulge_ = paramBulge_;

    float clampedBulgeAmount = ofClamp(paramBulgeAmount_, -160.0f, 160.0f);
    if (paramBulgeAmount_ != clampedBulgeAmount) paramBulgeAmount_ = clampedBulgeAmount;
    bulgeAmount_ = clampedBulgeAmount;

    float clampedBulgeRadius = ofClamp(paramBulgeRadius_, 0.1f, 1.5f);
    if (paramBulgeRadius_ != clampedBulgeRadius) paramBulgeRadius_ = clampedBulgeRadius;
    bulgeRadius_ = clampedBulgeRadius;

    float clampedBulgeSpeed = ofClamp(paramBulgeSpeed_, 0.0f, 4.0f);
    if (paramBulgeSpeed_ != clampedBulgeSpeed) paramBulgeSpeed_ = clampedBulgeSpeed;
    bulgeSpeed_ = clampedBulgeSpeed;

    float clampedFace = ofClamp(paramFaceOpacity_, 0.0f, 1.0f);
    if (paramFaceOpacity_ != clampedFace) {
        paramFaceOpacity_ = clampedFace;
    }
    faceOpacity_ = clampedFace;
}

void GridLayer::draw(const LayerDrawParams& params) {
    if (!enabled_) return;
    if (params.slotOpacity <= 0.0f) return;

    ensureVertexCapacity();

    const int seg = segments_;
    const float size = gridSize_;
    const float maxRadius = size * 0.70710678f;
    const float noiseTime = params.time * deformSpeed_;

    for (int z = 0; z <= seg; ++z) {
        for (int x = 0; x <= seg; ++x) {
            const float basePx = ofMap(x, 0, seg, -size * 0.5f, size * 0.5f);
            const float basePz = ofMap(z, 0, seg, -size * 0.5f, size * 0.5f);
            const float radial = glm::length(glm::vec2(basePx, basePz));
            const float radialNorm = maxRadius > 0.0f ? ofClamp(radial / maxRadius, 0.0f, 1.0f) : 0.0f;
            float px = basePx;
            float pz = basePz;
            float height = 0.0f;

            if (wave_ && waveAmount_ > 0.01f) {
                height += sinf(radial * 0.01f * waveFrequency_ - params.time * waveSpeed_) * waveAmount_;
            }

            if (bend_ && bendAmount_ > 0.01f) {
                height += sinf((basePx + basePz) * bendFrequency_ + params.time * bendSpeed_) * bendAmount_;
            }

            float deformOffset = 0.0f;
            if (deform_ && deformAmount_ > 0.01f) {
                const float nx = basePx / size;
                const float nz = basePz / size;
                const float coarse = ofNoise(nx * deformScale_ + 31.17f,
                                             nz * deformScale_ - 12.43f,
                                             noiseTime);
                const float detail = ofNoise(nx * deformScale_ * 2.71f - 6.53f,
                                             nz * deformScale_ * 2.71f + 44.91f,
                                             noiseTime * 1.63f + 8.19f);
                const float centeredNoise = ((coarse * 0.72f + detail * 0.28f) - 0.5f) * 2.0f;
                deformOffset = centeredNoise * deformAmount_;
            }
            height += deformOffset;

            if (bulge_ && std::abs(bulgeAmount_) > 0.01f) {
                const float normalizedDistance = bulgeRadius_ > 0.0f ? radialNorm / bulgeRadius_ : radialNorm;
                const float falloff = ofClamp(1.0f - normalizedDistance * normalizedDistance, 0.0f, 1.0f);
                height += falloff * bulgeAmount_ * cosf(params.time * bulgeSpeed_);
            }

            if (twist_ && std::abs(twistAmount_) > 0.01f) {
                constexpr float kDegToRad = 0.017453292519943295f;
                const float pulse = 1.0f + sinf(params.time * twistSpeed_) * 0.25f;
                const float angle = twistAmount_ * radialNorm * pulse * kDegToRad;
                const float c = cosf(angle);
                const float s = sinf(angle);
                px = basePx * c - basePz * s;
                pz = basePx * s + basePz * c;
            }

            waveVerts_[indexFor(x, z)] = { px, height, pz };
        }
    }

    params.camera.begin();
    ofEnableDepthTest();
    ofPushMatrix();
    ofTranslate(0, -40, 0);

    const float alphaScale = ofClamp(params.slotOpacity, 0.0f, 1.0f);
    const int baseAlpha = static_cast<int>(alphaScale * 255.0f);

    if (faceOpacity_ > 0.0f) {
        ofMesh mesh;
        mesh.setMode(OF_PRIMITIVE_TRIANGLES);
        mesh.addVertices(waveVerts_);
        for (int z = 0; z < seg; ++z) {
            for (int x = 0; x < seg; ++x) {
                int i00 = indexFor(x, z);
                int i10 = indexFor(x + 1, z);
                int i01 = indexFor(x, z + 1);
                int i11 = indexFor(x + 1, z + 1);
                mesh.addIndex(i00);
                mesh.addIndex(i10);
                mesh.addIndex(i11);
                mesh.addIndex(i00);
                mesh.addIndex(i11);
                mesh.addIndex(i01);
            }
        }
        ofPushStyle();
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        ofSetColor(kGridLineColor.r, kGridLineColor.g, kGridLineColor.b,
                   static_cast<int>(faceOpacity_ * baseAlpha));
        mesh.draw();
        ofDisableBlendMode();
        ofPopStyle();
    }

    ofSetColor(kGridLineColor.r, kGridLineColor.g, kGridLineColor.b, baseAlpha);
#ifndef TARGET_OPENGLES
    glLineWidth(1.0f);
#endif

    for (int x = 0; x <= seg; ++x) {
        ofPolyline line;
        line.getVertices().reserve(seg + 1);
        for (int z = 0; z <= seg; ++z) {
            line.addVertex(waveVerts_[indexFor(x, z)]);
        }
        line.draw();
    }

    for (int z = 0; z <= seg; ++z) {
        ofPolyline line;
        line.getVertices().reserve(seg + 1);
        for (int x = 0; x <= seg; ++x) {
            line.addVertex(waveVerts_[indexFor(x, z)]);
        }
        line.draw();
    }

    ofPopMatrix();
    ofDisableDepthTest();
    params.camera.end();
}

void GridLayer::cycleSegments() {
    paramSegments_ += kSegmentStep;
    if (paramSegments_ > 200.0f) {
        paramSegments_ = 20.0f;
    }
}

void GridLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

std::string GridLayer::deformationSummary() const {
    std::string summary;
    auto append = [&](const std::string& label) {
        if (!summary.empty()) {
            summary += "+";
        }
        summary += label;
    };

    if (wave_ && waveAmount_ > 0.01f) append("wave");
    if (bend_ && bendAmount_ > 0.01f) append("bend");
    if (deform_ && deformAmount_ > 0.01f) append("noise");
    if (twist_ && std::abs(twistAmount_) > 0.01f) append("twist");
    if (bulge_ && std::abs(bulgeAmount_) > 0.01f) append("bulge");
    return summary.empty() ? "flat" : summary;
}

int GridLayer::indexFor(int x, int z) const {
    return z * (segments_ + 1) + x;
}

void GridLayer::ensureVertexCapacity() {
    std::size_t expected = static_cast<std::size_t>((segments_ + 1) * (segments_ + 1));
    if (waveVerts_.size() != expected) {
        waveVerts_.assign(expected, glm::vec3(0));
    }
}
