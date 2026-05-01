#include "PerlinNoiseLayer.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include "ofUtils.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
    struct Palette {
        std::array<ofFloatColor, 4> stops;
    };

    constexpr int kPaletteCount = 5;

    const std::array<Palette, kPaletteCount> kPalettes = {{
        { { ofFloatColor(0.02f, 0.02f, 0.08f), ofFloatColor(0.00f, 0.25f, 0.50f), ofFloatColor(0.00f, 0.60f, 0.80f), ofFloatColor(0.60f, 0.90f, 1.00f) } },
        { { ofFloatColor(0.10f, 0.00f, 0.00f), ofFloatColor(0.60f, 0.05f, 0.15f), ofFloatColor(0.90f, 0.40f, 0.10f), ofFloatColor(1.00f, 0.85f, 0.40f) } },
        { { ofFloatColor(0.05f, 0.10f, 0.02f), ofFloatColor(0.10f, 0.40f, 0.08f), ofFloatColor(0.15f, 0.70f, 0.30f), ofFloatColor(0.70f, 0.95f, 0.60f) } },
        { { ofFloatColor(0.12f, 0.00f, 0.20f), ofFloatColor(0.45f, 0.00f, 0.55f), ofFloatColor(0.90f, 0.20f, 0.60f), ofFloatColor(1.00f, 0.80f, 0.90f) } },
        { { ofFloatColor(0.05f, 0.05f, 0.05f), ofFloatColor(0.30f, 0.30f, 0.30f), ofFloatColor(0.70f, 0.70f, 0.70f), ofFloatColor(1.00f, 1.00f, 1.00f) } }
    }};

    ofFloatColor samplePalette(const Palette& palette, float t) {
        t = ofClamp(t, 0.0f, 1.0f);
        float scaled = t * static_cast<float>(palette.stops.size() - 1);
        int idx = static_cast<int>(std::floor(scaled));
        int next = std::min(idx + 1, static_cast<int>(palette.stops.size()) - 1);
        float frac = scaled - static_cast<float>(idx);
        return palette.stops[static_cast<std::size_t>(idx)].getLerped(palette.stops[static_cast<std::size_t>(next)], frac);
    }

    const Palette& paletteForIndex(int idx) {
        if (kPaletteCount == 0) {
            static Palette fallback{ { ofFloatColor(0, 0, 0), ofFloatColor(1, 1, 1), ofFloatColor(1, 1, 1), ofFloatColor(1, 1, 1) } };
            return fallback;
        }
        idx %= kPaletteCount;
        if (idx < 0) idx += kPaletteCount;
        return kPalettes[static_cast<std::size_t>(idx)];
    }
}

void PerlinNoiseLayer::configure(const ofJson& config) {
    if (config.contains("defaults")) {
        const auto& def = config["defaults"];
        paramScale_ = def.value("scale", paramScale_);
        paramSpeed_ = def.value("speed", paramSpeed_);
        paramBrightness_ = def.value("brightness", paramBrightness_);
        paramContrast_ = def.value("contrast", paramContrast_);
        paramAlpha_ = def.value("alpha", paramAlpha_);
        paramTexelZoom_ = def.value("texelZoom", paramTexelZoom_);
        if (def.contains("color") && def["color"].is_array() && def["color"].size() >= 3) {
            paramColorR_ = def["color"][0].get<float>();
            paramColorG_ = def["color"][1].get<float>();
            paramColorB_ = def["color"][2].get<float>();
        }
        paramOctaves_ = def.value("octaves", paramOctaves_);
        paramLacunarity_ = def.value("lacunarity", paramLacunarity_);
        paramPersistence_ = def.value("persistence", paramPersistence_);
        paramPaletteIndex_ = def.value("palette", paramPaletteIndex_);
        paramPaletteRate_ = def.value("paletteRate", paramPaletteRate_);
    }

    if (config.contains("textureSize") && config["textureSize"].is_array() && config["textureSize"].size() >= 2) {
        textureSize_.x = std::max(16, config["textureSize"][0].get<int>());
        textureSize_.y = std::max(16, config["textureSize"][1].get<int>());
    }
    baseTextureSize_ = textureSize_;
}

void PerlinNoiseLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "layer.perlin" : registryPrefix();

    paramOctaves_ = ofClamp(paramOctaves_, 1.0f, 8.0f);
    paramLacunarity_ = ofClamp(paramLacunarity_, 1.0f, 6.0f);
    paramPersistence_ = ofClamp(paramPersistence_, 0.05f, 1.0f);
    paramPaletteRate_ = ofClamp(paramPaletteRate_, -3.0f, 3.0f);
    paramPaletteIndex_ = ofClamp(paramPaletteIndex_, 0.0f, static_cast<float>(std::max(0, kPaletteCount - 1)));
    paramTexelZoom_ = ofClamp(paramTexelZoom_, 0.25f, 4.0f);

    ParameterRegistry::Descriptor visMeta;
    visMeta.label = "Perlin Visible";
    visMeta.group = "Generative";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, visMeta);

    ParameterRegistry::Descriptor scaleMeta;
    scaleMeta.label = "Perlin Scale";
    scaleMeta.group = "Generative";
    scaleMeta.range.min = 0.1f;
    scaleMeta.range.max = 10.0f;
    scaleMeta.range.step = 0.1f;
    registry.addFloat(prefix + ".scale", &paramScale_, paramScale_, scaleMeta);

    ParameterRegistry::Descriptor speedMeta;
    speedMeta.label = "Perlin Speed";
    speedMeta.group = "Generative";
    speedMeta.range.min = 0.0f;
    speedMeta.range.max = 5.0f;
    speedMeta.range.step = 0.05f;
    registry.addFloat(prefix + ".speed", &paramSpeed_, paramSpeed_, speedMeta);

    ParameterRegistry::Descriptor brightMeta;
    brightMeta.label = "Perlin Brightness";
    brightMeta.group = "Generative";
    brightMeta.range.min = 0.0f;
    brightMeta.range.max = 2.0f;
    brightMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".brightness", &paramBrightness_, paramBrightness_, brightMeta);

    ParameterRegistry::Descriptor contrastMeta;
    contrastMeta.label = "Perlin Contrast";
    contrastMeta.group = "Generative";
    contrastMeta.range.min = 0.1f;
    contrastMeta.range.max = 4.0f;
    contrastMeta.range.step = 0.05f;
    registry.addFloat(prefix + ".contrast", &paramContrast_, paramContrast_, contrastMeta);

    ParameterRegistry::Descriptor alphaMeta;
    alphaMeta.label = "Perlin Alpha";
    alphaMeta.group = "Generative";
    alphaMeta.range.min = 0.0f;
    alphaMeta.range.max = 1.0f;
    alphaMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".alpha", &paramAlpha_, paramAlpha_, alphaMeta);

    ParameterRegistry::Descriptor texelMeta;
    texelMeta.label = "Perlin Texel Zoom";
    texelMeta.group = "Generative";
    texelMeta.range.min = 0.25f;
    texelMeta.range.max = 4.0f;
    texelMeta.range.step = 0.05f;
    texelMeta.description = "Adjust internal noise grid resolution (1.0 matches GoL cells)";
    registry.addFloat(prefix + ".texelZoom", &paramTexelZoom_, paramTexelZoom_, texelMeta);

    ParameterRegistry::Descriptor colorMeta;
    colorMeta.group = "Generative";
    colorMeta.range.min = 0.0f;
    colorMeta.range.max = 2.0f;
    colorMeta.range.step = 0.01f;

    colorMeta.label = "Perlin Red";
    registry.addFloat(prefix + ".colorR", &paramColorR_, paramColorR_, colorMeta);
    colorMeta.label = "Perlin Green";
    registry.addFloat(prefix + ".colorG", &paramColorG_, paramColorG_, colorMeta);
    colorMeta.label = "Perlin Blue";
    registry.addFloat(prefix + ".colorB", &paramColorB_, paramColorB_, colorMeta);

    ParameterRegistry::Descriptor octaveMeta;
    octaveMeta.label = "Perlin Octaves";
    octaveMeta.group = "Generative";
    octaveMeta.range.min = 1.0f;
    octaveMeta.range.max = 8.0f;
    octaveMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".octaves", &paramOctaves_, paramOctaves_, octaveMeta);

    ParameterRegistry::Descriptor lacuMeta;
    lacuMeta.label = "Perlin Lacunarity";
    lacuMeta.group = "Generative";
    lacuMeta.range.min = 1.0f;
    lacuMeta.range.max = 6.0f;
    lacuMeta.range.step = 0.1f;
    registry.addFloat(prefix + ".lacunarity", &paramLacunarity_, paramLacunarity_, lacuMeta);

    ParameterRegistry::Descriptor persistMeta;
    persistMeta.label = "Perlin Persistence";
    persistMeta.group = "Generative";
    persistMeta.range.min = 0.05f;
    persistMeta.range.max = 1.0f;
    persistMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".persistence", &paramPersistence_, paramPersistence_, persistMeta);

    ParameterRegistry::Descriptor paletteMeta;
    paletteMeta.label = "Perlin Palette";
    paletteMeta.group = "Generative";
    paletteMeta.range.min = 0.0f;
    paletteMeta.range.max = static_cast<float>(std::max(0, kPaletteCount - 1));
    paletteMeta.range.step = 1.0f;
    paletteMeta.description = "0=Ocean 1=Solar 2=Forest 3=Neon 4=Monochrome";
    registry.addFloat(prefix + ".palette", &paramPaletteIndex_, paramPaletteIndex_, paletteMeta);

    ParameterRegistry::Descriptor paletteRateMeta;
    paletteRateMeta.label = "Palette Rate";
    paletteRateMeta.group = "Generative";
    paletteRateMeta.range.min = -3.0f;
    paletteRateMeta.range.max = 3.0f;
    paletteRateMeta.range.step = 0.01f;
    paletteRateMeta.description = "Cycles palette selection over time (units: palettes/sec)";
    registry.addFloat(prefix + ".paletteRate", &paramPaletteRate_, paramPaletteRate_, paletteRateMeta);

    lastTexelZoom_ = -1.0f;
    applyTexelZoom();
    palettePhase_ = 0.0f;
    refreshPixels(noiseZ_);
}

void PerlinNoiseLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) return;

    noiseZ_ += params.dt * paramSpeed_;

    if (std::abs(paramPaletteRate_) > 0.0001f) {
        palettePhase_ += params.dt * paramPaletteRate_;
        float wrap = static_cast<float>(std::max(1, kPaletteCount));
        palettePhase_ = ofWrap(palettePhase_, 0.0f, wrap);
    }

    applyTexelZoom();
    refreshPixels(noiseZ_);
}

void PerlinNoiseLayer::draw(const LayerDrawParams& params) {
    if (!enabled_) return;
    if (params.slotOpacity <= 0.0f) return;

    if (!texture_.isAllocated()) {
        return;
    }

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);

    ofSetColor(255, 255, 255, static_cast<int>(ofClamp(params.slotOpacity * paramAlpha_, 0.0f, 1.0f) * 255.0f));
    texture_.draw(0, 0, params.viewport.x, params.viewport.y);

    ofPopView();
    ofPopStyle();
}

void PerlinNoiseLayer::onWindowResized(int width, int height) {
    (void)width;
    (void)height;
}

void PerlinNoiseLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

void PerlinNoiseLayer::applyTexelZoom() {
    float zoom = ofClamp(paramTexelZoom_, 0.25f, 4.0f);
    glm::ivec2 target{
        std::max(8, static_cast<int>(std::round(static_cast<float>(baseTextureSize_.x) * zoom))),
        std::max(8, static_cast<int>(std::round(static_cast<float>(baseTextureSize_.y) * zoom)))
    };

    if (target == textureSize_ && std::abs(zoom - lastTexelZoom_) < 0.0001f) {
        lastTexelZoom_ = zoom;
        return;
    }

    textureSize_ = target;
    allocateTexture();
    lastTexelZoom_ = zoom;
}

void PerlinNoiseLayer::allocateTexture() {
    pixels_.allocate(textureSize_.x, textureSize_.y, 4);
    texture_.allocate(textureSize_.x, textureSize_.y, GL_RGBA32F);
    texture_.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
}

void PerlinNoiseLayer::refreshPixels(float timeOffset) {
    if (!pixels_.isAllocated()) return;

    float scale = std::max(0.01f, paramScale_);
    float brightness = std::max(0.0f, paramBrightness_);
    float contrast = std::max(0.01f, paramContrast_);
    int octaves = ofClamp(static_cast<int>(std::round(paramOctaves_)), 1, 8);
    float lacunarity = std::max(1.0f, paramLacunarity_);
    float persistence = ofClamp(paramPersistence_, 0.05f, 1.0f);

    float paletteSelector = paramPaletteIndex_ + palettePhase_;
    if (kPaletteCount == 0) paletteSelector = 0.0f;

    for (int y = 0; y < textureSize_.y; ++y) {
        for (int x = 0; x < textureSize_.x; ++x) {
            float nx = (static_cast<float>(x) + 0.5f) / static_cast<float>(textureSize_.x);
            float ny = (static_cast<float>(y) + 0.5f) / static_cast<float>(textureSize_.y);

            float frequency = 1.0f;
            float amplitude = 1.0f;
            float total = 0.0f;
            float norm = 0.0f;
            for (int octave = 0; octave < octaves; ++octave) {
                float sample = ofNoise(nx * scale * frequency, ny * scale * frequency, timeOffset * frequency);
                total += sample * amplitude;
                norm += amplitude;
                amplitude *= persistence;
                frequency *= lacunarity;
            }
            float value = norm > 0.0f ? total / norm : 0.0f;
            float adjusted = (value - 0.5f) * contrast + 0.5f;
            adjusted = ofClamp(adjusted, 0.0f, 1.0f);
            float level = ofClamp(adjusted * brightness, 0.0f, 1.0f);

            ofFloatColor color(level, level, level, 1.0f);
            if (kPaletteCount > 0) {
                float wrapped = paletteSelector;
                float range = static_cast<float>(kPaletteCount);
                wrapped = std::fmod(wrapped, range);
                if (wrapped < 0.0f) wrapped += range;
                int idxA = static_cast<int>(std::floor(wrapped));
                int idxB = (idxA + 1) % kPaletteCount;
                float blend = wrapped - static_cast<float>(idxA);
                ofFloatColor colA = samplePalette(paletteForIndex(idxA), level);
                ofFloatColor colB = samplePalette(paletteForIndex(idxB), level);
                color = colA.getLerped(colB, blend);
            }

            color.r = ofClamp(color.r * paramColorR_, 0.0f, 1.0f);
            color.g = ofClamp(color.g * paramColorG_, 0.0f, 1.0f);
            color.b = ofClamp(color.b * paramColorB_, 0.0f, 1.0f);
            color *= level;
            color.a = ofClamp(paramAlpha_, 0.0f, 1.0f);
            pixels_.setColor(x, y, color);
        }
    }

    texture_.loadData(pixels_);
}

int PerlinNoiseLayer::paletteCount() const {
    return kPaletteCount;
}
