#pragma once

#include "Layer.h"

#include <cstdint>
#include <vector>

class ConstellationStarfieldLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    struct Star {
        glm::vec2 pos{ 0.0f, 0.0f };
        float depth = 0.0f;
        float size = 1.0f;
        float brightness = 1.0f;
        float tint = 0.0f;
        float phase = 0.0f;
    };

    struct Link {
        int a = 0;
        int b = 0;
        float strength = 1.0f;
    };

    void registerFloat(ParameterRegistry& registry,
                       const std::string& id,
                       float* target,
                       float initial,
                       const std::string& label,
                       float min,
                       float max,
                       float step,
                       const std::string& description = std::string());
    void readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b);
    void clampParams();
    void resetStars();
    void updateAudioState();
    glm::vec2 projectedStar(const Star& star, float width, float height, float timeSeconds) const;

    bool paramEnabled_ = true;
    float paramAlpha_ = 1.0f;
    float paramStarCount_ = 220.0f;
    float paramConstellationCount_ = 7.0f;
    float paramConstellationAlpha_ = 0.34f;
    float paramConstellationReach_ = 0.18f;
    float paramStarSize_ = 1.25f;
    float paramDepthParallax_ = 0.22f;
    float paramDriftSpeed_ = 0.018f;
    float paramTwinkle_ = 0.72f;
    float paramHighsTwinkle_ = 1.05f;
    float paramMidsLines_ = 0.75f;
    float paramBassDimming_ = 0.38f;
    float paramAudioAmount_ = 0.82f;
    float paramAudioSmoothing_ = 0.48f;
    float paramSeed_ = 4242.0f;
    bool paramReseedRequested_ = false;
    float paramBgAlpha_ = 0.20f;
    float paramBgR_ = 0.002f;
    float paramBgG_ = 0.005f;
    float paramBgB_ = 0.016f;
    float paramStarR_ = 0.78f;
    float paramStarG_ = 0.92f;
    float paramStarB_ = 1.0f;
    float paramLineR_ = 0.34f;
    float paramLineG_ = 0.74f;
    float paramLineB_ = 1.0f;

    bool enabled_ = true;
    bool hasAudio_ = false;
    std::uint32_t seedState_ = 4242;
    float level_ = 0.0f;
    float peak_ = 0.0f;
    float bass_ = 0.0f;
    float mids_ = 0.0f;
    float highs_ = 0.0f;
    std::vector<Star> stars_;
    std::vector<Link> links_;
};
