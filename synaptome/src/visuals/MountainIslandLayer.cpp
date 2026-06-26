#include "MountainIslandLayer.h"

#include "../io/AudioAnalysisBus.h"
#include "ofGraphics.h"
#include "ofMath.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <utility>

namespace {
    constexpr float kMaxDt = 1.0f / 20.0f;

    struct TriangulationTriangle {
        int a = 0;
        int b = 0;
        int c = 0;
    };

    struct TriangulationEdge {
        int a = 0;
        int b = 0;
    };

    float followAmount(float smoothing) {
        return 1.0f - ofClamp(smoothing, 0.0f, 0.98f);
    }

    float signedNoise(float x, float y, float z) {
        return ofNoise(x, y, z) * 2.0f - 1.0f;
    }

    float smootherStep(float value) {
        const float t = ofClamp(value, 0.0f, 1.0f);
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
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

    float randomRange(std::mt19937& rng, float minValue, float maxValue) {
        std::uniform_real_distribution<float> dist(minValue, maxValue);
        return dist(rng);
    }

    int randomInt(std::mt19937& rng, int minValue, int maxValue) {
        std::uniform_int_distribution<int> dist(minValue, maxValue);
        return dist(rng);
    }

    glm::vec3 polarPoint(float theta, float radius, float y) {
        return glm::vec3(std::cos(theta) * radius, y, std::sin(theta) * radius);
    }

    float orient2d(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    void addBoundaryEdge(std::vector<TriangulationEdge>& edges, const TriangulationEdge& edge) {
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
        const float dmax = std::max(1.0f, std::max(span.x, span.y));
        const glm::vec2 center = (minPoint + maxPoint) * 0.5f;
        const int superA = static_cast<int>(points.size());
        const int superB = superA + 1;
        const int superC = superA + 2;
        points.push_back(center + glm::vec2(-2.2f * dmax - 10.0f, -dmax - 10.0f));
        points.push_back(center + glm::vec2(0.0f, 2.2f * dmax + 20.0f));
        points.push_back(center + glm::vec2(2.2f * dmax + 10.0f, -dmax - 10.0f));

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
                addBoundaryEdge(polygon, { tri.a, tri.b });
                addBoundaryEdge(polygon, { tri.b, tri.c });
                addBoundaryEdge(polygon, { tri.c, tri.a });
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

    void addTriangleWithColors(ofMesh& mesh,
                               const glm::vec3& a,
                               const glm::vec3& b,
                               const glm::vec3& c,
                               const ofFloatColor& colorA,
                               const ofFloatColor& colorB,
                               const ofFloatColor& colorC) {
        const int base = static_cast<int>(mesh.getNumVertices());
        mesh.addVertex(a);
        mesh.addColor(colorA);
        mesh.addVertex(b);
        mesh.addColor(colorB);
        mesh.addVertex(c);
        mesh.addColor(colorC);
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

    glm::vec2 safeNormalize(const glm::vec2& value, const glm::vec2& fallback = glm::vec2(1.0f, 0.0f)) {
        const float len = glm::length(value);
        if (len <= 0.0001f || !std::isfinite(len)) {
            return fallback;
        }
        return value / len;
    }

    void addCloudTriangle(ofMesh& mesh,
                          const glm::vec3& a,
                          const glm::vec3& b,
                          const glm::vec3& c,
                          const ofFloatColor& baseColor,
                          const ofFloatColor& shadeColor,
                          float alpha) {
        glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
        if (!std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z)) {
            normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        const float topness = ofClamp(normal.y * 0.5f + 0.5f, 0.0f, 1.0f);
        const float sun = ofClamp(glm::dot(normal, glm::normalize(glm::vec3(-0.08f, 0.98f, 0.10f))) * 0.5f + 0.5f, 0.0f, 1.0f);
        const ofFloatColor whiteFloor = colorFrom(std::max({ baseColor.r, shadeColor.r, 1.32f }),
                                                  std::max({ baseColor.g, shadeColor.g, 1.32f }),
                                                  std::max({ baseColor.b, shadeColor.b, 1.32f }),
                                                  1.0f);
        const float brightness = ofClamp(1.06f + topness * 0.04f + sun * 0.04f, 1.0f, 1.16f);
        ofFloatColor color = colorFrom(whiteFloor.r * brightness,
                                       whiteFloor.g * brightness,
                                       whiteFloor.b * brightness,
                                       1.0f);
        color.a = alpha * ofClamp(0.96f + topness * 0.04f, 0.0f, 1.0f);
        addTriangle(mesh, a, b, c, color);
    }

    glm::vec3 cloudEllipsoidPoint(const glm::vec3& offset,
                                  const glm::vec3& radii,
                                  float theta,
                                  float phi,
                                  float seed) {
        const float cphi = std::cos(phi);
        const glm::vec3 unit(cphi * std::cos(theta),
                             std::sin(phi),
                             cphi * std::sin(theta));
        const float wrinkle = 1.0f + signedNoise(unit.x * 2.7f + seed,
                                                 unit.y * 2.2f - seed * 0.41f,
                                                 unit.z * 2.5f + seed * 0.23f) * 0.085f;
        return offset + glm::vec3(unit.x * radii.x, unit.y * radii.y, unit.z * radii.z) * wrinkle;
    }

    void addCloudPuff(ofMesh& mesh,
                      const glm::vec3& offset,
                      const glm::vec3& radii,
                      const ofFloatColor& baseColor,
                      const ofFloatColor& shadeColor,
                      float alpha,
                      float seed,
                      int segments,
                      int rings) {
        const int kSegments = std::max(3, segments);
        const int kRings = std::max(2, rings);
        for (int ring = 0; ring < kRings; ++ring) {
            const float v0 = static_cast<float>(ring) / static_cast<float>(kRings);
            const float v1 = static_cast<float>(ring + 1) / static_cast<float>(kRings);
            const float phi0 = ofLerp(-HALF_PI, HALF_PI, v0);
            const float phi1 = ofLerp(-HALF_PI, HALF_PI, v1);
            for (int segment = 0; segment < kSegments; ++segment) {
                const float u0 = static_cast<float>(segment) / static_cast<float>(kSegments);
                const float u1 = static_cast<float>(segment + 1) / static_cast<float>(kSegments);
                const float theta0 = u0 * TWO_PI;
                const float theta1 = u1 * TWO_PI;

                const glm::vec3 p00 = cloudEllipsoidPoint(offset, radii, theta0, phi0, seed);
                const glm::vec3 p10 = cloudEllipsoidPoint(offset, radii, theta1, phi0, seed);
                const glm::vec3 p01 = cloudEllipsoidPoint(offset, radii, theta0, phi1, seed);
                const glm::vec3 p11 = cloudEllipsoidPoint(offset, radii, theta1, phi1, seed);
                addCloudTriangle(mesh, p00, p10, p11, baseColor, shadeColor, alpha);
                addCloudTriangle(mesh, p00, p11, p01, baseColor, shadeColor, alpha);
            }
        }
    }
}

void MountainIslandLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }

    const auto& def = config["defaults"];
    paramEnabled_ = def.value("visible", paramEnabled_);
    paramAlpha_ = def.value("alpha", paramAlpha_);
    paramSceneScale_ = def.value("sceneScale", paramSceneScale_);
    paramSceneOffsetY_ = def.value("sceneOffsetY", paramSceneOffsetY_);
    paramSceneOffsetZ_ = def.value("sceneOffsetZ", paramSceneOffsetZ_);
    paramSpinAngle_ = def.value("spinAngle", paramSpinAngle_);
    paramSpinSpeed_ = def.value("spinSpeed", paramSpinSpeed_);
    paramWaterRadius_ = def.value("waterRadius", paramWaterRadius_);
    paramWaterLevel_ = def.value("waterLevel", paramWaterLevel_);
    paramWaterRimDepth_ = def.value("waterRimDepth", paramWaterRimDepth_);
    paramWaterHighlight_ = def.value("waterHighlight", paramWaterHighlight_);
    paramWaterWaveAmount_ = def.value("waterWaveAmount", paramWaterWaveAmount_);
    paramShoreGlow_ = def.value("shoreGlow", paramShoreGlow_);
    paramWorldDepth_ = def.value("worldDepth", paramWorldDepth_);
    paramSubmergedLandDepth_ = def.value("submergedLandDepth", paramSubmergedLandDepth_);
    paramSolidWorldAlpha_ = def.value("solidWorldAlpha", paramSolidWorldAlpha_);
    paramIslandRadius_ = def.value("islandRadius", paramIslandRadius_);
    paramPointCount_ = def.value("pointCount", paramPointCount_);
    paramBoundaryPoints_ = def.value("boundaryPoints", paramBoundaryPoints_);
    paramTriangleTargetLength_ = def.value("triangleTargetLength", paramTriangleTargetLength_);
    paramMountainHeight_ = def.value("mountainHeight", paramMountainHeight_);
    paramRoughness_ = def.value("roughness", paramRoughness_);
    paramShorelineJitter_ = def.value("shorelineJitter", paramShorelineJitter_);
    paramPeakCount_ = def.value("peakCount", paramPeakCount_);
    paramSnowLine_ = def.value("snowLine", paramSnowLine_);
    paramTreeLine_ = def.value("treeLine", paramTreeLine_);
    paramSandHeight_ = def.value("sandHeight", paramSandHeight_);
    paramUpliftRadius_ = def.value("upliftRadius", paramUpliftRadius_);
    paramUpliftScatter_ = def.value("upliftScatter", paramUpliftScatter_);
    paramUpliftFalloff_ = def.value("upliftFalloff", paramUpliftFalloff_);
    paramSeabedUndulation_ = def.value("seabedUndulation", paramSeabedUndulation_);
    paramEdgeUndulation_ = def.value("edgeUndulation", paramEdgeUndulation_);
    paramWireAlpha_ = def.value("wireAlpha", paramWireAlpha_);
    paramGrowth_ = def.value("growth", paramGrowth_);
    paramAutoGrow_ = def.value("autoGrow", paramAutoGrow_);
    paramGrowthRate_ = def.value("growthRate", paramGrowthRate_);
    paramGrowthCurve_ = def.value("growthCurve", paramGrowthCurve_);
    paramCloudCount_ = def.value("cloudCount", paramCloudCount_);
    paramCloudPuffCount_ = def.value("cloudPuffCount", paramCloudPuffCount_);
    paramCloudFacetSegments_ = def.value("cloudFacetSegments", paramCloudFacetSegments_);
    paramCloudFacetRings_ = def.value("cloudFacetRings", paramCloudFacetRings_);
    paramCloudScale_ = def.value("cloudScale", paramCloudScale_);
    paramCloudDensity_ = def.value("cloudDensity", paramCloudDensity_);
    paramCloudBaseHeight_ = def.value("cloudBaseHeight", paramCloudBaseHeight_);
    paramCloudLayerDepth_ = def.value("cloudLayerDepth", paramCloudLayerDepth_);
    paramCloudClearance_ = def.value("cloudClearance", paramCloudClearance_);
    paramCloudWindAngle_ = def.value("cloudWindAngle", paramCloudWindAngle_);
    paramCloudWindSpeed_ = def.value("cloudWindSpeed", paramCloudWindSpeed_);
    paramCloudTurbulence_ = def.value("cloudTurbulence", paramCloudTurbulence_);
    paramCloudMountainAvoidance_ = def.value("cloudMountainAvoidance", paramCloudMountainAvoidance_);
    paramCloudUpdraft_ = def.value("cloudUpdraft", paramCloudUpdraft_);
    paramCloudShadowAlpha_ = def.value("cloudShadowAlpha", paramCloudShadowAlpha_);
    paramAudioAmount_ = def.value("audioAmount", paramAudioAmount_);
    paramAudioSmoothing_ = def.value("audioSmoothing", paramAudioSmoothing_);
    paramBassLift_ = def.value("bassLift", paramBassLift_);
    paramHighsGlint_ = def.value("highsGlint", paramHighsGlint_);
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
    paramShallowR_ = def.value("shallowR", paramShallowR_);
    paramShallowG_ = def.value("shallowG", paramShallowG_);
    paramShallowB_ = def.value("shallowB", paramShallowB_);
    paramShoreR_ = def.value("shoreR", paramShoreR_);
    paramShoreG_ = def.value("shoreG", paramShoreG_);
    paramShoreB_ = def.value("shoreB", paramShoreB_);
    paramLowlandR_ = def.value("lowlandR", paramLowlandR_);
    paramLowlandG_ = def.value("lowlandG", paramLowlandG_);
    paramLowlandB_ = def.value("lowlandB", paramLowlandB_);
    paramRockR_ = def.value("rockR", paramRockR_);
    paramRockG_ = def.value("rockG", paramRockG_);
    paramRockB_ = def.value("rockB", paramRockB_);
    paramSnowR_ = def.value("snowR", paramSnowR_);
    paramSnowG_ = def.value("snowG", paramSnowG_);
    paramSnowB_ = def.value("snowB", paramSnowB_);
    paramWireR_ = def.value("wireR", paramWireR_);
    paramWireG_ = def.value("wireG", paramWireG_);
    paramWireB_ = def.value("wireB", paramWireB_);
    paramCloudR_ = def.value("cloudR", paramCloudR_);
    paramCloudG_ = def.value("cloudG", paramCloudG_);
    paramCloudB_ = def.value("cloudB", paramCloudB_);
    paramCloudShadeR_ = def.value("cloudShadeR", paramCloudShadeR_);
    paramCloudShadeG_ = def.value("cloudShadeG", paramCloudShadeG_);
    paramCloudShadeB_ = def.value("cloudShadeB", paramCloudShadeB_);

    readColor(def, "skyTopColor", paramSkyTopR_, paramSkyTopG_, paramSkyTopB_);
    readColor(def, "skyHorizonColor", paramSkyHorizonR_, paramSkyHorizonG_, paramSkyHorizonB_);
    readColor(def, "waterColor", paramWaterR_, paramWaterG_, paramWaterB_);
    readColor(def, "shallowColor", paramShallowR_, paramShallowG_, paramShallowB_);
    readColor(def, "shoreColor", paramShoreR_, paramShoreG_, paramShoreB_);
    readColor(def, "lowlandColor", paramLowlandR_, paramLowlandG_, paramLowlandB_);
    readColor(def, "rockColor", paramRockR_, paramRockG_, paramRockB_);
    readColor(def, "snowColor", paramSnowR_, paramSnowG_, paramSnowB_);
    readColor(def, "wireColor", paramWireR_, paramWireG_, paramWireB_);
    readColor(def, "cloudColor", paramCloudR_, paramCloudG_, paramCloudB_);
    readColor(def, "cloudShadeColor", paramCloudShadeR_, paramCloudShadeG_, paramCloudShadeB_);
    clampParams();
}

void MountainIslandLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "generative.mountainIsland" : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "Mountain Island";
    meta.label = "Action: Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    registerFloat(registry, prefix + ".alpha", &paramAlpha_, paramAlpha_, "Alpha: Scene", 0.0f, 1.0f, 0.01f, "normalized");
    registerFloat(registry, prefix + ".sceneScale", &paramSceneScale_, paramSceneScale_, "Scale: Scene", 0.25f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".sceneOffsetY", &paramSceneOffsetY_, paramSceneOffsetY_, "Scale: Scene Offset Y", -500.0f, 500.0f, 1.0f);
    registerFloat(registry, prefix + ".sceneOffsetZ", &paramSceneOffsetZ_, paramSceneOffsetZ_, "Scale: Scene Offset Z", -1000.0f, 1000.0f, 1.0f);
    registerFloat(registry, prefix + ".spinAngle", &paramSpinAngle_, paramSpinAngle_, "Motion: Spin Angle", -180.0f, 180.0f, 0.1f, "deg");
    registerFloat(registry, prefix + ".spinSpeed", &paramSpinSpeed_, paramSpinSpeed_, "Motion: World Spin", -90.0f, 90.0f, 0.1f, "deg");

    registerFloat(registry, prefix + ".waterRadius", &paramWaterRadius_, paramWaterRadius_, "Scale: Sphere Radius", 220.0f, 1800.0f, 5.0f);
    registerFloat(registry, prefix + ".waterLevel", &paramWaterLevel_, paramWaterLevel_, "Scale: Water Level (Fixed)", 0.0f, 0.0f, 1.0f);
    registerFloat(registry, prefix + ".waterRimDepth", &paramWaterRimDepth_, paramWaterRimDepth_, "Scale: Water Rim Depth", 0.0f, 240.0f, 1.0f);
    registerFloat(registry, prefix + ".waterHighlight", &paramWaterHighlight_, paramWaterHighlight_, "Glow: Water Highlight", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".waterWaveAmount", &paramWaterWaveAmount_, paramWaterWaveAmount_, "Motion: Water Shimmer", 0.0f, 18.0f, 0.1f);
    registerFloat(registry, prefix + ".shoreGlow", &paramShoreGlow_, paramShoreGlow_, "Glow: Shoreline", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".submergedLandDepth", &paramSubmergedLandDepth_, paramSubmergedLandDepth_, "Scale: Submerged Land Depth", 0.0f, 260.0f, 1.0f);
    registerFloat(registry, prefix + ".solidWorldAlpha", &paramSolidWorldAlpha_, paramSolidWorldAlpha_, "Alpha: Solid Lower World", 0.0f, 1.0f, 0.01f);

    registerFloat(registry, prefix + ".islandRadius", &paramIslandRadius_, paramIslandRadius_, "Scale: Island Radius", 120.0f, 1400.0f, 5.0f);
    registerFloat(registry, prefix + ".pointCount", &paramPointCount_, paramPointCount_, "Count: Terrain Samples", 80.0f, 1200.0f, 1.0f);
    registerFloat(registry, prefix + ".boundaryPoints", &paramBoundaryPoints_, paramBoundaryPoints_, "Count: Shore Samples", 32.0f, 220.0f, 1.0f);
    registerFloat(registry, prefix + ".triangleTargetLength", &paramTriangleTargetLength_, paramTriangleTargetLength_, "Scale: Triangle Target Edge", 45.0f, 260.0f, 1.0f);
    registerFloat(registry, prefix + ".mountainHeight", &paramMountainHeight_, paramMountainHeight_, "Scale: Mountain Height", 20.0f, 900.0f, 5.0f);
    registerFloat(registry, prefix + ".roughness", &paramRoughness_, paramRoughness_, "Force: Terrain Roughness", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".shorelineJitter", &paramShorelineJitter_, paramShorelineJitter_, "Force: Shoreline Jitter", 0.0f, 0.42f, 0.01f);
    registerFloat(registry, prefix + ".peakCount", &paramPeakCount_, paramPeakCount_, "Count: Mountain Peaks", 1.0f, 9.0f, 1.0f);
    registerFloat(registry, prefix + ".snowLine", &paramSnowLine_, paramSnowLine_, "Scale: Snow Line", 0.25f, 0.95f, 0.01f);
    registerFloat(registry, prefix + ".treeLine", &paramTreeLine_, paramTreeLine_, "Scale: Tree Line", 0.05f, 0.72f, 0.01f);
    registerFloat(registry, prefix + ".sandHeight", &paramSandHeight_, paramSandHeight_, "Scale: Sand Height", 8.0f, 260.0f, 1.0f);
    registerFloat(registry, prefix + ".upliftRadius", &paramUpliftRadius_, paramUpliftRadius_, "Scale: Uplift Radius", 0.18f, 1.80f, 0.01f);
    registerFloat(registry, prefix + ".upliftScatter", &paramUpliftScatter_, paramUpliftScatter_, "Scale: Uplift Scatter", 0.0f, 1.25f, 0.01f);
    registerFloat(registry, prefix + ".upliftFalloff", &paramUpliftFalloff_, paramUpliftFalloff_, "Force: Uplift Falloff", 0.25f, 5.0f, 0.01f);
    registerFloat(registry, prefix + ".seabedUndulation", &paramSeabedUndulation_, paramSeabedUndulation_, "Scale: Seabed Undulation", 0.0f, 140.0f, 1.0f);
    registerFloat(registry, prefix + ".edgeUndulation", &paramEdgeUndulation_, paramEdgeUndulation_, "Scale: Sphere Edge Undulation", 0.0f, 180.0f, 1.0f);
    registerFloat(registry, prefix + ".wireAlpha", &paramWireAlpha_, paramWireAlpha_, "Alpha: Delaunay Edges", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".growth", &paramGrowth_, paramGrowth_, "Growth: Land Emergence", 0.0f, 1.0f, 0.01f, "normalized");
    registerFloat(registry, prefix + ".growthRate", &paramGrowthRate_, paramGrowthRate_, "Growth: Auto Rate", 0.0f, 0.30f, 0.001f);
    registerFloat(registry, prefix + ".growthCurve", &paramGrowthCurve_, paramGrowthCurve_, "Growth: Emergence Curve", 0.35f, 3.0f, 0.01f);

    meta = {};
    meta.group = "Mountain Island";
    meta.label = "Growth: Auto Grow";
    registry.addBool(prefix + ".autoGrow", &paramAutoGrow_, paramAutoGrow_, meta);

    meta = {};
    meta.group = "Mountain Island";
    meta.label = "Action: Reseed Growth";
    registry.addBool(prefix + ".growthReseed", &paramGrowthReseedRequested_, paramGrowthReseedRequested_, meta);

    registerFloat(registry, prefix + ".cloudCount", &paramCloudCount_, paramCloudCount_, "Count: 3D Clouds", 0.0f, 18.0f, 1.0f);
    registerFloat(registry, prefix + ".cloudPuffCount", &paramCloudPuffCount_, paramCloudPuffCount_, "Count: Cloud Puffs", 3.0f, 18.0f, 1.0f);
    registerFloat(registry, prefix + ".cloudFacetSegments", &paramCloudFacetSegments_, paramCloudFacetSegments_, "Count: Cloud Facet Segments", 6.0f, 18.0f, 1.0f);
    registerFloat(registry, prefix + ".cloudFacetRings", &paramCloudFacetRings_, paramCloudFacetRings_, "Count: Cloud Facet Rings", 4.0f, 10.0f, 1.0f);
    registerFloat(registry, prefix + ".cloudScale", &paramCloudScale_, paramCloudScale_, "Scale: 3D Clouds", 0.25f, 2.2f, 0.01f);
    registerFloat(registry, prefix + ".cloudDensity", &paramCloudDensity_, paramCloudDensity_, "Alpha: Cloud Density", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".cloudBaseHeight", &paramCloudBaseHeight_, paramCloudBaseHeight_, "Scale: Cloud Base Height", 120.0f, 1200.0f, 5.0f);
    registerFloat(registry, prefix + ".cloudLayerDepth", &paramCloudLayerDepth_, paramCloudLayerDepth_, "Scale: Cloud Layer Depth", 0.0f, 420.0f, 5.0f);
    registerFloat(registry, prefix + ".cloudClearance", &paramCloudClearance_, paramCloudClearance_, "Scale: Mountain Clearance", 40.0f, 420.0f, 5.0f);
    registerFloat(registry, prefix + ".cloudWindAngle", &paramCloudWindAngle_, paramCloudWindAngle_, "Motion: Cloud Wind Angle", -180.0f, 180.0f, 1.0f, "deg");
    registerFloat(registry, prefix + ".cloudWindSpeed", &paramCloudWindSpeed_, paramCloudWindSpeed_, "Motion: Cloud Wind Speed", 0.0f, 160.0f, 1.0f);
    registerFloat(registry, prefix + ".cloudTurbulence", &paramCloudTurbulence_, paramCloudTurbulence_, "Force: Cloud Turbulence", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".cloudMountainAvoidance", &paramCloudMountainAvoidance_, paramCloudMountainAvoidance_, "Force: Mountain Avoidance", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".cloudUpdraft", &paramCloudUpdraft_, paramCloudUpdraft_, "Force: Cloud Vertical Drift", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".cloudShadowAlpha", &paramCloudShadowAlpha_, paramCloudShadowAlpha_, "Alpha: Cloud Shadows", 0.0f, 0.6f, 0.01f);

    registerFloat(registry, prefix + ".audioAmount", &paramAudioAmount_, paramAudioAmount_, "Audio: Amount", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".audioSmoothing", &paramAudioSmoothing_, paramAudioSmoothing_, "Audio: Smoothing", 0.0f, 0.98f, 0.01f);
    registerFloat(registry, prefix + ".bassLift", &paramBassLift_, paramBassLift_, "Audio: Bass Mountain Lift", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".highsGlint", &paramHighsGlint_, paramHighsGlint_, "Audio: Water Glint", 0.0f, 3.0f, 0.01f);

    meta = {};
    meta.group = "Mountain Island";
    meta.label = "Action: Reseed";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);
    registerFloat(registry, prefix + ".seed", &paramSeed_, paramSeed_, "Seed: Island", 0.0f, 99999999.0f, 1.0f);

    registerFloat(registry, prefix + ".skyTopR", &paramSkyTopR_, paramSkyTopR_, "Color: Sky Top R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".skyTopG", &paramSkyTopG_, paramSkyTopG_, "Color: Sky Top G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".skyTopB", &paramSkyTopB_, paramSkyTopB_, "Color: Sky Top B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".skyHorizonR", &paramSkyHorizonR_, paramSkyHorizonR_, "Color: Sky Horizon R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".skyHorizonG", &paramSkyHorizonG_, paramSkyHorizonG_, "Color: Sky Horizon G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".skyHorizonB", &paramSkyHorizonB_, paramSkyHorizonB_, "Color: Sky Horizon B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".waterR", &paramWaterR_, paramWaterR_, "Color: Water R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".waterG", &paramWaterG_, paramWaterG_, "Color: Water G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".waterB", &paramWaterB_, paramWaterB_, "Color: Water B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".shallowR", &paramShallowR_, paramShallowR_, "Color: Shallow R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".shallowG", &paramShallowG_, paramShallowG_, "Color: Shallow G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".shallowB", &paramShallowB_, paramShallowB_, "Color: Shallow B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".shoreR", &paramShoreR_, paramShoreR_, "Color: Shore R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".shoreG", &paramShoreG_, paramShoreG_, "Color: Shore G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".shoreB", &paramShoreB_, paramShoreB_, "Color: Shore B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".lowlandR", &paramLowlandR_, paramLowlandR_, "Color: Lowland R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".lowlandG", &paramLowlandG_, paramLowlandG_, "Color: Lowland G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".lowlandB", &paramLowlandB_, paramLowlandB_, "Color: Lowland B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".rockR", &paramRockR_, paramRockR_, "Color: Rock R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".rockG", &paramRockG_, paramRockG_, "Color: Rock G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".rockB", &paramRockB_, paramRockB_, "Color: Rock B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".snowR", &paramSnowR_, paramSnowR_, "Color: Snow R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".snowG", &paramSnowG_, paramSnowG_, "Color: Snow G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".snowB", &paramSnowB_, paramSnowB_, "Color: Snow B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".wireR", &paramWireR_, paramWireR_, "Color: Wire R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".wireG", &paramWireG_, paramWireG_, "Color: Wire G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".wireB", &paramWireB_, paramWireB_, "Color: Wire B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".cloudR", &paramCloudR_, paramCloudR_, "Color: Cloud R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".cloudG", &paramCloudG_, paramCloudG_, "Color: Cloud G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".cloudB", &paramCloudB_, paramCloudB_, "Color: Cloud B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".cloudShadeR", &paramCloudShadeR_, paramCloudShadeR_, "Color: Cloud Shade R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".cloudShadeG", &paramCloudShadeG_, paramCloudShadeG_, "Color: Cloud Shade G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".cloudShadeB", &paramCloudShadeB_, paramCloudShadeB_, "Color: Cloud Shade B", 0.0f, 1.5f, 0.01f);

    rebuildIsland();
}

void MountainIslandLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) {
        return;
    }

    clampParams();
    if (paramGrowthReseedRequested_) {
        paramSeed_ = std::fmod(paramSeed_ + 1.0f, 99999999.0f);
        paramGrowth_ = 0.0f;
        paramReseedRequested_ = true;
        paramGrowthReseedRequested_ = false;
    }
    if (paramAutoGrow_ && paramGrowthRate_ > 0.0f) {
        paramGrowth_ = ofClamp(paramGrowth_ + ofClamp(params.dt, 0.0f, kMaxDt) * paramGrowthRate_, 0.0f, 1.0f);
    }
    updateAudioState(ofClamp(params.dt, 0.0f, kMaxDt));
    updateClouds(ofClamp(params.dt, 0.0f, kMaxDt), params.time);
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    spinPhase_ += params.dt * paramSpinSpeed_ * (1.0f + mids_ * audio * 0.10f);

    const std::uint32_t desiredSeed = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    const int desiredPointCount = static_cast<int>(std::round(paramPointCount_));
    const int desiredBoundaryCount = static_cast<int>(std::round(paramBoundaryPoints_));
    const int desiredPeakCount = static_cast<int>(std::round(paramPeakCount_));
    const int desiredCloudCount = static_cast<int>(std::round(paramCloudCount_));
    const int desiredCloudPuffCount = static_cast<int>(std::round(paramCloudPuffCount_));
    const float signature = meshSignature();
    const float cloudsSignature = cloudSignature();

    if (paramReseedRequested_ ||
        desiredSeed != seedState_ ||
        desiredPointCount != pointCountState_ ||
        desiredBoundaryCount != boundaryPointState_ ||
        desiredPeakCount != peakCountState_ ||
        std::abs(signature - meshSignatureState_) > 0.0001f) {
        rebuildIsland();
        paramReseedRequested_ = false;
    }

    if (desiredCloudCount != cloudCountState_ ||
        desiredCloudPuffCount != cloudPuffCountState_ ||
        std::abs(cloudsSignature - cloudSignatureState_) > 0.0001f) {
        resetClouds();
    }
}

void MountainIslandLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f) {
        return;
    }
    if (islandMesh_.getNumVertices() == 0) {
        rebuildIsland();
    }

    const float alpha = ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float lift = 1.0f + bass_ * paramBassLift_ * audio * 0.035f;

    ofPushStyle();

    params.camera.begin();
    ofEnableDepthTest();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);

    ofPushMatrix();
    ofTranslate(0.0f, paramSceneOffsetY_, paramSceneOffsetZ_);
    ofScale(paramSceneScale_, paramSceneScale_ * lift, paramSceneScale_);
    ofRotateYDeg(paramSpinAngle_ + spinPhase_);

    drawSolidWorld(alpha);
    drawIsland(alpha);
    drawWaterDisk(alpha, params.time);
    drawShore(alpha);
    drawCloudShadows(alpha, params.time);
    drawWaterHighlights(alpha, params.time);
    drawClouds(alpha, params.time);

    ofPopMatrix();
    glDepthMask(GL_TRUE);
    ofDisableDepthTest();
    params.camera.end();
    ofPopStyle();
}

void MountainIslandLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

void MountainIslandLayer::registerFloat(ParameterRegistry& registry,
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
    meta.group = "Mountain Island";
    meta.label = label;
    meta.range.min = min;
    meta.range.max = max;
    meta.range.step = step;
    meta.units = units;
    meta.description = description;
    registry.addFloat(id, target, initial, meta);
}

void MountainIslandLayer::readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b) {
    if (!defaults.contains(key) || !defaults[key].is_array() || defaults[key].size() < 3) {
        return;
    }
    r = defaults[key][0].get<float>();
    g = defaults[key][1].get<float>();
    b = defaults[key][2].get<float>();
}

void MountainIslandLayer::clampParams() {
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramSceneScale_ = ofClamp(paramSceneScale_, 0.25f, 2.0f);
    paramSceneOffsetY_ = ofClamp(paramSceneOffsetY_, -500.0f, 500.0f);
    paramSceneOffsetZ_ = ofClamp(paramSceneOffsetZ_, -1000.0f, 1000.0f);
    paramSpinAngle_ = ofClamp(paramSpinAngle_, -180.0f, 180.0f);
    paramSpinSpeed_ = ofClamp(paramSpinSpeed_, -90.0f, 90.0f);
    paramWaterRadius_ = ofClamp(paramWaterRadius_, 220.0f, 1800.0f);
    paramWaterLevel_ = 0.0f;
    paramWaterRimDepth_ = ofClamp(paramWaterRimDepth_, 0.0f, 240.0f);
    paramWaterHighlight_ = ofClamp(paramWaterHighlight_, 0.0f, 2.0f);
    paramWaterWaveAmount_ = ofClamp(paramWaterWaveAmount_, 0.0f, 18.0f);
    paramShoreGlow_ = ofClamp(paramShoreGlow_, 0.0f, 2.0f);
    paramWorldDepth_ = paramWaterRadius_;
    paramSubmergedLandDepth_ = ofClamp(paramSubmergedLandDepth_, 4.0f, std::max(4.0f, paramWaterRadius_ - 8.0f));
    paramSolidWorldAlpha_ = ofClamp(paramSolidWorldAlpha_, 0.0f, 1.0f);
    paramIslandRadius_ = std::min(ofClamp(paramIslandRadius_, 120.0f, 1400.0f), paramWaterRadius_ * 0.88f);
    paramPointCount_ = std::round(ofClamp(paramPointCount_, 80.0f, 1200.0f));
    paramBoundaryPoints_ = std::round(ofClamp(paramBoundaryPoints_, 32.0f, 220.0f));
    paramTriangleTargetLength_ = ofClamp(paramTriangleTargetLength_, 45.0f, 260.0f);
    paramMountainHeight_ = ofClamp(paramMountainHeight_, 20.0f, 900.0f);
    paramRoughness_ = ofClamp(paramRoughness_, 0.0f, 2.0f);
    paramShorelineJitter_ = ofClamp(paramShorelineJitter_, 0.0f, 0.42f);
    paramPeakCount_ = std::round(ofClamp(paramPeakCount_, 1.0f, 9.0f));
    paramSnowLine_ = ofClamp(paramSnowLine_, 0.25f, 0.95f);
    paramTreeLine_ = std::min(ofClamp(paramTreeLine_, 0.05f, 0.72f), paramSnowLine_ - 0.04f);
    paramSandHeight_ = ofClamp(paramSandHeight_, 8.0f, 260.0f);
    paramUpliftRadius_ = ofClamp(paramUpliftRadius_, 0.18f, 1.80f);
    paramUpliftScatter_ = ofClamp(paramUpliftScatter_, 0.0f, 1.25f);
    paramUpliftFalloff_ = ofClamp(paramUpliftFalloff_, 0.25f, 5.0f);
    paramSeabedUndulation_ = ofClamp(paramSeabedUndulation_, 0.0f, 140.0f);
    paramEdgeUndulation_ = ofClamp(paramEdgeUndulation_, 0.0f, 180.0f);
    paramWireAlpha_ = ofClamp(paramWireAlpha_, 0.0f, 1.0f);
    paramGrowth_ = ofClamp(paramGrowth_, 0.0f, 1.0f);
    paramGrowthRate_ = ofClamp(paramGrowthRate_, 0.0f, 0.30f);
    paramGrowthCurve_ = ofClamp(paramGrowthCurve_, 0.35f, 3.0f);
    paramCloudCount_ = std::round(ofClamp(paramCloudCount_, 0.0f, 18.0f));
    paramCloudPuffCount_ = std::round(ofClamp(paramCloudPuffCount_, 3.0f, 18.0f));
    paramCloudFacetSegments_ = std::round(ofClamp(paramCloudFacetSegments_, 6.0f, 18.0f));
    paramCloudFacetRings_ = std::round(ofClamp(paramCloudFacetRings_, 4.0f, 10.0f));
    paramCloudScale_ = ofClamp(paramCloudScale_, 0.25f, 2.2f);
    paramCloudDensity_ = ofClamp(paramCloudDensity_, 0.0f, 1.0f);
    paramCloudBaseHeight_ = ofClamp(paramCloudBaseHeight_, 120.0f, 1200.0f);
    paramCloudLayerDepth_ = ofClamp(paramCloudLayerDepth_, 0.0f, 420.0f);
    paramCloudClearance_ = ofClamp(paramCloudClearance_, 40.0f, 420.0f);
    paramCloudWindAngle_ = ofClamp(paramCloudWindAngle_, -180.0f, 180.0f);
    paramCloudWindSpeed_ = ofClamp(paramCloudWindSpeed_, 0.0f, 160.0f);
    paramCloudTurbulence_ = ofClamp(paramCloudTurbulence_, 0.0f, 2.0f);
    paramCloudMountainAvoidance_ = ofClamp(paramCloudMountainAvoidance_, 0.0f, 3.0f);
    paramCloudUpdraft_ = ofClamp(paramCloudUpdraft_, 0.0f, 3.0f);
    paramCloudShadowAlpha_ = ofClamp(paramCloudShadowAlpha_, 0.0f, 0.6f);
    paramAudioAmount_ = ofClamp(paramAudioAmount_, 0.0f, 2.0f);
    paramAudioSmoothing_ = ofClamp(paramAudioSmoothing_, 0.0f, 0.98f);
    paramBassLift_ = ofClamp(paramBassLift_, 0.0f, 2.0f);
    paramHighsGlint_ = ofClamp(paramHighsGlint_, 0.0f, 3.0f);
    paramSeed_ = std::round(ofClamp(paramSeed_, 0.0f, 99999999.0f));

    paramSkyTopR_ = ofClamp(paramSkyTopR_, 0.0f, 1.5f);
    paramSkyTopG_ = ofClamp(paramSkyTopG_, 0.0f, 1.5f);
    paramSkyTopB_ = ofClamp(paramSkyTopB_, 0.0f, 1.5f);
    paramSkyHorizonR_ = ofClamp(paramSkyHorizonR_, 0.0f, 1.5f);
    paramSkyHorizonG_ = ofClamp(paramSkyHorizonG_, 0.0f, 1.5f);
    paramSkyHorizonB_ = ofClamp(paramSkyHorizonB_, 0.0f, 1.5f);
    paramWaterR_ = ofClamp(paramWaterR_, 0.0f, 1.5f);
    paramWaterG_ = ofClamp(paramWaterG_, 0.0f, 1.5f);
    paramWaterB_ = ofClamp(paramWaterB_, 0.0f, 1.5f);
    paramShallowR_ = ofClamp(paramShallowR_, 0.0f, 1.5f);
    paramShallowG_ = ofClamp(paramShallowG_, 0.0f, 1.5f);
    paramShallowB_ = ofClamp(paramShallowB_, 0.0f, 1.5f);
    paramShoreR_ = ofClamp(paramShoreR_, 0.0f, 1.5f);
    paramShoreG_ = ofClamp(paramShoreG_, 0.0f, 1.5f);
    paramShoreB_ = ofClamp(paramShoreB_, 0.0f, 1.5f);
    paramLowlandR_ = ofClamp(paramLowlandR_, 0.0f, 1.5f);
    paramLowlandG_ = ofClamp(paramLowlandG_, 0.0f, 1.5f);
    paramLowlandB_ = ofClamp(paramLowlandB_, 0.0f, 1.5f);
    paramRockR_ = ofClamp(paramRockR_, 0.0f, 1.5f);
    paramRockG_ = ofClamp(paramRockG_, 0.0f, 1.5f);
    paramRockB_ = ofClamp(paramRockB_, 0.0f, 1.5f);
    paramSnowR_ = ofClamp(paramSnowR_, 0.0f, 1.5f);
    paramSnowG_ = ofClamp(paramSnowG_, 0.0f, 1.5f);
    paramSnowB_ = ofClamp(paramSnowB_, 0.0f, 1.5f);
    paramWireR_ = ofClamp(paramWireR_, 0.0f, 1.5f);
    paramWireG_ = ofClamp(paramWireG_, 0.0f, 1.5f);
    paramWireB_ = ofClamp(paramWireB_, 0.0f, 1.5f);
    paramCloudR_ = ofClamp(paramCloudR_, 0.0f, 1.5f);
    paramCloudG_ = ofClamp(paramCloudG_, 0.0f, 1.5f);
    paramCloudB_ = ofClamp(paramCloudB_, 0.0f, 1.5f);
    paramCloudShadeR_ = ofClamp(paramCloudShadeR_, 0.0f, 1.5f);
    paramCloudShadeG_ = ofClamp(paramCloudShadeG_, 0.0f, 1.5f);
    paramCloudShadeB_ = ofClamp(paramCloudShadeB_, 0.0f, 1.5f);
}

float MountainIslandLayer::growthAmount() const {
    return std::pow(ofClamp(paramGrowth_, 0.0f, 1.0f), paramGrowthCurve_);
}

float MountainIslandLayer::shelfTopY() const {
    return paramWaterLevel_ - paramSubmergedLandDepth_;
}

float MountainIslandLayer::sphereRadiusAtY(float y) const {
    const float sphereRadius = std::max(1.0f, paramWaterRadius_);
    const float localY = ofClamp(y - paramWaterLevel_, -sphereRadius, sphereRadius);
    return std::sqrt(std::max(0.0f, sphereRadius * sphereRadius - localY * localY));
}

float MountainIslandLayer::landLayerRadius() const {
    return sphereRadiusAtY(shelfTopY());
}

float MountainIslandLayer::hemisphereYForRadius(float radius) const {
    const float sphereRadius = std::max(1.0f, paramWaterRadius_);
    const float clampedRadius = ofClamp(radius, 0.0f, sphereRadius);
    return paramWaterLevel_ - std::sqrt(std::max(0.0f, sphereRadius * sphereRadius - clampedRadius * clampedRadius));
}

float MountainIslandLayer::seaFloorBaseYFor(const glm::vec2& point) const {
    const float radius = glm::length(point);
    const float shellY = hemisphereYForRadius(radius);
    const float edgeFade = smootherStep(ofMap(radius, landLayerRadius() * 0.72f, landLayerRadius(), 0.0f, 1.0f, true));
    const float undulation = signedNoise(point.x * 0.0031f + seedState_ * 0.00007f,
                                         point.y * 0.0034f - seedState_ * 0.00009f,
                                         2.0f) *
        paramSeabedUndulation_ * (1.0f - edgeFade * 0.45f);
    return ofClamp(shelfTopY() + undulation, shellY + 2.0f, paramWaterLevel_ - 4.0f);
}

glm::vec3 MountainIslandLayer::grownTerrainPoint(const glm::vec3& basePoint, const glm::vec3& finalPoint, float growth) const {
    return glm::mix(basePoint, finalPoint, growth);
}

float MountainIslandLayer::meshSignature() const {
    return paramIslandRadius_ * 0.017f +
        paramWaterLevel_ * 0.019f +
        paramWorldDepth_ * 0.021f +
        paramSubmergedLandDepth_ * 0.022f +
        paramTriangleTargetLength_ * 0.0225f +
        paramMountainHeight_ * 0.023f +
        paramRoughness_ * 37.0f +
        paramShorelineJitter_ * 41.0f +
        paramSnowLine_ * 43.0f +
        paramTreeLine_ * 47.0f +
        paramSandHeight_ * 49.0f +
        paramUpliftRadius_ * 50.0f +
        paramUpliftScatter_ * 51.0f +
        paramUpliftFalloff_ * 52.0f +
        paramSeabedUndulation_ * 54.0f +
        paramEdgeUndulation_ * 56.0f +
        paramShoreR_ * 53.0f +
        paramShoreG_ * 59.0f +
        paramShoreB_ * 61.0f +
        paramLowlandR_ * 67.0f +
        paramLowlandG_ * 71.0f +
        paramLowlandB_ * 73.0f +
        paramRockR_ * 79.0f +
        paramRockG_ * 83.0f +
        paramRockB_ * 89.0f +
        paramSnowR_ * 97.0f +
        paramSnowG_ * 101.0f +
        paramSnowB_ * 103.0f +
        paramWireR_ * 107.0f +
        paramWireG_ * 109.0f +
        paramWireB_ * 113.0f;
}

float MountainIslandLayer::cloudSignature() const {
    return paramCloudScale_ * 0.031f +
        paramCloudDensity_ * 0.037f +
        paramCloudFacetSegments_ * 0.039f +
        paramCloudFacetRings_ * 0.040f +
        paramCloudBaseHeight_ * 0.041f +
        paramCloudLayerDepth_ * 0.043f +
        paramCloudClearance_ * 0.047f +
        paramCloudR_ * 0.053f +
        paramCloudG_ * 0.059f +
        paramCloudB_ * 0.061f +
        paramCloudShadeR_ * 0.067f +
        paramCloudShadeG_ * 0.071f +
        paramCloudShadeB_ * 0.073f;
}

void MountainIslandLayer::rebuildIsland() {
    clampParams();
    seedState_ = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    pointCountState_ = static_cast<int>(std::round(paramPointCount_));
    boundaryPointState_ = static_cast<int>(std::round(paramBoundaryPoints_));
    peakCountState_ = static_cast<int>(std::round(paramPeakCount_));
    meshSignatureState_ = meshSignature();

    std::mt19937 rng(seedState_);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    peaks_.clear();
    peaks_.reserve(static_cast<std::size_t>(peakCountState_));
    for (int i = 0; i < peakCountState_; ++i) {
        const float theta = randomRange(rng, 0.0f, TWO_PI);
        const float radius = std::pow(unit(rng), 0.62f) * landLayerRadius() * paramUpliftScatter_;
        Peak peak;
        peak.position = glm::vec2(std::cos(theta) * radius, std::sin(theta) * radius);
        peak.radius = paramIslandRadius_ * paramUpliftRadius_ * randomRange(rng, 0.70f, 1.45f);
        peak.height = randomRange(rng, 0.62f, 1.22f);
        peaks_.push_back(peak);
    }

    shoreline2d_.clear();
    shoreline3d_.clear();
    shoreline2d_.reserve(static_cast<std::size_t>(boundaryPointState_));
    shoreline3d_.reserve(static_cast<std::size_t>(boundaryPointState_));

    const float landRadius = landLayerRadius();
    const float shelfY = shelfTopY();
    const float targetEdge = std::max(45.0f, paramTriangleTargetLength_);
    std::vector<glm::vec2> points2d;
    std::vector<glm::vec3> basePoints3d;
    std::vector<glm::vec3> points3d;
    points2d.reserve(static_cast<std::size_t>(pointCountState_ + boundaryPointState_ * 4 + 64));
    basePoints3d.reserve(points2d.capacity());
    points3d.reserve(points2d.capacity());

    auto addInteriorPoint = [&](const glm::vec2& point) {
        if (glm::length(point) >= landRadius * 0.996f) {
            return;
        }
        const float normalizedRadius = glm::length(point) / std::max(1.0f, paramIslandRadius_);
        const float baseY = seaFloorBaseYFor(point);
        points2d.push_back(point);
        basePoints3d.push_back(glm::vec3(point.x, baseY, point.y));
        points3d.push_back(glm::vec3(point.x, terrainHeightFor(point, normalizedRadius, baseY), point.y));
    };

    for (int i = 0; i < boundaryPointState_; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(boundaryPointState_);
        const float baseTheta = t * TWO_PI;
        const float theta = baseTheta + randomRange(rng, -0.026f, 0.026f);
        const float edgeY = ofClamp(shelfY +
                                        signedNoise(std::cos(baseTheta) * 2.0f + seedState_ * 0.00005f,
                                                    std::sin(baseTheta) * 2.0f - seedState_ * 0.00003f,
                                                    3.0f) *
                                            paramEdgeUndulation_,
                                    -paramWaterRadius_ + 6.0f,
                                    paramWaterLevel_ - 4.0f);
        const float edgeRadius = sphereRadiusAtY(edgeY);
        const glm::vec2 shorePoint(std::cos(theta) * edgeRadius,
                                   std::sin(theta) * edgeRadius);
        const glm::vec3 basePoint(shorePoint.x, edgeY, shorePoint.y);
        shoreline2d_.push_back(shorePoint);
        shoreline3d_.push_back(basePoint);
        points2d.push_back(shorePoint);
        basePoints3d.push_back(basePoint);
        points3d.push_back(basePoint);
    }

    const int ringCount = std::max(5, std::min(18, static_cast<int>(std::ceil(landRadius / targetEdge))));
    for (int ring = 1; ring < ringCount; ++ring) {
        const float v = static_cast<float>(ring) / static_cast<float>(ringCount);
        const float radius = landRadius * std::pow(v, 0.92f) * 0.982f;
        const int segments = std::max(14, static_cast<int>(std::ceil(TWO_PI * radius / targetEdge)));
        for (int segment = 0; segment < segments; ++segment) {
            const float t = (static_cast<float>(segment) + randomRange(rng, -0.18f, 0.18f)) / static_cast<float>(segments);
            const float theta = t * TWO_PI;
            const float jitter = randomRange(rng, -0.16f, 0.16f) * targetEdge * (0.35f + v * 0.40f);
            const float sampleRadius = ofClamp(radius + jitter, targetEdge * 0.35f, landRadius * 0.985f);
            addInteriorPoint(glm::vec2(std::cos(theta) * sampleRadius, std::sin(theta) * sampleRadius));
        }
    }

    for (int i = 0; i < boundaryPointState_; i += 2) {
        const float theta = static_cast<float>(i) / static_cast<float>(boundaryPointState_) * TWO_PI +
            randomRange(rng, -0.05f, 0.05f);
        const float radius = std::min(paramIslandRadius_ * randomRange(rng, 0.72f, 1.04f), landRadius * 0.92f);
        addInteriorPoint(glm::vec2(std::cos(theta) * radius, std::sin(theta) * radius));
    }

    points2d.push_back(glm::vec2(0.0f, 0.0f));
    const float centerBaseY = seaFloorBaseYFor(glm::vec2(0.0f, 0.0f));
    basePoints3d.push_back(glm::vec3(0.0f, centerBaseY, 0.0f));
    points3d.push_back(glm::vec3(0.0f, terrainHeightFor(glm::vec2(0.0f, 0.0f), 0.0f, centerBaseY), 0.0f));

    int attempts = 0;
    const int desiredTotal = pointCountState_ + boundaryPointState_ + boundaryPointState_ / 2 + 1;
    while (static_cast<int>(points2d.size()) < desiredTotal && attempts < pointCountState_ * 24) {
        ++attempts;
        const float theta = randomRange(rng, 0.0f, TWO_PI);
        const bool centralSample = unit(rng) < 0.72f;
        const float radius = centralSample
            ? std::pow(unit(rng), 0.54f) * paramIslandRadius_ * randomRange(rng, 0.05f, 1.08f)
            : std::pow(unit(rng), 0.78f) * landRadius * randomRange(rng, 0.18f, 0.98f);
        const glm::vec2 point(std::cos(theta) * radius, std::sin(theta) * radius);
        if (!pointInPolygon(point, shoreline2d_)) {
            continue;
        }
        addInteriorPoint(point);
    }

    seaFloorBaseMesh_.clear();
    islandMesh_.clear();
    islandSkirt_.clear();
    ridgeLines_.clear();
    shoreLine_.clear();
    seaFloorBaseMesh_.setMode(OF_PRIMITIVE_TRIANGLES);
    islandMesh_.setMode(OF_PRIMITIVE_TRIANGLES);
    islandSkirt_.setMode(OF_PRIMITIVE_TRIANGLES);
    ridgeLines_.setMode(OF_PRIMITIVE_LINES);
    shoreLine_.setMode(OF_PRIMITIVE_LINES);

    const auto triangles = delaunayTriangulate(points2d);
    const ofFloatColor lineColor = colorFrom(paramWireR_, paramWireG_, paramWireB_, 0.16f);
    const float maxTriangleEdge = paramTriangleTargetLength_ * 2.35f;
    for (const auto& tri : triangles) {
        const glm::vec2 centroid2d = (points2d[tri.a] + points2d[tri.b] + points2d[tri.c]) / 3.0f;
        if (!pointInPolygon(centroid2d, shoreline2d_)) {
            continue;
        }
        const float edgeAB = glm::length(points2d[tri.a] - points2d[tri.b]);
        const float edgeBC = glm::length(points2d[tri.b] - points2d[tri.c]);
        const float edgeCA = glm::length(points2d[tri.c] - points2d[tri.a]);
        if (std::max({ edgeAB, edgeBC, edgeCA }) > maxTriangleEdge) {
            continue;
        }

        glm::vec3 a = points3d[tri.a];
        glm::vec3 b = points3d[tri.b];
        glm::vec3 c = points3d[tri.c];
        glm::vec3 baseA = basePoints3d[tri.a];
        glm::vec3 baseB = basePoints3d[tri.b];
        glm::vec3 baseC = basePoints3d[tri.c];
        if (glm::cross(b - a, c - a).y < 0.0f) {
            std::swap(b, c);
            std::swap(baseB, baseC);
        }

        const ofFloatColor color = terrainColorFor(a, b, c, 1.0f);
        addTriangle(seaFloorBaseMesh_, baseA, baseB, baseC, color);
        addTriangle(islandMesh_, a, b, c, color);

        const float heightDelta = (std::abs(a.y - b.y) + std::abs(b.y - c.y) + std::abs(c.y - a.y)) /
            std::max(1.0f, paramMountainHeight_);
        const float edgeAlpha = ofClamp(0.18f + heightDelta * 1.65f, 0.12f, 1.0f);
        addLine(ridgeLines_, a, b, withAlphaScale(lineColor, edgeAlpha));
        addLine(ridgeLines_, b, c, withAlphaScale(lineColor, edgeAlpha));
        addLine(ridgeLines_, c, a, withAlphaScale(lineColor, edgeAlpha));
    }

    const ofFloatColor skirtTop = colorFrom(paramShoreR_ * 0.72f, paramShoreG_ * 0.66f, paramShoreB_ * 0.52f, 0.72f);
    const ofFloatColor skirtDeep = colorFrom(paramWaterR_ * 0.42f, paramWaterG_ * 0.46f, paramWaterB_ * 0.50f, 0.48f);
    const ofFloatColor shoreColor = colorFrom(paramShallowR_, paramShallowG_, paramShallowB_, 0.62f);
    for (std::size_t i = 0; i < shoreline3d_.size(); ++i) {
        const std::size_t next = (i + 1) % shoreline3d_.size();
        const glm::vec3 topA = shoreline3d_[i];
        const glm::vec3 topB = shoreline3d_[next];
        const glm::vec3 bottomA(topA.x * 0.93f, paramWaterLevel_ - paramWaterRimDepth_ * 0.70f, topA.z * 0.93f);
        const glm::vec3 bottomB(topB.x * 0.93f, paramWaterLevel_ - paramWaterRimDepth_ * 0.70f, topB.z * 0.93f);
        const float facing = ofClamp((topA.z + topB.z) / std::max(1.0f, paramIslandRadius_) * 0.18f + 0.55f, 0.0f, 1.0f);
        addQuad(islandSkirt_, bottomA, bottomB, topB, topA, skirtDeep.getLerped(skirtTop, facing));
        addLine(shoreLine_, topA + glm::vec3(0.0f, 2.0f, 0.0f), topB + glm::vec3(0.0f, 2.0f, 0.0f), shoreColor);
    }

    resetClouds();
}

void MountainIslandLayer::resetClouds() {
    clampParams();
    cloudCountState_ = static_cast<int>(std::round(paramCloudCount_));
    cloudPuffCountState_ = static_cast<int>(std::round(paramCloudPuffCount_));
    cloudSignatureState_ = cloudSignature();

    std::mt19937 rng(seedState_ ^ 0x9E3779B9u);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    clouds_.clear();
    clouds_.reserve(static_cast<std::size_t>(cloudCountState_));

    const float windRadians = ofDegToRad(paramCloudWindAngle_);
    const glm::vec2 windDir(std::cos(windRadians), std::sin(windRadians));
    for (int i = 0; i < cloudCountState_; ++i) {
        const float theta = randomRange(rng, 0.0f, TWO_PI);
        const float radius = std::sqrt(unit(rng)) * paramWaterRadius_ * randomRange(rng, 0.18f, 0.92f);
        const glm::vec2 xz(std::cos(theta) * radius, std::sin(theta) * radius);
        const float layer = randomRange(rng, -0.45f, 0.55f) * paramCloudLayerDepth_;

        CloudCluster cloud;
        cloud.baseY = paramWaterLevel_ + paramCloudBaseHeight_ + layer;
        cloud.position = glm::vec3(xz.x, cloud.baseY, xz.y);
        cloud.velocity = windDir * paramCloudWindSpeed_ * randomRange(rng, 0.42f, 0.86f);
        cloud.yaw = randomRange(rng, 0.0f, 360.0f);
        cloud.seed = randomRange(rng, 0.0f, 10000.0f);
        cloud.scale = randomRange(rng, 0.72f, 1.26f);
        cloud.bobPhase = randomRange(rng, 0.0f, TWO_PI);
        buildCloudMesh(cloud, rng);
        clouds_.push_back(std::move(cloud));
    }
}

void MountainIslandLayer::updateClouds(float dt, float timeSeconds) {
    if (clouds_.empty() || paramCloudDensity_ <= 0.0f || paramCloudWindSpeed_ <= 0.0f) {
        return;
    }

    const float windRadians = ofDegToRad(paramCloudWindAngle_);
    const glm::vec2 windDir(std::cos(windRadians), std::sin(windRadians));
    const glm::vec2 wind = windDir * paramCloudWindSpeed_;
    const float limit = paramWaterRadius_ * 1.08f;
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const glm::vec2 windCross(-windDir.y, windDir.x);

    for (auto& cloud : clouds_) {
        const glm::vec2 xz(cloud.position.x, cloud.position.z);
        const float ground = terrainHeightAt(xz);
        const glm::vec2 gradient = terrainGradientAt(xz);
        const float gradientStrength = ofClamp(glm::length(gradient) * 4.0f, 0.0f, 1.0f);
        glm::vec2 tangent = safeNormalize(glm::vec2(-gradient.y, gradient.x), windCross);
        if (glm::dot(tangent, windDir) < 0.0f) {
            tangent *= -1.0f;
        }

        const float obstacleY = ground + paramCloudClearance_;
        const float altitudeConflict = smootherStep(ofMap(obstacleY - cloud.baseY,
                                                          -paramCloudClearance_ * 1.10f,
                                                          paramCloudClearance_ * 0.85f,
                                                          0.0f,
                                                          1.0f,
                                                          true));
        const float terrainInfluence = smootherStep(ofMap(ground - paramWaterLevel_,
                                                          paramMountainHeight_ * 0.05f,
                                                          paramMountainHeight_ * 0.52f,
                                                          0.0f,
                                                          1.0f,
                                                          true));
        const float avoidance = std::max(altitudeConflict, terrainInfluence * gradientStrength * 0.72f) *
            paramCloudMountainAvoidance_;
        const glm::vec2 radialOut = safeNormalize(xz, windDir);
        const glm::vec2 curl(signedNoise(cloud.position.x * 0.0025f + cloud.seed,
                                         cloud.position.z * 0.0025f,
                                         timeSeconds * 0.045f),
                             signedNoise(cloud.position.x * 0.0025f - cloud.seed * 0.37f,
                                         cloud.position.z * 0.0025f + 17.0f,
                                         timeSeconds * 0.045f + 4.0f));

        const glm::vec2 targetVelocity =
            wind +
            safeNormalize(curl, windCross) * paramCloudWindSpeed_ * paramCloudTurbulence_ * 0.44f +
            tangent * paramCloudWindSpeed_ * avoidance * 1.65f +
            radialOut * paramCloudWindSpeed_ * altitudeConflict * 0.46f;
        cloud.velocity = glm::mix(cloud.velocity, targetVelocity, ofClamp(dt * (0.75f + avoidance * 0.65f), 0.0f, 1.0f));
        cloud.position.x += cloud.velocity.x * dt;
        cloud.position.z += cloud.velocity.y * dt;
        cloud.position.y = cloud.baseY + paramCloudUpdraft_ *
            signedNoise(cloud.seed * 0.013f, timeSeconds * 0.035f, 8.0f) *
            paramCloudLayerDepth_ * 0.035f;

        cloud.yaw += dt * (2.0f + mids_ * audio * 2.5f) + avoidance * dt * 10.0f;

        const float distanceFromCenter = glm::length(glm::vec2(cloud.position.x, cloud.position.z));
        if (distanceFromCenter > limit) {
            const glm::vec2 wrapped = safeNormalize(glm::vec2(cloud.position.x, cloud.position.z), -windDir) * -limit * 0.72f;
            cloud.position.x = wrapped.x;
            cloud.position.z = wrapped.y;
            cloud.position.y = cloud.baseY;
        }
    }
}

void MountainIslandLayer::updateAudioState(float dt) {
    const auto snapshot = AudioAnalysisBus::instance().snapshot();
    const float follow = followAmount(paramAudioSmoothing_);
    hasAudio_ = snapshot.valid;
    if (snapshot.valid) {
        level_ = ofLerp(level_, snapshot.level, follow);
        peak_ = ofLerp(peak_, snapshot.peak, follow);
        bass_ = ofLerp(bass_, snapshot.bass, follow);
        mids_ = ofLerp(mids_, snapshot.mids, follow);
        highs_ = ofLerp(highs_, snapshot.highs, follow);
    } else {
        const float release = ofClamp(dt * 1.8f + follow * 0.12f, 0.0f, 1.0f);
        level_ = ofLerp(level_, 0.0f, release);
        peak_ = ofLerp(peak_, 0.0f, release);
        bass_ = ofLerp(bass_, 0.0f, release);
        mids_ = ofLerp(mids_, 0.0f, release);
        highs_ = ofLerp(highs_, 0.0f, release);
    }
}

float MountainIslandLayer::terrainHeightFor(const glm::vec2& point, float normalizedRadius, float baseY) const {
    (void)normalizedRadius;
    const float landRadius = std::max(1.0f, landLayerRadius());
    const float edgePin = 1.0f - smootherStep(ofMap(glm::length(point), landRadius * 0.70f, landRadius * 0.995f, 0.0f, 1.0f, true));
    const float radialFold = std::max(0.0f, signedNoise(point.x * 0.006f + seedState_ * 0.00011f,
                                                       point.y * 0.006f - seedState_ * 0.00007f,
                                                       0.5f));
    const float ridge = std::pow(std::max(0.0f, signedNoise(point.x * 0.014f - seedState_ * 0.00017f,
                                                           point.y * 0.012f + seedState_ * 0.00013f,
                                                           4.0f)),
                                 1.45f);
    const float detail = signedNoise(point.x * 0.034f + 7.0f,
                                     point.y * 0.036f - 13.0f,
                                     seedState_ * 0.00001f);

    float peakField = 0.0f;
    for (const auto& peak : peaks_) {
        const float distance = glm::length(point - peak.position) / std::max(1.0f, peak.radius);
        peakField += std::exp(-distance * distance * paramUpliftFalloff_) * peak.height;
    }

    const float shorelineFold = signedNoise(point.x * 0.004f + seedState_ * 0.00019f,
                                            point.y * 0.004f - seedState_ * 0.00023f,
                                            6.0f) * paramShorelineJitter_ * edgePin * 0.18f;
    const float broadUplift = smootherStep(1.0f - std::exp(-peakField * 0.68f));
    const float summitLift = std::pow(std::max(0.0f, peakField - 0.72f), 1.18f) * 0.075f;
    const float detailMask = smootherStep(ofMap(broadUplift, 0.18f, 0.92f, 0.0f, 1.0f, true));
    const float height01 =
        broadUplift * 0.78f +
        summitLift +
        broadUplift * radialFold * 0.16f * paramRoughness_ +
        ridge * detailMask * 0.16f * paramRoughness_ +
        detail * detailMask * 0.040f * paramRoughness_ +
        shorelineFold;
    const float uplift = std::max(0.0f, height01) * (paramSubmergedLandDepth_ + paramMountainHeight_);
    return baseY + uplift * edgePin;
}

float MountainIslandLayer::terrainHeightAt(const glm::vec2& point) const {
    if (shoreline2d_.empty() || !pointInPolygon(point, shoreline2d_)) {
        return seaFloorBaseYFor(point);
    }
    const float normalizedRadius = glm::length(point) / std::max(1.0f, paramIslandRadius_);
    const float baseY = seaFloorBaseYFor(point);
    const float finalHeight = terrainHeightFor(point, normalizedRadius, baseY);
    return ofLerp(baseY, finalHeight, growthAmount());
}

glm::vec2 MountainIslandLayer::terrainGradientAt(const glm::vec2& point) const {
    const float eps = std::max(8.0f, paramIslandRadius_ * 0.018f);
    const float hL = terrainHeightAt(glm::vec2(point.x - eps, point.y));
    const float hR = terrainHeightAt(glm::vec2(point.x + eps, point.y));
    const float hD = terrainHeightAt(glm::vec2(point.x, point.y - eps));
    const float hU = terrainHeightAt(glm::vec2(point.x, point.y + eps));
    return glm::vec2(hR - hL, hU - hD) / std::max(1.0f, eps * 2.0f);
}

void MountainIslandLayer::buildCloudMesh(CloudCluster& cloud, std::mt19937& rng) const {
    cloud.mesh.clear();
    cloud.mesh.setMode(OF_PRIMITIVE_TRIANGLES);

    const int puffCount = static_cast<int>(std::round(paramCloudPuffCount_));
    const int facetSegments = static_cast<int>(std::round(paramCloudFacetSegments_));
    const int facetRings = static_cast<int>(std::round(paramCloudFacetRings_));
    const ofFloatColor baseColor = colorFrom(paramCloudR_, paramCloudG_, paramCloudB_, 1.0f);
    const ofFloatColor shadeColor = colorFrom(paramCloudShadeR_, paramCloudShadeG_, paramCloudShadeB_, 1.0f);

    for (int i = 0; i < puffCount; ++i) {
        const float t = puffCount > 1 ? static_cast<float>(i) / static_cast<float>(puffCount - 1) : 0.5f;
        const float centerBias = 1.0f - std::abs(t - 0.5f) * 1.35f;
        const glm::vec3 offset(randomRange(rng, -88.0f, 88.0f) + (t - 0.5f) * 96.0f,
                               randomRange(rng, -18.0f, 24.0f) + centerBias * 10.0f,
                               randomRange(rng, -46.0f, 46.0f));
        const glm::vec3 radii(randomRange(rng, 34.0f, 70.0f) * (0.86f + centerBias * 0.20f),
                              randomRange(rng, 16.0f, 34.0f) * (0.90f + centerBias * 0.18f),
                              randomRange(rng, 24.0f, 54.0f));
        const float puffAlpha = randomRange(rng, 0.92f, 1.0f);
        addCloudPuff(cloud.mesh,
                     offset,
                     radii,
                     baseColor,
                     shadeColor,
                     puffAlpha,
                     cloud.seed * 0.013f + static_cast<float>(i) * 1.71f,
                     facetSegments,
                     facetRings);
    }
}

ofFloatColor MountainIslandLayer::terrainColorForPoint(const glm::vec3& point,
                                                       const glm::vec3& normal,
                                                       float alpha) const {
    const float aboveWater = point.y - paramWaterLevel_;
    const float height01 = ofClamp(aboveWater / std::max(1.0f, paramMountainHeight_), 0.0f, 1.0f);
    const float slope = ofClamp(1.0f - normal.y, 0.0f, 1.0f);
    const float sun = ofClamp(glm::dot(normal, glm::normalize(glm::vec3(-0.10f, 0.97f, 0.08f))) * 0.5f + 0.5f, 0.0f, 1.0f);

    const ofFloatColor shore = colorFrom(paramShoreR_, paramShoreG_, paramShoreB_, alpha);
    const ofFloatColor lowland = colorFrom(paramLowlandR_, paramLowlandG_, paramLowlandB_, alpha);
    const ofFloatColor rock = colorFrom(paramRockR_, paramRockG_, paramRockB_, alpha);
    const ofFloatColor snow = colorFrom(paramSnowR_, paramSnowG_, paramSnowB_, alpha);

    const float sandToGreen = smootherStep(ofMap(aboveWater,
                                                 paramSandHeight_,
                                                 paramSandHeight_ + paramMountainHeight_ * 0.24f,
                                                 0.0f,
                                                 1.0f,
                                                 true));
    const float greenToRock = smootherStep(ofMap(height01, paramTreeLine_, paramSnowLine_, 0.0f, 1.0f, true));
    const float rockToSnow = smootherStep(ofMap(height01, paramSnowLine_, 1.0f, 0.0f, 1.0f, true));
    const float waterContact = 1.0f - smootherStep(ofMap(aboveWater,
                                                         0.0f,
                                                         paramSandHeight_,
                                                         0.0f,
                                                         1.0f,
                                                         true));

    ofFloatColor color = shore.getLerped(lowland, sandToGreen);
    color = color.getLerped(rock, greenToRock * (0.68f + slope * 0.24f));
    color = color.getLerped(snow, rockToSnow);
    color = color.getLerped(shore, waterContact);

    const float shade = ofClamp(0.66f + sun * 0.38f - slope * 0.08f, 0.48f, 1.16f);
    color.r *= shade;
    color.g *= shade;
    color.b *= shade;
    color.a = alpha * ofClamp(0.86f + sun * 0.14f, 0.0f, 1.0f);
    return color;
}

ofFloatColor MountainIslandLayer::terrainColorFor(const glm::vec3& a,
                                                  const glm::vec3& b,
                                                  const glm::vec3& c,
                                                  float alpha) const {
    glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
    if (!std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z)) {
        normal = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    return terrainColorForPoint((a + b + c) / 3.0f, normal, alpha);
}

void MountainIslandLayer::drawSky(const LayerDrawParams& params, float alpha) const {
    const float width = static_cast<float>(std::max(1, params.viewport.x));
    const float height = static_cast<float>(std::max(1, params.viewport.y));

    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);
    ofDisableDepthTest();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);

    ofMesh sky;
    sky.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    sky.addVertex(glm::vec3(0.0f, 0.0f, 0.0f));
    sky.addColor(colorFrom(paramSkyTopR_, paramSkyTopG_, paramSkyTopB_, alpha));
    sky.addVertex(glm::vec3(0.0f, height, 0.0f));
    sky.addColor(colorFrom(paramSkyHorizonR_, paramSkyHorizonG_, paramSkyHorizonB_, alpha));
    sky.addVertex(glm::vec3(width, 0.0f, 0.0f));
    sky.addColor(colorFrom(paramSkyTopR_, paramSkyTopG_, paramSkyTopB_, alpha));
    sky.addVertex(glm::vec3(width, height, 0.0f));
    sky.addColor(colorFrom(paramSkyHorizonR_, paramSkyHorizonG_, paramSkyHorizonB_, alpha));
    sky.draw();

    ofEnableBlendMode(OF_BLENDMODE_ADD);
    ofSetColor(colorFrom(paramShallowR_ * 0.60f, paramShallowG_ * 0.52f, paramShallowB_ * 0.46f, alpha * 0.055f));
    ofDrawEllipse(width * 0.52f, height * 0.58f, width * 0.74f, height * 0.18f);
    ofPopView();
}

void MountainIslandLayer::drawWaterDisk(float alpha, float timeSeconds) const {
    const int radialSegments = 26;
    const int angularSegments = 128;
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float glintEnergy = 0.35f + level_ * audio * 0.28f + highs_ * paramHighsGlint_ * audio * 0.12f;

    ofMesh surface;
    surface.setMode(OF_PRIMITIVE_TRIANGLES);
    for (int row = 0; row <= radialSegments; ++row) {
        const float v = static_cast<float>(row) / static_cast<float>(radialSegments);
        const float radius = paramWaterRadius_ * std::sqrt(v);
        for (int col = 0; col <= angularSegments; ++col) {
            const float u = static_cast<float>(col) / static_cast<float>(angularSegments);
            const float theta = u * TWO_PI;
            const float edge = smootherStep(ofMap(v, 0.82f, 1.0f, 0.0f, 1.0f, true));
            const float islandProximity = 1.0f - smootherStep(ofMap(radius, paramIslandRadius_ * 0.82f, paramWaterRadius_, 0.0f, 1.0f, true));
            const float shimmer = std::max(0.0f, signedNoise(std::cos(theta) * 2.8f + v * 1.7f,
                                                             std::sin(theta) * 2.8f - v * 1.3f,
                                                             timeSeconds * 0.08f));
            const ofFloatColor deep = colorFrom(paramWaterR_, paramWaterG_, paramWaterB_, alpha * (0.60f - edge * 0.18f));
            const ofFloatColor shallow = colorFrom(paramShallowR_, paramShallowG_, paramShallowB_, alpha * 0.58f);
            const ofFloatColor color = deep
                .getLerped(shallow, islandProximity * 0.42f + shimmer * glintEnergy * 0.08f)
                .getLerped(colorFrom(paramSkyHorizonR_, paramSkyHorizonG_, paramSkyHorizonB_, alpha * 0.30f), edge * 0.28f);
            surface.addVertex(polarPoint(theta, radius, paramWaterLevel_));
            surface.addColor(color);
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

    ofMesh rim;
    rim.setMode(OF_PRIMITIVE_TRIANGLES);
    const ofFloatColor rimTop = colorFrom(paramShallowR_ * 0.56f, paramShallowG_ * 0.50f, paramShallowB_ * 0.48f, alpha * 0.42f);
    const ofFloatColor rimBottom = colorFrom(paramWaterR_ * 0.22f, paramWaterG_ * 0.24f, paramWaterB_ * 0.28f, alpha * 0.64f);
    const float waterBottomY = shelfTopY();
    const float waterBottomRadius = landLayerRadius();
    for (int col = 0; col < angularSegments; ++col) {
        const float theta0 = static_cast<float>(col) / static_cast<float>(angularSegments) * TWO_PI;
        const float theta1 = static_cast<float>(col + 1) / static_cast<float>(angularSegments) * TWO_PI;
        const glm::vec3 topA = polarPoint(theta0, paramWaterRadius_, paramWaterLevel_);
        const glm::vec3 topB = polarPoint(theta1, paramWaterRadius_, paramWaterLevel_);
        const glm::vec3 bottomA = polarPoint(theta0, waterBottomRadius, waterBottomY);
        const glm::vec3 bottomB = polarPoint(theta1, waterBottomRadius, waterBottomY);
        addQuad(rim, bottomA, bottomB, topB, topA, rimBottom.getLerped(rimTop, 0.44f));
    }

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    glDepthMask(GL_FALSE);
    surface.draw();
    rim.draw();
    glDepthMask(GL_TRUE);
}

void MountainIslandLayer::drawWaterHighlights(float alpha, float timeSeconds) const {
    if (paramWaterHighlight_ <= 0.0f) {
        return;
    }

    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float glint = paramWaterHighlight_ * (0.28f + highs_ * paramHighsGlint_ * audio + peak_ * audio * 0.16f);
    if (glint <= 0.004f) {
        return;
    }

    ofMesh arcs;
    arcs.setMode(OF_PRIMITIVE_LINES);
    const int rings = 18;
    const int arcCount = 22;
    const ofFloatColor colorA = colorFrom(paramShallowR_ * 1.16f, paramShallowG_ * 1.14f, paramShallowB_ * 1.10f, alpha * glint * 0.16f);
    const ofFloatColor colorB = colorFrom(paramWireR_, paramWireG_, paramWireB_, alpha * glint * 0.10f);

    for (int ring = 2; ring < rings; ++ring) {
        const float v = static_cast<float>(ring) / static_cast<float>(rings - 1);
        const float radius = ofLerp(paramIslandRadius_ * 0.86f, paramWaterRadius_ * 0.96f, smootherStep(v));
        for (int i = 0; i < arcCount; ++i) {
            const float t = (static_cast<float>(i) + 0.5f) / static_cast<float>(arcCount);
            const float skip = ofNoise(t * 8.0f + ring * 0.3f, timeSeconds * 0.22f);
            if (skip < 0.48f) {
                continue;
            }
            const float theta = t * TWO_PI + signedNoise(v * 3.0f, t * 5.0f, timeSeconds * 0.05f) * 0.12f;
            const float arc = ofLerp(0.018f, 0.060f, 1.0f - v) * (0.75f + skip * 0.40f);
            const float y = paramWaterLevel_ + 2.6f + std::sin(timeSeconds * 0.86f + t * 17.0f) * paramWaterWaveAmount_ * 0.10f;
            addLine(arcs,
                    polarPoint(theta - arc, radius, y),
                    polarPoint(theta + arc, radius, y),
                    colorA.getLerped(colorB, v));
        }
    }

    ofEnableBlendMode(OF_BLENDMODE_ADD);
    glDepthMask(GL_FALSE);
#ifndef TARGET_OPENGLES
    glLineWidth(1.0f);
#endif
    arcs.draw();
    glDepthMask(GL_TRUE);
}

void MountainIslandLayer::drawSolidWorld(float alpha) const {
    if (paramSolidWorldAlpha_ <= 0.0f) {
        return;
    }

    const int radialSegments = 34;
    const int angularSegments = 144;
    const float baseAlpha = alpha * paramSolidWorldAlpha_;
    const float sphereRadius = paramWaterRadius_;

    ofMesh lowerShell;
    lowerShell.setMode(OF_PRIMITIVE_TRIANGLES);
    const ofFloatColor deepRock = colorFrom(paramRockR_ * 0.40f, paramRockG_ * 0.38f, paramRockB_ * 0.36f, baseAlpha * 0.98f);
    const ofFloatColor litCrust = colorFrom(paramRockR_ * 0.60f, paramRockG_ * 0.56f, paramRockB_ * 0.52f, baseAlpha * 0.92f);

    lowerShell.addVertex(glm::vec3(0.0f, paramWaterLevel_ - sphereRadius, 0.0f));
    lowerShell.addColor(deepRock);

    for (int row = 1; row <= radialSegments; ++row) {
        const float v = static_cast<float>(row) / static_cast<float>(radialSegments);
        const float phi = v * HALF_PI;
        const float radius = std::sin(phi) * sphereRadius;
        const float y = paramWaterLevel_ - std::cos(phi) * sphereRadius;
        const ofFloatColor rowColor = deepRock.getLerped(litCrust, smootherStep(v) * 0.72f);
        for (int col = 0; col <= angularSegments; ++col) {
            const float u = static_cast<float>(col) / static_cast<float>(angularSegments);
            const float theta = u * TWO_PI;
            lowerShell.addVertex(polarPoint(theta, radius, y));
            lowerShell.addColor(rowColor);
        }
    }

    for (int col = 0; col < angularSegments; ++col) {
        lowerShell.addIndex(0);
        lowerShell.addIndex(1 + col);
        lowerShell.addIndex(1 + col + 1);
    }

    for (int row = 1; row < radialSegments; ++row) {
        for (int col = 0; col < angularSegments; ++col) {
            const int ring0 = 1 + (row - 1) * (angularSegments + 1);
            const int ring1 = 1 + row * (angularSegments + 1);
            const int i00 = ring0 + col;
            const int i10 = i00 + 1;
            const int i01 = ring1 + col;
            const int i11 = ring1 + col + 1;
            lowerShell.addIndex(i00);
            lowerShell.addIndex(i01);
            lowerShell.addIndex(i11);
            lowerShell.addIndex(i00);
            lowerShell.addIndex(i11);
            lowerShell.addIndex(i10);
        }
    }

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    glDepthMask(GL_TRUE);
    lowerShell.draw();
}

void MountainIslandLayer::drawIsland(float alpha) const {
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    glDepthMask(GL_TRUE);
    if (islandMesh_.getNumVertices() == 0 || seaFloorBaseMesh_.getNumVertices() == 0) {
        return;
    }

    const float growth = growthAmount();
    ofMesh grownTerrain;
    grownTerrain.setMode(OF_PRIMITIVE_TRIANGLES);

    const auto& baseVertices = seaFloorBaseMesh_.getVertices();
    const auto& vertices = islandMesh_.getVertices();
    const auto& indices = islandMesh_.getIndices();
    if (!indices.empty()) {
        for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
            const auto ia = static_cast<std::size_t>(indices[i]);
            const auto ib = static_cast<std::size_t>(indices[i + 1]);
            const auto ic = static_cast<std::size_t>(indices[i + 2]);
            if (ia >= vertices.size() || ib >= vertices.size() || ic >= vertices.size() ||
                ia >= baseVertices.size() || ib >= baseVertices.size() || ic >= baseVertices.size()) {
                continue;
            }
            const glm::vec3 a = grownTerrainPoint(baseVertices[ia], vertices[ia], growth);
            const glm::vec3 b = grownTerrainPoint(baseVertices[ib], vertices[ib], growth);
            const glm::vec3 c = grownTerrainPoint(baseVertices[ic], vertices[ic], growth);
            glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
            if (!std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z)) {
                normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
            addTriangleWithColors(grownTerrain,
                                  a,
                                  b,
                                  c,
                                  terrainColorForPoint(a, normal, alpha),
                                  terrainColorForPoint(b, normal, alpha),
                                  terrainColorForPoint(c, normal, alpha));
        }
    } else {
        for (std::size_t i = 0; i + 2 < vertices.size() && i + 2 < baseVertices.size(); i += 3) {
            const glm::vec3 a = grownTerrainPoint(baseVertices[i], vertices[i], growth);
            const glm::vec3 b = grownTerrainPoint(baseVertices[i + 1], vertices[i + 1], growth);
            const glm::vec3 c = grownTerrainPoint(baseVertices[i + 2], vertices[i + 2], growth);
            glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
            if (!std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z)) {
                normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
            addTriangleWithColors(grownTerrain,
                                  a,
                                  b,
                                  c,
                                  terrainColorForPoint(a, normal, alpha),
                                  terrainColorForPoint(b, normal, alpha),
                                  terrainColorForPoint(c, normal, alpha));
        }
    }

    grownTerrain.draw();
}

void MountainIslandLayer::drawShore(float alpha) const {
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    glDepthMask(GL_FALSE);
#ifndef TARGET_OPENGLES
    glLineWidth(1.0f);
#endif
    const float growth = growthAmount();
    if (paramWireAlpha_ > 0.0f && islandMesh_.getNumVertices() > 0 && seaFloorBaseMesh_.getNumVertices() > 0) {
        ofMesh grownLines;
        grownLines.setMode(OF_PRIMITIVE_LINES);
        const auto& baseVertices = seaFloorBaseMesh_.getVertices();
        const auto& vertices = islandMesh_.getVertices();
        const auto& indices = islandMesh_.getIndices();
        const ofFloatColor lineColor = colorFrom(paramWireR_, paramWireG_, paramWireB_, alpha * paramWireAlpha_ * 0.16f);
        for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
            const auto ia = static_cast<std::size_t>(indices[i]);
            const auto ib = static_cast<std::size_t>(indices[i + 1]);
            const auto ic = static_cast<std::size_t>(indices[i + 2]);
            if (ia >= vertices.size() || ib >= vertices.size() || ic >= vertices.size() ||
                ia >= baseVertices.size() || ib >= baseVertices.size() || ic >= baseVertices.size()) {
                continue;
            }
            const glm::vec3 a = grownTerrainPoint(baseVertices[ia], vertices[ia], growth);
            const glm::vec3 b = grownTerrainPoint(baseVertices[ib], vertices[ib], growth);
            const glm::vec3 c = grownTerrainPoint(baseVertices[ic], vertices[ic], growth);
            const float heightDelta = (std::abs(a.y - b.y) + std::abs(b.y - c.y) + std::abs(c.y - a.y)) /
                std::max(1.0f, paramMountainHeight_);
            const float edgeAlpha = ofClamp(0.08f + heightDelta * 1.10f, 0.04f, 0.72f);
            addLine(grownLines, a, b, withAlphaScale(lineColor, edgeAlpha));
            addLine(grownLines, b, c, withAlphaScale(lineColor, edgeAlpha));
            addLine(grownLines, c, a, withAlphaScale(lineColor, edgeAlpha));
        }
        grownLines.draw();
    }

    if (paramShoreGlow_ > 0.0f && islandMesh_.getNumVertices() > 0 && seaFloorBaseMesh_.getNumVertices() > 0) {
        ofMesh waterline;
        waterline.setMode(OF_PRIMITIVE_LINES);
        const auto& baseVertices = seaFloorBaseMesh_.getVertices();
        const auto& vertices = islandMesh_.getVertices();
        const auto& indices = islandMesh_.getIndices();
        const ofFloatColor shoreColor = colorFrom(paramShallowR_ * 1.08f,
                                                  paramShallowG_ * 1.06f,
                                                  paramShallowB_ * 1.02f,
                                                  alpha * paramShoreGlow_ * 0.72f);

        auto collectCrossing = [&](const glm::vec3& a,
                                   const glm::vec3& b,
                                   std::vector<glm::vec3>& crossings) {
            const float da = a.y - paramWaterLevel_;
            const float db = b.y - paramWaterLevel_;
            if ((da < 0.0f && db < 0.0f) || (da > 0.0f && db > 0.0f) || std::abs(da - db) <= 0.0001f) {
                return;
            }
            const float t = ofClamp(-da / (db - da), 0.0f, 1.0f);
            glm::vec3 p = glm::mix(a, b, t);
            p.y = paramWaterLevel_ + 3.0f;
            crossings.push_back(p);
        };

        for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
            const auto ia = static_cast<std::size_t>(indices[i]);
            const auto ib = static_cast<std::size_t>(indices[i + 1]);
            const auto ic = static_cast<std::size_t>(indices[i + 2]);
            if (ia >= vertices.size() || ib >= vertices.size() || ic >= vertices.size() ||
                ia >= baseVertices.size() || ib >= baseVertices.size() || ic >= baseVertices.size()) {
                continue;
            }

            const glm::vec3 a = grownTerrainPoint(baseVertices[ia], vertices[ia], growth);
            const glm::vec3 b = grownTerrainPoint(baseVertices[ib], vertices[ib], growth);
            const glm::vec3 c = grownTerrainPoint(baseVertices[ic], vertices[ic], growth);
            std::vector<glm::vec3> crossings;
            crossings.reserve(3);
            collectCrossing(a, b, crossings);
            collectCrossing(b, c, crossings);
            collectCrossing(c, a, crossings);
            if (crossings.size() >= 2) {
                addLine(waterline, crossings[0], crossings[1], shoreColor);
            }
        }
        waterline.draw();
    }
    glDepthMask(GL_TRUE);
}

void MountainIslandLayer::drawCloudShadows(float alpha, float timeSeconds) const {
    if (clouds_.empty() || paramCloudShadowAlpha_ <= 0.0f || paramCloudDensity_ <= 0.0f) {
        return;
    }

    const glm::vec2 lightDrift = safeNormalize(glm::vec2(-0.34f, 0.46f));
    ofMesh shadows;
    shadows.setMode(OF_PRIMITIVE_TRIANGLES);
    constexpr int kSegments = 18;

    for (const auto& cloud : clouds_) {
        const glm::vec2 xz(cloud.position.x, cloud.position.z);
        const float ground = terrainHeightAt(xz);
        const float heightAboveGround = std::max(1.0f, cloud.position.y - ground);
        const glm::vec2 shadowCenter = xz - lightDrift * heightAboveGround * 0.16f;
        const float shadowGround = terrainHeightAt(shadowCenter);
        const float fade = 1.0f - smootherStep(ofMap(heightAboveGround,
                                                      paramCloudClearance_ * 0.8f,
                                                      paramCloudBaseHeight_ + paramCloudLayerDepth_ * 0.9f,
                                                      0.0f,
                                                      1.0f,
                                                      true));
        const float shadowAlpha = alpha * paramCloudShadowAlpha_ * paramCloudDensity_ * ofClamp(0.22f + fade * 0.78f, 0.0f, 1.0f);
        if (shadowAlpha <= 0.002f) {
            continue;
        }

        const float yaw = ofDegToRad(cloud.yaw * 0.35f + std::sin(timeSeconds * 0.07f + cloud.seed) * 8.0f);
        const float c = std::cos(yaw);
        const float s = std::sin(yaw);
        const float scale = paramCloudScale_ * cloud.scale;
        const float radiusX = 112.0f * scale;
        const float radiusZ = 62.0f * scale;
        const int base = static_cast<int>(shadows.getNumVertices());
        shadows.addVertex(glm::vec3(shadowCenter.x, shadowGround + 3.4f, shadowCenter.y));
        shadows.addColor(colorFrom(0.010f, 0.022f, 0.026f, shadowAlpha * 0.72f));
        for (int i = 0; i <= kSegments; ++i) {
            const float theta = static_cast<float>(i) / static_cast<float>(kSegments) * TWO_PI;
            const float wobble = 1.0f + signedNoise(std::cos(theta) * 1.6f + cloud.seed,
                                                    std::sin(theta) * 1.6f,
                                                    timeSeconds * 0.035f) * 0.10f;
            const float lx = std::cos(theta) * radiusX * wobble;
            const float lz = std::sin(theta) * radiusZ * wobble;
            const float x = shadowCenter.x + lx * c - lz * s;
            const float z = shadowCenter.y + lx * s + lz * c;
            shadows.addVertex(glm::vec3(x, terrainHeightAt(glm::vec2(x, z)) + 3.2f, z));
            shadows.addColor(colorFrom(0.010f, 0.022f, 0.026f, 0.0f));
        }
        for (int i = 1; i <= kSegments; ++i) {
            shadows.addIndex(base);
            shadows.addIndex(base + i);
            shadows.addIndex(base + i + 1);
        }
    }

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    glDepthMask(GL_FALSE);
    shadows.draw();
    glDepthMask(GL_TRUE);
}

void MountainIslandLayer::drawClouds(float alpha, float timeSeconds) const {
    if (clouds_.empty() || paramCloudDensity_ <= 0.0f) {
        return;
    }

    std::vector<const CloudCluster*> drawOrder;
    drawOrder.reserve(clouds_.size());
    for (const auto& cloud : clouds_) {
        drawOrder.push_back(&cloud);
    }
    std::sort(drawOrder.begin(), drawOrder.end(), [](const CloudCluster* a, const CloudCluster* b) {
        return a->position.z < b->position.z;
    });

    (void)timeSeconds;
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    glDepthMask(GL_FALSE);
    for (const auto* cloud : drawOrder) {
        const float scale = paramCloudScale_ * cloud->scale;
        ofPushMatrix();
        ofTranslate(cloud->position.x, cloud->position.y, cloud->position.z);
        ofRotateYDeg(cloud->yaw);
        ofScale(scale, scale, scale);
        drawMeshWithAlpha(cloud->mesh, alpha * paramCloudDensity_);
        ofPopMatrix();
    }
    glDepthMask(GL_TRUE);
}
