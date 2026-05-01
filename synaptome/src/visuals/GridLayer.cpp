#include "GridLayer.h"
#include "ofGraphics.h"
#include <cmath>

namespace {
    constexpr float kSegmentStep = 20.0f;
    const ofColor kGridLineColor(0, 255, 160);
}

void GridLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "layer.grid" : registryPrefix();

    ParameterRegistry::Descriptor boolMeta;
    boolMeta.label = "Grid Visible";
    boolMeta.group = "Visibility";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, boolMeta);

    ParameterRegistry::Descriptor segMeta;
    segMeta.label = "Grid Segments";
    segMeta.group = "Grid";
    segMeta.range.min = 20.0f;
    segMeta.range.max = 200.0f;
    segMeta.range.step = kSegmentStep;
    registry.addFloat(prefix + ".segments", &paramSegments_, paramSegments_, segMeta);

    ParameterRegistry::Descriptor faceMeta;
    faceMeta.label = "Grid Face Opacity";
    faceMeta.group = "Grid";
    faceMeta.range.min = 0.0f;
    faceMeta.range.max = 1.0f;
    faceMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".faceOpacity", &paramFaceOpacity_, paramFaceOpacity_, faceMeta);
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

    for (int z = 0; z <= seg; ++z) {
        for (int x = 0; x <= seg; ++x) {
            const float px = ofMap(x, 0, seg, -size * 0.5f, size * 0.5f);
            const float pz = ofMap(z, 0, seg, -size * 0.5f, size * 0.5f);
            const float r = glm::length(glm::vec2(px, pz)) * 0.01f;
            const float wave = sinf(r * 3.0f - params.time * 2.0f) * 10.0f;
            const float bend = sinf((px + pz) * 0.01f + params.time * 1.2f) * 8.0f;
            waveVerts_[indexFor(x, z)] = { px, wave + bend, pz };
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

int GridLayer::indexFor(int x, int z) const {
    return z * (segments_ + 1) + x;
}

void GridLayer::ensureVertexCapacity() {
    std::size_t expected = static_cast<std::size_t>((segments_ + 1) * (segments_ + 1));
    if (waveVerts_.size() != expected) {
        waveVerts_.assign(expected, glm::vec3(0));
    }
}
