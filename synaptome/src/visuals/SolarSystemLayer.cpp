#include "SolarSystemLayer.h"

#include "../io/AudioAnalysisBus.h"
#include "ofGraphics.h"
#include "ofMath.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <random>
#include <utility>

namespace {
    constexpr float kMinDt = 0.0f;
    constexpr float kMaxDt = 1.0f / 20.0f;
    constexpr float kRadToDeg = 57.295779513082320876f;

    struct ObservedPlanetData {
        const char* host;
        const char* planet;
        float radiusEarth;
        float massEarth;
        float periodDays;
        float semiMajorAu;
        float eccentricity;
        float starTeff;
        float starRadiusSolar;
        float starMassSolar;
        int systemPlanetCount;
        int discoveryYear;
    };

    const std::array<ObservedPlanetData, 36> kObservedPlanets = {
        ObservedPlanetData{ "55 Cnc", "55 Cnc e", 1.875f, 7.990f, 0.7365474f, 0.01544f, 0.05000f, 5172.0f, 0.9430f, 0.9050f, 7, 2004 },
        ObservedPlanetData{ "55 Cnc", "55 Cnc b", 13.900f, 263.9785f, 14.651552f, 0.11800f, 0.00290f, 5198.0f, 0.9800f, 1.0150f, 7, 1996 },
        ObservedPlanetData{ "55 Cnc", "55 Cnc c", 8.510f, 54.4738f, 44.393600f, 0.24700f, 0.08800f, 5198.0f, 0.9800f, 1.0150f, 7, 2004 },
        ObservedPlanetData{ "55 Cnc", "55 Cnc f", 7.590f, 44.8120f, 260.58000f, 0.80200f, 0.06300f, 5198.0f, 0.9800f, 1.0150f, 7, 2007 },
        ObservedPlanetData{ "55 Cnc", "55 Cnc d", 13.000f, 1232.493f, 4799.0000f, 5.60000f, 0.09130f, 5198.0f, 0.9800f, 1.0150f, 7, 2002 },
        ObservedPlanetData{ "GJ 876", "GJ 876 d", 2.510f, 6.8300f, 1.937780f, 0.02080665f, 0.20700f, 3293.74f, 0.3000f, 0.3200f, 4, 2005 },
        ObservedPlanetData{ "GJ 876", "GJ 876 c", 14.000f, 226.9846f, 30.088100f, 0.12959000f, 0.25591f, 3293.74f, 0.3000f, 0.3200f, 4, 2000 },
        ObservedPlanetData{ "GJ 876", "GJ 876 b", 13.300f, 723.2235f, 61.116600f, 0.20831700f, 0.03240f, 3293.74f, 0.3000f, 0.3200f, 4, 1998 },
        ObservedPlanetData{ "GJ 876", "GJ 876 e", 3.920f, 14.6000f, 124.26000f, 0.33430000f, 0.05500f, 3293.74f, 0.3000f, 0.3200f, 4, 2010 },
        ObservedPlanetData{ "HD 10180", "HD 10180 c", 0.0f, 2741.5878f, 5.759690f, 0.06412f, 0.07300f, 5911.0f, 1.1090f, 1.0600f, 6, 2010 },
        ObservedPlanetData{ "HD 10180", "HD 10180 d", 0.0f, 3295.8806f, 16.35700f, 0.12859f, 0.13100f, 5911.0f, 1.1090f, 1.0600f, 6, 2010 },
        ObservedPlanetData{ "HD 10180", "HD 10180 e", 5.390f, 25.1000f, 49.74800f, 0.26990f, 0.05100f, 5911.0f, 1.1090f, 1.0600f, 6, 2010 },
        ObservedPlanetData{ "HD 10180", "HD 10180 f", 5.240f, 23.9000f, 122.7440f, 0.49290f, 0.11900f, 5911.0f, 1.1090f, 1.0600f, 6, 2010 },
        ObservedPlanetData{ "HD 10180", "HD 10180 g", 0.0f, 3375.3377f, 604.6700f, 1.42700f, 0.26300f, 5911.0f, 1.1090f, 1.0600f, 6, 2010 },
        ObservedPlanetData{ "HD 10180", "HD 10180 h", 9.400f, 64.4000f, 2205.000f, 3.38100f, 0.09500f, 5911.0f, 1.1090f, 1.0600f, 6, 2010 },
        ObservedPlanetData{ "HR 8799", "HR 8799 e", 13.11453f, 3178.3000f, 20815.60f, 16.400f, 0.15000f, 7400.0f, 1.49338f, 1.5100f, 4, 2010 },
        ObservedPlanetData{ "HR 8799", "HR 8799 d", 13.00000f, 3000.0000f, 37000.00f, 24.000f, 0.60000f, 7204.58f, 1.49338f, 1.5000f, 4, 2008 },
        ObservedPlanetData{ "HR 8799", "HR 8799 c", 13.00000f, 3000.0000f, 69000.00f, 38.000f, 0.50000f, 7204.58f, 1.49338f, 1.5000f, 4, 2008 },
        ObservedPlanetData{ "HR 8799", "HR 8799 b", 13.00000f, 2000.0000f, 170000.0f, 68.000f, 0.0f, 7204.58f, 1.49338f, 1.5000f, 4, 2008 },
        ObservedPlanetData{ "TOI-178", "TOI-178 b", 1.200f, 0.9600f, 1.9145601f, 0.02607f, 0.0f, 4316.0f, 0.6620f, 0.6470f, 6, 2021 },
        ObservedPlanetData{ "TOI-178", "TOI-178 c", 1.754f, 4.6400f, 3.2384860f, 0.03700f, 0.00032f, 4316.0f, 0.6620f, 0.6470f, 6, 2021 },
        ObservedPlanetData{ "TOI-178", "TOI-178 d", 2.695f, 5.2000f, 6.5575690f, 0.05920f, 0.00680f, 4316.0f, 0.6620f, 0.6470f, 6, 2021 },
        ObservedPlanetData{ "TOI-178", "TOI-178 e", 2.301f, 3.4800f, 9.9631800f, 0.07830f, 0.00038f, 4316.0f, 0.6620f, 0.6470f, 6, 2021 },
        ObservedPlanetData{ "TOI-178", "TOI-178 f", 2.417f, 5.6300f, 15.233350f, 0.10390f, 0.00045f, 4316.0f, 0.6620f, 0.6470f, 6, 2021 },
        ObservedPlanetData{ "TOI-178", "TOI-178 g", 2.939f, 4.4000f, 20.716630f, 0.12750f, 0.00043f, 4316.0f, 0.6620f, 0.6470f, 6, 2021 },
        ObservedPlanetData{ "TRAPPIST-1", "TRAPPIST-1 b", 1.116f, 1.3740f, 1.5108260f, 0.01154f, 0.00622f, 2566.0f, 0.1192f, 0.0898f, 7, 2016 },
        ObservedPlanetData{ "TRAPPIST-1", "TRAPPIST-1 c", 1.097f, 1.3080f, 2.4219370f, 0.01580f, 0.00654f, 2566.0f, 0.1192f, 0.0898f, 7, 2016 },
        ObservedPlanetData{ "TRAPPIST-1", "TRAPPIST-1 d", 0.788f, 0.3880f, 4.0492190f, 0.02227f, 0.00837f, 2566.0f, 0.1192f, 0.0898f, 7, 2016 },
        ObservedPlanetData{ "TRAPPIST-1", "TRAPPIST-1 e", 0.920f, 0.6920f, 6.1010130f, 0.02925f, 0.00510f, 2566.0f, 0.1192f, 0.0898f, 7, 2017 },
        ObservedPlanetData{ "TRAPPIST-1", "TRAPPIST-1 f", 1.045f, 1.0390f, 9.2075400f, 0.03849f, 0.01007f, 2566.0f, 0.1192f, 0.0898f, 7, 2017 },
        ObservedPlanetData{ "TRAPPIST-1", "TRAPPIST-1 g", 1.129f, 1.3210f, 12.352446f, 0.04683f, 0.00208f, 2566.0f, 0.1192f, 0.0898f, 7, 2017 },
        ObservedPlanetData{ "TRAPPIST-1", "TRAPPIST-1 h", 0.755f, 0.3260f, 18.772866f, 0.06189f, 0.00567f, 2566.0f, 0.1192f, 0.0898f, 7, 2017 },
        ObservedPlanetData{ "WASP-47", "WASP-47 e", 1.830f, 9.0000f, 0.789610f, 0.01673f, 0.00000f, 5565.0f, 1.1560f, 1.0580f, 4, 2015 },
        ObservedPlanetData{ "WASP-47", "WASP-47 b", 12.860f, 374.000f, 4.159151f, 0.05200f, 0.00060f, 5565.0f, 1.1560f, 1.0580f, 4, 2012 },
        ObservedPlanetData{ "WASP-47", "WASP-47 d", 3.650f, 15.500f, 9.030501f, 0.08500f, 0.00100f, 5565.0f, 1.1560f, 1.0580f, 4, 2015 },
        ObservedPlanetData{ "WASP-47", "WASP-47 c", 0.0f, 447.000f, 589.5700f, 1.39300f, 0.26400f, 5565.0f, 1.1560f, 1.0580f, 4, 2015 }
    };

    const std::array<const char*, 7> kObservedHosts = {
        "55 Cnc",
        "GJ 876",
        "HD 10180",
        "HR 8799",
        "TOI-178",
        "TRAPPIST-1",
        "WASP-47"
    };

    float followAmount(float smoothing) {
        return 1.0f - ofClamp(smoothing, 0.0f, 0.98f);
    }

    float wrap01(float value) {
        value = std::fmod(value, 1.0f);
        return value < 0.0f ? value + 1.0f : value;
    }

    float clamp01(float value) {
        return ofClamp(value, 0.0f, 1.0f);
    }

    float smooth01(float value) {
        const float t = clamp01(value);
        return t * t * (3.0f - 2.0f * t);
    }

    float safeLog10(float value, float floorValue) {
        return std::log10(std::max(floorValue, value));
    }

    float logMap(float value, float inputMin, float inputMax, float outputMin, float outputMax) {
        const float t = ofClamp((safeLog10(value, inputMin) - std::log10(inputMin)) /
                                std::max(0.0001f, std::log10(inputMax) - std::log10(inputMin)),
                                0.0f,
                                1.0f);
        return outputMin + (outputMax - outputMin) * t;
    }

    float gaussianish(std::mt19937& rng) {
        std::uniform_real_distribution<float> unit(0.0f, 1.0f);
        return (unit(rng) + unit(rng) + unit(rng) + unit(rng) - 2.0f) * 0.5f;
    }

    float randRange(std::mt19937& rng, float minValue, float maxValue) {
        std::uniform_real_distribution<float> dist(minValue, maxValue);
        return dist(rng);
    }

    int randInt(std::mt19937& rng, int minValue, int maxValue) {
        std::uniform_int_distribution<int> dist(minValue, maxValue);
        return dist(rng);
    }

    float estimateRadiusEarth(const ObservedPlanetData& data) {
        if (data.radiusEarth > 0.0f) {
            return data.radiusEarth;
        }
        if (data.massEarth >= 90.0f) {
            return ofClamp(10.5f + std::log10(std::max(90.0f, data.massEarth) / 90.0f) * 2.1f, 8.0f, 14.4f);
        }
        if (data.massEarth > 0.0f) {
            return ofClamp(std::pow(data.massEarth, 0.29f), 0.45f, 6.0f);
        }
        return 1.0f;
    }

    ofFloatColor colorFrom(float r, float g, float b, float a) {
        return ofFloatColor(ofClamp(r, 0.0f, 1.5f),
                            ofClamp(g, 0.0f, 1.5f),
                            ofClamp(b, 0.0f, 1.5f),
                            ofClamp(a, 0.0f, 1.0f));
    }

    void setFloatColor(const ofFloatColor& color) {
        ofSetColor(static_cast<int>(ofClamp(color.r, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(color.g, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(color.b, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(color.a, 0.0f, 1.0f) * 255.0f));
    }

    ofFloatColor starColorForTemperature(float teff) {
        const ofFloatColor ember(1.0f, 0.26f, 0.10f, 1.0f);
        const ofFloatColor amber(1.0f, 0.62f, 0.20f, 1.0f);
        const ofFloatColor sun(1.0f, 0.86f, 0.48f, 1.0f);
        const ofFloatColor white(0.92f, 0.96f, 1.0f, 1.0f);
        const ofFloatColor blue(0.55f, 0.74f, 1.0f, 1.0f);

        if (teff < 3600.0f) {
            return ember.getLerped(amber, ofClamp((teff - 2400.0f) / 1200.0f, 0.0f, 1.0f));
        }
        if (teff < 5400.0f) {
            return amber.getLerped(sun, ofClamp((teff - 3600.0f) / 1800.0f, 0.0f, 1.0f));
        }
        if (teff < 6600.0f) {
            return sun.getLerped(white, ofClamp((teff - 5400.0f) / 1200.0f, 0.0f, 1.0f));
        }
        return white.getLerped(blue, ofClamp((teff - 6600.0f) / 2000.0f, 0.0f, 1.0f));
    }

    ofFloatColor planetColorFor(const ObservedPlanetData& data, float radiusEarth, float variation, std::mt19937& rng) {
        variation = ofClamp(variation, 0.0f, 1.0f);
        const float hotOrbit = 1.0f - ofClamp(logMap(data.semiMajorAu, 0.01f, 1.0f, 0.0f, 1.0f), 0.0f, 1.0f);
        const float heavyAtmosphere = ofClamp(std::log10(std::max(1.0f, data.massEarth)) / 3.2f, 0.0f, 1.0f);
        const float roll = randRange(rng, 0.0f, 1.0f);
        ofFloatColor result;
        if (radiusEarth < 1.45f) {
            const ofFloatColor basalt(0.30f, 0.29f, 0.27f, 1.0f);
            const ofFloatColor dust(0.58f, 0.52f, 0.43f, 1.0f);
            const ofFloatColor iron(0.62f, 0.38f, 0.28f, 1.0f);
            const ofFloatColor blueGray(0.34f, 0.43f, 0.50f, 1.0f);
            result = basalt.getLerped(dust, roll * 0.62f)
                           .getLerped(blueGray, randRange(rng, 0.0f, 0.22f) * (1.0f - hotOrbit))
                           .getLerped(iron, hotOrbit * (0.12f + variation * 0.16f));
        } else if (radiusEarth < 4.3f) {
            const ofFloatColor paleCyan(0.56f, 0.70f, 0.72f, 1.0f);
            const ofFloatColor grayBlue(0.44f, 0.52f, 0.60f, 1.0f);
            const ofFloatColor methaneHaze(0.50f, 0.66f, 0.62f, 1.0f);
            const ofFloatColor warmCloud(0.66f, 0.61f, 0.52f, 1.0f);
            result = grayBlue.getLerped(paleCyan, roll * 0.50f)
                             .getLerped(methaneHaze, heavyAtmosphere * 0.24f)
                             .getLerped(warmCloud, hotOrbit * 0.16f);
        } else if (radiusEarth < 9.5f) {
            const ofFloatColor iceBlue(0.52f, 0.67f, 0.76f, 1.0f);
            const ofFloatColor grayTeal(0.42f, 0.59f, 0.57f, 1.0f);
            const ofFloatColor softBlue(0.48f, 0.58f, 0.70f, 1.0f);
            result = iceBlue.getLerped(grayTeal, roll * 0.46f)
                            .getLerped(softBlue, heavyAtmosphere * 0.20f)
                            .getLerped(ofFloatColor(0.62f, 0.56f, 0.48f, 1.0f), hotOrbit * 0.10f);
        } else {
            const ofFloatColor cream(0.78f, 0.70f, 0.58f, 1.0f);
            const ofFloatColor ochre(0.66f, 0.54f, 0.36f, 1.0f);
            const ofFloatColor tan(0.58f, 0.49f, 0.39f, 1.0f);
            const ofFloatColor grayBrown(0.46f, 0.42f, 0.36f, 1.0f);
            result = cream.getLerped(ochre, roll * 0.44f)
                          .getLerped(tan, heavyAtmosphere * 0.22f)
                          .getLerped(grayBrown, hotOrbit * 0.12f);
        }

        const float albedoJitter = randRange(rng, 0.86f, 1.08f);
        result.r = ofClamp(result.r * albedoJitter, 0.04f, 0.88f);
        result.g = ofClamp(result.g * albedoJitter, 0.04f, 0.88f);
        result.b = ofClamp(result.b * albedoJitter, 0.04f, 0.88f);
        result = result.getLerped(ofFloatColor(0.52f, 0.52f, 0.50f, 1.0f), 0.18f + variation * 0.06f);
        result.a = 1.0f;
        return result;
    }

    glm::vec3 safeNormalize(const glm::vec3& value, const glm::vec3& fallback = glm::vec3(0.0f, 1.0f, 0.0f)) {
        const float lenSq = value.x * value.x + value.y * value.y + value.z * value.z;
        if (lenSq <= 0.000001f) {
            return fallback;
        }
        const float invLen = 1.0f / std::sqrt(lenSq);
        return glm::vec3(value.x * invLen, value.y * invLen, value.z * invLen);
    }

    glm::vec3 crossProduct(const glm::vec3& a, const glm::vec3& b) {
        return glm::vec3(a.y * b.z - a.z * b.y,
                         a.z * b.x - a.x * b.z,
                         a.x * b.y - a.y * b.x);
    }

    float dotProduct(const glm::vec3& a, const glm::vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    glm::vec3 sphericalPoint(float latitude, float longitude, float radius) {
        const float c = std::cos(latitude);
        return glm::vec3(std::cos(longitude) * c * radius,
                         std::sin(latitude) * radius,
                         std::sin(longitude) * c * radius);
    }

    bool sameHost(const char* a, const char* b) {
        return std::string(a) == std::string(b);
    }
}

void SolarSystemLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }

    const auto& def = config["defaults"];
    paramEnabled_ = def.value("visible", paramEnabled_);
    paramShowOrbits_ = def.value("showOrbits", paramShowOrbits_);
    paramShowTrails_ = def.value("showTrails", paramShowTrails_);
    paramShowMoons_ = def.value("showMoons", paramShowMoons_);
    paramShowRings_ = def.value("showRings", paramShowRings_);
    paramShowAsteroids_ = def.value("showAsteroids", paramShowAsteroids_);
    paramShowComets_ = def.value("showComets", paramShowComets_);
    paramShowWaveformBelt_ = def.value("showWaveformBelt", paramShowWaveformBelt_);
    paramAlpha_ = def.value("alpha", paramAlpha_);
    paramScale_ = def.value("scale", paramScale_);
    paramSceneZoom_ = def.value("sceneZoom", paramSceneZoom_);
    paramOrbitSpread_ = def.value("orbitSpread", paramOrbitSpread_);
    paramOrbitSpeed_ = def.value("orbitSpeed", paramOrbitSpeed_);
    paramOrbitTilt_ = def.value("orbitTilt", paramOrbitTilt_);
    paramOrbitRotation_ = def.value("orbitRotation", paramOrbitRotation_);
    paramOrbitPlaneVariation_ = def.value("orbitPlaneVariation", paramOrbitPlaneVariation_);
    paramEccentricity_ = def.value("eccentricity", paramEccentricity_);
    paramDepth_ = def.value("depth", paramDepth_);
    paramStarSize_ = def.value("starSize", paramStarSize_);
    paramStarGlow_ = def.value("starGlow", paramStarGlow_);
    paramStarRadiance_ = def.value("starRadiance", paramStarRadiance_);
    paramStarEmissionAudio_ = def.value("starEmissionAudio", paramStarEmissionAudio_);
    paramStarSurfaceTurbulence_ = def.value("starSurfaceTurbulence", paramStarSurfaceTurbulence_);
    paramSolarBurstIntensity_ = def.value("solarBurstIntensity", paramSolarBurstIntensity_);
    paramSideFillLight_ = def.value("sideFillLight", paramSideFillLight_);
    paramVisitorEvents_ = def.value("visitorEvents", paramVisitorEvents_);
    paramArtifactActivity_ = def.value("artifactActivity", paramArtifactActivity_);
    paramPlanetSize_ = def.value("planetSize", paramPlanetSize_);
    paramObservedDiversity_ = def.value("observedDiversity", paramObservedDiversity_);
    paramPlanetVariation_ = def.value("planetVariation", paramPlanetVariation_);
    paramAsteroidDensity_ = def.value("asteroidDensity", paramAsteroidDensity_);
    paramCometDensity_ = def.value("cometDensity", paramCometDensity_);
    paramOrbitAlpha_ = def.value("orbitAlpha", paramOrbitAlpha_);
    paramOrbitThickness_ = def.value("orbitThickness", paramOrbitThickness_);
    paramTrailAlpha_ = def.value("trailAlpha", paramTrailAlpha_);
    paramTrailLength_ = def.value("trailLength", paramTrailLength_);
    paramTrailSteps_ = def.value("trailSteps", paramTrailSteps_);
    paramTrailStampGain_ = def.value("trailStampGain", paramTrailStampGain_);
    paramTrailStampLife_ = def.value("trailStampLife", paramTrailStampLife_);
    paramAtmosphereGrowth_ = def.value("atmosphereGrowth", paramAtmosphereGrowth_);
    paramLifeReactivity_ = def.value("lifeReactivity", paramLifeReactivity_);
    paramBiosphereThreshold_ = def.value("biosphereThreshold", paramBiosphereThreshold_);
    paramCivilizationGrowth_ = def.value("civilizationGrowth", paramCivilizationGrowth_);
    paramMoonSize_ = def.value("moonSize", paramMoonSize_);
    paramMoonSpeed_ = def.value("moonSpeed", paramMoonSpeed_);
    paramAudioAmount_ = def.value("audioAmount", paramAudioAmount_);
    paramAudioSmoothing_ = def.value("audioSmoothing", paramAudioSmoothing_);
    paramBassScale_ = def.value("bassScale", paramBassScale_);
    paramMidsSpeed_ = def.value("midsSpeed", paramMidsSpeed_);
    paramHighsSparkle_ = def.value("highsSparkle", paramHighsSparkle_);
    paramWaveformAmount_ = def.value("waveformAmount", paramWaveformAmount_);
    paramBgAlpha_ = def.value("bgAlpha", paramBgAlpha_);
    paramBgR_ = def.value("bgR", paramBgR_);
    paramBgG_ = def.value("bgG", paramBgG_);
    paramBgB_ = def.value("bgB", paramBgB_);
    paramStarR_ = def.value("starR", paramStarR_);
    paramStarG_ = def.value("starG", paramStarG_);
    paramStarB_ = def.value("starB", paramStarB_);
    paramOrbitR_ = def.value("orbitR", paramOrbitR_);
    paramOrbitG_ = def.value("orbitG", paramOrbitG_);
    paramOrbitB_ = def.value("orbitB", paramOrbitB_);
    paramTrailR_ = def.value("trailR", paramTrailR_);
    paramTrailG_ = def.value("trailG", paramTrailG_);
    paramTrailB_ = def.value("trailB", paramTrailB_);
    paramSeed_ = def.value("seed", paramSeed_);
    readColor(def, "backgroundColor", paramBgR_, paramBgG_, paramBgB_);
    readColor(def, "starColor", paramStarR_, paramStarG_, paramStarB_);
    readColor(def, "orbitColor", paramOrbitR_, paramOrbitG_, paramOrbitB_);
    readColor(def, "trailColor", paramTrailR_, paramTrailG_, paramTrailB_);
    clampParams();
}

void SolarSystemLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "generative.solarSystem" : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "Solar System";
    meta.label = "Solar System Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    meta.label = "White Orbit Guides";
    registry.addBool(prefix + ".showOrbits", &paramShowOrbits_, paramShowOrbits_, meta);

    meta.label = "Planet Travel Trails";
    registry.addBool(prefix + ".showTrails", &paramShowTrails_, paramShowTrails_, meta);

    meta.label = "Solar System Moons";
    registry.addBool(prefix + ".showMoons", &paramShowMoons_, paramShowMoons_, meta);

    meta.label = "Solar System Rings";
    registry.addBool(prefix + ".showRings", &paramShowRings_, paramShowRings_, meta);

    meta.label = "Solar System Asteroids";
    registry.addBool(prefix + ".showAsteroids", &paramShowAsteroids_, paramShowAsteroids_, meta);

    meta.label = "Solar System Comets";
    registry.addBool(prefix + ".showComets", &paramShowComets_, paramShowComets_, meta);

    meta.label = "Heliosphere Field";
    registry.addBool(prefix + ".showWaveformBelt", &paramShowWaveformBelt_, paramShowWaveformBelt_, meta);

    registerFloat(registry, prefix + ".alpha", &paramAlpha_, paramAlpha_, "Orrery Alpha", 0.0f, 1.0f, 0.01f, "normalized");
    registerFloat(registry, prefix + ".scale", &paramScale_, paramScale_, "Orrery Scale", 0.25f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".sceneZoom", &paramSceneZoom_, paramSceneZoom_, "Scene Zoom", 0.35f, 2.5f, 0.01f);
    registerFloat(registry, prefix + ".orbitSpread", &paramOrbitSpread_, paramOrbitSpread_, "Orbit Spread", 0.65f, 2.25f, 0.01f);
    registerFloat(registry, prefix + ".orbitSpeed", &paramOrbitSpeed_, paramOrbitSpeed_, "Orrery Speed", -3.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".orbitTilt", &paramOrbitTilt_, paramOrbitTilt_, "Orrery Tilt", -80.0f, 80.0f, 1.0f, "deg");
    registerFloat(registry, prefix + ".orbitRotation", &paramOrbitRotation_, paramOrbitRotation_, "Orrery Rotation", -180.0f, 180.0f, 1.0f, "deg");
    registerFloat(registry, prefix + ".orbitPlaneVariation", &paramOrbitPlaneVariation_, paramOrbitPlaneVariation_, "Orbit Plane Variation", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".eccentricity", &paramEccentricity_, paramEccentricity_, "Observed Eccentricity Lift", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".depth", &paramDepth_, paramDepth_, "Orrery Depth", 0.1f, 2.5f, 0.01f);
    registerFloat(registry, prefix + ".starSize", &paramStarSize_, paramStarSize_, "Star Size", 0.01f, 0.14f, 0.001f, "viewport");
    registerFloat(registry, prefix + ".starGlow", &paramStarGlow_, paramStarGlow_, "Star Glow", 0.0f, 5.0f, 0.01f);
    registerFloat(registry, prefix + ".starRadiance", &paramStarRadiance_, paramStarRadiance_, "Star Radiance", 0.0f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".starEmissionAudio", &paramStarEmissionAudio_, paramStarEmissionAudio_, "Star Audio Emission", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".starSurfaceTurbulence", &paramStarSurfaceTurbulence_, paramStarSurfaceTurbulence_, "Star Surface Turbulence", 0.0f, 2.5f, 0.01f);
    registerFloat(registry, prefix + ".solarBurstIntensity", &paramSolarBurstIntensity_, paramSolarBurstIntensity_, "Solar Burst Intensity", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".sideFillLight", &paramSideFillLight_, paramSideFillLight_, "Side Fill Light", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".visitorEvents", &paramVisitorEvents_, paramVisitorEvents_, "Visitor Events", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".artifactActivity", &paramArtifactActivity_, paramArtifactActivity_, "Orbital Artifacts", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".planetSize", &paramPlanetSize_, paramPlanetSize_, "Planet Size", 0.25f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".observedDiversity", &paramObservedDiversity_, paramObservedDiversity_, "Observed Diversity", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".planetVariation", &paramPlanetVariation_, paramPlanetVariation_, "Planet Variation", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".asteroidDensity", &paramAsteroidDensity_, paramAsteroidDensity_, "Asteroid Density", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".cometDensity", &paramCometDensity_, paramCometDensity_, "Comet Density", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".orbitAlpha", &paramOrbitAlpha_, paramOrbitAlpha_, "Orbit Alpha", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".orbitThickness", &paramOrbitThickness_, paramOrbitThickness_, "Orbit Thickness", 0.5f, 6.0f, 0.1f, "px");
    registerFloat(registry, prefix + ".trailAlpha", &paramTrailAlpha_, paramTrailAlpha_, "Orbit Trail Alpha", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".trailLength", &paramTrailLength_, paramTrailLength_, "Orbit Trail Length", 0.02f, 1.0f, 0.01f, "orbit");
    registerFloat(registry, prefix + ".trailSteps", &paramTrailSteps_, paramTrailSteps_, "Trail Steps", 8.0f, 96.0f, 1.0f);
    registerFloat(registry, prefix + ".trailStampGain", &paramTrailStampGain_, paramTrailStampGain_, "Trail Audio Stamp Gain", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".trailStampLife", &paramTrailStampLife_, paramTrailStampLife_, "Trail Stamp Life", 2.0f, 36.0f, 0.1f, "s");
    registerFloat(registry, prefix + ".atmosphereGrowth", &paramAtmosphereGrowth_, paramAtmosphereGrowth_, "Atmosphere Audio Growth", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".lifeReactivity", &paramLifeReactivity_, paramLifeReactivity_, "Planet Life Reactivity", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".biosphereThreshold", &paramBiosphereThreshold_, paramBiosphereThreshold_, "Biosphere Threshold", 0.05f, 1.2f, 0.01f);
    registerFloat(registry, prefix + ".civilizationGrowth", &paramCivilizationGrowth_, paramCivilizationGrowth_, "Satellite Emergence", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".moonSize", &paramMoonSize_, paramMoonSize_, "Moon Size", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".moonSpeed", &paramMoonSpeed_, paramMoonSpeed_, "Moon Speed", -4.0f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".audioAmount", &paramAudioAmount_, paramAudioAmount_, "Orrery Audio Amount", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".audioSmoothing", &paramAudioSmoothing_, paramAudioSmoothing_, "Orrery Audio Smoothing", 0.0f, 0.98f, 0.01f);
    registerFloat(registry, prefix + ".bassScale", &paramBassScale_, paramBassScale_, "Bass Solar Bloom", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".midsSpeed", &paramMidsSpeed_, paramMidsSpeed_, "Mids Solar Wind", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".highsSparkle", &paramHighsSparkle_, paramHighsSparkle_, "Highs Sparkle", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".waveformAmount", &paramWaveformAmount_, paramWaveformAmount_, "Waveform Field Ripple", 0.0f, 0.20f, 0.001f);

    meta = {};
    meta.group = "Solar System";
    meta.label = "Solar System Reseed";
    meta.description = "Generate a new observed-data-driven 3D system. Seed 0 uses a fresh random seed.";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);

    registerFloat(registry, prefix + ".seed", &paramSeed_, paramSeed_, "Solar System Seed", 0.0f, 99999.0f, 1.0f, "0 = random on setup/reseed");
    registerFloat(registry, prefix + ".bgAlpha", &paramBgAlpha_, paramBgAlpha_, "Orrery Bg Alpha", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgR", &paramBgR_, paramBgR_, "Orrery Bg R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgG", &paramBgG_, paramBgG_, "Orrery Bg G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgB", &paramBgB_, paramBgB_, "Orrery Bg B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".starR", &paramStarR_, paramStarR_, "Star R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".starG", &paramStarG_, paramStarG_, "Star G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".starB", &paramStarB_, paramStarB_, "Star B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".orbitR", &paramOrbitR_, paramOrbitR_, "Orbit R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".orbitG", &paramOrbitG_, paramOrbitG_, "Orbit G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".orbitB", &paramOrbitB_, paramOrbitB_, "Orbit B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".trailR", &paramTrailR_, paramTrailR_, "Trail R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".trailG", &paramTrailG_, paramTrailG_, "Trail G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".trailB", &paramTrailB_, paramTrailB_, "Trail B", 0.0f, 1.0f, 0.01f);

    resetSystem();
}

void SolarSystemLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) {
        return;
    }

    clampParams();
    const float roundedSeed = std::round(paramSeed_);
    if (!systemReady_ ||
        paramReseedRequested_ ||
        roundedSeed != seedParamState_ ||
        std::abs(paramObservedDiversity_ - diversityState_) > 0.001f ||
        std::abs(paramPlanetVariation_ - planetVariationState_) > 0.001f ||
        std::abs(paramOrbitPlaneVariation_ - orbitPlaneVariationState_) > 0.001f ||
        std::abs(paramAsteroidDensity_ - asteroidDensityState_) > 0.001f ||
        std::abs(paramCometDensity_ - cometDensityState_) > 0.001f) {
        resetSystem();
        paramReseedRequested_ = false;
    }

    const float dt = ofClamp(params.dt, kMinDt, kMaxDt);
    updateAudioState(dt, params.time);

    const float audioLift = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float speedLift = 1.0f + mids_ * paramMidsSpeed_ * audioLift * 0.04f;
    orbitTime_ += dt * paramOrbitSpeed_ * speedLift;
    pulseEnvelope_ = ofLerp(pulseEnvelope_, 0.0f, ofClamp(dt * 5.0f, 0.0f, 1.0f));

    if (atmosphereEnergy_.size() != bodies_.size()) {
        atmosphereEnergy_.assign(bodies_.size(), 0.0f);
    }
    if (trailStamps_.size() != bodies_.size()) {
        trailStamps_.assign(bodies_.size(), {});
    }
    if (lifeStates_.size() != bodies_.size()) {
        initializeLifeStates();
    }
    if (lastTrailStampAngle_.size() != bodies_.size()) {
        lastTrailStampAngle_.assign(bodies_.size(), -10000.0f);
    }

    const float audioEnergy = hasAudio_
        ? ofClamp(level_ * 0.62f + bass_ * 0.34f + peak_ * 0.42f, 0.0f, 1.7f)
        : 0.0f;
    const float globalStampEnergy = hasAudio_
        ? ofClamp(peak_ * 0.78f + level_ * 0.36f + highs_ * 0.18f, 0.0f, 1.6f)
        : 0.0f;
    for (std::size_t i = 0; i < bodies_.size(); ++i) {
        const auto& body = bodies_[i];
        auto& life = lifeStates_[i];
        const float planetBandTarget = planetBandEnergyFor(i);
        const float stampEnergy = ofClamp(globalStampEnergy * 0.42f + planetBandTarget * (0.96f + life.affinity * 0.24f), 0.0f, 1.9f);
        const bool strongStamp = stampEnergy > 0.18f;
        const float bandFollow = planetBandTarget > life.bandEnergy
            ? ofClamp(dt * (1.35f + paramLifeReactivity_ * 0.90f), 0.0f, 1.0f)
            : ofClamp(dt * 0.22f, 0.0f, 1.0f);
        life.bandEnergy = ofLerp(life.bandEnergy, planetBandTarget, bandFollow);

        const float baseAtmosphere = body.atmosphere * 0.24f;
        const float atmosphereDrive = ofClamp(audioEnergy * 0.32f + life.bandEnergy * 0.86f, 0.0f, 1.8f);
        const float targetAtmosphere = ofClamp(baseAtmosphere + atmosphereDrive * paramAtmosphereGrowth_ * (0.36f + body.atmosphere * 0.38f + life.affinity * 0.20f), 0.0f, 1.0f);
        const float follow = targetAtmosphere > atmosphereEnergy_[i]
            ? ofClamp(dt * (0.90f + paramAtmosphereGrowth_ * 1.10f), 0.0f, 1.0f)
            : ofClamp(dt * 0.070f, 0.0f, 1.0f);
        atmosphereEnergy_[i] = ofLerp(atmosphereEnergy_[i], targetAtmosphere, follow);

        const float atmosphereReady = smooth01((atmosphereEnergy_[i] - 0.28f) / 0.46f);
        const float lifeThreshold = ofClamp(paramBiosphereThreshold_ * (1.08f - life.affinity * 0.28f) +
                                                life.threshold * 0.16f,
                                            0.08f,
                                            1.25f);
        const float lifeDrive = ofClamp(life.bandEnergy * paramLifeReactivity_ * (0.76f + life.affinity * 0.54f) +
                                            peak_ * 0.16f,
                                        0.0f,
                                        2.0f);
        const float overThreshold = smooth01((lifeDrive - lifeThreshold) / 0.55f);
        const float biosphereRise = overThreshold * atmosphereReady * (0.035f + life.affinity * 0.075f) * paramLifeReactivity_;
        const float biosphereDecay = 0.006f + (1.0f - atmosphereReady) * 0.018f;
        life.biosphereEnergy = ofClamp(life.biosphereEnergy + dt * (biosphereRise - biosphereDecay * (1.0f - overThreshold * 0.55f)),
                                       0.0f,
                                       1.0f);

        const float stableWorld = smooth01((life.biosphereEnergy - 0.42f) / 0.46f) * atmosphereReady;
        const float stabilityRise = stableWorld * (0.030f + life.bandEnergy * 0.055f + life.affinity * 0.035f) * paramCivilizationGrowth_;
        const float stabilityDecay = 0.004f + (1.0f - stableWorld) * 0.012f;
        life.stability = ofClamp(life.stability + dt * (stabilityRise - stabilityDecay), 0.0f, 1.0f);

        const float civilizationGate = smooth01((life.stability - 0.46f) / 0.40f) * stableWorld;
        const float civilizationRise = civilizationGate * (0.026f + life.biosphereEnergy * 0.045f) * paramCivilizationGrowth_;
        const float civilizationDecay = 0.0025f + (1.0f - civilizationGate) * 0.006f;
        life.civilizationEnergy = ofClamp(life.civilizationEnergy + dt * (civilizationRise - civilizationDecay),
                                          0.0f,
                                          1.0f);

        for (auto& stamp : trailStamps_[i]) {
            stamp.age += dt;
        }
        trailStamps_[i].erase(std::remove_if(trailStamps_[i].begin(),
                                             trailStamps_[i].end(),
                                             [](const TrailStamp& stamp) {
                                                 return stamp.age >= stamp.lifetime;
                                             }),
                              trailStamps_[i].end());

        if (strongStamp && paramTrailStampGain_ > 0.0f && paramShowTrails_) {
            const float angle = orbitTime_ * body.speed * TWO_PI + body.phase;
            const float direction = paramOrbitSpeed_ >= 0.0f ? 1.0f : -1.0f;
            const float angleDelta = lastTrailStampAngle_[i] < -9999.0f
                ? 1.0f
                : wrap01((angle - lastTrailStampAngle_[i]) * direction / TWO_PI);
            const float minAngleGap = ofClamp(0.012f + (1.0f - stampEnergy) * 0.030f, 0.010f, 0.048f);
            if (angleDelta >= minAngleGap || stampEnergy > 0.78f) {
                TrailStamp stamp;
                stamp.angle = angle;
                stamp.strength = ofClamp(stampEnergy * paramTrailStampGain_, 0.0f, 2.4f);
                stamp.age = 0.0f;
                stamp.lifetime = paramTrailStampLife_ * (0.72f + stamp.strength * 0.22f);
                stamp.seed = body.seed + static_cast<float>(trailStamps_[i].size()) * 17.0f + orbitTime_ * 11.0f;
                trailStamps_[i].push_back(stamp);
                if (trailStamps_[i].size() > 48) {
                    trailStamps_[i].erase(trailStamps_[i].begin(), trailStamps_[i].begin() + static_cast<std::ptrdiff_t>(trailStamps_[i].size() - 48));
                }
                lastTrailStampAngle_[i] = angle;
            }
        }
    }
}

void SolarSystemLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f) {
        return;
    }
    if (!systemReady_ || bodies_.empty()) {
        resetSystem();
    }

    const float width = static_cast<float>(std::max(1, params.viewport.x));
    const float height = static_cast<float>(std::max(1, params.viewport.y));
    const float minDim = std::min(width, height);
    const float alpha = ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float starRadius = minDim * paramStarSize_ *
        (1.0f + bass_ * paramBassScale_ * audio * 0.22f + pulseEnvelope_ * 0.20f);
    const float radiusScale = minDim * 0.40f * paramScale_ * paramOrbitSpread_;

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);
    drawBackground(width, height, params.slotOpacity, params.time);
    ofPopView();

    params.camera.begin();
    ofEnableDepthTest();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);

    ofPushMatrix();
    ofScale(paramSceneZoom_, paramSceneZoom_, paramSceneZoom_);

    const ofFloatColor starBase = colorFrom(paramStarR_, paramStarG_, paramStarB_, 1.0f).getLerped(sourceStarColor_, 0.72f);
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    glDepthMask(GL_FALSE);
    drawStarGlow(starRadius, alpha, params.time);
    drawStarRadiance(starRadius, alpha, params.time);
    glDepthMask(GL_TRUE);
    ofDisableBlendMode();
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    drawLivingStarSurface(starRadius, alpha, params.time);
    glDisable(GL_CULL_FACE);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);

    ofPushMatrix();
    ofRotateXDeg(paramOrbitTilt_);
    ofRotateYDeg(paramOrbitRotation_);
    ofScale(1.0f, paramDepth_, 1.0f);

    if (paramShowOrbits_) {
#ifndef TARGET_OPENGLES
        glLineWidth(ofClamp(paramOrbitThickness_, 0.5f, 6.0f));
#endif
        glDepthMask(GL_FALSE);
        for (const auto& body : bodies_) {
            drawOrbitLine(body, radiusScale, alpha * paramOrbitAlpha_);
        }
        glDepthMask(GL_TRUE);
    }

    if (paramShowWaveformBelt_) {
        drawHeliosphereField(radiusScale, alpha, params.time);
    }

    if (paramShowAsteroids_) {
        drawAsteroids(radiusScale, alpha, params.time);
    }

    if (paramShowComets_) {
        drawComets(radiusScale, alpha, params.time);
    }

    drawVisitors(radiusScale, alpha, params.time);

    if (paramShowTrails_) {
#ifndef TARGET_OPENGLES
        glLineWidth(std::max(1.0f, paramOrbitThickness_ * 1.35f));
#endif
        glDepthMask(GL_FALSE);
        for (std::size_t bodyIndex = 0; bodyIndex < bodies_.size(); ++bodyIndex) {
            drawBodyTrail(bodyIndex, bodies_[bodyIndex], radiusScale, alpha);
        }
        glDepthMask(GL_TRUE);
    }

    for (std::size_t bodyIndex = 0; bodyIndex < bodies_.size(); ++bodyIndex) {
        const auto& body = bodies_[bodyIndex];
        const float angle = orbitTime_ * body.speed * TWO_PI + body.phase;
        const glm::vec3 position = orbitPointFor(body, angle, radiusScale, body.phase);
        const float twinkle = ofNoise(body.seed * 0.13f, params.time * (0.28f + body.speed * 0.03f));
        const float planetRadius = std::max(2.2f, minDim * body.radius * paramPlanetSize_);
        const LifeState* life = bodyIndex < lifeStates_.size() ? &lifeStates_[bodyIndex] : nullptr;
        const float biosphere = life != nullptr ? life->biosphereEnergy : 0.0f;
        const float civilization = life != nullptr ? life->civilizationEnergy : 0.0f;
        const float localBandEnergy = life != nullptr ? life->bandEnergy : 0.0f;
        const float currentBandEnergy = life != nullptr ? planetBandEnergyFor(bodyIndex) : 0.0f;
        const float atmospherePulse = smooth01(ofClamp(currentBandEnergy * 0.72f + peak_ * 0.20f + pulseEnvelope_ * 0.22f, 0.0f, 1.25f));
        const ofFloatColor lifeColor = life != nullptr ? life->biosphereColor : body.accentColor;
        ofFloatColor planetColor = body.color.getLerped(starBase, pulseEnvelope_ * 0.12f);
        planetColor = planetColor.getLerped(lifeColor,
                                            ofClamp(biosphere * (0.46f + (life != nullptr ? life->affinity : 0.0f) * 0.28f),
                                                    0.0f,
                                                    0.82f));
        const float atmosphereAudio = bodyIndex < atmosphereEnergy_.size() ? atmosphereEnergy_[bodyIndex] : 0.0f;
        const ofFloatColor atmosphereColor = planetColor.getLerped(lifeColor, ofClamp(biosphere * 0.64f + localBandEnergy * 0.10f, 0.0f, 0.88f))
                                                        .getLerped(ofFloatColor(0.52f, 0.76f, 1.0f, 1.0f), body.atmosphere * 0.14f);
        const float haloAlpha = alpha * (0.060f + body.atmosphere * 0.090f + atmosphereAudio * 0.18f + biosphere * 0.15f +
                                         civilization * 0.030f + twinkle * 0.045f + localBandEnergy * 0.045f + atmospherePulse * 0.16f +
                                         highs_ * paramHighsSparkle_ * audio * 0.018f);

        ofPushMatrix();
        ofTranslate(position.x, position.y, position.z);
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        glDepthMask(GL_FALSE);
        drawLowPolySphere(planetRadius * (1.68f + atmosphereAudio * 1.10f + biosphere * 0.72f + localBandEnergy * 0.22f + atmospherePulse * 0.62f),
                          ofFloatColor(atmosphereColor.r, atmosphereColor.g, atmosphereColor.b, haloAlpha),
                          haloAlpha,
                          body.seed + 2.0f,
                          4,
                          8,
                          false);
        const float breathAlpha = alpha * (0.022f + atmospherePulse * 0.18f + biosphere * 0.050f + atmosphereAudio * 0.030f);
        if (breathAlpha > 0.004f) {
            const ofFloatColor breathColor = atmosphereColor.getLerped(ofFloatColor(0.70f, 0.92f, 1.0f, 1.0f), 0.28f + atmospherePulse * 0.32f);
            drawLowPolySphere(planetRadius * (2.20f + atmosphereAudio * 1.25f + biosphere * 0.95f + atmospherePulse * 1.55f),
                              ofFloatColor(breathColor.r, breathColor.g, breathColor.b, breathAlpha),
                              breathAlpha,
                              body.seed + 87.0f + atmospherePulse * 13.0f,
                              3,
                              6,
                              false);
        }
        glDepthMask(GL_TRUE);

        ofDisableBlendMode();
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        const glm::vec3 sunDirectionLocal = safeNormalize(-position, glm::vec3(-1.0f, 0.0f, 0.0f));
        drawLowPolySphereLit(planetRadius,
                             ofFloatColor(planetColor.r * alpha, planetColor.g * alpha, planetColor.b * alpha, 1.0f),
                             1.0f,
                             body.seed,
                             4,
                             8,
                             false,
                             sunDirectionLocal);
        glDisable(GL_CULL_FACE);

        if (biosphere > 0.02f) {
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            glDepthMask(GL_FALSE);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);
            const float surfaceAlpha = alpha * biosphere * (0.030f + localBandEnergy * 0.030f);
            drawLowPolySphere(planetRadius * (1.012f + biosphere * 0.018f),
                              ofFloatColor(lifeColor.r, lifeColor.g, lifeColor.b, surfaceAlpha),
                              surfaceAlpha,
                              body.seed + 424.0f,
                              4,
                              8,
                              false);
            glDisable(GL_CULL_FACE);
            glDepthMask(GL_TRUE);
        }

        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        ofRotateYDeg(std::fmod(params.time * body.spin * 24.0f + body.seed, 360.0f));
        ofRotateZDeg(body.axialTilt * kRadToDeg);
        drawPlanetBands(body, planetRadius, alpha);

        if (paramShowRings_ && body.rings) {
            drawRingSet(body, planetRadius, alpha);
        }
        ofPopMatrix();

        if (paramShowMoons_ && paramMoonSize_ > 0.0f) {
            for (const auto& moon : body.moons) {
                const float moonAngle = orbitTime_ * moon.speed * paramMoonSpeed_ * TWO_PI + moon.phase;
                const glm::vec3 moonPosition = moonPointFor(body, moon, position, moonAngle, planetRadius);
                const float moonRadius = std::max(1.1f, planetRadius * moon.radius * paramMoonSize_);
                drawMoonOrbit(position, moon, planetRadius, alpha * 0.18f);
                ofPushMatrix();
                ofTranslate(moonPosition.x, moonPosition.y, moonPosition.z);
                ofDisableBlendMode();
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glFrontFace(GL_CCW);
                drawLowPolySphereLit(moonRadius,
                                     ofFloatColor(moon.color.r * alpha, moon.color.g * alpha, moon.color.b * alpha, 1.0f),
                                     1.0f,
                                     moon.seed,
                                     3,
                                     6,
                                     false,
                                     safeNormalize(position - moonPosition, glm::vec3(-1.0f, 0.0f, 0.0f)));
                glDisable(GL_CULL_FACE);
                ofEnableBlendMode(OF_BLENDMODE_ALPHA);
                ofPopMatrix();
            }
        }

        drawArtificialArtifacts(bodyIndex, body, position, planetRadius, alpha, params.time);
        drawLifeSatellite(bodyIndex, body, position, planetRadius, alpha, params.time);
    }

    ofPopMatrix();
    ofPopMatrix();
    ofDisableDepthTest();
    params.camera.end();

    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);
    drawStarEmissionOverlay(width, height, starRadius * paramSceneZoom_, alpha, params.time);
    ofPopView();

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofPopStyle();
}

void SolarSystemLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

void SolarSystemLayer::registerFloat(ParameterRegistry& registry,
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
    meta.group = "Solar System";
    meta.label = label;
    meta.range.min = min;
    meta.range.max = max;
    meta.range.step = step;
    meta.units = units;
    meta.description = description;
    registry.addFloat(id, target, initial, meta);
}

void SolarSystemLayer::readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b) {
    if (!defaults.contains(key) || !defaults[key].is_array() || defaults[key].size() < 3) {
        return;
    }
    r = defaults[key][0].get<float>();
    g = defaults[key][1].get<float>();
    b = defaults[key][2].get<float>();
}

void SolarSystemLayer::clampParams() {
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramScale_ = ofClamp(paramScale_, 0.25f, 1.5f);
    paramSceneZoom_ = ofClamp(paramSceneZoom_, 0.35f, 2.5f);
    paramOrbitSpread_ = ofClamp(paramOrbitSpread_, 0.65f, 2.25f);
    paramOrbitSpeed_ = ofClamp(paramOrbitSpeed_, -3.0f, 3.0f);
    paramOrbitTilt_ = ofClamp(paramOrbitTilt_, -80.0f, 80.0f);
    paramOrbitRotation_ = ofClamp(paramOrbitRotation_, -180.0f, 180.0f);
    paramOrbitPlaneVariation_ = ofClamp(paramOrbitPlaneVariation_, 0.0f, 2.0f);
    paramEccentricity_ = ofClamp(paramEccentricity_, 0.0f, 1.0f);
    paramDepth_ = ofClamp(paramDepth_, 0.1f, 2.5f);
    paramStarSize_ = ofClamp(paramStarSize_, 0.01f, 0.14f);
    paramStarGlow_ = ofClamp(paramStarGlow_, 0.0f, 5.0f);
    paramStarRadiance_ = ofClamp(paramStarRadiance_, 0.0f, 4.0f);
    paramStarEmissionAudio_ = ofClamp(paramStarEmissionAudio_, 0.0f, 3.0f);
    paramStarSurfaceTurbulence_ = ofClamp(paramStarSurfaceTurbulence_, 0.0f, 2.5f);
    paramSolarBurstIntensity_ = ofClamp(paramSolarBurstIntensity_, 0.0f, 3.0f);
    paramSideFillLight_ = ofClamp(paramSideFillLight_, 0.0f, 1.5f);
    paramVisitorEvents_ = ofClamp(paramVisitorEvents_, 0.0f, 1.0f);
    paramArtifactActivity_ = ofClamp(paramArtifactActivity_, 0.0f, 1.0f);
    paramPlanetSize_ = ofClamp(paramPlanetSize_, 0.25f, 3.0f);
    paramObservedDiversity_ = ofClamp(paramObservedDiversity_, 0.0f, 1.0f);
    paramPlanetVariation_ = ofClamp(paramPlanetVariation_, 0.0f, 1.0f);
    paramAsteroidDensity_ = ofClamp(paramAsteroidDensity_, 0.0f, 2.0f);
    paramCometDensity_ = ofClamp(paramCometDensity_, 0.0f, 2.0f);
    paramOrbitAlpha_ = ofClamp(paramOrbitAlpha_, 0.0f, 1.0f);
    paramOrbitThickness_ = ofClamp(paramOrbitThickness_, 0.5f, 6.0f);
    paramTrailAlpha_ = ofClamp(paramTrailAlpha_, 0.0f, 1.0f);
    paramTrailLength_ = ofClamp(paramTrailLength_, 0.02f, 1.0f);
    paramTrailSteps_ = std::round(ofClamp(paramTrailSteps_, 8.0f, 96.0f));
    paramTrailStampGain_ = ofClamp(paramTrailStampGain_, 0.0f, 3.0f);
    paramTrailStampLife_ = ofClamp(paramTrailStampLife_, 2.0f, 36.0f);
    paramAtmosphereGrowth_ = ofClamp(paramAtmosphereGrowth_, 0.0f, 2.0f);
    paramLifeReactivity_ = ofClamp(paramLifeReactivity_, 0.0f, 3.0f);
    paramBiosphereThreshold_ = ofClamp(paramBiosphereThreshold_, 0.05f, 1.2f);
    paramCivilizationGrowth_ = ofClamp(paramCivilizationGrowth_, 0.0f, 2.0f);
    paramMoonSize_ = ofClamp(paramMoonSize_, 0.0f, 2.0f);
    paramMoonSpeed_ = ofClamp(paramMoonSpeed_, -4.0f, 4.0f);
    paramAudioAmount_ = ofClamp(paramAudioAmount_, 0.0f, 3.0f);
    paramAudioSmoothing_ = ofClamp(paramAudioSmoothing_, 0.0f, 0.98f);
    paramBassScale_ = ofClamp(paramBassScale_, 0.0f, 1.5f);
    paramMidsSpeed_ = ofClamp(paramMidsSpeed_, 0.0f, 3.0f);
    paramHighsSparkle_ = ofClamp(paramHighsSparkle_, 0.0f, 3.0f);
    paramWaveformAmount_ = ofClamp(paramWaveformAmount_, 0.0f, 0.20f);
    paramBgAlpha_ = ofClamp(paramBgAlpha_, 0.0f, 1.0f);
    paramBgR_ = ofClamp(paramBgR_, 0.0f, 1.0f);
    paramBgG_ = ofClamp(paramBgG_, 0.0f, 1.0f);
    paramBgB_ = ofClamp(paramBgB_, 0.0f, 1.0f);
    paramStarR_ = ofClamp(paramStarR_, 0.0f, 1.0f);
    paramStarG_ = ofClamp(paramStarG_, 0.0f, 1.0f);
    paramStarB_ = ofClamp(paramStarB_, 0.0f, 1.0f);
    paramOrbitR_ = ofClamp(paramOrbitR_, 0.0f, 1.0f);
    paramOrbitG_ = ofClamp(paramOrbitG_, 0.0f, 1.0f);
    paramOrbitB_ = ofClamp(paramOrbitB_, 0.0f, 1.0f);
    paramTrailR_ = ofClamp(paramTrailR_, 0.0f, 1.0f);
    paramTrailG_ = ofClamp(paramTrailG_, 0.0f, 1.0f);
    paramTrailB_ = ofClamp(paramTrailB_, 0.0f, 1.0f);
    paramSeed_ = std::round(ofClamp(paramSeed_, 0.0f, 99999.0f));
}

void SolarSystemLayer::resetSystem() {
    clampParams();

    seedParamState_ = std::round(paramSeed_);
    diversityState_ = paramObservedDiversity_;
    planetVariationState_ = paramPlanetVariation_;
    orbitPlaneVariationState_ = paramOrbitPlaneVariation_;
    asteroidDensityState_ = paramAsteroidDensity_;
    cometDensityState_ = paramCometDensity_;
    seedState_ = nextRuntimeSeed();

    std::mt19937 rng(seedState_);
    const int hostIndex = randInt(rng, 0, static_cast<int>(kObservedHosts.size()) - 1);
    const char* host = kObservedHosts[static_cast<std::size_t>(hostIndex)];
    sourceSystem_ = host;

    std::vector<const ObservedPlanetData*> selected;
    for (const auto& data : kObservedPlanets) {
        if (sameHost(data.host, host)) {
            selected.push_back(&data);
        }
    }

    std::sort(selected.begin(), selected.end(), [](const ObservedPlanetData* a, const ObservedPlanetData* b) {
        return a->semiMajorAu < b->semiMajorAu;
    });
    if (selected.size() > 11) {
        selected.resize(11);
    }

    bodies_.clear();
    bodies_.reserve(selected.size());

    float maxEccentricity = 0.0f;
    float largestGap = 0.16f;
    for (std::size_t i = 0; i < selected.size(); ++i) {
        const ObservedPlanetData& data = *selected[i];
        sourceStarTemp_ = data.starTeff;
        sourceStarRadiusSolar_ = std::max(0.05f, data.starRadiusSolar);
        sourceStarColor_ = starColorForTemperature(sourceStarTemp_);

        const float radiusEarth = estimateRadiusEarth(data);
        const float massEarth = std::max(0.1f, data.massEarth);
        const float orbitNormData = logMap(data.semiMajorAu, 0.010f, 70.0f, 0.18f, 1.20f);
        const float sequence = selected.size() > 1
            ? static_cast<float>(i) / static_cast<float>(selected.size() - 1)
            : 0.5f;
        const float sequenceOrbit = 0.20f + sequence * 1.02f;
        const float orbitNorm = ofClamp(orbitNormData * (0.48f + paramObservedDiversity_ * 0.20f) +
                                        sequenceOrbit * (0.52f - paramObservedDiversity_ * 0.14f),
                                        0.16f,
                                        1.34f);

        Body body;
        body.sourceName = data.planet;
        body.observedRadiusEarth = radiusEarth;
        body.observedMassEarth = massEarth;
        body.observedOrbitAu = data.semiMajorAu;
        body.observedPeriodDays = data.periodDays;
        body.orbit = std::max(0.18f + static_cast<float>(i) * 0.055f, orbitNorm);
        body.radius = ofClamp((0.006f + std::pow(ofClamp(radiusEarth / 14.5f, 0.02f, 1.0f), 0.58f) * 0.030f) *
                                  randRange(rng, 1.0f - paramPlanetVariation_ * 0.14f, 1.0f + paramPlanetVariation_ * 0.24f),
                              0.008f,
                              0.040f);
        if (!bodies_.empty()) {
            const float orbitScaleUnits = std::max(0.26f, 0.40f * paramScale_ * paramOrbitSpread_);
            const float previousRadiusOrbit = bodies_.back().radius * paramPlanetSize_ / orbitScaleUnits;
            const float radiusOrbit = body.radius * paramPlanetSize_ / orbitScaleUnits;
            const float minGap = 0.070f + previousRadiusOrbit + radiusOrbit + paramPlanetVariation_ * 0.025f;
            body.orbit = std::max(body.orbit, bodies_.back().orbit + minGap);
            largestGap = std::max(largestGap, body.orbit - bodies_.back().orbit);
        }
        body.speed = 365.25f / std::max(0.1f, data.periodDays);
        body.phase = std::fmod(static_cast<float>(i) * 2.39996323f + randRange(rng, -0.24f, 0.24f), TWO_PI);
        if (body.phase < 0.0f) {
            body.phase += TWO_PI;
        }
        body.eccentricity = ofClamp(data.eccentricity, 0.0f, 0.82f);
        const float planeSpread = paramOrbitPlaneVariation_ * (0.72f + paramObservedDiversity_ * 0.32f + paramPlanetVariation_ * 0.18f);
        const float alternatingPlane = (i % 2 == 0 ? 1.0f : -1.0f) * randRange(rng, 0.018f, 0.075f) * planeSpread;
        body.inclination = ofClamp(gaussianish(rng) * (0.10f + paramObservedDiversity_ * 0.22f + paramPlanetVariation_ * 0.14f) * planeSpread +
                                       alternatingPlane,
                                   -0.62f,
                                   0.62f);
        body.orbitYaw = randRange(rng, -0.86f, 0.86f) * (0.30f + paramObservedDiversity_ * 0.55f + paramPlanetVariation_ * 0.38f) * planeSpread;
        body.axialTilt = randRange(rng, -0.7f, 0.7f) * (0.65f + paramPlanetVariation_ * 0.70f);
        body.spin = randRange(rng, -2.2f, 2.2f);
        if (std::abs(body.spin) < 0.35f) {
            body.spin += body.spin < 0.0f ? -0.55f : 0.55f;
        }
        body.seed = randRange(rng, 1.0f, 10000.0f);
        body.color = planetColorFor(data, radiusEarth, paramPlanetVariation_, rng);
        const float hotOrbit = 1.0f - ofClamp(logMap(data.semiMajorAu, 0.01f, 1.0f, 0.0f, 1.0f), 0.0f, 1.0f);
        const float gasBias = ofClamp((radiusEarth - 2.0f) / 10.5f, 0.0f, 1.0f);
        body.atmosphere = ofClamp(gasBias * 0.65f + hotOrbit * 0.22f + randRange(rng, 0.0f, 0.32f) * paramPlanetVariation_, 0.0f, 1.0f);
        body.banding = ofClamp(gasBias * 0.82f + randRange(rng, 0.0f, 0.62f) * paramPlanetVariation_, 0.0f, 1.0f);
        body.storm = ofClamp(gasBias * randRange(rng, 0.18f, 0.92f) + hotOrbit * 0.20f + gaussianish(rng) * 0.16f, 0.0f, 1.0f);
        body.accentColor = body.color.getLerped(ofFloatColor(1.0f, 0.92f, 0.76f, 1.0f), 0.34f + body.atmosphere * 0.22f)
                                     .getLerped(sourceStarColor_, hotOrbit * 0.18f);
        const float ringChance = 0.08f + paramPlanetVariation_ * 0.12f + gasBias * 0.22f;
        body.rings = radiusEarth > 8.0f || massEarth > 90.0f || (radiusEarth > 2.4f && randRange(rng, 0.0f, 1.0f) < ringChance);
        body.ringBands = body.rings ? randInt(rng, 1, 3 + static_cast<int>(std::round(paramPlanetVariation_))) : 0;
        body.ringInner = randRange(rng, 1.45f, 1.85f);
        body.ringOuter = body.ringInner + randRange(rng, 0.55f, 1.35f);
        body.ringTilt = randRange(rng, -0.55f, 0.55f);

        const int maxMoons = radiusEarth > 8.0f ? 4 : (radiusEarth > 3.0f ? 2 : 1);
        const float moonBias = ofClamp(std::log10(std::max(1.0f, massEarth)) / 3.2f, 0.0f, 1.0f);
        const int moonCount = static_cast<int>(std::round((moonBias * (0.34f + paramPlanetVariation_ * 0.18f) +
                                                           randRange(rng, 0.0f, 0.22f + paramPlanetVariation_ * 0.12f)) *
                                                          static_cast<float>(maxMoons)));
        for (int moonIndex = 0; moonIndex < moonCount; ++moonIndex) {
            Moon moon;
            moon.radius = randRange(rng, 0.13f, radiusEarth > 8.0f ? 0.34f : 0.25f);
            moon.distance = 2.2f + static_cast<float>(moonIndex) * randRange(rng, 0.55f, 0.92f) + randRange(rng, 0.0f, 0.42f);
            moon.speed = randRange(rng, 0.55f, 2.8f) * (moonIndex % 2 == 0 ? 1.0f : -1.0f);
            moon.phase = randRange(rng, 0.0f, TWO_PI);
            moon.inclination = randRange(rng, -0.48f, 0.48f);
            moon.seed = randRange(rng, 1.0f, 10000.0f);
            moon.color = ofFloatColor(0.54f + randRange(rng, 0.0f, 0.14f),
                                      0.56f + randRange(rng, 0.0f, 0.14f),
                                      0.58f + randRange(rng, 0.0f, 0.14f),
                                      1.0f)
                             .getLerped(body.accentColor, paramPlanetVariation_ * randRange(rng, 0.0f, 0.10f));
            body.moons.push_back(moon);
        }

        maxEccentricity = std::max(maxEccentricity, body.eccentricity);
        bodies_.push_back(body);
    }

    asteroids_.clear();
    const int asteroidCount = static_cast<int>(std::round((85.0f + static_cast<float>(bodies_.size()) * 11.0f + largestGap * 130.0f) * paramAsteroidDensity_));
    for (int i = 0; i < asteroidCount; ++i) {
        const float beltRoll = randRange(rng, 0.0f, 1.0f);
        float baseOrbit = 0.52f + gaussianish(rng) * 0.045f;
        if (!bodies_.empty()) {
            const std::size_t inner = std::min<std::size_t>(bodies_.size() - 1, static_cast<std::size_t>(std::max(0, randInt(rng, 0, static_cast<int>(bodies_.size()) - 1))));
            const std::size_t outer = std::min<std::size_t>(bodies_.size() - 1, inner + 1);
            baseOrbit = outer > inner
                ? (bodies_[inner].orbit + bodies_[outer].orbit) * 0.5f
                : bodies_[inner].orbit * (beltRoll < 0.5f ? 0.72f : 1.12f);
        }

        Asteroid asteroid;
        asteroid.orbit = ofClamp(baseOrbit + gaussianish(rng) * (0.035f + largestGap * 0.08f), 0.17f, 1.08f);
        asteroid.angle = randRange(rng, 0.0f, TWO_PI);
        asteroid.speed = randRange(rng, 0.015f, 0.11f) * (randRange(rng, 0.0f, 1.0f) < 0.08f ? -1.0f : 1.0f);
        asteroid.eccentricity = ofClamp(randRange(rng, 0.0f, 0.18f) + maxEccentricity * 0.08f, 0.0f, 0.32f);
        asteroid.inclination = gaussianish(rng) * (0.10f + paramObservedDiversity_ * 0.16f);
        asteroid.orbitYaw = randRange(rng, -0.28f, 0.28f);
        asteroid.radius = randRange(rng, 0.95f, 3.6f) * (beltRoll < 0.08f ? 1.8f : 1.0f);
        asteroid.seed = randRange(rng, 1.0f, 10000.0f);
        asteroid.color = ofFloatColor(0.42f + randRange(rng, 0.0f, 0.12f),
                                      0.39f + randRange(rng, 0.0f, 0.10f),
                                      0.35f + randRange(rng, 0.0f, 0.08f),
                                      1.0f);
        asteroids_.push_back(asteroid);
    }

    comets_.clear();
    const int cometCount = static_cast<int>(std::round((2.0f + maxEccentricity * 8.0f + paramObservedDiversity_ * 4.0f) * paramCometDensity_));
    for (int i = 0; i < cometCount; ++i) {
        Comet comet;
        comet.orbit = randRange(rng, 0.58f, 1.25f);
        comet.angle = randRange(rng, 0.0f, TWO_PI);
        comet.speed = randRange(rng, 0.035f, 0.16f) * (randRange(rng, 0.0f, 1.0f) < 0.35f ? -1.0f : 1.0f);
        comet.eccentricity = ofClamp(0.64f + randRange(rng, 0.0f, 0.30f) + maxEccentricity * 0.18f, 0.58f, 0.94f);
        comet.inclination = randRange(rng, -0.88f, 0.88f) * (0.55f + paramObservedDiversity_ * 0.45f);
        comet.orbitYaw = randRange(rng, -1.4f, 1.4f);
        comet.radius = randRange(rng, 1.8f, 4.4f);
        comet.tail = randRange(rng, 0.035f, 0.090f);
        comet.seed = randRange(rng, 1.0f, 10000.0f);
        comet.color = ofFloatColor(0.58f, 0.72f + randRange(rng, 0.0f, 0.08f), 0.82f, 1.0f);
        comets_.push_back(comet);
    }

    artifacts_.clear();
    if (!bodies_.empty()) {
        const int artifactCount = std::min<int>(4, std::max<int>(1, static_cast<int>(std::round(static_cast<float>(bodies_.size()) * 0.42f))));
        for (int i = 0; i < artifactCount; ++i) {
            const int target = randInt(rng, 0, static_cast<int>(bodies_.size()) - 1);
            const Body& targetBody = bodies_[static_cast<std::size_t>(target)];
            Artifact artifact;
            artifact.type = targetBody.observedRadiusEarth > 4.0f && randRange(rng, 0.0f, 1.0f) < 0.48f ? 1 : 0;
            artifact.bodyIndex = target;
            artifact.delay = randRange(rng, 28.0f, 96.0f) + static_cast<float>(i) * randRange(rng, 10.0f, 26.0f);
            artifact.phase = randRange(rng, 0.0f, TWO_PI);
            artifact.orbitDistance = randRange(rng, 2.75f, artifact.type == 1 ? 4.35f : 3.65f);
            artifact.speed = randRange(rng, 0.11f, 0.42f) * (randRange(rng, 0.0f, 1.0f) < 0.35f ? -1.0f : 1.0f);
            artifact.inclination = randRange(rng, -0.72f, 0.72f);
            artifact.size = randRange(rng, 0.045f, artifact.type == 1 ? 0.075f : 0.055f);
            artifact.seed = randRange(rng, 1.0f, 10000.0f);
            artifact.color = artifact.type == 1
                ? ofFloatColor(0.68f, 0.84f, 1.0f, 1.0f)
                : ofFloatColor(0.92f, 0.96f, 1.0f, 1.0f);
            artifacts_.push_back(artifact);
        }
    }

    visitors_.clear();
    const int visitorCount = 7;
    visitors_.reserve(visitorCount);
    for (int i = 0; i < visitorCount; ++i) {
        Visitor visitor;
        visitor.type = i == 0 ? 0 : (randRange(rng, 0.0f, 1.0f) < 0.72f ? 0 : 1);
        visitor.cycle = randRange(rng, 58.0f, visitor.type == 0 ? 138.0f : 180.0f);
        visitor.phase = randRange(rng, 0.0f, 1.0f);
        visitor.duration = randRange(rng, visitor.type == 0 ? 5.5f : 9.0f, visitor.type == 0 ? 11.0f : 16.0f);
        visitor.angle = randRange(rng, 0.0f, TWO_PI);
        visitor.inclination = randRange(rng, -0.86f, 0.86f);
        visitor.yaw = randRange(rng, -0.80f, 0.80f);
        visitor.impact = randRange(rng, -0.42f, 0.42f);
        visitor.size = randRange(rng, visitor.type == 0 ? 2.2f : 2.8f, visitor.type == 0 ? 5.4f : 6.2f);
        visitor.seed = randRange(rng, 1.0f, 10000.0f);
        visitor.color = visitor.type == 0
            ? ofFloatColor(0.48f + randRange(rng, 0.0f, 0.10f), 0.43f + randRange(rng, 0.0f, 0.10f), 0.36f + randRange(rng, 0.0f, 0.08f), 1.0f)
            : ofFloatColor(0.62f, 0.82f + randRange(rng, 0.0f, 0.10f), 1.0f, 1.0f);
        visitors_.push_back(visitor);
    }

    backgroundStars_.clear();
    backgroundStars_.reserve(420);
    for (int i = 0; i < 420; ++i) {
        BackgroundStar star;
        star.position = glm::vec2(randRange(rng, 0.0f, 1.0f), randRange(rng, 0.0f, 1.0f));
        star.depth = std::pow(randRange(rng, 0.0f, 1.0f), 0.72f);
        star.size = randRange(rng, 0.30f, 1.75f) * (0.45f + star.depth * 1.35f);
        star.twinkle = randRange(rng, 0.12f, 1.0f);
        star.drift = randRange(rng, -1.0f, 1.0f) * (0.10f + star.depth * 0.40f);
        const float warmRoll = randRange(rng, 0.0f, 1.0f);
        star.color = warmRoll < 0.28f
            ? ofFloatColor(0.92f + randRange(rng, 0.0f, 0.08f),
                           0.70f + randRange(rng, 0.0f, 0.18f),
                           0.46f + randRange(rng, 0.0f, 0.18f),
                           1.0f)
            : ofFloatColor(0.46f + randRange(rng, 0.0f, 0.34f),
                                  0.58f + randRange(rng, 0.0f, 0.32f),
                                  0.82f + randRange(rng, 0.0f, 0.18f),
                                  1.0f);
        backgroundStars_.push_back(star);
    }

    orbitTime_ = randRange(rng, 0.0f, 100.0f);
    trailStamps_.assign(bodies_.size(), {});
    initializeLifeStates();
    atmosphereEnergy_.assign(bodies_.size(), 0.0f);
    lastTrailStampAngle_.assign(bodies_.size(), -10000.0f);
    systemReady_ = true;
}

void SolarSystemLayer::initializeLifeStates() {
    lifeStates_.assign(bodies_.size(), {});
    const std::size_t count = bodies_.size();
    for (std::size_t i = 0; i < count; ++i) {
        const Body& body = bodies_[i];
        LifeState state;

        if (count == 1) {
            state.bandCenter = 0.50f;
        } else if (count == 2) {
            state.bandCenter = i == 0 ? 0.18f : 0.82f;
        } else if (count == 3) {
            state.bandCenter = i == 0 ? 0.08f : (i == 1 ? 0.50f : 0.92f);
        } else {
            const float sequence = static_cast<float>(i) / static_cast<float>(count - 1);
            const float jitter = (ofNoise(body.seed * 0.0031f, seedState_ * 0.00013f) - 0.5f) * 0.12f;
            state.bandCenter = ofClamp(0.08f + sequence * 0.84f + jitter, 0.03f, 0.97f);
        }

        const float rockyBias = 1.0f - ofClamp((body.observedRadiusEarth - 1.5f) / 10.0f, 0.0f, 1.0f);
        const float atmosphereBias = ofClamp(body.atmosphere * 0.55f + rockyBias * 0.28f, 0.0f, 1.0f);
        const float lifeRoll = ofNoise(body.seed * 0.017f, sourceStarTemp_ * 0.0002f);
        state.affinity = ofClamp(0.34f + atmosphereBias * 0.38f + lifeRoll * 0.34f, 0.18f, 1.0f);
        state.threshold = ofClamp(0.34f + (1.0f - state.affinity) * 0.28f + ofNoise(body.seed * 0.029f) * 0.14f,
                                  0.28f,
                                  0.78f);

        const float palette = ofNoise(body.seed * 0.041f, 7.0f);
        const ofFloatColor leaf(0.18f, 0.78f, 0.34f, 1.0f);
        const ofFloatColor viridian(0.08f, 0.70f, 0.46f, 1.0f);
        const ofFloatColor algae(0.46f, 0.74f, 0.24f, 1.0f);
        const ofFloatColor cyanLife(0.18f, 0.78f, 0.68f, 1.0f);
        if (palette < 0.33f) {
            state.biosphereColor = leaf.getLerped(viridian, palette / 0.33f);
        } else if (palette < 0.66f) {
            state.biosphereColor = viridian.getLerped(algae, (palette - 0.33f) / 0.33f);
        } else {
            state.biosphereColor = algae.getLerped(cyanLife, (palette - 0.66f) / 0.34f);
        }

        state.satellitePhase = std::fmod(body.seed * 0.019f, TWO_PI);
        state.satelliteSpeed = (0.14f + ofNoise(body.seed * 0.011f, 4.0f) * 0.34f) *
                               (ofNoise(body.seed * 0.037f) < 0.38f ? -1.0f : 1.0f);
        state.satelliteDistance = 3.0f + ofNoise(body.seed * 0.023f, 9.0f) * 1.25f;
        state.satelliteInclination = (ofNoise(body.seed * 0.031f, 2.0f) - 0.5f) * 1.10f;
        state.satelliteSize = 0.040f + ofNoise(body.seed * 0.047f, 5.0f) * 0.040f;
        state.satelliteSeed = body.seed + 1729.0f;

        lifeStates_[i] = state;
    }
}

void SolarSystemLayer::updateAudioState(float dt, float timeSeconds) {
    const auto snapshot = AudioAnalysisBus::instance().snapshot();
    const float follow = followAmount(paramAudioSmoothing_);
    hasAudio_ = snapshot.valid;

    if (!snapshot.valid) {
        const float release = ofClamp(follow * 0.20f + dt * 0.35f, 0.0f, 1.0f);
        level_ = ofLerp(level_, 0.0f, release);
        peak_ = ofLerp(peak_, 0.0f, release);
        bass_ = ofLerp(bass_, 0.0f, release);
        mids_ = ofLerp(mids_, 0.0f, release);
        highs_ = ofLerp(highs_, 0.0f, release);
        hasWaveform_ = false;
        return;
    }

    level_ = ofLerp(level_, snapshot.level, follow);
    peak_ = ofLerp(peak_, snapshot.peak, follow);
    bass_ = ofLerp(bass_, snapshot.bass, follow);
    mids_ = ofLerp(mids_, snapshot.mids, follow);
    highs_ = ofLerp(highs_, snapshot.highs, follow);

    if (snapshot.frame != lastAudioFrame_) {
        waveform_ = snapshot.waveform;
        hasWaveform_ = !waveform_.empty();
        lastAudioFrame_ = snapshot.frame;
    }

    if (peak_ > 0.56f && timeSeconds - lastPulseTime_ > 0.18f) {
        pulseEnvelope_ = std::max(pulseEnvelope_, ofClamp(peak_, 0.0f, 1.0f));
        lastPulseTime_ = timeSeconds;
    }
}

float SolarSystemLayer::waveformSampleFor(float normalizedIndex) const {
    if (waveform_.empty()) {
        return 0.0f;
    }

    const float index = wrap01(normalizedIndex) * static_cast<float>(waveform_.size() - 1);
    const std::size_t i0 = static_cast<std::size_t>(std::floor(index));
    const std::size_t i1 = std::min<std::size_t>(i0 + 1, waveform_.size() - 1);
    const float frac = index - static_cast<float>(i0);
    return ofClamp(ofLerp(waveform_[i0], waveform_[i1], frac), -1.0f, 1.0f);
}

float SolarSystemLayer::planetBandEnergyFor(std::size_t bodyIndex) const {
    if (!hasAudio_ || bodyIndex >= lifeStates_.size()) {
        return 0.0f;
    }

    const float center = ofClamp(lifeStates_[bodyIndex].bandCenter, 0.0f, 1.0f);
    float lowWeight = ofClamp(1.0f - std::abs(center) * 2.0f, 0.0f, 1.0f);
    float midWeight = ofClamp(1.0f - std::abs(center - 0.5f) * 2.0f, 0.0f, 1.0f);
    float highWeight = ofClamp(1.0f - std::abs(center - 1.0f) * 2.0f, 0.0f, 1.0f);
    const float weightSum = std::max(0.001f, lowWeight + midWeight + highWeight);
    lowWeight /= weightSum;
    midWeight /= weightSum;
    highWeight /= weightSum;

    const float spectral = bass_ * lowWeight + mids_ * midWeight + highs_ * highWeight;
    const float transient = peak_ * (0.08f + highWeight * 0.10f + midWeight * 0.05f);
    const float bodyNoise = bodyIndex < bodies_.size()
        ? ofNoise(bodies_[bodyIndex].seed * 0.005f, orbitTime_ * 0.09f) * 0.055f
        : 0.0f;
    return ofClamp((spectral + level_ * 0.10f + transient + bodyNoise) * paramAudioAmount_, 0.0f, 1.8f);
}

std::uint32_t SolarSystemLayer::nextRuntimeSeed() const {
    if (paramSeed_ > 0.0f) {
        return static_cast<std::uint32_t>(std::max(1.0f, std::round(paramSeed_)));
    }

    std::random_device rd;
    const auto millis = static_cast<std::uint32_t>(ofGetElapsedTimeMillis() & 0xFFFFFFFFu);
    return (rd() ^ (millis * 1664525u + 1013904223u)) & 0x7FFFFFFFu;
}

glm::vec3 SolarSystemLayer::rotateX(const glm::vec3& value, float radians) const {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return glm::vec3(value.x, value.y * c - value.z * s, value.y * s + value.z * c);
}

glm::vec3 SolarSystemLayer::rotateY(const glm::vec3& value, float radians) const {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return glm::vec3(value.x * c + value.z * s, value.y, -value.x * s + value.z * c);
}

glm::vec3 SolarSystemLayer::rotateZ(const glm::vec3& value, float radians) const {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return glm::vec3(value.x * c - value.y * s, value.x * s + value.y * c, value.z);
}

glm::vec3 SolarSystemLayer::orbitPointFor(const Body& body, float angle, float radiusScale, float waveformPhase, float extraRadius) const {
    (void)waveformPhase;
    const float radius = body.orbit * radiusScale + extraRadius;
    const float e = ofClamp(body.eccentricity * paramEccentricity_, 0.0f, 0.86f);
    const float a = radius;
    const float b = a * std::sqrt(std::max(0.001f, 1.0f - e * e));

    const float M = angle;
    float E = M;
    for (int i = 0; i < 5; ++i) {
        E = E - (E - e * std::sin(E) - M) / std::max(0.0001f, 1.0f - e * std::cos(E));
    }

    glm::vec3 local(a * (std::cos(E) - e),
                    0.0f,
                    b * std::sin(E));
    local = rotateX(local, body.inclination);
    local = rotateY(local, body.orbitYaw);
    return local;
}

glm::vec3 SolarSystemLayer::orbitPointFor(const Asteroid& asteroid, float angle, float radiusScale) const {
    const float radius = asteroid.orbit * radiusScale;
    const float eccentricity = asteroid.eccentricity;
    const float semiMajor = radius * (1.0f + eccentricity * 0.25f);
    const float semiMinor = semiMajor * std::sqrt(std::max(0.08f, 1.0f - eccentricity * eccentricity));
    glm::vec3 local(std::cos(angle) * semiMajor - eccentricity * semiMajor * 0.35f,
                    0.0f,
                    std::sin(angle) * semiMinor);
    local = rotateX(local, asteroid.inclination);
    local = rotateY(local, asteroid.orbitYaw);
    return local;
}

glm::vec3 SolarSystemLayer::orbitPointFor(const Comet& comet, float angle, float radiusScale) const {
    const float radius = comet.orbit * radiusScale;
    const float eccentricity = comet.eccentricity;
    const float semiMajor = radius * (1.0f + eccentricity * 0.45f);
    const float semiMinor = semiMajor * std::sqrt(std::max(0.025f, 1.0f - eccentricity * eccentricity));
    glm::vec3 local(std::cos(angle) * semiMajor - eccentricity * semiMajor * 0.72f,
                    0.0f,
                    std::sin(angle) * semiMinor);
    local = rotateX(local, comet.inclination);
    local = rotateY(local, comet.orbitYaw);
    return local;
}

glm::vec3 SolarSystemLayer::moonPointFor(const Body& body, const Moon& moon, const glm::vec3& bodyPosition, float angle, float planetRadius) const {
    const float distance = planetRadius * moon.distance;
    glm::vec3 local(std::cos(angle) * distance,
                    std::sin(angle * 0.7f + moon.phase) * distance * 0.18f,
                    std::sin(angle) * distance * 0.66f);
    local = rotateX(local, moon.inclination + body.axialTilt * 0.35f);
    local = rotateY(local, body.orbitYaw * 0.5f);
    return glm::vec3(bodyPosition.x + local.x, bodyPosition.y + local.y, bodyPosition.z + local.z);
}

void SolarSystemLayer::drawBackground(float width, float height, float alpha, float timeSeconds) const {
    if (paramBgAlpha_ > 0.0f) {
        setFloatColor(colorFrom(paramBgR_, paramBgG_, paramBgB_, paramBgAlpha_ * alpha));
        ofDrawRectangle(0.0f, 0.0f, width, height);
    }

    ofEnableBlendMode(OF_BLENDMODE_ADD);
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float sparkle = (0.04f + highs_ * paramHighsSparkle_ * audio * 0.42f) * alpha;
    for (std::size_t i = 0; i < backgroundStars_.size(); ++i) {
        const auto& star = backgroundStars_[i];
        const float twinkle = ofNoise(star.twinkle * 41.0f + static_cast<float>(i) * 0.07f,
                                      timeSeconds * (0.035f + star.twinkle * 0.10f));
        const float parallax = 0.0015f + star.depth * 0.0042f;
        const float x = wrap01(star.position.x + timeSeconds * parallax * star.drift);
        const float y = wrap01(star.position.y + std::sin(timeSeconds * 0.012f + star.twinkle * 11.0f) * parallax * 0.62f);
        const float layer = 0.34f + star.depth * 0.86f;
        ofFloatColor color = star.color;
        color.a = alpha * (0.026f + twinkle * 0.20f * layer + sparkle * (0.10f + star.depth * 0.25f));
        setFloatColor(color);
        const float radius = star.size * (0.56f + twinkle * 1.12f);
        ofDrawCircle(x * width, y * height, radius);
        if (star.depth > 0.84f && radius > 1.3f) {
            ofFloatColor glint = color;
            glint.a *= 0.42f;
            setFloatColor(glint);
            const float sx = x * width;
            const float sy = y * height;
            const float arm = radius * (1.8f + star.depth * 1.4f);
            ofDrawLine(sx - arm, sy, sx + arm, sy);
            ofDrawLine(sx, sy - arm * 0.55f, sx, sy + arm * 0.55f);
        }
    }
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
}

void SolarSystemLayer::drawLowPolySphere(float radius,
                                         const ofFloatColor& color,
                                         float alpha,
                                         float seed,
                                         int rings,
                                         int segments,
                                         bool wireframe) const {
    drawLowPolySphereLit(radius,
                         color,
                         alpha,
                         seed,
                         rings,
                         segments,
                         wireframe,
                         glm::vec3(-0.38f, -0.62f, 0.70f));
}

void SolarSystemLayer::drawLowPolySphereLit(float radius,
                                            const ofFloatColor& color,
                                            float alpha,
                                            float seed,
                                            int rings,
                                            int segments,
                                            bool wireframe,
                                            const glm::vec3& lightDirLocal) const {
    if (radius <= 0.0f || alpha <= 0.0f) {
        return;
    }

    ofMesh mesh;
    mesh.setMode(OF_PRIMITIVE_TRIANGLES);

    const glm::vec3 light = safeNormalize(lightDirLocal, glm::vec3(-0.38f, -0.62f, 0.70f));
    const glm::vec3 fillLight = safeNormalize(glm::vec3(0.72f, -0.18f, -0.58f));
    const float fillAmount = ofClamp(paramSideFillLight_, 0.0f, 1.5f);
    auto addTriangle = [&](const glm::vec3& inA, const glm::vec3& inB, const glm::vec3& inC) {
        glm::vec3 a = inA;
        glm::vec3 b = inB;
        glm::vec3 c = inC;
        glm::vec3 normal = safeNormalize(crossProduct(glm::vec3(b.x - a.x, b.y - a.y, b.z - a.z),
                                                      glm::vec3(c.x - a.x, c.y - a.y, c.z - a.z)),
                                         safeNormalize(glm::vec3(a.x + b.x + c.x, a.y + b.y + c.y, a.z + b.z + c.z)));
        const glm::vec3 center = safeNormalize(glm::vec3(a.x + b.x + c.x, a.y + b.y + c.y, a.z + b.z + c.z));
        if (dotProduct(normal, center) < 0.0f) {
            std::swap(b, c);
            normal = glm::vec3(-normal.x, -normal.y, -normal.z);
        }
        const float key = std::max(0.0f, dotProduct(normal, light));
        const float fill = std::max(0.0f, dotProduct(normal, fillLight));
        const float lift = 0.31f + fillAmount * 0.14f;
        const float shade = ofClamp(lift + key * 0.58f + fill * fillAmount * 0.24f + normal.y * 0.065f, 0.30f + fillAmount * 0.10f, 1.18f);
        const float facet = 0.88f + ofNoise(normal.x * 7.0f + seed, normal.y * 7.0f - seed, normal.z * 7.0f) * 0.22f;
        ofFloatColor faceColor(ofClamp(color.r * shade * facet, 0.0f, 1.0f),
                               ofClamp(color.g * shade * facet, 0.0f, 1.0f),
                               ofClamp(color.b * shade * facet, 0.0f, 1.0f),
                               ofClamp(alpha, 0.0f, 1.0f));
        mesh.addVertex(a);
        mesh.addColor(faceColor);
        mesh.addVertex(b);
        mesh.addColor(faceColor);
        mesh.addVertex(c);
        mesh.addColor(faceColor);
    };

    const float phi = (1.0f + std::sqrt(5.0f)) * 0.5f;
    std::array<glm::vec3, 12> vertices = {
        glm::vec3(-1.0f, phi, 0.0f),
        glm::vec3(1.0f, phi, 0.0f),
        glm::vec3(-1.0f, -phi, 0.0f),
        glm::vec3(1.0f, -phi, 0.0f),
        glm::vec3(0.0f, -1.0f, phi),
        glm::vec3(0.0f, 1.0f, phi),
        glm::vec3(0.0f, -1.0f, -phi),
        glm::vec3(0.0f, 1.0f, -phi),
        glm::vec3(phi, 0.0f, -1.0f),
        glm::vec3(phi, 0.0f, 1.0f),
        glm::vec3(-phi, 0.0f, -1.0f),
        glm::vec3(-phi, 0.0f, 1.0f)
    };
    for (auto& vertex : vertices) {
        vertex = safeNormalize(vertex);
    }

    const std::array<std::array<int, 3>, 20> baseFaces = { {
        { 0, 11, 5 },
        { 0, 5, 1 },
        { 0, 1, 7 },
        { 0, 7, 10 },
        { 0, 10, 11 },
        { 1, 5, 9 },
        { 5, 11, 4 },
        { 11, 10, 2 },
        { 10, 7, 6 },
        { 7, 1, 8 },
        { 3, 9, 4 },
        { 3, 4, 2 },
        { 3, 2, 6 },
        { 3, 6, 8 },
        { 3, 8, 9 },
        { 4, 9, 5 },
        { 2, 4, 11 },
        { 6, 2, 10 },
        { 8, 6, 7 },
        { 9, 8, 1 }
    } };

    std::vector<std::array<glm::vec3, 3>> triangles;
    triangles.reserve(baseFaces.size());
    for (const auto& face : baseFaces) {
        triangles.push_back({ vertices[static_cast<std::size_t>(face[0])],
                              vertices[static_cast<std::size_t>(face[1])],
                              vertices[static_cast<std::size_t>(face[2])] });
    }

    const int detailHint = std::max(std::max(3, rings), std::max(5, segments));
    const int subdivisions = detailHint >= 10 ? 2 : (detailHint >= 7 ? 1 : 0);
    for (int level = 0; level < subdivisions; ++level) {
        std::vector<std::array<glm::vec3, 3>> refined;
        refined.reserve(triangles.size() * 4);
        for (const auto& tri : triangles) {
            const glm::vec3 ab = safeNormalize(tri[0] + tri[1]);
            const glm::vec3 bc = safeNormalize(tri[1] + tri[2]);
            const glm::vec3 ca = safeNormalize(tri[2] + tri[0]);
            refined.push_back({ tri[0], ab, ca });
            refined.push_back({ tri[1], bc, ab });
            refined.push_back({ tri[2], ca, bc });
            refined.push_back({ ab, bc, ca });
        }
        triangles = std::move(refined);
    }

    auto geodesicPoint = [&](const glm::vec3& unit) {
        const float jitter = 0.975f + ofNoise(unit.x * 4.73f + seed * 0.013f,
                                             unit.y * 5.19f - seed * 0.017f,
                                             unit.z * 4.41f + seed * 0.011f) *
                                        0.050f;
        return unit * radius * jitter;
    };

    for (const auto& tri : triangles) {
        addTriangle(geodesicPoint(tri[0]), geodesicPoint(tri[1]), geodesicPoint(tri[2]));
    }

    ofSetColor(255);
    mesh.draw();

    if (wireframe) {
        ofFloatColor edge = color.getLerped(ofFloatColor(1.0f, 1.0f, 1.0f, 1.0f), 0.55f);
        edge.a = alpha * 0.16f;
        setFloatColor(edge);
#ifndef TARGET_OPENGLES
        glLineWidth(1.0f);
#endif
        mesh.drawWireframe();
    }
}

void SolarSystemLayer::drawLivingStarSurface(float starRadius, float alpha, float timeSeconds) const {
    if (starRadius <= 0.0f || alpha <= 0.0f) {
        return;
    }

    const ofFloatColor starBase = colorFrom(paramStarR_, paramStarG_, paramStarB_, 1.0f).getLerped(sourceStarColor_, 0.72f);
    const float audio = hasAudio_ ? paramAudioAmount_ * paramStarEmissionAudio_ : 0.0f;
    const float bassDrive = bass_ * paramBassScale_ * audio;
    const float peakDrive = peak_ * audio;
    const float highDrive = highs_ * paramHighsSparkle_ * audio;
    const float turbulence = paramStarSurfaceTurbulence_ * (0.58f + bassDrive * 0.72f + peakDrive * 0.55f + pulseEnvelope_ * 0.85f);
    const float flow = timeSeconds * (0.34f + mids_ * paramMidsSpeed_ * audio * 0.18f + paramStarSurfaceTurbulence_ * 0.06f);
    const float seed = static_cast<float>(seedState_) * 0.001f + 19.31f;

    struct StarSurfaceVertex {
        glm::vec3 position;
        float heat = 0.0f;
        float spike = 0.0f;
    };

    auto sampleSurface = [&](const glm::vec3& inUnit) {
        const glm::vec3 unit = safeNormalize(inUnit);
        const float convection = ofNoise(unit.x * 3.7f + seed,
                                         unit.y * 3.7f + flow * 0.19f,
                                         unit.z * 3.7f - flow * 0.13f);
        const float licks = std::pow(ofNoise(unit.x * 12.0f - flow * 0.58f,
                                             unit.y * 12.0f + seed * 0.43f,
                                             unit.z * 12.0f + flow * 0.42f),
                                     2.6f);
        const float needles = std::pow(ofNoise(unit.x * 28.0f + flow * 1.45f + seed,
                                               unit.y * 28.0f - flow * 1.16f,
                                               unit.z * 28.0f + seed * 0.27f),
                                       6.2f);
        const float burstNeedles = std::pow(ofNoise(unit.x * 44.0f - flow * 2.55f,
                                                    unit.y * 44.0f + flow * 2.05f + seed,
                                                    unit.z * 44.0f - seed * 0.31f),
                                            9.0f);
        const float surfaceWave = (convection - 0.44f) * 0.115f * turbulence;
        const float spike = (licks * 0.125f + needles * 0.43f + burstNeedles * (0.58f + peakDrive * 0.44f)) *
                            turbulence *
                            (0.76f + highDrive * 0.28f);
        const float breathing = std::sin(timeSeconds * 1.45f + unit.x * 2.1f + unit.z * 2.7f) * 0.018f * (1.0f + bassDrive);
        const float radiusScale = ofClamp(1.0f + surfaceWave + spike + breathing, 0.82f, 1.84f);

        StarSurfaceVertex out;
        out.position = unit * starRadius * radiusScale;
        out.spike = spike;
        out.heat = ofClamp(0.35f + convection * 0.34f + licks * 0.28f + needles * 0.32f + burstNeedles * 0.42f + bassDrive * 0.16f, 0.0f, 1.65f);
        return out;
    };

    auto addTriangle = [&](ofMesh& mesh, StarSurfaceVertex a, StarSurfaceVertex b, StarSurfaceVertex c) {
        glm::vec3 normal = safeNormalize(crossProduct(glm::vec3(b.position.x - a.position.x, b.position.y - a.position.y, b.position.z - a.position.z),
                                                      glm::vec3(c.position.x - a.position.x, c.position.y - a.position.y, c.position.z - a.position.z)),
                                         safeNormalize(a.position + b.position + c.position));
        const glm::vec3 center = safeNormalize(a.position + b.position + c.position);
        if (dotProduct(normal, center) < 0.0f) {
            std::swap(b, c);
            normal = glm::vec3(-normal.x, -normal.y, -normal.z);
        }

        const float heat = ofClamp((a.heat + b.heat + c.heat) / 3.0f, 0.0f, 1.45f);
        const float spike = std::max(a.spike, std::max(b.spike, c.spike));
        const float facet = 0.88f + ofNoise(center.x * 9.0f + seed, center.y * 9.0f - seed, center.z * 9.0f + flow) * 0.20f;
        const ofFloatColor ember = starBase.getLerped(ofFloatColor(1.0f, 0.30f, 0.04f, 1.0f), 0.34f);
        const ofFloatColor gold = starBase.getLerped(ofFloatColor(1.0f, 0.82f, 0.18f, 1.0f), 0.40f);
        const ofFloatColor whiteHot = ofFloatColor(1.0f, 0.98f, 0.78f, 1.0f);
        ofFloatColor faceColor = ember.getLerped(gold, ofClamp(heat * 0.62f, 0.0f, 1.0f))
                                      .getLerped(whiteHot, ofClamp(spike * 1.85f + heat * 0.20f, 0.0f, 0.82f));
        const float emission = ofClamp((0.78f + heat * 0.28f + spike * 0.95f + pulseEnvelope_ * 0.22f) * facet, 0.0f, 1.28f);
        faceColor.r = ofClamp(faceColor.r * emission * alpha, 0.0f, 1.0f);
        faceColor.g = ofClamp(faceColor.g * emission * alpha, 0.0f, 1.0f);
        faceColor.b = ofClamp(faceColor.b * emission * alpha, 0.0f, 1.0f);
        faceColor.a = 1.0f;

        mesh.addVertex(a.position);
        mesh.addColor(faceColor);
        mesh.addVertex(b.position);
        mesh.addColor(faceColor);
        mesh.addVertex(c.position);
        mesh.addColor(faceColor);
    };

    const float phi = (1.0f + std::sqrt(5.0f)) * 0.5f;
    std::array<glm::vec3, 12> vertices = {
        glm::vec3(-1.0f, phi, 0.0f),
        glm::vec3(1.0f, phi, 0.0f),
        glm::vec3(-1.0f, -phi, 0.0f),
        glm::vec3(1.0f, -phi, 0.0f),
        glm::vec3(0.0f, -1.0f, phi),
        glm::vec3(0.0f, 1.0f, phi),
        glm::vec3(0.0f, -1.0f, -phi),
        glm::vec3(0.0f, 1.0f, -phi),
        glm::vec3(phi, 0.0f, -1.0f),
        glm::vec3(phi, 0.0f, 1.0f),
        glm::vec3(-phi, 0.0f, -1.0f),
        glm::vec3(-phi, 0.0f, 1.0f)
    };
    for (auto& vertex : vertices) {
        vertex = safeNormalize(vertex);
    }

    const std::array<std::array<int, 3>, 20> baseFaces = { {
        { 0, 11, 5 },
        { 0, 5, 1 },
        { 0, 1, 7 },
        { 0, 7, 10 },
        { 0, 10, 11 },
        { 1, 5, 9 },
        { 5, 11, 4 },
        { 11, 10, 2 },
        { 10, 7, 6 },
        { 7, 1, 8 },
        { 3, 9, 4 },
        { 3, 4, 2 },
        { 3, 2, 6 },
        { 3, 6, 8 },
        { 3, 8, 9 },
        { 4, 9, 5 },
        { 2, 4, 11 },
        { 6, 2, 10 },
        { 8, 6, 7 },
        { 9, 8, 1 }
    } };

    std::vector<std::array<glm::vec3, 3>> triangles;
    triangles.reserve(baseFaces.size());
    for (const auto& face : baseFaces) {
        triangles.push_back({ vertices[static_cast<std::size_t>(face[0])],
                              vertices[static_cast<std::size_t>(face[1])],
                              vertices[static_cast<std::size_t>(face[2])] });
    }

    const int subdivisions = 3;
    for (int level = 0; level < subdivisions; ++level) {
        std::vector<std::array<glm::vec3, 3>> refined;
        refined.reserve(triangles.size() * 4);
        for (const auto& tri : triangles) {
            const glm::vec3 ab = safeNormalize(tri[0] + tri[1]);
            const glm::vec3 bc = safeNormalize(tri[1] + tri[2]);
            const glm::vec3 ca = safeNormalize(tri[2] + tri[0]);
            refined.push_back({ tri[0], ab, ca });
            refined.push_back({ tri[1], bc, ab });
            refined.push_back({ tri[2], ca, bc });
            refined.push_back({ ab, bc, ca });
        }
        triangles = std::move(refined);
    }

    ofMesh mesh;
    mesh.setMode(OF_PRIMITIVE_TRIANGLES);
    mesh.getVertices().reserve(triangles.size() * 3);
    mesh.getColors().reserve(triangles.size() * 3);
    for (const auto& tri : triangles) {
        addTriangle(mesh, sampleSurface(tri[0]), sampleSurface(tri[1]), sampleSurface(tri[2]));
    }

    ofSetColor(255);
    mesh.draw();

    if (paramStarSurfaceTurbulence_ > 0.01f) {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        ofFloatColor wire = starBase.getLerped(ofFloatColor(1.0f, 0.96f, 0.72f, 1.0f), 0.58f);
        wire.a = alpha * ofClamp(0.045f + turbulence * 0.020f + highDrive * 0.035f, 0.0f, 0.18f);
        setFloatColor(wire);
#ifndef TARGET_OPENGLES
        glLineWidth(1.0f);
#endif
        mesh.drawWireframe();
        ofDisableBlendMode();
    }
}

void SolarSystemLayer::drawStarEmissionOverlay(float width, float height, float starRadius, float alpha, float timeSeconds) const {
    if (width <= 1.0f || height <= 1.0f || starRadius <= 0.0f || alpha <= 0.0f || paramStarGlow_ <= 0.0f) {
        return;
    }

    const ofFloatColor starBase = colorFrom(paramStarR_, paramStarG_, paramStarB_, 1.0f).getLerped(sourceStarColor_, 0.72f);
    const float audio = hasAudio_ ? paramAudioAmount_ * paramStarEmissionAudio_ : 0.0f;
    const float bassDrive = bass_ * paramBassScale_;
    const float levelDrive = level_ * 0.70f + peak_ * 0.90f;
    const float highDrive = highs_ * paramHighsSparkle_;
    const float pulse = pulseEnvelope_ * 1.25f + bassDrive * audio * 1.10f + levelDrive * audio * 0.65f;
    const float emission = paramStarGlow_ * (1.0f + pulse);
    const float radiance = paramStarRadiance_ * (0.75f + highDrive * audio * 0.35f);
    const float screenRadius = std::max(6.0f, starRadius);
    const glm::vec2 center(width * 0.5f, height * 0.5f);
    const float viewportLimit = std::max(width, height);

    auto addRadialDisc = [&](ofMesh& mesh,
                             float innerRadius,
                             float outerRadius,
                             const ofFloatColor& inner,
                             const ofFloatColor& outer,
                             float spin,
                             int steps) {
        for (int i = 0; i < steps; ++i) {
            const float a0 = static_cast<float>(i) / static_cast<float>(steps) * TWO_PI + spin;
            const float a1 = static_cast<float>(i + 1) / static_cast<float>(steps) * TWO_PI + spin;
            const glm::vec3 p0(center.x + std::cos(a0) * innerRadius, center.y + std::sin(a0) * innerRadius, 0.0f);
            const glm::vec3 p1(center.x + std::cos(a1) * innerRadius, center.y + std::sin(a1) * innerRadius, 0.0f);
            const glm::vec3 p2(center.x + std::cos(a0) * outerRadius, center.y + std::sin(a0) * outerRadius, 0.0f);
            const glm::vec3 p3(center.x + std::cos(a1) * outerRadius, center.y + std::sin(a1) * outerRadius, 0.0f);

            mesh.addVertex(p0);
            mesh.addColor(inner);
            mesh.addVertex(p2);
            mesh.addColor(outer);
            mesh.addVertex(p1);
            mesh.addColor(inner);

            mesh.addVertex(p1);
            mesh.addColor(inner);
            mesh.addVertex(p2);
            mesh.addColor(outer);
            mesh.addVertex(p3);
            mesh.addColor(outer);
        }
    };

    ofEnableBlendMode(OF_BLENDMODE_ADD);

    ofMesh bloom;
    bloom.setMode(OF_PRIMITIVE_TRIANGLES);
    const int bloomLayers = 7;
    for (int layer = 0; layer < bloomLayers; ++layer) {
        const float pct = static_cast<float>(layer) / static_cast<float>(bloomLayers - 1);
        const float drift = ofNoise(seedState_ * 0.015f, timeSeconds * (0.18f + pct * 0.05f), pct * 3.0f);
        const float falloff = std::pow(1.0f - pct, 1.45f);
        const float innerRadius = screenRadius * (0.56f + pct * 0.14f);
        const float outerRadius = std::min(viewportLimit * 0.74f,
                                           screenRadius * (1.65f + pct * (2.9f + emission * 0.48f) + emission * 0.23f + drift * 0.55f));
        ofFloatColor inner = starBase.getLerped(ofFloatColor(1.0f, 0.96f, 0.76f, 1.0f), 0.42f + pct * 0.16f);
        ofFloatColor outer = starBase.getLerped(ofFloatColor(1.0f, 0.50f, 0.12f, 1.0f), 0.20f);
        inner.a = alpha * (0.20f + emission * 0.026f) * falloff;
        outer.a = alpha * (0.028f + emission * 0.0035f) * falloff;
        addRadialDisc(bloom,
                      innerRadius,
                      outerRadius,
                      inner,
                      outer,
                      timeSeconds * (0.010f + pct * 0.012f) + drift * 0.2f,
                      96);
    }
    bloom.draw();

    ofMesh rays;
    rays.setMode(OF_PRIMITIVE_LINES);
    const int rayCount = 42;
    for (int i = 0; i < rayCount; ++i) {
        const float pct = static_cast<float>(i) / static_cast<float>(rayCount);
        const float angle = pct * TWO_PI + ofNoise(seedState_ * 0.009f, pct * 8.0f) * 0.42f;
        const float flicker = 0.58f + ofNoise(seedState_ * 0.031f + pct * 2.0f, timeSeconds * 1.1f) * 0.76f;
        const float inner = screenRadius * (0.92f + flicker * 0.14f);
        const float outer = std::min(viewportLimit * 0.66f,
                                     screenRadius * (2.9f + radiance * 1.45f + emission * 0.24f + flicker * 1.35f));
        ofFloatColor hot = starBase.getLerped(ofFloatColor(1.0f, 0.98f, 0.84f, 1.0f), 0.52f);
        hot.a = alpha * (0.12f + radiance * 0.018f + highDrive * audio * 0.035f) * flicker;
        const ofFloatColor edge(hot.r, hot.g, hot.b, 0.0f);
        rays.addVertex(glm::vec3(center.x + std::cos(angle) * inner, center.y + std::sin(angle) * inner, 0.0f));
        rays.addColor(hot);
        rays.addVertex(glm::vec3(center.x + std::cos(angle) * outer, center.y + std::sin(angle) * outer, 0.0f));
        rays.addColor(edge);
    }
#ifndef TARGET_OPENGLES
    glLineWidth(std::max(1.5f, 2.0f + emission * 0.22f + highDrive * audio * 1.4f));
#endif
    rays.draw();

    const float burstEnergy = ofClamp(paramSolarBurstIntensity_ *
                                          (0.20f + pulseEnvelope_ * 1.15f + peak_ * audio * 1.18f + bassDrive * audio * 0.72f + level_ * audio * 0.35f),
                                      0.0f,
                                      5.0f);
    if (burstEnergy > 0.01f) {
        ofMesh plumes;
        plumes.setMode(OF_PRIMITIVE_TRIANGLES);

        auto rotatedDir = [](float angle) {
            return glm::vec2(std::cos(angle), std::sin(angle));
        };
        auto perpendicular = [](const glm::vec2& value) {
            return glm::vec2(-value.y, value.x);
        };
        auto addRibbonSegment = [&](const glm::vec2& left0,
                                    const glm::vec2& right0,
                                    const glm::vec2& left1,
                                    const glm::vec2& right1,
                                    const ofFloatColor& color0,
                                    const ofFloatColor& color1) {
            plumes.addVertex(glm::vec3(left0.x, left0.y, 0.0f));
            plumes.addColor(color0);
            plumes.addVertex(glm::vec3(left1.x, left1.y, 0.0f));
            plumes.addColor(color1);
            plumes.addVertex(glm::vec3(right0.x, right0.y, 0.0f));
            plumes.addColor(color0);

            plumes.addVertex(glm::vec3(right0.x, right0.y, 0.0f));
            plumes.addColor(color0);
            plumes.addVertex(glm::vec3(left1.x, left1.y, 0.0f));
            plumes.addColor(color1);
            plumes.addVertex(glm::vec3(right1.x, right1.y, 0.0f));
            plumes.addColor(color1);
        };

        const int burstCount = 9;
        for (int i = 0; i < burstCount; ++i) {
            const float pct = static_cast<float>(i) / static_cast<float>(burstCount);
            const float gate = ofNoise(seedState_ * 0.023f + pct * 4.1f, timeSeconds * 0.33f + pct * 2.7f);
            const float trigger = ofClamp((gate - 0.34f) / 0.66f, 0.0f, 1.0f);
            const float localEnergy = burstEnergy * (0.34f + trigger * trigger * 1.25f);
            const float angle = pct * TWO_PI + ofNoise(seedState_ * 0.011f, pct * 8.0f) * 0.78f + timeSeconds * (0.010f + trigger * 0.006f);
            const float curl = (ofNoise(seedState_ * 0.017f, pct * 6.0f, timeSeconds * 0.11f) - 0.5f) * (0.62f + localEnergy * 0.10f);
            const float start = screenRadius * (0.82f + trigger * 0.10f);
            const float length = std::min(viewportLimit * 0.60f,
                                          screenRadius * (2.0f + localEnergy * 0.58f + trigger * 2.15f));
            const float baseWidth = screenRadius * (0.095f + localEnergy * 0.018f + trigger * 0.035f);
            const int segments = 8;

            for (int segment = 0; segment < segments; ++segment) {
                const float t0 = static_cast<float>(segment) / static_cast<float>(segments);
                const float t1 = static_cast<float>(segment + 1) / static_cast<float>(segments);
                const float a0 = angle + curl * std::pow(t0, 1.25f) + std::sin(timeSeconds * 0.72f + pct * 9.0f + t0 * 2.1f) * 0.035f;
                const float a1 = angle + curl * std::pow(t1, 1.25f) + std::sin(timeSeconds * 0.72f + pct * 9.0f + t1 * 2.1f) * 0.035f;
                const glm::vec2 dir0 = rotatedDir(a0);
                const glm::vec2 dir1 = rotatedDir(a1);
                const glm::vec2 c0 = center + dir0 * (start + (length - start) * t0);
                const glm::vec2 c1 = center + dir1 * (start + (length - start) * t1);
                const float w0 = baseWidth * std::pow(1.0f - t0, 0.72f) * (0.66f + trigger * 0.50f);
                const float w1 = baseWidth * std::pow(1.0f - t1, 0.72f) * (0.66f + trigger * 0.50f);
                const glm::vec2 side0 = perpendicular(dir0);
                const glm::vec2 side1 = perpendicular(dir1);

                ofFloatColor hot = starBase.getLerped(ofFloatColor(1.0f, 0.97f, 0.76f, 1.0f), 0.52f);
                ofFloatColor edge = starBase.getLerped(ofFloatColor(1.0f, 0.24f, 0.04f, 1.0f), 0.36f);
                hot.a = alpha * localEnergy * std::pow(1.0f - t0, 1.35f) * 0.075f;
                edge.a = alpha * localEnergy * std::pow(1.0f - t1, 1.55f) * 0.020f;
                addRibbonSegment(c0 - side0 * w0,
                                 c0 + side0 * w0,
                                 c1 - side1 * w1,
                                 c1 + side1 * w1,
                                 hot,
                                 edge);
            }
        }
        plumes.draw();
    }

    const float ringEnergy = ofClamp(pulse + peak_ * audio * 0.85f, 0.0f, 3.0f);
    if (ringEnergy > 0.01f) {
        ofMesh shockwaves;
        shockwaves.setMode(OF_PRIMITIVE_TRIANGLES);
        const int rings = 3;
        for (int i = 0; i < rings; ++i) {
            const float phase = wrap01(timeSeconds * (0.18f + bassDrive * audio * 0.09f) + static_cast<float>(i) / static_cast<float>(rings));
            const float radius = screenRadius * (1.25f + phase * (3.8f + emission * 0.35f));
            const float thickness = screenRadius * (0.035f + ringEnergy * 0.018f);
            ofFloatColor inner = starBase.getLerped(ofFloatColor(1.0f, 0.98f, 0.82f, 1.0f), 0.45f);
            ofFloatColor outer = inner;
            inner.a = alpha * ringEnergy * (1.0f - phase) * 0.070f;
            outer.a = 0.0f;
            addRadialDisc(shockwaves, radius, radius + thickness, inner, outer, phase * TWO_PI * 0.25f, 96);
        }
        shockwaves.draw();
    }

    ofMesh coreBloom;
    coreBloom.setMode(OF_PRIMITIVE_TRIANGLES);
    ofFloatColor core = starBase.getLerped(ofFloatColor(1.0f, 0.99f, 0.88f, 1.0f), 0.70f);
    ofFloatColor rim = starBase.getLerped(ofFloatColor(1.0f, 0.70f, 0.18f, 1.0f), 0.30f);
    core.a = alpha * (0.34f + emission * 0.030f);
    rim.a = alpha * (0.090f + emission * 0.010f);
    addRadialDisc(coreBloom, screenRadius * 0.34f, screenRadius * (1.34f + emission * 0.030f), core, rim, -timeSeconds * 0.018f, 72);
    coreBloom.draw();

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
}

void SolarSystemLayer::drawStarGlow(float starRadius, float alpha, float timeSeconds) const {
    if (starRadius <= 0.0f || alpha <= 0.0f || paramStarGlow_ <= 0.0f) {
        return;
    }

    const ofFloatColor starBase = colorFrom(paramStarR_, paramStarG_, paramStarB_, 1.0f).getLerped(sourceStarColor_, 0.72f);
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float bloom = paramStarGlow_ * (1.0f + bass_ * paramBassScale_ * audio * 0.55f + pulseEnvelope_ * 0.55f);
    const float flutter = 0.86f + ofNoise(seedState_ * 0.019f, timeSeconds * 0.34f) * 0.28f;

    auto addDisc = [&](ofMesh& mesh,
                       float innerRadius,
                       float outerRadius,
                       const ofFloatColor& inner,
                       const ofFloatColor& outer,
                       float spin,
                       int steps) {
        for (int i = 0; i < steps; ++i) {
            const float a0 = static_cast<float>(i) / static_cast<float>(steps) * TWO_PI + spin;
            const float a1 = static_cast<float>(i + 1) / static_cast<float>(steps) * TWO_PI + spin;
            const glm::vec3 p0(std::cos(a0) * innerRadius, std::sin(a0) * innerRadius, 0.0f);
            const glm::vec3 p1(std::cos(a1) * innerRadius, std::sin(a1) * innerRadius, 0.0f);
            const glm::vec3 p2(std::cos(a0) * outerRadius, std::sin(a0) * outerRadius, 0.0f);
            const glm::vec3 p3(std::cos(a1) * outerRadius, std::sin(a1) * outerRadius, 0.0f);

            mesh.addVertex(p0);
            mesh.addColor(inner);
            mesh.addVertex(p2);
            mesh.addColor(outer);
            mesh.addVertex(p1);
            mesh.addColor(inner);

            mesh.addVertex(p1);
            mesh.addColor(inner);
            mesh.addVertex(p2);
            mesh.addColor(outer);
            mesh.addVertex(p3);
            mesh.addColor(outer);
        }
    };

    ofEnableBlendMode(OF_BLENDMODE_ADD);

    ofMesh corona;
    corona.setMode(OF_PRIMITIVE_TRIANGLES);
    const int layers = 5;
    for (int layer = 0; layer < layers; ++layer) {
        const float pct = static_cast<float>(layer) / static_cast<float>(layers - 1);
        const float soft = 1.0f - pct * 0.72f;
        const float innerRadius = starRadius * (0.86f + pct * 0.16f);
        const float outerRadius = starRadius * (2.0f + pct * (2.2f + bloom * 0.95f) + bloom * 0.38f);
        ofFloatColor inner = starBase.getLerped(ofFloatColor(1.0f, 0.96f, 0.70f, 1.0f), 0.32f + pct * 0.18f);
        ofFloatColor outer = starBase.getLerped(ofFloatColor(1.0f, 0.82f, 0.36f, 1.0f), 0.18f);
        inner.a = alpha * (0.12f + bloom * 0.030f) * soft * flutter;
        outer.a = alpha * (0.010f + bloom * 0.004f) * (1.0f - pct) * flutter;
        addDisc(corona,
                innerRadius,
                outerRadius,
                inner,
                outer,
                timeSeconds * (0.018f + pct * 0.010f) + pct * 0.37f,
                84);
    }
    corona.draw();

    ofMesh innerFire;
    innerFire.setMode(OF_PRIMITIVE_TRIANGLES);
    ofFloatColor hot = starBase.getLerped(ofFloatColor(1.0f, 0.98f, 0.86f, 1.0f), 0.58f);
    ofFloatColor edge = starBase.getLerped(ofFloatColor(1.0f, 0.58f, 0.16f, 1.0f), 0.34f);
    hot.a = alpha * (0.18f + bloom * 0.026f) * flutter;
    edge.a = alpha * (0.040f + bloom * 0.012f) * flutter;
    addDisc(innerFire, starRadius * 0.74f, starRadius * (1.42f + bloom * 0.13f), hot, edge, -timeSeconds * 0.025f, 64);
    innerFire.draw();
}

void SolarSystemLayer::drawStarRadiance(float starRadius, float alpha, float timeSeconds) const {
    if (starRadius <= 0.0f || alpha <= 0.0f || paramStarRadiance_ <= 0.0f) {
        return;
    }

    const ofFloatColor starBase = colorFrom(paramStarR_, paramStarG_, paramStarB_, 1.0f).getLerped(sourceStarColor_, 0.72f);
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float bassLuminosity = 1.0f + bass_ * paramBassScale_ * audio * 0.65f;
    const float midWind = 1.0f + mids_ * paramMidsSpeed_ * audio * 0.35f;
    const float highSpark = highs_ * paramHighsSparkle_ * audio;
    const float radiance = paramStarRadiance_ * bassLuminosity * (1.0f + pulseEnvelope_ * 0.42f);

    ofEnableBlendMode(OF_BLENDMODE_ADD);
    ofMesh rays;
    rays.setMode(OF_PRIMITIVE_LINES);
    const int rayCount = 44;
    for (int i = 0; i < rayCount; ++i) {
        const float pct = static_cast<float>(i) / static_cast<float>(rayCount);
        const float angle = pct * TWO_PI + ofNoise(seedState_ * 0.007f, pct * 9.0f) * 0.34f;
        const float y = (ofNoise(seedState_ * 0.013f, pct * 14.0f, 0.37f) - 0.5f) * 0.62f;
        const float flat = std::sqrt(std::max(0.08f, 1.0f - y * y));
        const glm::vec3 dir = safeNormalize(glm::vec3(std::cos(angle) * flat, y, std::sin(angle) * flat),
                                             glm::vec3(std::cos(angle), 0.0f, std::sin(angle)));
        const float flicker = 0.72f + ofNoise(seedState_ * 0.021f + pct * 3.1f, timeSeconds * 0.26f) * 0.56f;
        const float inner = starRadius * (1.02f + ofNoise(pct * 8.0f, seedState_ * 0.003f) * 0.18f);
        const float outer = starRadius * (3.1f + radiance * 2.85f * midWind + flicker * 1.35f);
        ofFloatColor hot = starBase.getLerped(ofFloatColor(1.0f, 0.98f, 0.82f, 1.0f), 0.34f);
        ofFloatColor cool = starBase.getLerped(ofFloatColor(0.52f, 0.72f, 1.0f, 1.0f), 0.18f);
        hot.a = alpha * (0.070f + radiance * 0.042f) * flicker;
        cool.a = 0.0f;
        rays.addVertex(dir * inner);
        rays.addColor(hot);
        rays.addVertex(dir * outer);
        rays.addColor(cool);
    }
#ifndef TARGET_OPENGLES
    glLineWidth(std::max(1.0f, paramOrbitThickness_ * 0.90f));
#endif
    rays.draw();

    for (int i = 0; i < 2; ++i) {
        const float pct = static_cast<float>(i);
        ofFloatColor corona = starBase.getLerped(ofFloatColor(1.0f, 0.95f, 0.70f, 1.0f), 0.20f + pct * 0.18f);
        corona.a = alpha * (0.052f + radiance * 0.031f) * (1.0f - pct * 0.30f);
        drawLowPolySphere(starRadius * (3.15f + pct * 1.30f + radiance * 0.92f),
                          corona,
                          corona.a,
                          87.0f + pct * 18.0f + timeSeconds * 0.11f,
                          4,
                          9,
                          false);
    }

    if (highSpark > 0.001f) {
        ofMesh sparks;
        sparks.setMode(OF_PRIMITIVE_LINES);
        const int sparkCount = 22;
        for (int i = 0; i < sparkCount; ++i) {
            const float pct = static_cast<float>(i) / static_cast<float>(sparkCount);
            const float angle = pct * TWO_PI + ofNoise(seedState_ * 0.019f, pct * 6.0f) * 0.42f;
            const float y = (ofNoise(pct * 17.0f, seedState_ * 0.005f) - 0.5f) * 0.52f;
            const float flat = std::sqrt(std::max(0.12f, 1.0f - y * y));
            const glm::vec3 dir = safeNormalize(glm::vec3(std::cos(angle) * flat, y, std::sin(angle) * flat));
            const float inner = starRadius * (2.2f + ofNoise(pct * 5.0f, timeSeconds * 0.7f) * 1.1f);
            const float length = starRadius * (0.24f + ofNoise(pct * 11.0f, timeSeconds * 0.9f) * 0.42f);
            ofFloatColor spark = starBase.getLerped(ofFloatColor(0.90f, 0.96f, 1.0f, 1.0f), 0.42f);
            spark.a = alpha * highSpark * 0.18f;
            sparks.addVertex(dir * inner);
            sparks.addColor(spark);
            sparks.addVertex(dir * (inner + length));
            sparks.addColor(ofFloatColor(spark.r, spark.g, spark.b, 0.0f));
        }
#ifndef TARGET_OPENGLES
        glLineWidth(std::max(1.0f, paramOrbitThickness_ * 0.75f));
#endif
        sparks.draw();
    }
}

void SolarSystemLayer::drawPlanetBands(const Body& body, float planetRadius, float alpha) const {
    if (planetRadius <= 0.0f || alpha <= 0.0f || body.banding <= 0.05f) {
        return;
    }

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
#ifndef TARGET_OPENGLES
    glLineWidth(1.0f + body.banding * 1.4f);
#endif
    const int bandCount = 1 + static_cast<int>(std::round(body.banding * 4.0f));
    for (int band = 0; band < bandCount; ++band) {
        const float pct = bandCount <= 1 ? 0.5f : static_cast<float>(band) / static_cast<float>(bandCount - 1);
        const float latitude = ofMap(pct, 0.0f, 1.0f, -0.52f, 0.52f) +
                               (ofNoise(body.seed * 0.021f, band * 1.7f) - 0.5f) * 0.16f;
        const float y = std::sin(latitude) * planetRadius * 1.018f;
        const float ringRadius = std::cos(latitude) * planetRadius * 1.018f;
        ofPolyline stripe;
        const int steps = 72;
        for (int i = 0; i <= steps; ++i) {
            const float angle = static_cast<float>(i) / static_cast<float>(steps) * TWO_PI;
            const float wobble = 1.0f + (ofNoise(body.seed * 0.017f, band * 2.0f, i * 0.08f) - 0.5f) * body.banding * 0.035f;
            stripe.addVertex(glm::vec3(std::cos(angle) * ringRadius * wobble,
                                       y,
                                       std::sin(angle) * ringRadius * wobble));
        }
        ofFloatColor color = body.accentColor.getLerped(ofFloatColor(1.0f, 1.0f, 1.0f, 1.0f), 0.18f + body.atmosphere * 0.22f);
        color.a = alpha * (0.16f + body.banding * 0.22f) * (0.72f + pct * 0.18f);
        setFloatColor(color);
        stripe.draw();
    }

    if (body.storm > 0.46f) {
        ofPushMatrix();
        ofRotateYDeg(std::fmod(body.seed * 0.071f, 360.0f));
        ofRotateXDeg(ofMap(body.storm, 0.46f, 1.0f, -18.0f, 24.0f, true));
        ofTranslate(0.0f, planetRadius * (0.12f + body.storm * 0.12f), planetRadius * 1.045f);
        ofScale(1.65f + body.storm * 0.95f, 0.58f + body.storm * 0.30f, 0.32f);
        ofFloatColor stormColor = body.accentColor.getLerped(ofFloatColor(1.0f, 0.96f, 0.84f, 1.0f), 0.34f);
        const float stormDim = alpha * (0.54f + body.storm * 0.26f);
        ofDisableBlendMode();
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        drawLowPolySphere(planetRadius * (0.050f + body.storm * 0.040f),
                          ofFloatColor(stormColor.r * stormDim, stormColor.g * stormDim, stormColor.b * stormDim, 1.0f),
                          1.0f,
                          body.seed + 911.0f,
                          3,
                          6,
                          false);
        glDisable(GL_CULL_FACE);
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        ofPopMatrix();
    }
}

void SolarSystemLayer::drawOrbitLine(const Body& body, float radiusScale, float alpha) const {
    if (alpha <= 0.0f) {
        return;
    }

    ofMesh orbitMesh;
    orbitMesh.setMode(OF_PRIMITIVE_LINE_STRIP);
    const int steps = 180;
    for (int i = 0; i <= steps; ++i) {
        const float pct = static_cast<float>(i) / static_cast<float>(steps);
        const float angle = pct * TWO_PI;
        const glm::vec3 p = orbitPointFor(body, angle, radiusScale, pct + body.seed * 0.0001f);
        const float linePulse = 0.62f + 0.38f * ofNoise(body.seed * 0.013f, pct * 4.0f, orbitTime_ * 0.17f);
        const ofFloatColor guideColor = colorFrom(paramOrbitR_, paramOrbitG_, paramOrbitB_, 1.0f)
                                            .getLerped(ofFloatColor(1.0f, 1.0f, 1.0f, 1.0f), 0.38f);
        orbitMesh.addVertex(p);
        orbitMesh.addColor(colorFrom(guideColor.r,
                                     guideColor.g,
                                     guideColor.b,
                                     alpha * linePulse));
    }
    orbitMesh.draw();
}

void SolarSystemLayer::drawBodyTrail(std::size_t bodyIndex, const Body& body, float radiusScale, float alpha) const {
    const int trailSteps = static_cast<int>(std::round(paramTrailSteps_));
    const float currentAngle = orbitTime_ * body.speed * TWO_PI + body.phase;
    const float currentBandEnergy = planetBandEnergyFor(bodyIndex);
    ofMesh trail;
    trail.setMode(OF_PRIMITIVE_LINE_STRIP);
    for (int i = 0; i <= trailSteps; ++i) {
        const float pct = static_cast<float>(i) / static_cast<float>(std::max(1, trailSteps));
        const float angle = currentAngle - pct * TWO_PI * paramTrailLength_;
        const glm::vec3 p = orbitPointFor(body, angle, radiusScale, body.seed * 0.0001f + pct * 0.25f);
        const float fade = (1.0f - pct) * (1.0f - pct);
        const ofFloatColor baseTrail(0.72f, 0.78f, 0.86f, 1.0f);
        const ofFloatColor trailColor = baseTrail.getLerped(body.color, 0.04f);
        trail.addVertex(p);
        trail.addColor(colorFrom(trailColor.r,
                                 trailColor.g,
                                 trailColor.b,
                                 alpha * paramTrailAlpha_ * fade * (0.88f + currentBandEnergy * 0.24f)));
    }
    trail.draw();

    if (bodyIndex >= trailStamps_.size() || trailStamps_[bodyIndex].empty() || paramTrailStampGain_ <= 0.0f) {
        return;
    }

    const float direction = paramOrbitSpeed_ >= 0.0f ? 1.0f : -1.0f;
    const float tailWindow = std::max(0.035f, paramTrailLength_ * 1.18f);
    const float stampVisibility = alpha * ofClamp(0.16f + paramTrailAlpha_ * 1.65f, 0.16f, 1.25f);
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    for (const auto& stamp : trailStamps_[bodyIndex]) {
        const float behind = wrap01((currentAngle - stamp.angle) * direction / TWO_PI);
        if (behind > tailWindow) {
            continue;
        }

        const float ageFade = ofClamp(1.0f - stamp.age / std::max(0.001f, stamp.lifetime), 0.0f, 1.0f);
        const float distanceFade = ofClamp(1.0f - behind / tailWindow, 0.0f, 1.0f);
        const float energy = stamp.strength * ageFade * distanceFade;
        if (energy <= 0.006f) {
            continue;
        }

        const float localSpan = 0.006f + energy * 0.020f;
        ofMesh knot;
        knot.setMode(OF_PRIMITIVE_LINE_STRIP);
        const int knotSteps = 12;
        for (int i = 0; i <= knotSteps; ++i) {
            const float pct = static_cast<float>(i) / static_cast<float>(knotSteps);
            const float offset = (pct - 0.5f) * localSpan * TWO_PI;
            const glm::vec3 p = orbitPointFor(body, stamp.angle + offset, radiusScale, body.seed * 0.0001f + pct);
            const float centerFade = 1.0f - std::abs(pct - 0.5f) * 1.65f;
            const ofFloatColor hot = body.color.getLerped(ofFloatColor(0.90f, 0.96f, 1.0f, 1.0f), 0.66f);
            knot.addVertex(p);
            knot.addColor(colorFrom(hot.r,
                                    hot.g,
                                    hot.b,
                                    stampVisibility * energy * std::max(0.0f, centerFade) * 1.55f));
        }
#ifndef TARGET_OPENGLES
        glLineWidth(std::max(1.25f, paramOrbitThickness_ * (2.1f + energy * 10.0f + currentBandEnergy * 1.8f)));
#endif
        knot.draw();

        const glm::vec3 spot = orbitPointFor(body, stamp.angle, radiusScale, body.seed * 0.00013f);
        ofPushMatrix();
        ofTranslate(spot.x, spot.y, spot.z);
        const float spotRadius = std::max(2.2f, radiusScale * (0.0065f + energy * 0.024f));
        const ofFloatColor glow = body.color.getLerped(ofFloatColor(0.84f, 0.94f, 1.0f, 1.0f), 0.58f);
        drawLowPolySphere(spotRadius * (1.85f + energy * 0.32f),
                          ofFloatColor(glow.r, glow.g, glow.b, stampVisibility * energy * 0.24f),
                          stampVisibility * energy * 0.24f,
                          stamp.seed + 43.0f,
                          3,
                          6,
                          false);
        drawLowPolySphere(spotRadius,
                          ofFloatColor(glow.r, glow.g, glow.b, stampVisibility * energy * 0.95f),
                          stampVisibility * energy * 0.95f,
                          stamp.seed,
                          3,
                          6,
                          false);
        ofPopMatrix();
    }
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
}

void SolarSystemLayer::drawMoonOrbit(const glm::vec3& bodyPosition, const Moon& moon, float planetRadius, float alpha) const {
    if (alpha <= 0.0f) {
        return;
    }

    ofPolyline line;
    const int steps = 64;
    const float distance = planetRadius * moon.distance;
    for (int i = 0; i <= steps; ++i) {
        const float angle = static_cast<float>(i) / static_cast<float>(steps) * TWO_PI;
        glm::vec3 local(std::cos(angle) * distance,
                        0.0f,
                        std::sin(angle) * distance * 0.66f);
        local = rotateX(local, moon.inclination);
        line.addVertex(glm::vec3(bodyPosition.x + local.x, bodyPosition.y + local.y, bodyPosition.z + local.z));
    }
    setFloatColor(ofFloatColor(0.92f, 0.96f, 1.0f, alpha));
    line.draw();
}

void SolarSystemLayer::drawRingSet(const Body& body, float planetRadius, float alpha) const {
    if (alpha <= 0.0f || body.ringBands <= 0) {
        return;
    }

    ofPushMatrix();
    ofRotateXDeg(body.ringTilt * kRadToDeg);
    ofRotateYDeg(body.seed * 0.013f);
    for (int band = 0; band < body.ringBands; ++band) {
        const float pct = body.ringBands <= 1 ? 0.0f : static_cast<float>(band) / static_cast<float>(body.ringBands - 1);
        const float radius = planetRadius * (body.ringInner + (body.ringOuter - body.ringInner) * pct);
        ofPolyline ring;
        const int steps = 84;
        for (int i = 0; i <= steps; ++i) {
            const float angle = static_cast<float>(i) / static_cast<float>(steps) * TWO_PI;
            ring.addVertex(glm::vec3(std::cos(angle) * radius,
                                     0.0f,
                                     std::sin(angle) * radius * (0.42f + pct * 0.10f)));
        }
        const float bandAlpha = alpha * (0.12f - pct * 0.035f);
        ofFloatColor color = body.color.getLerped(ofFloatColor(0.76f, 0.70f, 0.60f, 1.0f), 0.58f);
        color.a = bandAlpha;
        setFloatColor(color);
        ring.draw();
    }
    ofPopMatrix();
}

void SolarSystemLayer::drawVisitors(float radiusScale, float alpha, float timeSeconds) const {
    if (alpha <= 0.0f || paramVisitorEvents_ <= 0.0f || visitors_.empty()) {
        return;
    }

    const float visitorAlpha = alpha * paramVisitorEvents_;
    for (const auto& visitor : visitors_) {
        const float activeWindow = ofClamp(visitor.duration / std::max(1.0f, visitor.cycle), 0.025f, 0.22f);
        const float cycleT = wrap01(timeSeconds / visitor.cycle + visitor.phase);
        if (cycleT > activeWindow) {
            continue;
        }

        const float localT = ofClamp(cycleT / activeWindow, 0.0f, 1.0f);
        const float fade = smooth01(localT / 0.20f) * (1.0f - smooth01((localT - 0.72f) / 0.28f));
        if (fade <= 0.001f) {
            continue;
        }

        auto pathPoint = [&](float t) {
            const float travel = ofLerp(-1.42f, 1.42f, t);
            const float wobble = (ofNoise(visitor.seed * 0.013f, t * 3.0f, timeSeconds * 0.07f) - 0.5f) * 0.12f;
            glm::vec3 dir(std::cos(visitor.angle), 0.0f, std::sin(visitor.angle));
            glm::vec3 side(-std::sin(visitor.angle), 0.0f, std::cos(visitor.angle));
            glm::vec3 p = dir * radiusScale * travel + side * radiusScale * (visitor.impact + wobble);
            p.y += std::sin(t * PI) * radiusScale * 0.18f * std::sin(visitor.inclination);
            p = rotateX(p, visitor.inclination);
            p = rotateY(p, visitor.yaw);
            return p;
        };

        ofEnableBlendMode(OF_BLENDMODE_ADD);
        glDepthMask(GL_FALSE);
        ofMesh trail;
        trail.setMode(OF_PRIMITIVE_LINE_STRIP);
        const int trailSteps = visitor.type == 0 ? 14 : 20;
        for (int i = 0; i <= trailSteps; ++i) {
            const float pct = static_cast<float>(i) / static_cast<float>(trailSteps);
            const float t = ofClamp(localT - pct * (visitor.type == 0 ? 0.11f : 0.16f), 0.0f, 1.0f);
            const glm::vec3 p = pathPoint(t);
            ofFloatColor color = visitor.color;
            color.a = visitorAlpha * fade * (1.0f - pct) * (visitor.type == 0 ? 0.18f : 0.28f);
            trail.addVertex(p);
            trail.addColor(color);
        }
#ifndef TARGET_OPENGLES
        glLineWidth(visitor.type == 0 ? 1.2f : 1.6f);
#endif
        trail.draw();
        glDepthMask(GL_TRUE);

        const glm::vec3 p = pathPoint(localT);
        ofPushMatrix();
        ofTranslate(p.x, p.y, p.z);
        ofRotateYDeg(visitor.angle * kRadToDeg + timeSeconds * 38.0f);
        ofRotateXDeg(visitor.inclination * kRadToDeg);
        if (visitor.type == 0) {
            ofDisableBlendMode();
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);
            const float dim = visitorAlpha * fade * (0.78f + highs_ * paramHighsSparkle_ * paramAudioAmount_ * 0.10f);
            drawLowPolySphere(visitor.size,
                              ofFloatColor(visitor.color.r * dim, visitor.color.g * dim, visitor.color.b * dim, 1.0f),
                              1.0f,
                              visitor.seed + timeSeconds * 0.4f,
                              3,
                              6,
                              false);
            glDisable(GL_CULL_FACE);
        } else {
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            const float bodyAlpha = visitorAlpha * fade * 0.70f;
            setFloatColor(ofFloatColor(visitor.color.r, visitor.color.g, visitor.color.b, bodyAlpha));
            ofPushMatrix();
            ofScale(1.65f, 0.28f, 0.82f);
            ofDrawBox(0.0f, 0.0f, 0.0f, visitor.size);
            ofPopMatrix();
            setFloatColor(ofFloatColor(0.92f, 0.98f, 1.0f, bodyAlpha * 0.62f));
            ofDrawBox(0.0f, visitor.size * 0.23f, 0.0f, visitor.size * 0.58f);
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            setFloatColor(ofFloatColor(0.58f, 0.86f, 1.0f, bodyAlpha * 0.55f));
            ofDrawLine(-visitor.size * 1.35f, 0.0f, 0.0f, visitor.size * 1.35f, 0.0f, 0.0f);
        }
        ofPopMatrix();
    }
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
}

void SolarSystemLayer::drawArtificialArtifacts(std::size_t bodyIndex,
                                               const Body& body,
                                               const glm::vec3& bodyPosition,
                                               float planetRadius,
                                               float alpha,
                                               float timeSeconds) const {
    if (alpha <= 0.0f || paramArtifactActivity_ <= 0.0f || artifacts_.empty()) {
        return;
    }

    for (const auto& artifact : artifacts_) {
        if (artifact.bodyIndex != static_cast<int>(bodyIndex)) {
            continue;
        }

        const float appear = smooth01((timeSeconds - artifact.delay) / 26.0f);
        if (appear <= 0.001f) {
            continue;
        }

        const float artifactAlpha = alpha * paramArtifactActivity_ * appear;
        const float angle = timeSeconds * artifact.speed + artifact.phase;
        const float distance = planetRadius * artifact.orbitDistance;
        glm::vec3 local(std::cos(angle) * distance,
                        std::sin(angle * 0.62f + artifact.phase) * distance * 0.16f,
                        std::sin(angle) * distance * 0.62f);
        local = rotateX(local, artifact.inclination + body.axialTilt * 0.24f);
        local = rotateY(local, body.orbitYaw * 0.45f);
        const glm::vec3 p = bodyPosition + local;
        const float size = std::max(1.25f, planetRadius * artifact.size);

        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        ofPushMatrix();
        ofTranslate(p.x, p.y, p.z);
        ofRotateYDeg(std::fmod(timeSeconds * 42.0f + artifact.seed, 360.0f));
        ofRotateXDeg(artifact.inclination * kRadToDeg);

        if (artifact.type == 0) {
            setFloatColor(ofFloatColor(artifact.color.r, artifact.color.g, artifact.color.b, artifactAlpha * 0.62f));
            ofDrawBox(0.0f, 0.0f, 0.0f, size);
            setFloatColor(ofFloatColor(0.40f, 0.62f, 0.90f, artifactAlpha * 0.42f));
            ofDrawBox(-size * 0.92f, 0.0f, 0.0f, size * 0.70f, size * 0.12f, size * 0.34f);
            ofDrawBox(size * 0.92f, 0.0f, 0.0f, size * 0.70f, size * 0.12f, size * 0.34f);
        } else {
            ofPolyline ring;
            const int steps = 36;
            for (int i = 0; i <= steps; ++i) {
                const float pct = static_cast<float>(i) / static_cast<float>(steps);
                const float a = pct * TWO_PI;
                ring.addVertex(glm::vec3(std::cos(a) * size * 1.25f, 0.0f, std::sin(a) * size * 0.82f));
            }
            setFloatColor(ofFloatColor(artifact.color.r, artifact.color.g, artifact.color.b, artifactAlpha * 0.48f));
#ifndef TARGET_OPENGLES
            glLineWidth(1.0f);
#endif
            ring.draw();
            setFloatColor(ofFloatColor(0.82f, 0.90f, 1.0f, artifactAlpha * 0.55f));
            ofDrawBox(0.0f, 0.0f, 0.0f, size * 0.78f);
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            setFloatColor(ofFloatColor(0.50f, 0.78f, 1.0f, artifactAlpha * 0.25f));
            ofDrawLine(-size * 1.7f, 0.0f, 0.0f, size * 1.7f, 0.0f, 0.0f);
        }

        ofPopMatrix();
    }
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
}

void SolarSystemLayer::drawLifeSatellite(std::size_t bodyIndex,
                                         const Body& body,
                                         const glm::vec3& bodyPosition,
                                         float planetRadius,
                                         float alpha,
                                         float timeSeconds) const {
    if (alpha <= 0.0f || paramCivilizationGrowth_ <= 0.0f || bodyIndex >= lifeStates_.size()) {
        return;
    }

    const LifeState& life = lifeStates_[bodyIndex];
    const float appear = smooth01((life.civilizationEnergy - 0.10f) / 0.58f);
    if (appear <= 0.001f) {
        return;
    }

    const float activity = 0.42f + paramArtifactActivity_ * 0.58f;
    const float satelliteAlpha = alpha * appear * activity * (0.56f + life.bandEnergy * 0.22f);
    const float angle = timeSeconds * life.satelliteSpeed + life.satellitePhase;
    const float distance = planetRadius * life.satelliteDistance;
    glm::vec3 local(std::cos(angle) * distance,
                    std::sin(angle * 0.72f + life.satellitePhase) * distance * 0.18f,
                    std::sin(angle) * distance * 0.64f);
    local = rotateX(local, life.satelliteInclination + body.axialTilt * 0.18f);
    local = rotateY(local, body.orbitYaw * 0.38f);
    const glm::vec3 p = bodyPosition + local;
    const float size = std::max(1.2f, planetRadius * life.satelliteSize * (1.0f + life.civilizationEnergy * 0.28f));
    const ofFloatColor signalColor = life.biosphereColor.getLerped(ofFloatColor(0.76f, 0.90f, 1.0f, 1.0f), 0.36f);

    ofEnableBlendMode(OF_BLENDMODE_ADD);
#ifndef TARGET_OPENGLES
    glLineWidth(std::max(1.0f, 1.0f + life.bandEnergy * 1.6f));
#endif
    ofPolyline orbit;
    const int steps = 72;
    for (int i = 0; i <= steps; ++i) {
        const float pct = static_cast<float>(i) / static_cast<float>(steps);
        const float a = pct * TWO_PI;
        glm::vec3 ringLocal(std::cos(a) * distance,
                            std::sin(a * 0.72f + life.satellitePhase) * distance * 0.18f,
                            std::sin(a) * distance * 0.64f);
        ringLocal = rotateX(ringLocal, life.satelliteInclination + body.axialTilt * 0.18f);
        ringLocal = rotateY(ringLocal, body.orbitYaw * 0.38f);
        orbit.addVertex(bodyPosition + ringLocal);
    }
    setFloatColor(ofFloatColor(signalColor.r, signalColor.g, signalColor.b, satelliteAlpha * 0.18f));
    orbit.draw();

    ofPushMatrix();
    ofTranslate(p.x, p.y, p.z);
    ofRotateYDeg(std::fmod(timeSeconds * 58.0f + life.satelliteSeed, 360.0f));
    ofRotateXDeg(life.satelliteInclination * kRadToDeg);

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    setFloatColor(ofFloatColor(0.86f, 0.92f, 0.96f, satelliteAlpha * 0.78f));
    ofDrawBox(0.0f, 0.0f, 0.0f, size);
    setFloatColor(ofFloatColor(signalColor.r, signalColor.g, signalColor.b, satelliteAlpha * 0.55f));
    ofDrawBox(-size * 1.05f, 0.0f, 0.0f, size * 0.82f, size * 0.12f, size * 0.36f);
    ofDrawBox(size * 1.05f, 0.0f, 0.0f, size * 0.82f, size * 0.12f, size * 0.36f);

    if (life.bandEnergy > 0.08f) {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        const float pulse = smooth01(life.bandEnergy) * appear;
        setFloatColor(ofFloatColor(signalColor.r, signalColor.g, signalColor.b, satelliteAlpha * pulse * 0.42f));
        ofDrawLine(-size * (1.7f + pulse), 0.0f, 0.0f, size * (1.7f + pulse), 0.0f, 0.0f);
        ofNoFill();
        ofDrawCircle(0.0f, 0.0f, size * (1.35f + pulse * 1.15f));
        ofFill();
    }

    ofPopMatrix();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
}

void SolarSystemLayer::drawHeliosphereField(float radiusScale, float alpha, float timeSeconds) const {
    if (alpha <= 0.0f) {
        return;
    }

    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float wind = mids_ * paramMidsSpeed_ * audio;
    const float fieldAlpha = alpha * (0.040f + wind * 0.115f + bass_ * paramBassScale_ * audio * 0.030f);
    const int lines = 56;
    const int steps = 46;
    const float inner = radiusScale * 0.045f;
    const float outer = radiusScale * (0.88f + wind * 0.18f);
    const float curl = 0.14f + wind * 0.42f;

    ofMesh field;
    field.setMode(OF_PRIMITIVE_LINES);
    for (int l = 0; l < lines; ++l) {
        const float base = static_cast<float>(l) / static_cast<float>(lines) * TWO_PI;
        const float lineSeed = static_cast<float>(l) * 0.137f + seedState_ * 0.0001f;
        glm::vec3 prev;
        bool hasPrev = false;

        for (int s = 0; s < steps; ++s) {
            const float pct = static_cast<float>(s) / static_cast<float>(std::max(1, steps - 1));
            const float sample = hasWaveform_ ? waveformSampleFor(wrap01(pct + lineSeed)) : 0.0f;
            const float r = ofLerp(inner, outer, pct) + sample * paramWaveformAmount_ * radiusScale * 0.08f;
            const float angle = base + pct * curl * TWO_PI + timeSeconds * (0.013f + wind * 0.060f);
            const float y = std::sin(base * 2.0f + pct * (5.0f + wind * 2.2f) + timeSeconds * (0.11f + wind * 0.25f)) *
                            radiusScale *
                            (0.018f + wind * 0.012f);
            glm::vec3 p(std::cos(angle) * r, y, std::sin(angle) * r);
            p = rotateX(p, 0.10f * std::sin(base));

            if (hasPrev) {
                const float fade = (1.0f - pct) * (1.0f - pct);
                ofFloatColor color = sourceStarColor_.getLerped(ofFloatColor(0.70f, 0.86f, 1.0f, 1.0f), 0.25f);
                color.a = fieldAlpha * fade * (0.50f + wind * 0.35f + std::abs(sample) * 0.80f);
                field.addVertex(prev);
                field.addColor(color);
                field.addVertex(p);
                field.addColor(color);
            }

            prev = p;
            hasPrev = true;
        }
    }
#ifndef TARGET_OPENGLES
    glLineWidth(std::max(1.0f, paramOrbitThickness_ * 0.55f));
#endif
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    glDepthMask(GL_FALSE);
    field.draw();
    glDepthMask(GL_TRUE);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
}

void SolarSystemLayer::drawAsteroids(float radiusScale, float alpha, float timeSeconds) const {
    ofDisableBlendMode();
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    for (const auto& asteroid : asteroids_) {
        const float angle = asteroid.angle + orbitTime_ * asteroid.speed * TWO_PI;
        const glm::vec3 p = orbitPointFor(asteroid, angle, radiusScale);
        const float highShimmer = highs_ * paramHighsSparkle_ * paramAudioAmount_;
        const float flicker = 0.62f + ofNoise(asteroid.seed * 0.01f, timeSeconds * (0.21f + highShimmer * 0.42f)) * (0.18f + highShimmer * 0.16f);
        const float dim = alpha * 0.62f * flicker;
        ofPushMatrix();
        ofTranslate(p.x, p.y, p.z);
        ofRotateYDeg(std::fmod(timeSeconds * 19.0f + asteroid.seed, 360.0f));
        ofRotateXDeg(asteroid.seed * 0.07f);
        ofScale(1.0f, 0.62f + ofNoise(asteroid.seed) * 0.55f, 0.78f + ofNoise(asteroid.seed + 17.0f) * 0.42f);
        drawLowPolySphere(asteroid.radius,
                          ofFloatColor(asteroid.color.r * dim, asteroid.color.g * dim, asteroid.color.b * dim, 1.0f),
                          1.0f,
                          asteroid.seed,
                          3,
                          5,
                          false);
        ofPopMatrix();
    }
    glDisable(GL_CULL_FACE);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
}

void SolarSystemLayer::drawComets(float radiusScale, float alpha, float timeSeconds) const {
    for (const auto& comet : comets_) {
        const float angle = comet.angle + orbitTime_ * comet.speed * TWO_PI;
        const glm::vec3 p = orbitPointFor(comet, angle, radiusScale);
        ofEnableBlendMode(OF_BLENDMODE_ADD);

        ofMesh tail;
        tail.setMode(OF_PRIMITIVE_LINE_STRIP);
        const float highShimmer = highs_ * paramHighsSparkle_ * paramAudioAmount_;
        const int steps = 18 + static_cast<int>(std::round(highShimmer * 8.0f));
        for (int i = 0; i <= steps; ++i) {
            const float pct = static_cast<float>(i) / static_cast<float>(steps);
            const glm::vec3 q = orbitPointFor(comet, angle - pct * comet.tail * TWO_PI * (1.0f + highShimmer * 0.45f), radiusScale);
            tail.addVertex(q);
            tail.addColor(ofFloatColor(comet.color.r,
                                       comet.color.g,
                                       comet.color.b,
                                       alpha * (1.0f - pct) * (0.34f + highShimmer * 0.26f)));
        }
#ifndef TARGET_OPENGLES
        glLineWidth(std::max(1.0f, paramOrbitThickness_ * 1.1f));
#endif
        tail.draw();

        ofPushMatrix();
        ofTranslate(p.x, p.y, p.z);
        ofDisableBlendMode();
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        drawLowPolySphere(comet.radius,
                          ofFloatColor(comet.color.r * alpha * 0.82f,
                                       comet.color.g * alpha * 0.82f,
                                       comet.color.b * alpha * 0.82f,
                                       1.0f),
                          1.0f,
                          comet.seed + timeSeconds,
                          3,
                          6,
                          false);
        glDisable(GL_CULL_FACE);
        ofPopMatrix();
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    }
}
