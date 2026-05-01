#include "StlModelLayer.h"

#include "ofFileUtils.h"
#include "ofGraphics.h"
#include "ofLog.h"
#include "ofMath.h"
#include "ofUtils.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace {
    std::unordered_map<std::string, ofMesh>& stlMeshCache() {
        static std::unordered_map<std::string, ofMesh> cache;
        return cache;
    }

    ofColor parseColorNode(const ofJson& value, const ofColor& fallback) {
        if (value.is_array() && value.size() >= 3) {
            return ofColor(static_cast<unsigned char>(ofClamp(value[0].get<float>(), 0.0f, 255.0f)),
                           static_cast<unsigned char>(ofClamp(value[1].get<float>(), 0.0f, 255.0f)),
                           static_cast<unsigned char>(ofClamp(value[2].get<float>(), 0.0f, 255.0f)));
        }
        if (value.is_string()) {
            std::string hex = value.get<std::string>();
            hex.erase(std::remove_if(hex.begin(), hex.end(), [](unsigned char ch) {
                return std::isspace(ch) != 0;
            }), hex.end());
            if (!hex.empty() && hex.front() == '#') {
                hex.erase(hex.begin());
            }
            if (hex.size() == 6) {
                try {
                    unsigned int encoded = static_cast<unsigned int>(std::stoul(hex, nullptr, 16));
                    return ofColor(static_cast<unsigned char>((encoded >> 16) & 0xFF),
                                   static_cast<unsigned char>((encoded >> 8) & 0xFF),
                                   static_cast<unsigned char>(encoded & 0xFF));
                } catch (...) {
                }
            }
        }
        return fallback;
    }
}

void StlModelLayer::configure(const ofJson& config) {
    assetPath_ = config.value("assetPath", assetPath_);
    if (config.contains("color")) {
        const ofColor parsed = parseColorNode(config["color"], liveColor_);
        paramColorR_ = static_cast<float>(parsed.r);
        paramColorG_ = static_cast<float>(parsed.g);
        paramColorB_ = static_cast<float>(parsed.b);
    }

    if (config.contains("defaults") && config["defaults"].is_object()) {
        const auto& defaults = config["defaults"];
        paramSpinDeg_ = defaults.value("spin", paramSpinDeg_);
        paramHoverAmp_ = defaults.value("hover", paramHoverAmp_);
        paramBaseHeight_ = defaults.value("baseHeight", paramBaseHeight_);
        paramOrbitRadius_ = defaults.value("orbitRadius", paramOrbitRadius_);
        paramOrbitSpeedDeg_ = defaults.value("orbitSpeed", paramOrbitSpeedDeg_);
        paramRotateXDeg_ = defaults.value("rotateX", paramRotateXDeg_);
        paramRotateYDeg_ = defaults.value("rotateY", paramRotateYDeg_);
        paramRotateZDeg_ = defaults.value("rotateZ", paramRotateZDeg_);
        if (defaults.contains("color") && defaults["color"].is_array() && defaults["color"].size() >= 3) {
            paramColorR_ = defaults["color"][0].get<float>();
            paramColorG_ = defaults["color"][1].get<float>();
            paramColorB_ = defaults["color"][2].get<float>();
        }
        paramScale_ = defaults.value("scale", paramScale_);
        paramLineOpacity_ = defaults.value("lineOpacity", paramLineOpacity_);
        paramFaceOpacity_ = defaults.value("faceOpacity", paramFaceOpacity_);
        paramEnabled_ = defaults.value("visible", paramEnabled_);
    }
}

void StlModelLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "layer.stlModel" : registryPrefix();

    ParameterRegistry::Descriptor visMeta;
    visMeta.label = "Model Visible";
    visMeta.group = "Visibility";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, visMeta);

    ParameterRegistry::Descriptor spinMeta;
    spinMeta.label = "Model Spin";
    spinMeta.group = "Model";
    spinMeta.range.min = -360.0f;
    spinMeta.range.max = 360.0f;
    spinMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".spin", &paramSpinDeg_, paramSpinDeg_, spinMeta);

    ParameterRegistry::Descriptor hoverMeta;
    hoverMeta.label = "Model Hover";
    hoverMeta.group = "Model";
    hoverMeta.range.min = 0.0f;
    hoverMeta.range.max = 200.0f;
    hoverMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".hover", &paramHoverAmp_, paramHoverAmp_, hoverMeta);

    ParameterRegistry::Descriptor heightMeta;
    heightMeta.label = "Model Base Height";
    heightMeta.group = "Model";
    heightMeta.range.min = -400.0f;
    heightMeta.range.max = 400.0f;
    heightMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".baseHeight", &paramBaseHeight_, paramBaseHeight_, heightMeta);

    ParameterRegistry::Descriptor orbitRadiusMeta;
    orbitRadiusMeta.label = "Model Orbit Radius";
    orbitRadiusMeta.group = "Model";
    orbitRadiusMeta.range.min = 0.0f;
    orbitRadiusMeta.range.max = 600.0f;
    orbitRadiusMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".orbitRadius", &paramOrbitRadius_, paramOrbitRadius_, orbitRadiusMeta);

    ParameterRegistry::Descriptor orbitSpeedMeta;
    orbitSpeedMeta.label = "Model Orbit Speed";
    orbitSpeedMeta.group = "Model";
    orbitSpeedMeta.range.min = -360.0f;
    orbitSpeedMeta.range.max = 360.0f;
    orbitSpeedMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".orbitSpeed", &paramOrbitSpeedDeg_, paramOrbitSpeedDeg_, orbitSpeedMeta);

    ParameterRegistry::Descriptor rotateMeta;
    rotateMeta.group = "Model";
    rotateMeta.range.min = -180.0f;
    rotateMeta.range.max = 180.0f;
    rotateMeta.range.step = 1.0f;
    rotateMeta.label = "Model Rotate X";
    registry.addFloat(prefix + ".rotateX", &paramRotateXDeg_, paramRotateXDeg_, rotateMeta);
    rotateMeta.label = "Model Rotate Y";
    registry.addFloat(prefix + ".rotateY", &paramRotateYDeg_, paramRotateYDeg_, rotateMeta);
    rotateMeta.label = "Model Rotate Z";
    registry.addFloat(prefix + ".rotateZ", &paramRotateZDeg_, paramRotateZDeg_, rotateMeta);

    ParameterRegistry::Descriptor colorMeta;
    colorMeta.group = "Model";
    colorMeta.range.min = 0.0f;
    colorMeta.range.max = 255.0f;
    colorMeta.range.step = 1.0f;
    colorMeta.label = "Model Red";
    registry.addFloat(prefix + ".colorR", &paramColorR_, paramColorR_, colorMeta);
    colorMeta.label = "Model Green";
    registry.addFloat(prefix + ".colorG", &paramColorG_, paramColorG_, colorMeta);
    colorMeta.label = "Model Blue";
    registry.addFloat(prefix + ".colorB", &paramColorB_, paramColorB_, colorMeta);

    ParameterRegistry::Descriptor scaleMeta;
    scaleMeta.label = "Model Scale";
    scaleMeta.group = "Model";
    scaleMeta.range.min = 20.0f;
    scaleMeta.range.max = 500.0f;
    scaleMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".scale", &paramScale_, paramScale_, scaleMeta);

    ParameterRegistry::Descriptor lineMeta;
    lineMeta.label = "Model Line Opacity";
    lineMeta.group = "Model";
    lineMeta.range.min = 0.0f;
    lineMeta.range.max = 1.0f;
    lineMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".lineOpacity", &paramLineOpacity_, paramLineOpacity_, lineMeta);

    ParameterRegistry::Descriptor faceMeta;
    faceMeta.label = "Model Face Opacity";
    faceMeta.group = "Model";
    faceMeta.range.min = 0.0f;
    faceMeta.range.max = 1.0f;
    faceMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".faceOpacity", &paramFaceOpacity_, paramFaceOpacity_, faceMeta);

    meshLoaded_ = loadMesh();
}

void StlModelLayer::update(const LayerUpdateParams& params) {
    (void)params;

    enabled_ = paramEnabled_;

    const float clampedSpin = ofClamp(paramSpinDeg_, -360.0f, 360.0f);
    if (paramSpinDeg_ != clampedSpin) paramSpinDeg_ = clampedSpin;
    spinDeg_ = clampedSpin;

    const float clampedHover = ofClamp(paramHoverAmp_, 0.0f, 200.0f);
    if (paramHoverAmp_ != clampedHover) paramHoverAmp_ = clampedHover;
    hoverAmp_ = clampedHover;

    const float clampedBaseHeight = ofClamp(paramBaseHeight_, -400.0f, 400.0f);
    if (paramBaseHeight_ != clampedBaseHeight) paramBaseHeight_ = clampedBaseHeight;
    baseHeight_ = clampedBaseHeight;

    const float clampedOrbitRadius = ofClamp(paramOrbitRadius_, 0.0f, 600.0f);
    if (paramOrbitRadius_ != clampedOrbitRadius) paramOrbitRadius_ = clampedOrbitRadius;
    orbitRadius_ = clampedOrbitRadius;

    const float clampedOrbitSpeed = ofClamp(paramOrbitSpeedDeg_, -360.0f, 360.0f);
    if (paramOrbitSpeedDeg_ != clampedOrbitSpeed) paramOrbitSpeedDeg_ = clampedOrbitSpeed;
    orbitSpeedDeg_ = clampedOrbitSpeed;

    const float clampedRotateX = ofClamp(paramRotateXDeg_, -180.0f, 180.0f);
    if (paramRotateXDeg_ != clampedRotateX) paramRotateXDeg_ = clampedRotateX;
    rotateXDeg_ = clampedRotateX;

    const float clampedRotateY = ofClamp(paramRotateYDeg_, -180.0f, 180.0f);
    if (paramRotateYDeg_ != clampedRotateY) paramRotateYDeg_ = clampedRotateY;
    rotateYDeg_ = clampedRotateY;

    const float clampedRotateZ = ofClamp(paramRotateZDeg_, -180.0f, 180.0f);
    if (paramRotateZDeg_ != clampedRotateZ) paramRotateZDeg_ = clampedRotateZ;
    rotateZDeg_ = clampedRotateZ;

    paramColorR_ = ofClamp(paramColorR_, 0.0f, 255.0f);
    paramColorG_ = ofClamp(paramColorG_, 0.0f, 255.0f);
    paramColorB_ = ofClamp(paramColorB_, 0.0f, 255.0f);
    liveColor_.r = static_cast<unsigned char>(paramColorR_);
    liveColor_.g = static_cast<unsigned char>(paramColorG_);
    liveColor_.b = static_cast<unsigned char>(paramColorB_);

    const float clampedScale = ofClamp(paramScale_, 20.0f, 500.0f);
    if (paramScale_ != clampedScale) paramScale_ = clampedScale;
    scale_ = clampedScale;

    const float clampedLine = ofClamp(paramLineOpacity_, 0.0f, 1.0f);
    if (paramLineOpacity_ != clampedLine) paramLineOpacity_ = clampedLine;
    lineOpacity_ = clampedLine;

    const float clampedFace = ofClamp(paramFaceOpacity_, 0.0f, 1.0f);
    if (paramFaceOpacity_ != clampedFace) paramFaceOpacity_ = clampedFace;
    faceOpacity_ = clampedFace;
}

void StlModelLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || !meshLoaded_ || params.slotOpacity <= 0.0f) return;

    params.camera.begin();
    ofEnableDepthTest();

    ofPushMatrix();
    const float hoverY = std::sin(params.time * 0.8f) * hoverAmp_;
    const float orbitAngle = std::fmod(params.time * orbitSpeedDeg_, 360.0f);
    ofRotateYDeg(orbitAngle);
    ofTranslate(orbitRadius_, baseHeight_ + hoverY, 0.0f);
    ofRotateYDeg(std::fmod(params.time * spinDeg_, 360.0f));
    ofRotateXDeg(rotateXDeg_);
    ofRotateYDeg(rotateYDeg_);
    ofRotateZDeg(rotateZDeg_);
    ofScale(scale_, scale_, scale_);

    const float alphaScale = ofClamp(params.slotOpacity, 0.0f, 1.0f);
    const int baseAlpha = static_cast<int>(alphaScale * 255.0f);

    if (faceOpacity_ > 0.0f) {
        ofPushStyle();
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        ofSetColor(liveColor_.r, liveColor_.g, liveColor_.b,
                   static_cast<int>(faceOpacity_ * baseAlpha));
        mesh_.draw();
        ofDisableBlendMode();
        ofPopStyle();
    }

    if (lineOpacity_ > 0.0f) {
        ofSetColor(liveColor_.r, liveColor_.g, liveColor_.b,
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

void StlModelLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

bool StlModelLayer::loadMesh() {
    const std::string resolvedPath = ofToDataPath(assetPath_, true);
    auto& cache = stlMeshCache();
    auto cached = cache.find(resolvedPath);
    if (cached != cache.end()) {
        mesh_ = cached->second;
        mesh_.setMode(OF_PRIMITIVE_TRIANGLES);
        const bool ok = mesh_.getNumVertices() > 0;
        if (ok) {
            ofLogNotice("StlModelLayer") << "Loaded cached STL mesh " << assetPath_
                                         << " (" << mesh_.getNumVertices() << " vertices)";
        }
        return ok;
    }

    const uint64_t startedMs = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    ofMesh loaded;
    const bool ok = loadBinaryStl(resolvedPath, loaded) || loadAsciiStl(resolvedPath, loaded);
    if (!ok) {
        ofLogWarning("StlModelLayer") << "Failed to load STL mesh from " << assetPath_;
        mesh_.clear();
        return false;
    }

    normalizeMesh(loaded);
    loaded.setMode(OF_PRIMITIVE_TRIANGLES);
    cache[resolvedPath] = loaded;
    mesh_ = loaded;
    mesh_.setMode(OF_PRIMITIVE_TRIANGLES);
    const bool loadedOk = mesh_.getNumVertices() > 0;
    if (loadedOk) {
        const uint64_t elapsedMs = static_cast<uint64_t>(ofGetElapsedTimeMillis()) - startedMs;
        ofLogNotice("StlModelLayer") << "Parsed STL mesh " << assetPath_
                                     << " (" << mesh_.getNumVertices()
                                     << " vertices) in " << elapsedMs << " ms";
    }
    return loadedOk;
}

bool StlModelLayer::loadBinaryStl(const std::string& path, ofMesh& mesh) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }

    in.seekg(0, std::ios::end);
    const std::streamoff size = in.tellg();
    if (size < 84) {
        return false;
    }
    in.seekg(80, std::ios::beg);

    uint32_t triangleCount = 0;
    in.read(reinterpret_cast<char*>(&triangleCount), sizeof(triangleCount));
    if (!in) {
        return false;
    }

    const std::uint64_t expectedSize = 84ull + static_cast<std::uint64_t>(triangleCount) * 50ull;
    if (expectedSize != static_cast<std::uint64_t>(size)) {
        return false;
    }

    mesh.clear();
    mesh.setMode(OF_PRIMITIVE_TRIANGLES);
    for (uint32_t i = 0; i < triangleCount; ++i) {
        std::array<float, 12> values {};
        in.read(reinterpret_cast<char*>(values.data()), sizeof(float) * values.size());
        std::uint16_t attributeBytes = 0;
        in.read(reinterpret_cast<char*>(&attributeBytes), sizeof(attributeBytes));
        if (!in) {
            mesh.clear();
            return false;
        }

        appendTriangle(mesh,
                       glm::vec3(values[3], values[4], values[5]),
                       glm::vec3(values[6], values[7], values[8]),
                       glm::vec3(values[9], values[10], values[11]));
    }
    return mesh.getNumVertices() > 0;
}

bool StlModelLayer::loadAsciiStl(const std::string& path, ofMesh& mesh) {
    std::ifstream in(path);
    if (!in) {
        return false;
    }

    mesh.clear();
    mesh.setMode(OF_PRIMITIVE_TRIANGLES);
    std::string line;
    std::vector<glm::vec3> vertices;
    vertices.reserve(3);

    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        if (token != "vertex") {
            continue;
        }

        glm::vec3 vertex(0.0f);
        if (!(iss >> vertex.x >> vertex.y >> vertex.z)) {
            mesh.clear();
            return false;
        }
        vertices.push_back(vertex);
        if (vertices.size() == 3) {
            appendTriangle(mesh, vertices[0], vertices[1], vertices[2]);
            vertices.clear();
        }
    }

    if (!vertices.empty()) {
        mesh.clear();
        return false;
    }
    return mesh.getNumVertices() > 0;
}

void StlModelLayer::normalizeMesh(ofMesh& mesh) {
    if (mesh.getNumVertices() == 0) {
        return;
    }

    glm::vec3 minV(std::numeric_limits<float>::max());
    glm::vec3 maxV(std::numeric_limits<float>::lowest());
    for (const auto& vertex : mesh.getVertices()) {
        minV.x = std::min(minV.x, vertex.x);
        minV.y = std::min(minV.y, vertex.y);
        minV.z = std::min(minV.z, vertex.z);
        maxV.x = std::max(maxV.x, vertex.x);
        maxV.y = std::max(maxV.y, vertex.y);
        maxV.z = std::max(maxV.z, vertex.z);
    }

    const glm::vec3 center = (minV + maxV) * 0.5f;
    const glm::vec3 span = maxV - minV;
    const float maxDim = std::max(span.x, std::max(span.y, span.z));
    const float invScale = maxDim > 0.0f ? (1.0f / maxDim) : 1.0f;

    auto& vertices = mesh.getVertices();
    for (auto& vertex : vertices) {
        vertex = (vertex - center) * invScale;
    }
}

void StlModelLayer::appendTriangle(ofMesh& mesh,
                                   const glm::vec3& a,
                                   const glm::vec3& b,
                                   const glm::vec3& c) {
    glm::vec3 normal = glm::cross(b - a, c - a);
    const float length = glm::length(normal);
    if (length <= std::numeric_limits<float>::epsilon()) {
        normal = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        normal /= length;
    }

    mesh.addVertex(a);
    mesh.addVertex(b);
    mesh.addVertex(c);
    mesh.addNormal(normal);
    mesh.addNormal(normal);
    mesh.addNormal(normal);
}
