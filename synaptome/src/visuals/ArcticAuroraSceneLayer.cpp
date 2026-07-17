#include "ArcticAuroraSceneLayer.h"

#include "../io/AudioAnalysisBus.h"
#include "ofGraphics.h"
#include "ofMath.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <utility>

namespace {
    constexpr float kMaxDt = 1.0f / 20.0f;
    constexpr int kWaterCols = 56;
    constexpr int kWaterRows = 48;
    constexpr int kAuroraEnergySamples = 96;

    float followAmount(float smoothing) {
        return 1.0f - ofClamp(smoothing, 0.0f, 0.98f);
    }

    float signedNoise(float x, float y, float z) {
        return ofNoise(x, y, z) * 2.0f - 1.0f;
    }

    ofFloatColor colorFrom(float r, float g, float b, float a) {
        return ofFloatColor(ofClamp(r, 0.0f, 1.5f),
                            ofClamp(g, 0.0f, 1.5f),
                            ofClamp(b, 0.0f, 1.5f),
                            ofClamp(a, 0.0f, 1.0f));
    }

    ofFloatColor withAlphaScale(const ofFloatColor& color, float alphaScale) {
        return ofFloatColor(color.r, color.g, color.b, ofClamp(color.a * alphaScale, 0.0f, 1.0f));
    }

    ofFloatColor tinted(const ofFloatColor& color, float r, float g, float b) {
        return ofFloatColor(color.r * r, color.g * g, color.b * b, color.a);
    }

    ofFloatColor scaledAlpha(const ofFloatColor& color, float alpha) {
        return ofFloatColor(color.r, color.g, color.b, ofClamp(color.a * alpha, 0.0f, 1.0f));
    }

    float smootherStep(float value) {
        const float t = ofClamp(value, 0.0f, 1.0f);
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    float waterWaveAt(float x, float z, float time, float amount) {
        const float broad = std::sin(x * 0.0085f + z * 0.012f + time * 0.42f);
        const float cross = std::sin(x * 0.018f - z * 0.0075f + time * 0.31f);
        const float detail = signedNoise(x * 0.004f + 18.0f, z * 0.005f - 7.0f, time * 0.11f);
        return (broad * 0.52f + cross * 0.30f + detail * 0.18f) * amount;
    }

    glm::vec3 polarPoint(float theta, float radius, float y) {
        return glm::vec3(std::cos(theta) * radius, y, std::sin(theta) * radius);
    }

    float radiusFromDepthParam(float value) {
        return std::max(1.0f, std::abs(value));
    }

    struct AuroraFlow {
        glm::vec2 planarOffset = glm::vec2(0.0f, 0.0f);
        float verticalOffset = 0.0f;
        float energy = 1.0f;
    };

    void addTriangle(ofMesh& mesh,
                     const glm::vec3& a,
                     const glm::vec3& b,
                     const glm::vec3& c,
                     const ofFloatColor& color) {
        const int base = static_cast<int>(mesh.getNumVertices());
        mesh.addVertex(a);
        mesh.addColor(color);
        mesh.addVertex(b);
        mesh.addColor(color);
        mesh.addVertex(c);
        mesh.addColor(color);
        mesh.addIndex(base);
        mesh.addIndex(base + 1);
        mesh.addIndex(base + 2);
    }

    void addQuad(ofMesh& mesh,
                 const glm::vec3& a,
                 const glm::vec3& b,
                 const glm::vec3& c,
                 const glm::vec3& d,
                 const ofFloatColor& color) {
        addTriangle(mesh, a, b, c, color);
        addTriangle(mesh, a, c, d, color);
    }

    void addLine(ofMesh& mesh,
                 const glm::vec3& a,
                 const glm::vec3& b,
                 const ofFloatColor& color) {
        mesh.addVertex(a);
        mesh.addColor(color);
        mesh.addVertex(b);
        mesh.addColor(color);
    }

    float randomRange(std::mt19937& rng, float minValue, float maxValue) {
        std::uniform_real_distribution<float> dist(minValue, maxValue);
        return dist(rng);
    }

    int randomInt(std::mt19937& rng, int minValue, int maxValue) {
        std::uniform_int_distribution<int> dist(minValue, maxValue);
        return dist(rng);
    }

    void drawMeshWithAlpha(const ofMesh& source, float alpha) {
        if (source.getNumVertices() == 0 || alpha <= 0.0f) {
            return;
        }

        ofMesh mesh = source;
        auto& colors = mesh.getColors();
        for (auto& color : colors) {
            color.a = ofClamp(color.a * alpha, 0.0f, 1.0f);
        }
        mesh.draw();
    }

    struct TriangulationTriangle {
        int a = 0;
        int b = 0;
        int c = 0;
    };

    struct TriangulationEdge {
        int a = 0;
        int b = 0;
    };

    float orient2d(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    void addDelaunayBoundaryEdge(std::vector<TriangulationEdge>& edges, TriangulationEdge edge) {
        const int minA = std::min(edge.a, edge.b);
        const int maxB = std::max(edge.a, edge.b);
        for (auto it = edges.begin(); it != edges.end(); ++it) {
            if (std::min(it->a, it->b) == minA && std::max(it->a, it->b) == maxB) {
                edges.erase(it);
                return;
            }
        }
        edges.push_back(edge);
    }

    bool circumcircleContains(const glm::vec2& p,
                              const glm::vec2& a,
                              const glm::vec2& b,
                              const glm::vec2& c) {
        const float ax = a.x - p.x;
        const float ay = a.y - p.y;
        const float bx = b.x - p.x;
        const float by = b.y - p.y;
        const float cx = c.x - p.x;
        const float cy = c.y - p.y;

        const float det = (ax * ax + ay * ay) * (bx * cy - cx * by)
            - (bx * bx + by * by) * (ax * cy - cx * ay)
            + (cx * cx + cy * cy) * (ax * by - bx * ay);
        const float orientation = orient2d(a, b, c);
        return orientation > 0.0f ? det > 0.0001f : det < -0.0001f;
    }

    std::vector<TriangulationTriangle> delaunayTriangulate(const std::vector<glm::vec2>& inputPoints) {
        std::vector<TriangulationTriangle> result;
        if (inputPoints.size() < 3) {
            return result;
        }

        std::vector<glm::vec2> points = inputPoints;
        glm::vec2 minPoint = points.front();
        glm::vec2 maxPoint = points.front();
        for (const auto& point : points) {
            minPoint.x = std::min(minPoint.x, point.x);
            minPoint.y = std::min(minPoint.y, point.y);
            maxPoint.x = std::max(maxPoint.x, point.x);
            maxPoint.y = std::max(maxPoint.y, point.y);
        }

        const glm::vec2 span = maxPoint - minPoint;
        const float dmax = std::max(span.x, span.y);
        const glm::vec2 center = (minPoint + maxPoint) * 0.5f;
        const int superA = static_cast<int>(points.size());
        const int superB = superA + 1;
        const int superC = superA + 2;
        points.push_back(center + glm::vec2(-2.0f * dmax - 10.0f, -dmax - 10.0f));
        points.push_back(center + glm::vec2(0.0f, 2.0f * dmax + 20.0f));
        points.push_back(center + glm::vec2(2.0f * dmax + 10.0f, -dmax - 10.0f));

        std::vector<TriangulationTriangle> triangles;
        triangles.push_back({ superA, superB, superC });

        const int originalCount = static_cast<int>(inputPoints.size());
        for (int pointIndex = 0; pointIndex < originalCount; ++pointIndex) {
            std::vector<bool> bad(triangles.size(), false);
            std::vector<TriangulationEdge> polygon;
            for (std::size_t triIndex = 0; triIndex < triangles.size(); ++triIndex) {
                const auto& tri = triangles[triIndex];
                if (!circumcircleContains(points[pointIndex], points[tri.a], points[tri.b], points[tri.c])) {
                    continue;
                }
                bad[triIndex] = true;
                addDelaunayBoundaryEdge(polygon, { tri.a, tri.b });
                addDelaunayBoundaryEdge(polygon, { tri.b, tri.c });
                addDelaunayBoundaryEdge(polygon, { tri.c, tri.a });
            }

            std::vector<TriangulationTriangle> kept;
            kept.reserve(triangles.size() + polygon.size());
            for (std::size_t triIndex = 0; triIndex < triangles.size(); ++triIndex) {
                if (!bad[triIndex]) {
                    kept.push_back(triangles[triIndex]);
                }
            }

            for (const auto& edge : polygon) {
                if (edge.a == edge.b) {
                    continue;
                }
                TriangulationTriangle tri{ edge.a, edge.b, pointIndex };
                if (std::abs(orient2d(points[tri.a], points[tri.b], points[tri.c])) <= 0.0001f) {
                    continue;
                }
                if (orient2d(points[tri.a], points[tri.b], points[tri.c]) < 0.0f) {
                    std::swap(tri.a, tri.b);
                }
                kept.push_back(tri);
            }

            triangles = std::move(kept);
        }

        for (const auto& tri : triangles) {
            if (tri.a >= originalCount || tri.b >= originalCount || tri.c >= originalCount) {
                continue;
            }
            result.push_back(tri);
        }
        return result;
    }

    bool pointInPolygon(const glm::vec2& point, const std::vector<glm::vec2>& polygon) {
        bool inside = false;
        if (polygon.size() < 3) {
            return false;
        }
        for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
            const glm::vec2& a = polygon[i];
            const glm::vec2& b = polygon[j];
            const float denom = b.y - a.y;
            const float safeDenom = std::abs(denom) > 0.0001f ? denom : (denom < 0.0f ? -0.0001f : 0.0001f);
            const bool intersects = ((a.y > point.y) != (b.y > point.y))
                && (point.x < (b.x - a.x) * (point.y - a.y) / safeDenom + a.x);
            if (intersects) {
                inside = !inside;
            }
        }
        return inside;
    }

    ofFloatColor facetColorFor(const glm::vec3& a,
                               const glm::vec3& b,
                               const glm::vec3& c,
                               float halfWidth,
                               float halfDepth,
                               float height,
                               const ofFloatColor& snow,
                               const ofFloatColor& blue,
                               const ofFloatColor& shadow,
                               const ofFloatColor& accent) {
        glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
        if (!std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z)) {
            normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        const glm::vec3 centroid = (a + b + c) / 3.0f;
        const float topness = ofClamp((normal.y + 0.18f) / 1.18f, 0.0f, 1.0f);
        const float heightMix = ofClamp(centroid.y / std::max(1.0f, height), 0.0f, 1.0f);
        const float lateral = ofClamp(std::abs(centroid.x) / std::max(1.0f, halfWidth), 0.0f, 1.0f);
        const float backness = ofClamp(-centroid.z / std::max(1.0f, halfDepth), 0.0f, 1.0f);
        const float shade = ofClamp(0.24f + topness * 0.52f + heightMix * 0.18f - backness * 0.14f, 0.0f, 1.0f);
        ofFloatColor color = shadow.getLerped(blue, shade).getLerped(snow, topness * 0.42f + heightMix * 0.18f);
        color = color.getLerped(accent, lateral * (1.0f - topness) * 0.16f);
        color.a = ofClamp(0.72f + topness * 0.18f, 0.0f, 0.94f);
        return color;
    }
}

void ArcticAuroraSceneLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }

    const auto& def = config["defaults"];
    paramEnabled_ = def.value("visible", paramEnabled_);
    paramAlpha_ = def.value("alpha", paramAlpha_);
    paramSceneScale_ = def.value("sceneScale", paramSceneScale_);
    paramSceneOffsetY_ = def.value("sceneOffsetY", paramSceneOffsetY_);
    paramSceneOffsetZ_ = def.value("sceneOffsetZ", paramSceneOffsetZ_);
    paramWaterWidth_ = def.value("waterWidth", paramWaterWidth_);
    paramWaterNearZ_ = def.value("waterNearZ", paramWaterNearZ_);
    paramWaterFarZ_ = def.value("waterFarZ", paramWaterFarZ_);
    paramWaterLevel_ = def.value("waterLevel", paramWaterLevel_);
    paramWaterWaveIdle_ = def.value("waterWaveIdle", paramWaterWaveIdle_);
    paramWaterHighlight_ = def.value("waterHighlight", paramWaterHighlight_);
    paramWaterReflection_ = def.value("waterReflection", paramWaterReflection_);
    paramWaterHorizonFog_ = def.value("waterHorizonFog", paramWaterHorizonFog_);
    paramWaterAlpha_ = def.value("waterAlpha", paramWaterAlpha_);
    paramWaterBrightness_ = def.value("waterBrightness", paramWaterBrightness_);
    paramWaterTranslucency_ = def.value("waterTranslucency", paramWaterTranslucency_);
    paramWaterCurvature_ = def.value("waterCurvature", paramWaterCurvature_);
    paramWaterHemisphereDepth_ = def.value("waterHemisphereDepth", paramWaterHemisphereDepth_);
    paramWaterNoiseAmount_ = def.value("waterNoiseAmount", paramWaterNoiseAmount_);
    paramWaterNoiseScale_ = def.value("waterNoiseScale", paramWaterNoiseScale_);
    paramWaterRippleAmount_ = def.value("waterRippleAmount", paramWaterRippleAmount_);
    paramWaterRippleRadius_ = def.value("waterRippleRadius", paramWaterRippleRadius_);
    paramWaterAuroraLight_ = def.value("waterAuroraLight", paramWaterAuroraLight_);
    paramIcebergCount_ = def.value("icebergCount", paramIcebergCount_);
    paramIcebergScale_ = def.value("icebergScale", paramIcebergScale_);
    paramIcebergSpread_ = def.value("icebergSpread", paramIcebergSpread_);
    paramIcebergRimLight_ = def.value("icebergRimLight", paramIcebergRimLight_);
    paramIcebergBreakup_ = def.value("icebergBreakup", paramIcebergBreakup_);
    paramIcebergBreakupSpeed_ = def.value("icebergBreakupSpeed", paramIcebergBreakupSpeed_);
    paramAuroraWidth_ = def.value("auroraWidth", paramAuroraWidth_);
    paramAuroraBaseY_ = def.value("auroraBaseY", paramAuroraBaseY_);
    paramAuroraHeight_ = def.value("auroraHeight", paramAuroraHeight_);
    paramAuroraDepthNear_ = def.value("auroraDepthNear", paramAuroraDepthNear_);
    paramAuroraDepthFar_ = def.value("auroraDepthFar", paramAuroraDepthFar_);
    paramAuroraGlow_ = def.value("auroraGlow", paramAuroraGlow_);
    paramAuroraBloom_ = def.value("auroraBloom", paramAuroraBloom_);
    paramAuroraFoldStrength_ = def.value("auroraFoldStrength", paramAuroraFoldStrength_);
    paramAuroraRayDensity_ = def.value("auroraRayDensity", paramAuroraRayDensity_);
    paramAuroraCurtainCount_ = def.value("auroraCurtainCount", paramAuroraCurtainCount_);
    paramAudioAmount_ = def.value("audioAmount", paramAudioAmount_);
    paramAudioSmoothing_ = def.value("audioSmoothing", paramAudioSmoothing_);
    paramSeed_ = def.value("seed", paramSeed_);

    paramSkyTopR_ = def.value("skyTopR", paramSkyTopR_);
    paramSkyTopG_ = def.value("skyTopG", paramSkyTopG_);
    paramSkyTopB_ = def.value("skyTopB", paramSkyTopB_);
    paramSkyHorizonR_ = def.value("skyHorizonR", paramSkyHorizonR_);
    paramSkyHorizonG_ = def.value("skyHorizonG", paramSkyHorizonG_);
    paramSkyHorizonB_ = def.value("skyHorizonB", paramSkyHorizonB_);
    paramWaterR_ = def.value("waterR", paramWaterR_);
    paramWaterG_ = def.value("waterG", paramWaterG_);
    paramWaterB_ = def.value("waterB", paramWaterB_);
    paramAuroraR_ = def.value("auroraR", paramAuroraR_);
    paramAuroraG_ = def.value("auroraG", paramAuroraG_);
    paramAuroraB_ = def.value("auroraB", paramAuroraB_);
    paramAurora2R_ = def.value("aurora2R", paramAurora2R_);
    paramAurora2G_ = def.value("aurora2G", paramAurora2G_);
    paramAurora2B_ = def.value("aurora2B", paramAurora2B_);
    paramIceRimR_ = def.value("iceRimR", paramIceRimR_);
    paramIceRimG_ = def.value("iceRimG", paramIceRimG_);
    paramIceRimB_ = def.value("iceRimB", paramIceRimB_);
    paramIceAccentR_ = def.value("iceAccentR", paramIceAccentR_);
    paramIceAccentG_ = def.value("iceAccentG", paramIceAccentG_);
    paramIceAccentB_ = def.value("iceAccentB", paramIceAccentB_);

    readColor(def, "skyTopColor", paramSkyTopR_, paramSkyTopG_, paramSkyTopB_);
    readColor(def, "skyHorizonColor", paramSkyHorizonR_, paramSkyHorizonG_, paramSkyHorizonB_);
    readColor(def, "waterColor", paramWaterR_, paramWaterG_, paramWaterB_);
    readColor(def, "auroraColor", paramAuroraR_, paramAuroraG_, paramAuroraB_);
    readColor(def, "aurora2Color", paramAurora2R_, paramAurora2G_, paramAurora2B_);
    readColor(def, "iceRimColor", paramIceRimR_, paramIceRimG_, paramIceRimB_);
    readColor(def, "iceAccentColor", paramIceAccentR_, paramIceAccentG_, paramIceAccentB_);
    clampParams();
}

void ArcticAuroraSceneLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "generative.arcticAuroraScene" : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "3D Arctic Aurora";
    meta.label = "Layer: Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    registerFloat(registry, prefix + ".alpha", &paramAlpha_, paramAlpha_, "Layer: Opacity", 0.0f, 1.0f, 0.01f, "normalized");
    registerFloat(registry, prefix + ".sceneScale", &paramSceneScale_, paramSceneScale_, "Scene: Scale", 0.25f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".sceneOffsetY", &paramSceneOffsetY_, paramSceneOffsetY_, "Scene: Offset Y", -500.0f, 500.0f, 1.0f);
    registerFloat(registry, prefix + ".sceneOffsetZ", &paramSceneOffsetZ_, paramSceneOffsetZ_, "Scene: Offset Z", -1000.0f, 1000.0f, 1.0f);

    registerFloat(registry, prefix + ".waterWidth", &paramWaterWidth_, paramWaterWidth_, "Water: Diameter", 300.0f, 6000.0f, 10.0f);
    registerFloat(registry, prefix + ".waterNearZ", &paramWaterNearZ_, paramWaterNearZ_, "Water: Near Z", -300.0f, 700.0f, 10.0f);
    registerFloat(registry, prefix + ".waterFarZ", &paramWaterFarZ_, paramWaterFarZ_, "Water: Far Z", -2600.0f, -300.0f, 10.0f);
    registerFloat(registry, prefix + ".waterLevel", &paramWaterLevel_, paramWaterLevel_, "Water: Level", -400.0f, 100.0f, 1.0f);
    registerFloat(registry, prefix + ".waterWaveIdle", &paramWaterWaveIdle_, paramWaterWaveIdle_, "Water: Idle Wave", 0.0f, 28.0f, 0.5f);
    registerFloat(registry, prefix + ".waterHighlight", &paramWaterHighlight_, paramWaterHighlight_, "Water: Highlight", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".waterReflection", &paramWaterReflection_, paramWaterReflection_, "Water: Reflection", 0.0f, 1.8f, 0.01f);
    registerFloat(registry, prefix + ".waterHorizonFog", &paramWaterHorizonFog_, paramWaterHorizonFog_, "Water: Horizon Fog", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".waterAlpha", &paramWaterAlpha_, paramWaterAlpha_, "Water: Alpha", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".waterBrightness", &paramWaterBrightness_, paramWaterBrightness_, "Water: Brightness", 0.2f, 2.5f, 0.01f);
    registerFloat(registry, prefix + ".waterTranslucency", &paramWaterTranslucency_, paramWaterTranslucency_, "Water: Translucency", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".waterCurvature", &paramWaterCurvature_, paramWaterCurvature_, "Water: Curvature", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".waterHemisphereDepth", &paramWaterHemisphereDepth_, paramWaterHemisphereDepth_, "Water: Hemisphere Depth", 0.0f, 6000.0f, 10.0f);
    registerFloat(registry, prefix + ".waterNoiseAmount", &paramWaterNoiseAmount_, paramWaterNoiseAmount_, "Water: Noise Amount", 0.0f, 18.0f, 0.1f);
    registerFloat(registry, prefix + ".waterNoiseScale", &paramWaterNoiseScale_, paramWaterNoiseScale_, "Water: Noise Scale", 0.0005f, 0.012f, 0.0001f);
    registerFloat(registry, prefix + ".waterRippleAmount", &paramWaterRippleAmount_, paramWaterRippleAmount_, "Water: Ripple Amount", 0.0f, 18.0f, 0.1f);
    registerFloat(registry, prefix + ".waterRippleRadius", &paramWaterRippleRadius_, paramWaterRippleRadius_, "Water: Ripple Radius", 80.0f, 1000.0f, 5.0f);
    registerFloat(registry, prefix + ".waterAuroraLight", &paramWaterAuroraLight_, paramWaterAuroraLight_, "Water: Aurora Light", 0.0f, 2.5f, 0.01f);

    registerFloat(registry, prefix + ".icebergCount", &paramIcebergCount_, paramIcebergCount_, "Icebergs: Count", 0.0f, 14.0f, 1.0f);
    registerFloat(registry, prefix + ".icebergScale", &paramIcebergScale_, paramIcebergScale_, "Icebergs: Scale", 0.2f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".icebergSpread", &paramIcebergSpread_, paramIcebergSpread_, "Icebergs: Spread", 100.0f, 5000.0f, 10.0f);
    registerFloat(registry, prefix + ".icebergRimLight", &paramIcebergRimLight_, paramIcebergRimLight_, "Icebergs: Edge Lines", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".icebergBreakup", &paramIcebergBreakup_, paramIcebergBreakup_, "Icebergs: Breakup", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".icebergBreakupSpeed", &paramIcebergBreakupSpeed_, paramIcebergBreakupSpeed_, "Icebergs: Breakup Speed", 0.0f, 0.16f, 0.001f);

    registerFloat(registry, prefix + ".auroraWidth", &paramAuroraWidth_, paramAuroraWidth_, "Aurora: Arc Length", 300.0f, 8000.0f, 10.0f);
    registerFloat(registry, prefix + ".auroraBaseY", &paramAuroraBaseY_, paramAuroraBaseY_, "Aurora: Base Y", -100.0f, 700.0f, 1.0f);
    registerFloat(registry, prefix + ".auroraHeight", &paramAuroraHeight_, paramAuroraHeight_, "Aurora: Height", 100.0f, 900.0f, 5.0f);
    registerFloat(registry, prefix + ".auroraDepthNear", &paramAuroraDepthNear_, paramAuroraDepthNear_, "Aurora: Inner Radius", -1800.0f, 100.0f, 10.0f);
    registerFloat(registry, prefix + ".auroraDepthFar", &paramAuroraDepthFar_, paramAuroraDepthFar_, "Aurora: Outer Radius", -2600.0f, -300.0f, 10.0f);
    registerFloat(registry, prefix + ".auroraGlow", &paramAuroraGlow_, paramAuroraGlow_, "Aurora: Glow", 0.0f, 3.5f, 0.01f);
    registerFloat(registry, prefix + ".auroraBloom", &paramAuroraBloom_, paramAuroraBloom_, "Aurora: Bloom", 0.0f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".auroraFoldStrength", &paramAuroraFoldStrength_, paramAuroraFoldStrength_, "Aurora: Fold Strength", 0.0f, 2.5f, 0.01f);
    registerFloat(registry, prefix + ".auroraRayDensity", &paramAuroraRayDensity_, paramAuroraRayDensity_, "Aurora: Ray Density", 0.0f, 1.8f, 0.01f);
    registerFloat(registry, prefix + ".auroraCurtainCount", &paramAuroraCurtainCount_, paramAuroraCurtainCount_, "Aurora: Curtain Count", 1.0f, 6.0f, 1.0f);
    registerFloat(registry, prefix + ".audioAmount", &paramAudioAmount_, paramAudioAmount_, "Signal: Amount", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".audioSmoothing", &paramAudioSmoothing_, paramAudioSmoothing_, "Signal: Smoothing", 0.0f, 0.98f, 0.01f);

    meta = {};
    meta.group = "3D Arctic Aurora";
    meta.label = "Action: Reseed";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);
    registerFloat(registry, prefix + ".seed", &paramSeed_, paramSeed_, "Scene: Seed", 0.0f, 99999999.0f, 1.0f);

    registerFloat(registry, prefix + ".skyTopR", &paramSkyTopR_, paramSkyTopR_, "Sky: Top Color R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".skyTopG", &paramSkyTopG_, paramSkyTopG_, "Sky: Top Color G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".skyTopB", &paramSkyTopB_, paramSkyTopB_, "Sky: Top Color B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".skyHorizonR", &paramSkyHorizonR_, paramSkyHorizonR_, "Sky: Horizon Color R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".skyHorizonG", &paramSkyHorizonG_, paramSkyHorizonG_, "Sky: Horizon Color G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".skyHorizonB", &paramSkyHorizonB_, paramSkyHorizonB_, "Sky: Horizon Color B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".waterR", &paramWaterR_, paramWaterR_, "Water: Color R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".waterG", &paramWaterG_, paramWaterG_, "Water: Color G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".waterB", &paramWaterB_, paramWaterB_, "Water: Color B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".auroraR", &paramAuroraR_, paramAuroraR_, "Aurora: Color R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".auroraG", &paramAuroraG_, paramAuroraG_, "Aurora: Color G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".auroraB", &paramAuroraB_, paramAuroraB_, "Aurora: Color B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".aurora2R", &paramAurora2R_, paramAurora2R_, "Aurora: Secondary Color R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".aurora2G", &paramAurora2G_, paramAurora2G_, "Aurora: Secondary Color G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".aurora2B", &paramAurora2B_, paramAurora2B_, "Aurora: Secondary Color B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".iceRimR", &paramIceRimR_, paramIceRimR_, "Ice: Rim Color R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".iceRimG", &paramIceRimG_, paramIceRimG_, "Ice: Rim Color G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".iceRimB", &paramIceRimB_, paramIceRimB_, "Ice: Rim Color B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".iceAccentR", &paramIceAccentR_, paramIceAccentR_, "Ice: Accent Color R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".iceAccentG", &paramIceAccentG_, paramIceAccentG_, "Ice: Accent Color G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".iceAccentB", &paramIceAccentB_, paramIceAccentB_, "Ice: Accent Color B", 0.0f, 1.5f, 0.01f);

    resetLayout();
}

void ArcticAuroraSceneLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) {
        return;
    }

    clampParams();
    const float dt = ofClamp(params.dt, 0.0f, kMaxDt);
    sceneTime_ += dt;
    updateAudioState(dt);

    const std::uint32_t desiredSeed = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    const int desiredCount = static_cast<int>(std::round(paramIcebergCount_));
    if (desiredSeed != seedState_ || desiredCount != icebergCountState_ || paramReseedRequested_) {
        resetLayout();
        paramReseedRequested_ = false;
    }
}

void ArcticAuroraSceneLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f) {
        return;
    }

    const float alpha = ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);

    ofPushStyle();
    drawSky(params, alpha);

    params.camera.begin();
    ofEnableDepthTest();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);

    ofPushMatrix();
    ofTranslate(0.0f, paramSceneOffsetY_, paramSceneOffsetZ_);
    ofScale(paramSceneScale_, paramSceneScale_, paramSceneScale_);

    drawAuroraVolume(params, alpha);
    drawWaterPlane(params, alpha);
    drawWaterReflection(params, alpha);
    drawWaterHorizonMist(alpha);

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    glDepthMask(GL_TRUE);
    std::vector<const Iceberg*> icebergDrawOrder;
    icebergDrawOrder.reserve(icebergs_.size());
    for (const auto& iceberg : icebergs_) {
        icebergDrawOrder.push_back(&iceberg);
    }
    std::sort(icebergDrawOrder.begin(), icebergDrawOrder.end(), [this](const Iceberg* a, const Iceberg* b) {
        return icebergWorldPosition(*a).z < icebergWorldPosition(*b).z;
    });
    for (const auto* iceberg : icebergDrawOrder) {
        drawIceberg(*iceberg, alpha);
    }

    ofPopMatrix();
    glDepthMask(GL_TRUE);
    ofDisableDepthTest();
    params.camera.end();
    ofPopStyle();
}

void ArcticAuroraSceneLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

void ArcticAuroraSceneLayer::registerFloat(ParameterRegistry& registry,
                                           const std::string& id,
                                           float* target,
                                           float initial,
                                           const std::string& label,
                                           float min,
                                           float max,
                                           float step,
                                           const std::string& units,
                                           const std::string& description) {
    ParameterRegistry::Descriptor meta;
    meta.group = "3D Arctic Aurora";
    meta.label = label;
    meta.range.min = min;
    meta.range.max = max;
    meta.range.step = step;
    meta.units = units;
    meta.description = description;
    registry.addFloat(id, target, initial, meta);
}

void ArcticAuroraSceneLayer::readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b) {
    if (!defaults.contains(key) || !defaults[key].is_array() || defaults[key].size() < 3) {
        return;
    }
    r = defaults[key][0].get<float>();
    g = defaults[key][1].get<float>();
    b = defaults[key][2].get<float>();
}

void ArcticAuroraSceneLayer::clampParams() {
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramSceneScale_ = ofClamp(paramSceneScale_, 0.25f, 2.0f);
    paramSceneOffsetY_ = ofClamp(paramSceneOffsetY_, -500.0f, 500.0f);
    paramSceneOffsetZ_ = ofClamp(paramSceneOffsetZ_, -1000.0f, 1000.0f);
    paramWaterWidth_ = ofClamp(paramWaterWidth_, 300.0f, 6000.0f);
    paramWaterNearZ_ = ofClamp(paramWaterNearZ_, -300.0f, 700.0f);
    paramWaterFarZ_ = ofClamp(paramWaterFarZ_, -2600.0f, -300.0f);
    if (paramWaterFarZ_ > paramWaterNearZ_ - 200.0f) {
        paramWaterFarZ_ = paramWaterNearZ_ - 200.0f;
    }
    paramWaterLevel_ = ofClamp(paramWaterLevel_, -400.0f, 100.0f);
    paramWaterWaveIdle_ = ofClamp(paramWaterWaveIdle_, 0.0f, 28.0f);
    paramWaterHighlight_ = ofClamp(paramWaterHighlight_, 0.0f, 1.5f);
    paramWaterReflection_ = ofClamp(paramWaterReflection_, 0.0f, 1.8f);
    paramWaterHorizonFog_ = ofClamp(paramWaterHorizonFog_, 0.0f, 1.5f);
    paramWaterAlpha_ = ofClamp(paramWaterAlpha_, 0.0f, 1.0f);
    paramWaterBrightness_ = ofClamp(paramWaterBrightness_, 0.2f, 2.5f);
    paramWaterTranslucency_ = ofClamp(paramWaterTranslucency_, 0.0f, 1.5f);
    paramWaterCurvature_ = ofClamp(paramWaterCurvature_, 0.0f, 1.0f);
    paramWaterHemisphereDepth_ = ofClamp(paramWaterHemisphereDepth_, 0.0f, 6000.0f);
    paramWaterNoiseAmount_ = ofClamp(paramWaterNoiseAmount_, 0.0f, 18.0f);
    paramWaterNoiseScale_ = ofClamp(paramWaterNoiseScale_, 0.0005f, 0.012f);
    paramWaterRippleAmount_ = ofClamp(paramWaterRippleAmount_, 0.0f, 18.0f);
    paramWaterRippleRadius_ = ofClamp(paramWaterRippleRadius_, 80.0f, 1000.0f);
    paramWaterAuroraLight_ = ofClamp(paramWaterAuroraLight_, 0.0f, 2.5f);
    paramIcebergCount_ = std::round(ofClamp(paramIcebergCount_, 0.0f, 14.0f));
    paramIcebergScale_ = ofClamp(paramIcebergScale_, 0.2f, 2.0f);
    paramIcebergSpread_ = ofClamp(paramIcebergSpread_, 100.0f, 5000.0f);
    paramIcebergRimLight_ = ofClamp(paramIcebergRimLight_, 0.0f, 1.0f);
    paramIcebergBreakup_ = ofClamp(paramIcebergBreakup_, 0.0f, 1.0f);
    paramIcebergBreakupSpeed_ = ofClamp(paramIcebergBreakupSpeed_, 0.0f, 0.16f);
    paramAuroraWidth_ = ofClamp(paramAuroraWidth_, 300.0f, 8000.0f);
    paramAuroraBaseY_ = ofClamp(paramAuroraBaseY_, -100.0f, 700.0f);
    paramAuroraHeight_ = ofClamp(paramAuroraHeight_, 100.0f, 900.0f);
    paramAuroraDepthNear_ = ofClamp(paramAuroraDepthNear_, -1800.0f, 100.0f);
    paramAuroraDepthFar_ = ofClamp(paramAuroraDepthFar_, -2600.0f, -300.0f);
    if (paramAuroraDepthFar_ > paramAuroraDepthNear_ - 80.0f) {
        paramAuroraDepthFar_ = paramAuroraDepthNear_ - 80.0f;
    }
    paramAuroraGlow_ = ofClamp(paramAuroraGlow_, 0.0f, 3.5f);
    paramAuroraBloom_ = ofClamp(paramAuroraBloom_, 0.0f, 4.0f);
    paramAuroraFoldStrength_ = ofClamp(paramAuroraFoldStrength_, 0.0f, 2.5f);
    paramAuroraRayDensity_ = ofClamp(paramAuroraRayDensity_, 0.0f, 1.8f);
    paramAuroraCurtainCount_ = std::round(ofClamp(paramAuroraCurtainCount_, 1.0f, 6.0f));
    paramAudioAmount_ = ofClamp(paramAudioAmount_, 0.0f, 3.0f);
    paramAudioSmoothing_ = ofClamp(paramAudioSmoothing_, 0.0f, 0.98f);
}

void ArcticAuroraSceneLayer::resetLayout() {
    clampParams();
    seedState_ = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    icebergCountState_ = static_cast<int>(std::round(paramIcebergCount_));

    std::mt19937 rng(seedState_ == 0 ? 20260621u : seedState_);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::uniform_real_distribution<float> signedUnit(-1.0f, 1.0f);

    icebergs_.clear();
    icebergs_.reserve(static_cast<std::size_t>(std::max(0, icebergCountState_)));
    const float diskRadius = std::max(180.0f, paramWaterWidth_ * 0.45f);
    for (int i = 0; i < icebergCountState_; ++i) {
        const float lane = icebergCountState_ > 0
            ? static_cast<float>(i) / static_cast<float>(icebergCountState_)
            : 0.0f;
        const float angle = lane * TWO_PI + randomRange(rng, -0.36f, 0.36f);
        const float radiusLimit = std::min(diskRadius * 0.82f, paramIcebergSpread_ * 0.50f);
        const float radius = randomRange(rng, diskRadius * 0.16f, std::max(diskRadius * 0.24f, radiusLimit));
        const glm::vec3 position = polarPoint(angle, radius, ofLerp(-2.0f, 2.0f, unit(rng)));
        Iceberg iceberg;
        iceberg.position = position;
        iceberg.orbitRadius = radius;
        iceberg.orbitAngle = angle;
        iceberg.driftSpeed = randomRange(rng, -0.018f, 0.018f);
        iceberg.bobPhase = unit(rng) * TWO_PI;
        iceberg.bobAmount = ofLerp(1.6f, 4.8f, unit(rng));
        iceberg.scale = ofLerp(0.58f, 1.45f, unit(rng));
        iceberg.yaw = ofRadToDeg(-angle) + 90.0f + signedUnit(rng) * 34.0f;
        iceberg.seed = unit(rng) * 1000.0f;
        const glm::vec3 dimensions(ofLerp(118.0f, 220.0f, unit(rng)),
                                   ofLerp(112.0f, 236.0f, unit(rng)),
                                   ofLerp(108.0f, 215.0f, unit(rng)));
        buildIcebergMeshes(iceberg, dimensions, rng);
        icebergs_.push_back(iceberg);
    }

    std::sort(icebergs_.begin(), icebergs_.end(), [](const Iceberg& a, const Iceberg& b) {
        return a.position.z < b.position.z;
    });

    stars_.clear();
    stars_.reserve(260);
    for (int i = 0; i < 260; ++i) {
        Star star;
        star.position = glm::vec2(unit(rng), ofLerp(0.02f, 0.98f, unit(rng)));
        star.size = ofLerp(0.45f, 2.2f, unit(rng));
        star.alpha = ofLerp(0.14f, 0.82f, unit(rng));
        star.twinkle = unit(rng) * 100.0f;
        stars_.push_back(star);
    }
}

void ArcticAuroraSceneLayer::updateAudioState(float dt) {
    const auto snapshot = AudioAnalysisBus::instance().snapshot();
    hasAudio_ = snapshot.valid;
    auto boostedAudio = [](float value, float gain, float curve) {
        return std::pow(ofClamp(value * gain, 0.0f, 1.0f), curve);
    };

    const float targetLevel = snapshot.valid ? boostedAudio(snapshot.level, 3.2f, 0.68f) : 0.0f;
    const float targetPeak = snapshot.valid ? boostedAudio(std::max(snapshot.peak, snapshot.level), 2.6f, 0.62f) : 0.0f;
    const float targetBass = snapshot.valid ? boostedAudio(snapshot.bass, 2.5f, 0.70f) : 0.0f;
    const float targetMids = snapshot.valid ? boostedAudio(snapshot.mids, 2.9f, 0.66f) : 0.0f;
    const float targetHighs = snapshot.valid ? boostedAudio(snapshot.highs, 3.1f, 0.64f) : 0.0f;
    const float follow = ofClamp(followAmount(paramAudioSmoothing_) * dt * 24.0f, 0.0f, 1.0f);

    level_ = ofLerp(level_, targetLevel, follow);
    peak_ = ofLerp(peak_, targetPeak, follow);
    bass_ = ofLerp(bass_, targetBass, follow);
    mids_ = ofLerp(mids_, targetMids, follow);
    highs_ = ofLerp(highs_, targetHighs, follow);

    const float audioLift = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float targetEnergy = 0.18f + audioLift *
        ofClamp(level_ * 0.95f + bass_ * 0.50f + mids_ * 0.38f + peak_ * 0.44f, 0.0f, 1.65f);
    auroraEnergy_ = ofLerp(auroraEnergy_, targetEnergy, ofClamp(dt * 8.0f, 0.0f, 1.0f));

    const float targetPulse = hasAudio_ ? audioLift * ofClamp(peak_ * 1.18f + level_ * 0.42f + highs_ * 0.36f, 0.0f, 1.55f) : 0.0f;
    const float pulseFollow = targetPulse > auroraPulse_ ? dt * 18.0f : dt * 5.5f;
    auroraPulse_ = ofLerp(auroraPulse_, targetPulse, ofClamp(pulseFollow, 0.0f, 1.0f));

    if (energyField_.size() != kAuroraEnergySamples) {
        energyField_.assign(kAuroraEnergySamples, 0.18f);
        targetEnergyField_.assign(kAuroraEnergySamples, 0.18f);
    }

    const auto& waveform = snapshot.waveform;
    const bool useWaveform = hasAudio_ && !waveform.empty();
    const float fieldFollow = ofClamp(dt * (useWaveform ? 9.0f : 2.0f), 0.0f, 1.0f);
    for (int i = 0; i < kAuroraEnergySamples; ++i) {
        float waveEnergy = 0.0f;
        if (useWaveform) {
            const std::size_t start = static_cast<std::size_t>((static_cast<long long>(i) * waveform.size()) / kAuroraEnergySamples);
            const std::size_t end = std::max(start + 1,
                                             static_cast<std::size_t>((static_cast<long long>(i + 1) * waveform.size()) / kAuroraEnergySamples));
            float sum = 0.0f;
            int count = 0;
            for (std::size_t sample = start; sample < std::min(end, waveform.size()); ++sample) {
                sum += std::abs(waveform[sample]);
                ++count;
            }
            waveEnergy = count > 0 ? ofClamp((sum / static_cast<float>(count)) * 6.0f, 0.0f, 1.0f) : 0.0f;
        }

        const float u = static_cast<float>(i) / static_cast<float>(kAuroraEnergySamples - 1);
        const float slowDrift = ofNoise(u * 2.3f + 4.0f, sceneTime_ * 0.11f) * 0.24f;
        const float bandEnergy = level_ * 0.42f + mids_ * 0.30f + highs_ * 0.20f + peak_ * 0.24f;
        targetEnergyField_[i] = 0.18f + slowDrift + audioLift * ofClamp(waveEnergy * 1.05f + bandEnergy, 0.0f, 1.85f);
        energyField_[i] = ofLerp(energyField_[i], targetEnergyField_[i], fieldFollow);
    }
}

void ArcticAuroraSceneLayer::drawSky(const LayerDrawParams& params, float alpha) const {
    const float width = static_cast<float>(std::max(1, params.viewport.x));
    const float height = static_cast<float>(std::max(1, params.viewport.y));

    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);

    ofMesh sky;
    sky.setMode(OF_PRIMITIVE_TRIANGLES);
    const ofFloatColor top = colorFrom(paramSkyTopR_, paramSkyTopG_, paramSkyTopB_, alpha);
    const ofFloatColor horizon = colorFrom(paramSkyHorizonR_, paramSkyHorizonG_, paramSkyHorizonB_, alpha);
    addQuad(sky,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(width, 0.0f, 0.0f),
            glm::vec3(width, height, 0.0f),
            glm::vec3(0.0f, height, 0.0f),
            horizon);
    if (sky.getNumColors() >= 6) {
        sky.setColor(0, top);
        sky.setColor(1, top);
        sky.setColor(3, top);
    }
    sky.draw();

    ofEnableBlendMode(OF_BLENDMODE_ADD);
    for (const auto& star : stars_) {
        const float twinkle = 0.72f + 0.28f * std::sin(params.time * 0.7f + star.twinkle);
        ofSetColor(160, 230, 255, static_cast<int>(255.0f * alpha * star.alpha * twinkle));
        ofDrawCircle(star.position.x * width, star.position.y * height, star.size);
    }
    ofPopView();
}

void ArcticAuroraSceneLayer::drawAuroraVolume(const LayerDrawParams& params, float alpha) const {
    const int curtains = std::max(1, static_cast<int>(std::round(paramAuroraCurtainCount_)));
    const int cols = 128;
    const int rows = 14;
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float energy = ofClamp(auroraEnergy_, 0.14f, 2.4f);
    const float bloom = ofClamp(paramAuroraBloom_, 0.0f, 4.0f);
    const float glow = alpha * paramAuroraGlow_;
    const float foldStrength = paramAuroraFoldStrength_;
    const float rayDensity = paramAuroraRayDensity_;
    const float time = sceneTime_ + params.time * 0.025f;
    const float audioDrive = hasAudio_
        ? ofClamp(paramAudioAmount_ * (level_ * 0.72f + bass_ * 0.30f + mids_ * 0.42f + highs_ * 0.24f + peak_ * 0.50f),
                  0.0f,
                  2.8f)
        : 0.0f;
    const float pulseDrive = hasAudio_ ? ofClamp(auroraPulse_, 0.0f, 2.6f) : 0.0f;
    const float brightnessDrive = 1.0f + audioDrive * 0.62f + pulseDrive * 0.48f;
    const float foldDrive = 1.0f + audioDrive * 0.38f + pulseDrive * 0.22f;
    const float innerRadius = radiusFromDepthParam(paramAuroraDepthNear_);
    const float outerRadius = std::max(innerRadius + 80.0f, radiusFromDepthParam(paramAuroraDepthFar_));
    const float midRadius = (innerRadius + outerRadius) * 0.5f;
    const float requestedArc = paramAuroraWidth_ / std::max(240.0f, midRadius);
    const float widthDrive = hasAudio_
        ? ofClamp(paramAudioAmount_ * (level_ * 0.38f + mids_ * 0.42f + highs_ * 0.20f + peak_ * 0.32f),
                  0.0f,
                  1.8f)
        : 0.0f;
    const float spinDrive = hasAudio_
        ? ofClamp(paramAudioAmount_ * (level_ * 0.030f + mids_ * 0.020f + highs_ * 0.018f + peak_ * 0.034f),
                  0.0f,
                  0.075f)
        : 0.0f;
    const float baseArc = ofClamp(requestedArc * (1.0f + widthDrive * 0.34f + pulseDrive * 0.08f),
                                  PI * 0.38f,
                                  TWO_PI * 0.36f);
    const float baseWidth = ofClamp(paramAuroraWidth_ * (1.0f + widthDrive * 0.24f + pulseDrive * 0.045f),
                                    700.0f,
                                    std::max(900.0f, paramWaterWidth_ * 1.35f));
    const float depthSpan = std::max(80.0f, outerRadius - innerRadius);
    const glm::vec2 windDir = glm::normalize(glm::vec2(0.86f, -0.51f));
    const glm::vec2 crossWind(-windDir.y, windDir.x);
    const float windTravel = time * (92.0f + audioDrive * 11.0f + pulseDrive * 5.0f);

    const ofFloatColor primary = colorFrom(paramAuroraR_, paramAuroraG_, paramAuroraB_, 1.0f);
    const ofFloatColor secondary = colorFrom(paramAurora2R_, paramAurora2G_, paramAurora2B_, 1.0f);

    auto sampleUAt = [](float u) {
        return ofClamp(u, 0.0f, 1.0f);
    };

    auto edgeFadeAt = [](float u, float scale) {
        return smootherStep(std::min(u, 1.0f - u) * scale);
    };

    auto fieldAt = [this, sampleUAt](float u) {
        if (energyField_.empty()) {
            return auroraEnergy_;
        }

        const float index = sampleUAt(u) * static_cast<float>(energyField_.size() - 1);
        const int i0 = static_cast<int>(std::floor(index));
        const int i1 = std::min(i0 + 1, static_cast<int>(energyField_.size() - 1));
        return ofClamp(ofLerp(energyField_[i0], energyField_[i1], index - static_cast<float>(i0)), 0.0f, 2.8f);
    };

    auto wrap01 = [](float value) {
        return value - std::floor(value);
    };

    auto sceneXZAt = [&](float theta, float radius) {
        const float x = ((theta + HALF_PI) / std::max(0.001f, baseArc)) * baseWidth;
        return glm::vec2(x, -radius);
    };

    auto scenePointAt = [&](float theta, float radius, float y) {
        const glm::vec2 xz = sceneXZAt(theta, radius);
        return glm::vec3(xz.x, y, xz.y);
    };

    auto sceneEnergyAt = [&](const glm::vec2& xz) {
        const float advectedAlong = glm::dot(xz, windDir) - windTravel;
        const float advectedCross = glm::dot(xz, crossWind);
        const float fieldU = advectedAlong / std::max(520.0f, baseWidth * 0.68f) +
            advectedCross * 0.00010f;
        return fieldAt(wrap01(fieldU));
    };

    auto sharedSceneFlowAt = [&](float theta, float radiusT, float verticalT) {
        const float baseRadius = ofLerp(innerRadius, outerRadius, radiusT);
        const glm::vec2 xz = sceneXZAt(theta, baseRadius);
        const glm::vec2 advected = xz - windDir * windTravel;
        const float along = glm::dot(advected, windDir);
        const float across = glm::dot(advected, crossWind);
        const float flowTime = time * (0.055f + audioDrive * 0.004f) + pulseDrive * 0.010f;
        const float sceneEnergy = sceneEnergyAt(xz);
        const float path = along * 0.0018f + across * 0.00036f + radiusT * 1.15f;
        const float vertical = ofClamp(verticalT, 0.0f, 1.0f);
        const float streamA = signedNoise(along * 0.0011f + flowTime * 0.34f,
                                          across * 0.0017f + radiusT * 2.2f,
                                          vertical * 0.42f + flowTime * 0.58f);
        const float streamB = signedNoise(along * 0.0023f - flowTime * 0.26f + 19.0f,
                                          across * 0.0010f + vertical * 1.70f,
                                          radiusT * 0.72f + flowTime * 0.42f);
        const float streamC = std::sin(path * 2.7f + vertical * 1.8f + flowTime * 1.12f);
        const float sharedField = streamA * 0.50f + streamB * 0.24f + streamC * 0.26f;
        const float shearField = streamB * 0.54f - streamA * 0.20f +
            std::sin((path + vertical * 0.55f) * 3.8f - flowTime * 0.80f) * 0.26f;
        const float radiusWeight = ofLerp(0.82f, 1.14f, smootherStep(radiusT));
        const float verticalWeight = ofLerp(0.48f, 1.12f, smootherStep(vertical));
        const float edgeWeight = std::pow(std::abs(vertical * 2.0f - 1.0f), 1.18f);
        const float flowGain = (1.0f + foldStrength * 0.36f) *
            (1.0f + audioDrive * 0.09f + pulseDrive * 0.06f) *
            (0.90f + sceneEnergy * 0.18f);

        AuroraFlow flow;
        const float crossOffset = (sharedField * 96.0f + shearField * 42.0f) *
            flowGain * radiusWeight * ofLerp(0.58f, 1.0f, vertical);
        const float windOffset = (streamB * 22.0f + streamC * 16.0f) *
            flowGain * verticalWeight;
        flow.planarOffset = crossWind * crossOffset + windDir * windOffset;
        flow.verticalOffset = (shearField * 30.0f + sharedField * 16.0f) *
            flowGain * ofLerp(0.18f, 1.0f, edgeWeight);
        flow.energy = ofClamp(0.88f + std::abs(sharedField) * 0.24f + std::abs(shearField) * 0.11f +
                                  sceneEnergy * 0.10f + audioDrive * 0.040f + pulseDrive * 0.060f,
                              0.72f,
                              1.55f);
        return flow;
    };

    auto auroraColor = [&](float mix, float colorGain, float vertexAlpha) {
        ofFloatColor color = primary.getLerped(secondary, ofClamp(mix, 0.0f, 1.0f));
        color.r = ofClamp(color.r * colorGain, 0.0f, 1.5f);
        color.g = ofClamp(color.g * colorGain, 0.0f, 1.5f);
        color.b = ofClamp(color.b * colorGain, 0.0f, 1.5f);
        color.a = ofClamp(vertexAlpha, 0.0f, 1.0f);
        return color;
    };

    ofEnableBlendMode(OF_BLENDMODE_ADD);
    glDepthMask(GL_FALSE);

    for (int c = 0; c < curtains; ++c) {
        const float layerT = curtains > 1 ? static_cast<float>(c) / static_cast<float>(curtains - 1) : 0.0f;
        const float stackT = layerT - 0.5f;
        const float baseRadius = ofClamp(midRadius + stackT * depthSpan * 0.62f,
                                         innerRadius,
                                         outerRadius);
        const float height = paramAuroraHeight_ * ofLerp(1.08f, 0.86f, layerT);
        const float phase = time * (0.18f + layerT * 0.055f) + layerT * 9.2f;
        const float layerStrength = ofLerp(1.18f, 0.72f, layerT);
        const float direction = (c % 2 == 0) ? 1.0f : -1.0f;
        const float layerRate = direction * ofLerp(0.060f, 0.026f, layerT) +
            signedNoise(layerT * 3.2f + 0.5f, 11.0f, 0.0f) * 0.012f;
        const float arcBreath = 0.84f + 0.24f * ofNoise(layerT * 5.0f + 2.0f, time * 0.08f);
        const float layerArc = ofClamp(baseArc * arcBreath * (1.0f + widthDrive * 0.18f),
                                       PI * 0.28f,
                                       TWO_PI * 0.38f);
        const float layerCenter = -HALF_PI +
            stackT * baseArc * 0.34f +
            sceneTime_ * (layerRate * 0.16f + direction * spinDrive * 0.35f) +
            std::sin(time * (0.045f + layerT * 0.015f) + layerT * 6.0f) * 0.085f;

        auto subBandCenterAt = [&](float subBandT) {
            const float bandIndex = std::round(subBandT * 5.0f);
            const float subDirection = (std::fmod(static_cast<float>(c) + bandIndex, 2.0f) < 1.0f) ? 1.0f : -1.0f;
            const float subRate = subDirection * (0.018f + subBandT * 0.036f) +
                signedNoise(layerT * 4.1f + subBandT * 5.7f, 19.0f, 0.0f) * 0.010f;
            const float audioSpin = subDirection * spinDrive * (0.22f + subBandT * 0.70f);
            const float slowOrbit = std::sin(time * (0.055f + subBandT * 0.032f) +
                                             layerT * 5.3f +
                                             subBandT * 8.7f) *
                (0.035f + subBandT * 0.050f);
            return layerCenter +
                (subBandT - 0.50f) * 0.070f +
                sceneTime_ * (subRate * 0.28f + audioSpin * 0.22f) +
                slowOrbit;
        };

        auto subBandArcAt = [&](float subBandT) {
            const float widthNoise = signedNoise(layerT * 3.7f + subBandT * 2.8f,
                                                 time * 0.055f,
                                                 23.0f);
            return ofClamp(layerArc * (1.0f + widthNoise * 0.075f + widthDrive * subBandT * 0.035f),
                           PI * 0.24f,
                           TWO_PI * 0.39f);
        };

        auto verticalShapeAt = [&](float theta, float subBandT, float v) {
            const float radiusT = ofClamp(layerT + (subBandT - 0.5f) * 0.18f, 0.0f, 1.0f);
            const float shapeRadius = ofLerp(innerRadius, outerRadius, radiusT);
            const glm::vec2 xz = sceneXZAt(theta, shapeRadius);
            const glm::vec2 advected = xz - windDir * windTravel;
            const float along = glm::dot(advected, windDir);
            const float across = glm::dot(advected, crossWind);
            const float topWeight = std::pow(ofClamp(v, 0.0f, 1.0f), 1.45f);
            const float midWeight = std::pow(std::max(0.0f, std::sin(v * PI)), 0.70f);
            const float lowerWeight = std::pow(ofClamp(1.0f - v, 0.0f, 1.0f), 1.55f);
            const float tipWeight = std::pow(ofClamp(v, 0.0f, 1.0f), 2.35f);
            const float sceneEnergy = sceneEnergyAt(xz);
            const float broad = signedNoise(along * 0.0010f + subBandT * 2.4f,
                                            across * 0.0014f - layerT * 1.7f,
                                            time * 0.10f + subBandT * 0.65f);
            const float crest = std::sin(along * 0.0035f + across * 0.0007f + time * 0.30f + layerT * 3.8f + subBandT * 5.4f);
            const float fineLift = signedNoise(along * 0.0028f - subBandT * 1.6f,
                                               across * 0.0031f + layerT * 2.1f,
                                               time * 0.24f + 31.0f);
            const float audioColumn = hasAudio_
                ? ofClamp(audioDrive * 0.36f + pulseDrive * 0.22f + sceneEnergy * 0.30f, 0.0f, 1.55f)
                : 0.54f;
            const float audioNorm = smootherStep(ofClamp(audioColumn, 0.0f, 1.0f));
            const float quietTipTaper = (audioNorm - 0.55f) * (0.16f + tipWeight * 0.72f);
            const float columnLift = ofClamp(audioColumn - 0.45f, -0.45f, 0.92f) *
                (midWeight * 0.10f + tipWeight * 0.30f);
            const float heightScale = ofClamp(1.0f +
                                                  (broad * 0.24f + crest * 0.13f) *
                                                      (0.72f + foldStrength * 0.26f) *
                                                      (0.72f + topWeight * 0.52f) +
                                                  quietTipTaper +
                                                  columnLift,
                                              0.48f,
                                              1.72f);
            const float yOffset = (broad * 86.0f + crest * 42.0f) * topWeight +
                fineLift * (34.0f + audioColumn * 24.0f) * midWeight +
                fineLift * 42.0f * tipWeight -
                (1.0f - audioNorm) * 68.0f * tipWeight -
                std::abs(fineLift) * 24.0f * lowerWeight;
            return glm::vec2(heightScale, yOffset);
        };

        ofMesh atmosphere;
        atmosphere.setMode(OF_PRIMITIVE_TRIANGLES);
        const int atmosphereCols = 76;
        const int atmosphereRows = 5;
        const float atmosphereSubBand = 1.18f;
        const float atmosphereCenter = subBandCenterAt(atmosphereSubBand);
        const float atmosphereArc = subBandArcAt(atmosphereSubBand) * 1.18f;
        for (int y = 0; y <= atmosphereRows; ++y) {
            const float v = static_cast<float>(y) / static_cast<float>(atmosphereRows);
            const float vertical = std::pow(std::max(0.0f, std::sin(v * PI)), 0.55f);
            for (int x = 0; x <= atmosphereCols; ++x) {
                const float u = static_cast<float>(x) / static_cast<float>(atmosphereCols);
                const float sampleU = sampleUAt(u);
                const float baseTheta = atmosphereCenter + (u - 0.5f) * atmosphereArc;
                const AuroraFlow flow = sharedSceneFlowAt(baseTheta, layerT, v);
                const glm::vec2 yShape = verticalShapeAt(baseTheta, atmosphereSubBand, v);
                const float theta = baseTheta;
                const float localEnergy = sceneEnergyAt(sceneXZAt(baseTheta, baseRadius));
                const float radius = baseRadius +
                    std::sin((sampleU * 2.0f + 0.12f + layerT * 0.4f) * TWO_PI + time * 0.32f) * 24.0f +
                    signedNoise(sampleU * 1.8f, v * 1.1f + layerT, time * 0.18f) * 34.0f;
                const float yPos = paramAuroraBaseY_ - bloom * 54.0f +
                    flow.verticalOffset * 0.46f +
                    yShape.y * 0.34f +
                    v * (height * 0.72f * yShape.x + bloom * 160.0f);
                const float edge = edgeFadeAt(u, 3.7f);
                const float atmosphereAlpha = glow * bloom * layerStrength *
                    (0.018f + energy * 0.017f + pulseDrive * 0.026f + audioDrive * 0.010f) *
                    brightnessDrive * vertical * edge * flow.energy * (0.68f + localEnergy * 0.40f);
                glm::vec3 point = scenePointAt(theta, radius, yPos);
                point.x += flow.planarOffset.x * 0.64f;
                point.z += flow.planarOffset.y * 0.64f;
                atmosphere.addVertex(point);
                atmosphere.addColor(auroraColor(v * 0.28f + localEnergy * 0.12f + layerT * 0.10f,
                                                1.16f,
                                                atmosphereAlpha));
            }
        }
        for (int y = 0; y < atmosphereRows; ++y) {
            for (int x = 0; x < atmosphereCols; ++x) {
                const int i00 = y * (atmosphereCols + 1) + x;
                const int i10 = i00 + 1;
                const int i01 = (y + 1) * (atmosphereCols + 1) + x;
                const int i11 = i01 + 1;
                atmosphere.addIndex(i00);
                atmosphere.addIndex(i10);
                atmosphere.addIndex(i11);
                atmosphere.addIndex(i00);
                atmosphere.addIndex(i11);
                atmosphere.addIndex(i01);
            }
        }
        atmosphere.draw();

        auto curtainPoint = [&](float u, float v, float expandRadius, float expandY, float looseness, float subBandT) {
            const float sampleU = sampleUAt(u);
            const float subCenter = subBandCenterAt(subBandT);
            const float subArc = subBandArcAt(subBandT);
            const float baseTheta = subCenter + (u - 0.5f) * subArc;
            const float localEnergy = sceneEnergyAt(sceneXZAt(baseTheta, baseRadius));
            const float slow = signedNoise(sampleU * 2.4f + layerT * 3.3f + subBandT * 1.9f,
                                           v * 1.2f + 0.7f,
                                           phase);
            const float fine = signedNoise(sampleU * 8.8f + layerT * 5.1f + subBandT * 3.4f,
                                           v * 4.2f,
                                           phase * 1.35f);
            const float fold = (slow * 38.0f + fine * 11.0f) *
                (0.45f + foldStrength) *
                (1.0f + audio * (mids_ * 1.12f + peak_ * 0.34f + localEnergy * 0.48f)) *
                foldDrive;
            const float hangingWave = std::sin((sampleU * 4.6f + layerT * 0.9f) * TWO_PI + phase * 1.4f);
            const float verticalDrift = (hangingWave * 26.0f + fine * 16.0f) *
                (1.0f - v) *
                (0.55f + foldStrength * 0.45f) *
                (1.0f + audioDrive * 0.22f + highs_ * audio * 0.18f);
            const AuroraFlow flow = sharedSceneFlowAt(baseTheta, layerT, v);
            const glm::vec2 yShape = verticalShapeAt(baseTheta, subBandT, v);
            const float theta = baseTheta +
                std::sin(v * PI + phase + sampleU * 3.2f) * (0.010f + foldStrength * 0.010f) * looseness +
                slow * (0.008f + foldStrength * 0.009f) * (1.0f - v * 0.35f);
            const float radius = baseRadius + expandRadius +
                fold * ofLerp(0.54f, 1.02f, layerT) +
                std::sin((sampleU * 2.15f + layerT) * TWO_PI + phase) * (18.0f + expandRadius * 0.10f) +
                signedNoise(sampleU * 1.8f, v * 2.6f + 11.0f, phase * 0.8f) * expandRadius * 0.12f * looseness;
            const float edgeWeight = std::pow(std::abs(v * 2.0f - 1.0f), 1.20f);
            const float centeredV = v - 0.5f;
            const float baseHeight = height * yShape.x + expandY * 1.15f;
            const float edgeExpansion = (audioDrive * 54.0f + pulseDrive * 46.0f + localEnergy * 18.0f) *
                edgeWeight *
                (0.72f + foldStrength * 0.18f + looseness * 0.10f);
            const float edgeFlutter = (slow * 24.0f + fine * 14.0f + hangingWave * 18.0f) *
                edgeWeight *
                (0.62f + foldStrength * 0.34f) *
                (1.0f + audioDrive * 0.10f);
            const float lowerFray = (slow * 28.0f + fine * 15.0f) *
                std::pow(1.0f - v, 2.15f) *
                (0.48f + looseness * 0.16f);
            const float y = paramAuroraBaseY_ + baseHeight * 0.5f +
                centeredV * (baseHeight + edgeExpansion) +
                flow.verticalOffset * ofLerp(0.16f, 0.88f, edgeWeight) +
                yShape.y * (0.28f + edgeWeight * (0.46f + looseness * 0.10f)) +
                verticalDrift * (0.42f + edgeWeight * 0.36f) +
                edgeFlutter +
                lowerFray;
            glm::vec3 point = scenePointAt(theta, std::max(10.0f, radius), y);
            point.x += flow.planarOffset.x * (0.74f + looseness * 0.12f);
            point.z += flow.planarOffset.y * (0.74f + looseness * 0.12f);
            return point;
        };

        for (int shell = 2; shell >= 0; --shell) {
            const int shellCols = 72;
            const int shellRows = 8;
            const float shellT = static_cast<float>(shell) / 2.0f;
            const float subBandT = 0.22f + shellT * 0.84f;
            const float subCenter = subBandCenterAt(subBandT);
            const float subArc = subBandArcAt(subBandT);
            const float expandRadius = bloom * ofLerp(70.0f, 300.0f, shellT);
            const float expandY = bloom * ofLerp(55.0f, 220.0f, shellT);
            const float shellAlpha = glow * layerStrength *
                ofLerp(0.072f, 0.026f, shellT) *
                brightnessDrive *
                (0.82f + bloom * 0.28f + energy * 0.48f + pulseDrive * 0.78f + highs_ * audio * 0.22f);

            ofMesh shellMesh;
            shellMesh.setMode(OF_PRIMITIVE_TRIANGLES);
            for (int y = 0; y <= shellRows; ++y) {
                const float v = static_cast<float>(y) / static_cast<float>(shellRows);
                const float vertical = std::pow(std::max(0.0f, std::sin(v * PI)), 0.34f);
                for (int x = 0; x <= shellCols; ++x) {
                    const float u = static_cast<float>(x) / static_cast<float>(shellCols);
                    const float sampleU = sampleUAt(u);
                    const float edge = edgeFadeAt(u, 4.2f);
                    const float sampleTheta = subCenter + (u - 0.5f) * subArc;
                    const float localEnergy = sceneEnergyAt(sceneXZAt(sampleTheta, baseRadius));
                    const AuroraFlow flow = sharedSceneFlowAt(sampleTheta, layerT, v);
                    const float shimmer = 0.62f + 0.38f * ofNoise(sampleU * 5.0f + layerT, v * 2.0f, phase + highs_ * audio * 0.85f);
                    shellMesh.addVertex(curtainPoint(u, v, expandRadius, expandY, 1.0f + shellT, subBandT));
                    shellMesh.addColor(auroraColor(v * 0.40f + layerT * 0.34f,
                                                   1.12f,
                                                   shellAlpha * vertical * edge * shimmer * flow.energy * (0.60f + localEnergy * 0.62f)));
                }
            }
            for (int y = 0; y < shellRows; ++y) {
                for (int x = 0; x < shellCols; ++x) {
                    const int i00 = y * (shellCols + 1) + x;
                    const int i10 = i00 + 1;
                    const int i01 = (y + 1) * (shellCols + 1) + x;
                    const int i11 = i01 + 1;
                    shellMesh.addIndex(i00);
                    shellMesh.addIndex(i10);
                    shellMesh.addIndex(i11);
                    shellMesh.addIndex(i00);
                    shellMesh.addIndex(i11);
                    shellMesh.addIndex(i01);
                }
            }
            shellMesh.draw();
        }

        ofMesh curtain;
        curtain.setMode(OF_PRIMITIVE_TRIANGLES);
        const float coreSubBand = 0.12f;
        const float coreCenter = subBandCenterAt(coreSubBand);
        const float coreArc = subBandArcAt(coreSubBand);
        for (int y = 0; y <= rows; ++y) {
            const float v = static_cast<float>(y) / static_cast<float>(rows);
            const float vertical = std::pow(std::max(0.0f, std::sin(v * PI)), 0.48f);
            const float bottomFade = smootherStep(v * 7.0f);
            for (int x = 0; x <= cols; ++x) {
                const float u = static_cast<float>(x) / static_cast<float>(cols);
                const float sampleU = sampleUAt(u);
                const float edge = edgeFadeAt(u, 5.4f);
                const float sampleTheta = coreCenter + (u - 0.5f) * coreArc;
                const float localEnergy = sceneEnergyAt(sceneXZAt(sampleTheta, baseRadius));
                const AuroraFlow flow = sharedSceneFlowAt(sampleTheta, layerT, v);
                const float column = 0.74f + 0.26f * ofNoise(sampleU * 10.5f, layerT * 4.0f, phase * 1.8f);
                const float coreAlpha = glow * layerStrength *
                    brightnessDrive *
                    (0.16f + energy * 0.075f + localEnergy * 0.25f + pulseDrive * 0.15f + mids_ * audio * 0.08f) *
                    vertical * bottomFade * edge * column * flow.energy;
                curtain.addVertex(curtainPoint(u, v, 0.0f, 0.0f, 0.15f, coreSubBand));
                curtain.addColor(auroraColor(v * 0.56f + localEnergy * 0.14f + layerT * 0.22f,
                                             1.24f + localEnergy * 0.12f,
                                             coreAlpha));
            }
        }
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                const int i00 = y * (cols + 1) + x;
                const int i10 = i00 + 1;
                const int i01 = (y + 1) * (cols + 1) + x;
                const int i11 = i01 + 1;
                curtain.addIndex(i00);
                curtain.addIndex(i10);
                curtain.addIndex(i11);
                curtain.addIndex(i00);
                curtain.addIndex(i11);
                curtain.addIndex(i01);
            }
        }
        curtain.draw();

        ofMesh edgeGlow;
        edgeGlow.setMode(OF_PRIMITIVE_LINES);
        const float edgeRows[] = { 0.06f, 0.24f, 0.46f, 0.68f, 0.90f };
        for (float v : edgeRows) {
            for (int x = 0; x < cols; ++x) {
                const float u0 = static_cast<float>(x) / static_cast<float>(cols);
                const float u1 = static_cast<float>(x + 1) / static_cast<float>(cols);
                const float midU = (u0 + u1) * 0.5f;
                const float sampleU = sampleUAt(midU);
                const float edge = edgeFadeAt(midU, 4.8f);
                const float edgeSubBand = 0.34f + v * 0.92f;
                const float edgeCenter = subBandCenterAt(edgeSubBand);
                const float edgeArc = subBandArcAt(edgeSubBand);
                const float sampleTheta = edgeCenter + (midU - 0.5f) * edgeArc;
                const float localEnergy = sceneEnergyAt(sceneXZAt(sampleTheta, baseRadius));
                const AuroraFlow flow = sharedSceneFlowAt(sampleTheta, layerT, v);
                const float skip = ofNoise(sampleU * 13.0f + v * 3.0f, layerT * 7.0f, phase * 1.4f);
                if (skip < 0.22f) {
                    continue;
                }
                const float edgeAlpha = glow * layerStrength *
                    brightnessDrive *
                    (0.10f + localEnergy * 0.24f + pulseDrive * 0.16f + highs_ * audio * 0.16f) *
                    edge * flow.energy * ofLerp(0.55f, 1.0f, skip);
                const ofFloatColor color = auroraColor(v * 0.22f + layerT * 0.12f, 1.46f, edgeAlpha);
                edgeGlow.addVertex(curtainPoint(u0, v, -8.0f, -4.0f, 0.03f, edgeSubBand));
                edgeGlow.addColor(color);
                edgeGlow.addVertex(curtainPoint(u1, v, -8.0f, -4.0f, 0.03f, edgeSubBand));
                edgeGlow.addColor(color);
            }
        }

        ofSetLineWidth(ofClamp(1.6f + pulseDrive * 1.65f + highs_ * audio * 0.75f, 1.0f, 4.2f));
        edgeGlow.draw();

        if (rayDensity > 0.02f) {
            ofMesh rays;
            rays.setMode(OF_PRIMITIVE_LINES);
            const int rayStep = std::max(2, static_cast<int>(std::round(ofMap(rayDensity, 0.02f, 1.8f, 16.0f, 4.0f, true))));
            for (int x = 0; x <= cols; x += rayStep) {
                const float u = static_cast<float>(x) / static_cast<float>(cols);
                const float sampleU = sampleUAt(u);
                const float edge = edgeFadeAt(u, 4.6f);
                const float raySubBand = 0.42f;
                const float rayCenter = subBandCenterAt(raySubBand);
                const float rayArc = subBandArcAt(raySubBand);
                const float sampleTheta = rayCenter + (u - 0.5f) * rayArc;
                const float localEnergy = sceneEnergyAt(sceneXZAt(sampleTheta, baseRadius));
                const AuroraFlow flow = sharedSceneFlowAt(sampleTheta, layerT, 0.44f);
                const float gate = ofNoise(sampleU * 15.0f + layerT * 3.0f, phase * 2.1f);
                const float threshold = ofClamp(0.60f - rayDensity * 0.15f - localEnergy * 0.08f - highs_ * audio * 0.14f, 0.28f, 0.74f);
                if (gate < threshold) {
                    continue;
                }

                const float bottomV = ofClamp(0.05f + ofNoise(sampleU * 6.0f, layerT + 21.0f, phase) * 0.18f, 0.04f, 0.28f);
                const float topV = ofClamp(0.62f + ofNoise(sampleU * 4.0f + 9.0f, layerT, phase) * 0.22f, bottomV + 0.20f, 0.90f);
                glm::vec3 bottom = curtainPoint(u, bottomV, -10.0f, -10.0f, 0.04f, raySubBand);
                glm::vec3 top = curtainPoint(u + signedNoise(sampleU * 8.0f, layerT, phase) * 0.006f, topV, -10.0f, -10.0f, 0.04f, raySubBand);
                const float rayAlpha = glow * rayDensity * layerStrength *
                    brightnessDrive *
                    (0.08f + localEnergy * 0.16f + highs_ * audio * 0.18f + pulseDrive * 0.14f) *
                    edge * flow.energy * ofLerp(0.45f, 0.90f, gate);
                const ofFloatColor bottomColor = auroraColor(0.06f + layerT * 0.08f, 1.34f, rayAlpha);
                const ofFloatColor topColor = auroraColor(0.18f + localEnergy * 0.08f, 1.14f, rayAlpha * 0.32f);
                rays.addVertex(bottom);
                rays.addColor(bottomColor);
                rays.addVertex(top);
                rays.addColor(topColor);
            }

            ofSetLineWidth(ofClamp(0.8f + rayDensity * 0.9f, 0.8f, 2.0f));
            rays.draw();
        }
        ofSetLineWidth(1.0f);
    }

    glDepthMask(GL_TRUE);
}

float ArcticAuroraSceneLayer::waterSurfaceYAt(float x, float z, float time, float waveScale) const {
    const float diskRadius = std::max(180.0f, paramWaterWidth_ * 0.5f);
    const float radius = glm::length(glm::vec2(x, z));
    const float radial = ofClamp(radius / diskRadius, 0.0f, 1.0f);
    const float radialCalm = ofLerp(0.38f, 1.0f, smootherStep(radial * radial));
    const float wave = waterWaveAt(x, z, time, paramWaterWaveIdle_ * waveScale) * radialCalm;
    const float noise = signedNoise(x * paramWaterNoiseScale_ + 21.0f,
                                    z * paramWaterNoiseScale_ - 13.0f,
                                    time * 0.090f) *
        paramWaterNoiseAmount_ * waveScale * radialCalm;

    float ripple = 0.0f;
    if (paramWaterRippleAmount_ > 0.001f && !icebergs_.empty()) {
        const float rippleRadius = std::max(1.0f, paramWaterRippleRadius_);
        const glm::vec2 sample(x, z);
        for (const auto& iceberg : icebergs_) {
            glm::vec2 source(iceberg.position.x, iceberg.position.z);
            if (iceberg.orbitRadius > 0.0f) {
                const float angle = iceberg.orbitAngle + time * iceberg.driftSpeed;
                const float radialDrift = std::sin(time * 0.085f + iceberg.seed * 2.1f) * 18.0f +
                    signedNoise(iceberg.seed * 0.37f, time * 0.035f, 12.0f) * 9.0f;
                const float sourceRadius = std::max(20.0f, iceberg.orbitRadius + radialDrift);
                source = glm::vec2(std::cos(angle) * sourceRadius, std::sin(angle) * sourceRadius);
            }

            const float distance = glm::length(sample - source);
            const float falloff = std::exp(-(distance * distance) / (2.0f * rippleRadius * rippleRadius));
            const float phase = distance * 0.060f - time * (1.02f + iceberg.scale * 0.18f) + iceberg.seed * 0.73f;
            const float wake = std::sin(phase) + std::sin(phase * 0.58f + 1.7f) * 0.28f;
            ripple += wake * falloff * (0.55f + iceberg.scale * 0.45f);
        }
    }

    return paramWaterLevel_ + wave + noise + ripple * paramWaterRippleAmount_ * waveScale * radialCalm;
}

void ArcticAuroraSceneLayer::drawWaterPlane(const LayerDrawParams& params, float alpha) const {
    static_cast<void>(params);
    ofMesh surface;
    surface.setMode(OF_PRIMITIVE_TRIANGLES);
    const float time = sceneTime_;
    const int radialSegments = 44;
    const int angularSegments = 112;
    const float diskRadius = std::max(180.0f, paramWaterWidth_ * 0.5f);
    const float waterAlpha = alpha * paramWaterAlpha_;
    const float translucencyFade = ofClamp(1.12f - paramWaterTranslucency_ * 0.46f, 0.26f, 1.16f);
    const float colorGain = paramWaterBrightness_;
    const float auroraInnerRadius = radiusFromDepthParam(paramAuroraDepthNear_);
    const float auroraOuterRadius = std::max(auroraInnerRadius + 80.0f, radiusFromDepthParam(paramAuroraDepthFar_));
    const float auroraMidRadius = (auroraInnerRadius + auroraOuterRadius) * 0.5f;
    const float auroraLightSpan = std::max(180.0f, std::abs(auroraOuterRadius - auroraInnerRadius) * 0.70f + diskRadius * 0.08f);
    const float auroraLightEnergy = ofClamp(auroraEnergy_, 0.0f, 2.4f);
    const float auroraLightPulse = ofClamp(auroraPulse_, 0.0f, 2.6f);

    for (int row = 0; row <= radialSegments; ++row) {
        const float v = static_cast<float>(row) / static_cast<float>(radialSegments);
        const float radius = diskRadius * std::sqrt(v);
        const float edgeT = smootherStep(ofMap(v, 0.72f, 1.0f, 0.0f, 1.0f, true));
        for (int col = 0; col <= angularSegments; ++col) {
            const float u = static_cast<float>(col) / static_cast<float>(angularSegments);
            const float theta = u * TWO_PI;
            const float x = std::cos(theta) * radius;
            const float z = std::sin(theta) * radius;
            const float y = waterSurfaceYAt(x, z, time);
            const float zDepthT = smootherStep(ofMap(z,
                                                     paramWaterNearZ_,
                                                     paramWaterFarZ_,
                                                     0.0f,
                                                     1.0f,
                                                     true));
            const float edgeFade = 1.0f - edgeT * 0.78f;
            const float radialT = smootherStep(v);
            const float moonColumn = std::pow(ofClamp(1.0f - std::abs(std::sin(theta + 0.25f)) * 0.72f, 0.0f, 1.0f), 2.4f);
            const float moonRipple = ofClamp(0.5f + 0.5f * signedNoise(x * 0.010f + 3.0f, z * 0.010f - 8.0f, time * 0.19f),
                                             0.0f,
                                             1.0f);
            const float moonlight = paramWaterHighlight_ * (0.18f + moonColumn * moonRipple * ofLerp(0.42f, 0.78f, radialT));
            const float glint = ofClamp((0.06f + signedNoise(x * 0.012f, z * 0.018f, time * 0.25f) * 0.035f) *
                                            (0.55f + v * 0.45f),
                                        0.0f,
                                        0.14f);
            const float depthShade = ofLerp(1.10f, 0.72f, zDepthT);
            const float surfaceOpacity = ofLerp(0.62f, 0.34f, v) *
                ofLerp(1.06f, 0.74f, zDepthT) *
                translucencyFade;
            const ofFloatColor nearColor = colorFrom(paramWaterR_ * colorGain * (1.36f + glint) * depthShade,
                                                     paramWaterG_ * colorGain * (1.44f + glint * 1.5f) * depthShade,
                                                     paramWaterB_ * colorGain * (1.62f + glint * 1.8f) * depthShade,
                                                     waterAlpha * surfaceOpacity * edgeFade);
            const float brightnessT = ofMap(colorGain, 0.2f, 2.5f, 0.0f, 1.0f, true);
            const float moonGain = ofLerp(0.82f, 1.18f, brightnessT);
            const ofFloatColor moonColor = colorFrom((0.48f + moonlight * 0.24f) * moonGain,
                                                     (0.72f + moonlight * 0.20f) * ofLerp(0.82f, 1.14f, brightnessT),
                                                     (1.00f + moonlight * 0.18f) * ofLerp(0.82f, 1.10f, brightnessT),
                                                     nearColor.a);
            const ofFloatColor edgeColor = colorFrom(paramSkyHorizonR_ * 1.05f,
                                                     paramSkyHorizonG_ * 1.05f,
                                                     paramSkyHorizonB_ * 1.15f,
                                                     nearColor.a * 0.58f);
            const ofFloatColor color = nearColor
                .getLerped(moonColor, ofClamp(moonlight, 0.0f, 0.68f))
                .getLerped(edgeColor, ofClamp(edgeT * 0.26f + zDepthT * 0.20f, 0.0f, 0.48f));
            const float auroraRing = std::pow(ofClamp(1.0f - std::abs(radius - auroraMidRadius) / auroraLightSpan,
                                                      0.0f,
                                                      1.0f),
                                              1.45f);
            const float auroraColumn = 0.55f + 0.45f * ofNoise(std::cos(theta) * 1.35f + 9.0f,
                                                               std::sin(theta) * 1.35f - 3.0f,
                                                               time * 0.12f);
            const float auroraLight = ofClamp(paramWaterAuroraLight_ *
                                                  auroraRing *
                                                  auroraColumn *
                                                  (0.10f + auroraLightEnergy * 0.24f + auroraLightPulse * 0.22f),
                                              0.0f,
                                              0.62f);
            const ofFloatColor auroraTint = colorFrom(paramAuroraR_ * 0.58f + paramAurora2R_ * 0.18f,
                                                      paramAuroraG_ * 0.58f + paramAurora2G_ * 0.18f,
                                                      paramAuroraB_ * 0.58f + paramAurora2B_ * 0.18f,
                                                      color.a);
            surface.addVertex(glm::vec3(x, y, z));
            surface.addColor(color.getLerped(auroraTint, auroraLight));
        }
    }

    for (int row = 0; row < radialSegments; ++row) {
        for (int col = 0; col < angularSegments; ++col) {
            const int i00 = row * (angularSegments + 1) + col;
            const int i10 = i00 + 1;
            const int i01 = (row + 1) * (angularSegments + 1) + col;
            const int i11 = i01 + 1;
            surface.addIndex(i00);
            surface.addIndex(i10);
            surface.addIndex(i11);
            surface.addIndex(i00);
            surface.addIndex(i11);
            surface.addIndex(i01);
        }
    }

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    glDepthMask(GL_FALSE);
    if (paramWaterCurvature_ > 0.01f && paramWaterHemisphereDepth_ > 1.0f) {
        ofMesh lowerHemisphere;
        lowerHemisphere.setMode(OF_PRIMITIVE_TRIANGLES);
        const int hemiRows = 36;
        const float curvature = smootherStep(paramWaterCurvature_);
        const float hemiDepth = paramWaterHemisphereDepth_ * curvature;
        const float volumeAlpha = waterAlpha *
            curvature *
            ofClamp(0.46f + paramWaterTranslucency_ * 0.34f, 0.24f, 1.0f);
        const ofFloatColor bottomColor = colorFrom(paramWaterR_ * colorGain * 0.52f,
                                                   paramWaterG_ * colorGain * 0.68f,
                                                   paramWaterB_ * colorGain * 0.90f,
                                                   volumeAlpha * 0.16f);
        const ofFloatColor equatorColor = colorFrom(paramWaterR_ * colorGain * 0.92f,
                                                    paramWaterG_ * colorGain * 1.08f,
                                                    paramWaterB_ * colorGain * 1.30f,
                                                    volumeAlpha * 0.34f);
        const ofFloatColor farShade = colorFrom(paramSkyHorizonR_ * 0.58f,
                                                paramSkyHorizonG_ * 0.70f,
                                                paramSkyHorizonB_ * 0.95f,
                                                volumeAlpha * 0.18f);
        for (int row = 0; row <= hemiRows; ++row) {
            const float v = static_cast<float>(row) / static_cast<float>(hemiRows);
            const float phi = v * HALF_PI;
            const float radius = diskRadius * std::sin(phi);
            const float y = paramWaterLevel_ - hemiDepth * std::cos(phi);
            for (int col = 0; col <= angularSegments; ++col) {
                const float u = static_cast<float>(col) / static_cast<float>(angularSegments);
                const float theta = u * TWO_PI;
                const glm::vec3 point = polarPoint(theta, radius, y);
                const float zDepthT = smootherStep(ofMap(point.z,
                                                         paramWaterNearZ_,
                                                         paramWaterFarZ_,
                                                         0.0f,
                                                         1.0f,
                                                         true));
                const ofFloatColor color = bottomColor
                    .getLerped(equatorColor, smootherStep(v))
                    .getLerped(farShade, zDepthT * 0.22f);
                lowerHemisphere.addVertex(point);
                lowerHemisphere.addColor(color);
            }
        }
        for (int row = 0; row < hemiRows; ++row) {
            for (int col = 0; col < angularSegments; ++col) {
                const int i00 = row * (angularSegments + 1) + col;
                const int i10 = i00 + 1;
                const int i01 = (row + 1) * (angularSegments + 1) + col;
                const int i11 = i01 + 1;
                lowerHemisphere.addIndex(i00);
                lowerHemisphere.addIndex(i11);
                lowerHemisphere.addIndex(i10);
                lowerHemisphere.addIndex(i00);
                lowerHemisphere.addIndex(i01);
                lowerHemisphere.addIndex(i11);
            }
        }
        lowerHemisphere.draw();
    }
    surface.draw();

    ofMesh highlights;
    highlights.setMode(OF_PRIMITIVE_LINES);
    const ofFloatColor faintCyan = colorFrom(0.28f * colorGain,
                                             0.88f * colorGain,
                                             1.0f * colorGain,
                                             waterAlpha * paramWaterHighlight_ * 0.22f);
    const ofFloatColor edgeLine = colorFrom(0.46f,
                                            0.66f,
                                            0.90f,
                                            waterAlpha * paramWaterHighlight_ * 0.18f);
    for (int row = 5; row <= radialSegments; row += 4) {
        const float v = static_cast<float>(row) / static_cast<float>(radialSegments);
        const float radius = diskRadius * std::sqrt(v);
        const int arcCount = 18;
        for (int i = 0; i < arcCount; ++i) {
            const float centerT = (static_cast<float>(i) + 0.5f) / static_cast<float>(arcCount);
            const float theta = centerT * TWO_PI + signedNoise(v * 4.0f, static_cast<float>(i), time * 0.05f) * 0.055f;
            const float arcHalf = ofLerp(0.018f, 0.050f, 1.0f - v);
            const float skip = ofNoise(v * 12.0f, centerT * 8.0f, 0.25f);
            if (skip > 0.40f) {
                const float theta0 = theta - arcHalf;
                const float theta1 = theta + arcHalf;
                const float ax = std::cos(theta0) * radius;
                const float az = std::sin(theta0) * radius;
                const float bx = std::cos(theta1) * radius;
                const float bz = std::sin(theta1) * radius;
                const glm::vec3 a(ax, waterSurfaceYAt(ax, az, time, 0.20f) + 2.0f, az);
                const glm::vec3 b(bx, waterSurfaceYAt(bx, bz, time, 0.20f) + 2.0f, bz);
                addLine(highlights,
                        a,
                        b,
                        faintCyan.getLerped(edgeLine, smootherStep(ofMap(v, 0.65f, 1.0f, 0.0f, 1.0f, true))));
            }
        }
    }

    glDepthMask(GL_FALSE);
#ifndef TARGET_OPENGLES
    glLineWidth(1.0f);
#endif
    highlights.draw();
    glDepthMask(GL_TRUE);
}

void ArcticAuroraSceneLayer::drawWaterReflection(const LayerDrawParams& params, float alpha) const {
    static_cast<void>(params);
    if (paramWaterReflection_ <= 0.0f) {
        return;
    }

    ofMesh reflection;
    reflection.setMode(OF_PRIMITIVE_TRIANGLES);

    const int bands = 18;
    const float time = sceneTime_;
    const float audioLift = hasAudio_ ? ofClamp(auroraEnergy_, 0.16f, 1.35f) : 0.36f;
    const float baseAlpha = alpha * paramWaterReflection_ * (0.025f + audioLift * 0.060f);
    const float diskRadius = std::max(180.0f, paramWaterWidth_ * 0.5f);
    const float innerRadius = radiusFromDepthParam(paramAuroraDepthNear_);
    const float outerRadius = std::max(innerRadius + 80.0f, radiusFromDepthParam(paramAuroraDepthFar_));
    const float midRadius = (innerRadius + outerRadius) * 0.5f;
    const float requestedArc = paramAuroraWidth_ / std::max(240.0f, midRadius);
    const float arc = ofClamp(requestedArc, PI * 0.32f, TWO_PI * 0.43f) * 0.72f;
    const int curtains = std::max(1, static_cast<int>(std::round(paramAuroraCurtainCount_)));

    for (int c = 0; c < curtains; ++c) {
        const float layerT = curtains > 1 ? static_cast<float>(c) / static_cast<float>(curtains - 1) : 0.0f;
        const float direction = (c % 2 == 0) ? 1.0f : -1.0f;
        const float layerRate = direction * ofLerp(0.060f, 0.026f, layerT) +
            signedNoise(layerT * 3.2f + 0.5f, 11.0f, 0.0f) * 0.012f;
        const float layerCenter = -HALF_PI + layerT * TWO_PI + sceneTime_ * layerRate;

        for (int i = 0; i < bands; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(std::max(1, bands - 1));
            const float radius = ofLerp(diskRadius * 0.20f, diskRadius * 0.78f, smootherStep(t));
            const float theta = layerCenter + ofLerp(-arc * 0.5f, arc * 0.5f, t) +
                signedNoise(t * 4.0f, layerT * 3.0f + 4.0f, time * 0.17f) * 0.10f;
            const float halfTheta = ofLerp(0.045f, 0.13f, 1.0f - t) *
                (0.72f + ofNoise(t * 9.0f + layerT, time * 0.09f) * 0.44f);
            const float halfRadius = ofLerp(8.0f, 24.0f, 1.0f - t);
            const float fade = std::sin(t * PI) * ofLerp(1.0f, 0.62f, layerT);
            const ofFloatColor green = colorFrom(paramAuroraR_, paramAuroraG_, paramAuroraB_, baseAlpha * fade);
            const ofFloatColor violet = colorFrom(paramAurora2R_, paramAurora2G_, paramAurora2B_, baseAlpha * fade * 0.50f);
            const ofFloatColor color = green.getLerped(violet, ofNoise(t * 3.5f, 16.0f + layerT, time * 0.10f));

            auto reflectionPoint = [&](float sampleTheta, float sampleRadius) {
                const float px = std::cos(sampleTheta) * sampleRadius;
                const float pz = std::sin(sampleTheta) * sampleRadius;
                return glm::vec3(px, waterSurfaceYAt(px, pz, time, 0.12f) + 4.0f, pz);
            };

            addQuad(reflection,
                    reflectionPoint(theta - halfTheta, radius - halfRadius),
                    reflectionPoint(theta + halfTheta, radius - halfRadius),
                    reflectionPoint(theta + halfTheta * 0.82f, radius + halfRadius),
                    reflectionPoint(theta - halfTheta * 0.82f, radius + halfRadius),
                    color);
        }
    }

    ofEnableBlendMode(OF_BLENDMODE_ADD);
    glDepthMask(GL_FALSE);
    reflection.draw();
    glDepthMask(GL_TRUE);
}

void ArcticAuroraSceneLayer::drawWaterHorizonMist(float alpha) const {
    if (paramWaterHorizonFog_ <= 0.0f) {
        return;
    }

    ofMesh mist;
    mist.setMode(OF_PRIMITIVE_TRIANGLES);

    const int segments = 112;
    const float diskRadius = std::max(180.0f, paramWaterWidth_ * 0.5f);
    const float innerRadius = diskRadius * 0.78f;
    const float outerRadius = diskRadius * 1.06f;
    const float time = sceneTime_;
    const ofFloatColor inner = colorFrom(paramSkyHorizonR_ * 1.6f,
                                         paramSkyHorizonG_ * 1.8f,
                                         paramSkyHorizonB_ * 2.1f,
                                         alpha * paramWaterAlpha_ * paramWaterHorizonFog_ * 0.14f);
    const ofFloatColor outer = colorFrom(paramAuroraR_ * 0.28f,
                                         paramAuroraG_ * 0.26f,
                                         paramAuroraB_ * 0.25f,
                                         0.0f);

    for (int i = 0; i <= segments; ++i) {
        const float u = static_cast<float>(i) / static_cast<float>(segments);
        const float theta = u * TWO_PI;
        const float innerX = std::cos(theta) * innerRadius;
        const float innerZ = std::sin(theta) * innerRadius;
        const float outerX = std::cos(theta) * outerRadius;
        const float outerZ = std::sin(theta) * outerRadius;
        mist.addVertex(glm::vec3(innerX, waterSurfaceYAt(innerX, innerZ, time, 0.0f) + 7.0f, innerZ));
        mist.addColor(inner);
        mist.addVertex(glm::vec3(outerX, waterSurfaceYAt(outerX, outerZ, time, 0.0f) + 22.0f, outerZ));
        mist.addColor(outer);
    }
    for (int i = 0; i < segments; ++i) {
        const int i00 = i * 2;
        const int i10 = i00 + 1;
        const int i01 = (i + 1) * 2;
        const int i11 = i01 + 1;
        mist.addIndex(i00);
        mist.addIndex(i01);
        mist.addIndex(i11);
        mist.addIndex(i00);
        mist.addIndex(i11);
        mist.addIndex(i10);
    }

    ofEnableBlendMode(OF_BLENDMODE_ADD);
    glDepthMask(GL_FALSE);
    mist.draw();
    glDepthMask(GL_TRUE);
}

void ArcticAuroraSceneLayer::buildIcebergMeshes(Iceberg& iceberg, const glm::vec3& dimensions, std::mt19937& rng) {
    const int sides = randomInt(rng, 9, 13);
    const float halfWidth = dimensions.x * 0.5f;
    const float halfDepth = dimensions.z * 0.5f;
    const float visibleHeight = dimensions.y * randomRange(rng, 0.26f, 0.42f);
    const float underwaterDepth = dimensions.y * randomRange(rng, 0.68f, 1.02f);

    std::vector<glm::vec2> silhouette;
    std::vector<glm::vec2> facetPoints2d;
    std::vector<glm::vec3> facetPoints3d;
    std::vector<glm::vec3> waterRing;
    std::vector<glm::vec3> underRing;
    silhouette.reserve(static_cast<std::size_t>(sides));
    facetPoints2d.reserve(40);
    facetPoints3d.reserve(40);
    waterRing.reserve(static_cast<std::size_t>(sides));
    underRing.reserve(static_cast<std::size_t>(sides));

    for (int i = 0; i < sides; ++i) {
        const float baseAngle = TWO_PI * static_cast<float>(i) / static_cast<float>(sides);
        const float angle = baseAngle + randomRange(rng, -0.13f, 0.13f);
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        const float radial = randomRange(rng, 0.86f, 1.16f);
        const float waterX = c * halfWidth * radial * randomRange(rng, 0.88f, 1.13f);
        const float waterZ = s * halfDepth * radial * randomRange(rng, 0.82f, 1.18f);
        const float waterY = randomRange(rng, -2.0f, 3.0f);
        const float underScale = randomRange(rng, 0.54f, 0.78f);
        const glm::vec2 boundaryPoint(waterX, waterZ);
        const glm::vec3 waterPoint(waterX, waterY, waterZ);

        silhouette.push_back(boundaryPoint);
        facetPoints2d.push_back(boundaryPoint);
        facetPoints3d.push_back(waterPoint);
        waterRing.push_back(waterPoint);
        underRing.push_back(glm::vec3(waterX * underScale + randomRange(rng, -7.0f, 7.0f),
                                      -underwaterDepth * randomRange(rng, 0.34f, 0.60f),
                                      waterZ * underScale + randomRange(rng, -7.0f, 7.0f)));
    }

    const int interiorCount = randomInt(rng, 16, 25);
    int attempts = 0;
    while (static_cast<int>(facetPoints2d.size()) < sides + interiorCount && attempts < interiorCount * 12) {
        ++attempts;
        const float angle = randomRange(rng, 0.0f, TWO_PI);
        const float radius = std::sqrt(randomRange(rng, 0.0f, 1.0f)) * randomRange(rng, 0.12f, 0.86f);
        const glm::vec2 p(std::cos(angle) * halfWidth * radius,
                          std::sin(angle) * halfDepth * radius);
        if (!pointInPolygon(p, silhouette)) {
            continue;
        }
        const float centerFalloff = ofClamp(1.0f - radius, 0.0f, 1.0f);
        const float broadPlateau = std::pow(centerFalloff, 0.55f);
        const float ridge = std::max(0.0f, signedNoise(p.x * 0.010f + iceberg.seed,
                                                       p.y * 0.012f - iceberg.seed,
                                                       0.0f));
        const float peak = std::pow(std::max(0.0f, signedNoise(p.x * 0.027f - iceberg.seed * 0.7f,
                                                               p.y * 0.024f + iceberg.seed * 0.5f,
                                                               4.0f)),
                                    2.15f);
        const float shoulder = std::max(0.0f, std::sin((angle + iceberg.seed) * 3.0f)) *
            std::pow(centerFalloff, 0.85f);
        const float y = visibleHeight *
                (0.10f + broadPlateau * 0.34f + ridge * 0.16f + peak * 0.28f + shoulder * 0.10f)
            + randomRange(rng, -2.0f, 7.0f);
        facetPoints2d.push_back(p);
        facetPoints3d.push_back(glm::vec3(p.x, y, p.y));
    }

    const glm::vec3 keelTip(randomRange(rng, -halfWidth * 0.08f, halfWidth * 0.08f),
                            -underwaterDepth * randomRange(rng, 0.78f, 1.02f),
                            randomRange(rng, -halfDepth * 0.08f, halfDepth * 0.08f));

    iceberg.aboveWater.clear();
    iceberg.belowWater.clear();
    iceberg.rimLines.clear();
    iceberg.facets.clear();
    iceberg.aboveWater.setMode(OF_PRIMITIVE_TRIANGLES);
    iceberg.belowWater.setMode(OF_PRIMITIVE_TRIANGLES);
    iceberg.rimLines.setMode(OF_PRIMITIVE_LINES);

    const ofFloatColor snowColor = colorFrom(0.62f, 0.92f, 1.0f, 0.88f);
    const ofFloatColor blueColor = colorFrom(0.13f, 0.36f, 0.50f, 0.84f);
    const ofFloatColor shadowColor = colorFrom(0.028f, 0.105f, 0.17f, 0.88f);
    const ofFloatColor accentColor = colorFrom(paramIceAccentR_ * 0.24f,
                                               paramIceAccentG_ * 0.24f,
                                               paramIceAccentB_ * 0.28f,
                                               0.68f);
    const ofFloatColor underColor = colorFrom(0.030f, 0.145f, 0.210f, 0.52f);
    const ofFloatColor underEdgeColor = colorFrom(paramAuroraR_ * 0.36f + 0.055f,
                                                  paramAuroraG_ * 0.34f + 0.085f,
                                                  paramAuroraB_ * 0.40f + 0.120f,
                                                  0.46f);
    const ofFloatColor rimColor = colorFrom(0.62f, 0.88f, 1.0f, 0.20f);

    const auto triangles = delaunayTriangulate(facetPoints2d);
    for (const auto& tri : triangles) {
        const glm::vec2 centroid2d = (facetPoints2d[tri.a] + facetPoints2d[tri.b] + facetPoints2d[tri.c]) / 3.0f;
        if (!pointInPolygon(centroid2d, silhouette)) {
            continue;
        }
        glm::vec3 a = facetPoints3d[tri.a];
        glm::vec3 b = facetPoints3d[tri.b];
        glm::vec3 c = facetPoints3d[tri.c];
        if (glm::cross(b - a, c - a).y < 0.0f) {
            std::swap(b, c);
        }
        const ofFloatColor color = facetColorFor(a, b, c,
                                                 halfWidth,
                                                 halfDepth,
                                                 visibleHeight,
                                                 snowColor,
                                                 blueColor,
                                                 shadowColor,
                                                 accentColor);
        addTriangle(iceberg.aboveWater, a, b, c, color);

        const glm::vec3 centroid = (a + b + c) / 3.0f;
        glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
        if (!std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z)) {
            normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        glm::vec3 drift(centroid.x / std::max(1.0f, halfWidth) + randomRange(rng, -0.22f, 0.22f),
                        -0.08f + randomRange(rng, -0.04f, 0.05f),
                        centroid.z / std::max(1.0f, halfDepth) + randomRange(rng, -0.22f, 0.22f));
        if (glm::length(drift) <= 0.0001f) {
            drift = glm::vec3(normal.x, -0.08f, normal.z);
        }
        drift = glm::normalize(drift);

        const float ab = glm::length(a - b);
        const float bc = glm::length(b - c);
        const float ca = glm::length(c - a);
        glm::vec3 crackA = a;
        glm::vec3 crackB = b;
        if (bc > ab && bc >= ca) {
            crackA = b;
            crackB = c;
        } else if (ca > ab && ca > bc) {
            crackA = c;
            crackB = a;
        }

        const float radialFracture = ofClamp(glm::length(glm::vec2(centroid.x / std::max(1.0f, halfWidth),
                                                                    centroid.z / std::max(1.0f, halfDepth))),
                                             0.0f,
                                             1.2f);
        const float stressNoise = ofClamp(0.5f + 0.5f * signedNoise(centroid.x * 0.020f + iceberg.seed * 0.9f,
                                                                    centroid.z * 0.019f - iceberg.seed * 0.7f,
                                                                    centroid.y * 0.013f + 6.0f),
                                          0.0f,
                                          1.0f);
        Iceberg::Facet facet;
        facet.a = a;
        facet.b = b;
        facet.c = c;
        facet.centroid = centroid;
        facet.normal = normal;
        facet.drift = drift;
        facet.crackA = crackA;
        facet.crackB = crackB;
        facet.color = color;
        facet.fractureThreshold = ofClamp(0.20f + radialFracture * 0.38f + stressNoise * 0.26f, 0.16f, 0.92f);
        facet.fractureScale = ofLerp(0.62f, 1.42f, stressNoise) * ofLerp(0.82f, 1.22f, radialFracture);
        iceberg.facets.push_back(facet);

        const float edgeAlpha = ofClamp((std::abs(a.y - b.y) + std::abs(b.y - c.y) + std::abs(c.y - a.y)) /
                                        std::max(1.0f, visibleHeight) * 0.10f,
                                        0.0f,
                                        0.12f);
        if (edgeAlpha > 0.035f) {
            addLine(iceberg.rimLines, a, b, withAlphaScale(rimColor, edgeAlpha));
        }
    }

    for (int i = 0; i < sides; ++i) {
        const int next = (i + 1) % sides;
        const float sideFront = (waterRing[i].z + waterRing[next].z) * 0.5f / std::max(1.0f, halfDepth);
        const float softKeelDrop = underwaterDepth * randomRange(rng, 0.48f, 0.76f);
        const glm::vec3 localKeel((underRing[i].x + underRing[next].x) * 0.5f + randomRange(rng, -halfWidth * 0.045f, halfWidth * 0.045f),
                                  -softKeelDrop,
                                  (underRing[i].z + underRing[next].z) * 0.5f + randomRange(rng, -halfDepth * 0.045f, halfDepth * 0.045f));
        addQuad(iceberg.belowWater, underRing[i], underRing[next], waterRing[next], waterRing[i],
                underColor.getLerped(underEdgeColor, ofClamp(sideFront * 0.22f + 0.12f, 0.0f, 0.28f)));
        addTriangle(iceberg.belowWater, underRing[i], underRing[next], localKeel, withAlphaScale(underColor, 0.76f));
        addTriangle(iceberg.belowWater, keelTip, underRing[next], underRing[i], withAlphaScale(underColor, 0.46f));
    }
}

glm::vec3 ArcticAuroraSceneLayer::icebergWorldPosition(const Iceberg& iceberg) const {
    if (iceberg.orbitRadius <= 0.0f) {
        glm::vec3 position = iceberg.position;
        position.y = waterSurfaceYAt(position.x, position.z, sceneTime_) + iceberg.position.y;
        return position;
    }

    const float angle = iceberg.orbitAngle + sceneTime_ * iceberg.driftSpeed;
    const float radialDrift = std::sin(sceneTime_ * 0.085f + iceberg.seed * 2.1f) * 18.0f +
        signedNoise(iceberg.seed * 0.37f, sceneTime_ * 0.035f, 12.0f) * 9.0f;
    const float bob = std::sin(sceneTime_ * 0.42f + iceberg.bobPhase) * iceberg.bobAmount +
        std::sin(sceneTime_ * 0.19f + iceberg.seed) * iceberg.bobAmount * 0.35f;
    glm::vec3 position = polarPoint(angle, std::max(20.0f, iceberg.orbitRadius + radialDrift), 0.0f);
    position.y = waterSurfaceYAt(position.x, position.z, sceneTime_) + iceberg.position.y + bob;
    return position;
}

float ArcticAuroraSceneLayer::icebergWorldYaw(const Iceberg& iceberg) const {
    const float driftYaw = iceberg.orbitRadius > 0.0f
        ? ofRadToDeg(-(iceberg.orbitAngle + sceneTime_ * iceberg.driftSpeed)) + 90.0f
        : iceberg.yaw;
    return ofLerp(iceberg.yaw, driftYaw, 0.34f) +
        std::sin(sceneTime_ * 0.13f + iceberg.seed * 1.7f) * 4.0f;
}

void ArcticAuroraSceneLayer::drawIceberg(const Iceberg& iceberg, float alpha) const {
    const float scale = paramIcebergScale_ * iceberg.scale;
    const glm::vec3 position = icebergWorldPosition(iceberg);
    const float yaw = icebergWorldYaw(iceberg);

    ofPushMatrix();
    ofTranslate(position.x, position.y, position.z);
    ofRotateYDeg(yaw);
    ofScale(scale, scale, scale);

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    glDepthMask(GL_TRUE);
    drawMeshWithAlpha(iceberg.belowWater, alpha);
    if (!iceberg.facets.empty() && paramIcebergBreakup_ > 0.001f) {
        ofMesh fracturedTop;
        fracturedTop.setMode(OF_PRIMITIVE_TRIANGLES);
        ofMesh fractureLines;
        fractureLines.setMode(OF_PRIMITIVE_LINES);
        const float breakupClock = sceneTime_ * paramIcebergBreakupSpeed_ +
            paramIcebergBreakup_ * 0.16f +
            ofNoise(iceberg.seed * 0.37f, 11.0f) * 0.10f;
        const float maturity = ofClamp(paramIcebergBreakup_ * breakupClock, 0.0f, 1.0f);
        const ofFloatColor crackColor = colorFrom(0.58f, 0.92f, 1.0f, 0.26f);

        for (const auto& facet : iceberg.facets) {
            const float progress = smootherStep(ofMap(maturity,
                                                       facet.fractureThreshold,
                                                       1.0f,
                                                       0.0f,
                                                       1.0f,
                                                       true));
            const float subtleProgress = progress * progress;
            const glm::vec3 driftOffset = facet.drift * (subtleProgress * 18.0f * facet.fractureScale) +
                facet.normal * (progress * 2.4f);
            const float shrink = 1.0f - progress * 0.035f;
            const glm::vec3 a = facet.centroid + (facet.a - facet.centroid) * shrink + driftOffset;
            const glm::vec3 b = facet.centroid + (facet.b - facet.centroid) * shrink + driftOffset;
            const glm::vec3 c = facet.centroid + (facet.c - facet.centroid) * shrink + driftOffset;
            ofFloatColor color = facet.color;
            color.a = ofClamp(color.a * ofLerp(1.0f, 0.88f, progress), 0.0f, 1.0f);
            addTriangle(fracturedTop, a, b, c, color);

            if (progress > 0.035f) {
                const glm::vec3 lineOffset = driftOffset * 0.42f + facet.normal * (progress * 0.8f);
                ofFloatColor lineColor = crackColor;
                lineColor.a *= ofClamp(progress * paramIcebergBreakup_ * 1.45f, 0.0f, 1.0f);
                addLine(fractureLines,
                        facet.crackA + lineOffset,
                        facet.crackB + lineOffset,
                        lineColor);
            }
        }
        drawMeshWithAlpha(fracturedTop, alpha);

        if (fractureLines.getNumVertices() > 0) {
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            glDepthMask(GL_FALSE);
#ifndef TARGET_OPENGLES
            glLineWidth(1.0f);
#endif
            drawMeshWithAlpha(fractureLines, alpha);
            glDepthMask(GL_TRUE);
        }
    } else {
        drawMeshWithAlpha(iceberg.aboveWater, alpha);
    }

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    glDepthMask(GL_FALSE);
#ifndef TARGET_OPENGLES
    glLineWidth(1.0f);
#endif
    drawMeshWithAlpha(iceberg.rimLines, alpha * paramIcebergRimLight_ * 0.45f);
    glDepthMask(GL_TRUE);
    ofPopMatrix();
}
