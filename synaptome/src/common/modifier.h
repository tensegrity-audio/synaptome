#pragma once

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <utility>

namespace modifier {

struct Range {
    float min = 0.0f;
    float max = 1.0f;
    bool relativeToBase = false;

    bool isValid() const {
        return std::isfinite(min) && std::isfinite(max) && std::fabs(max - min) > 1e-6f;
    }
};

enum class Type : uint8_t {
    kKey,
    kMidiCc,
    kMidiNote,
    kOsc,
    kAutomation,
    kScript
};

enum class BlendMode : uint8_t {
    kAdditive,
    kAbsolute,
    kScale,
    kClamp,
    kToggle
};

struct Modifier {
    Type type = Type::kKey;
    BlendMode blend = BlendMode::kAdditive;
    Range inputRange{};
    Range outputRange{};
    bool enabled = true;
    bool invertInput = false;
};

inline float normalizeInput(const Range& range, float value) {
    if (!range.isValid()) {
        return 0.0f;
    }
    float denom = range.max - range.min;
    float t = (value - range.min) / denom;
    return std::clamp(t, 0.0f, 1.0f);
}

inline float mapToOutput(const Range& range, float normalized) {
    if (!range.isValid()) {
        return 0.0f;
    }
    float span = range.max - range.min;
    return range.min + span * normalized;
}

inline float applyAdditive(float baseValue, float mapped, bool relative) {
    float delta = relative ? baseValue * mapped : mapped;
    return baseValue + delta;
}

inline float applyAbsolute(float baseValue, float mapped, bool relative) {
    return relative ? baseValue * mapped : mapped;
}

inline float applyScale(float baseValue, float mapped, bool relative) {
    float factor = relative ? (1.0f + mapped) : mapped;
    return baseValue * factor;
}

inline float applyClamp(float baseValue, const Range& range) {
    float lo = std::min(range.min, range.max);
    float hi = std::max(range.min, range.max);
    return std::clamp(baseValue, lo, hi);
}

inline float applyClampRelative(float baseValue, const Range& range) {
    float lo = baseValue + baseValue * std::min(range.min, range.max);
    float hi = baseValue + baseValue * std::max(range.min, range.max);
    if (lo > hi) {
        std::swap(lo, hi);
    }
    return std::clamp(baseValue, lo, hi);
}

inline float applyToggle(const Modifier& modifier, float baseValue, float normalized) {
    bool on = normalized >= 0.5f;
    if (modifier.outputRange.relativeToBase) {
        float offValue = baseValue + baseValue * modifier.outputRange.min;
        float onValue = baseValue + baseValue * modifier.outputRange.max;
        return on ? onValue : offValue;
    }
    float offValue = modifier.outputRange.min;
    float onValue = modifier.outputRange.max;
    return on ? onValue : offValue;
}

inline float evaluateFloat(const Modifier& modifier, float baseValue, float inputValue) {
    if (!modifier.enabled) {
        return baseValue;
    }

    float n = normalizeInput(modifier.inputRange, inputValue);
    if (modifier.invertInput) {
        n = 1.0f - n;
    }

    if (modifier.blend == BlendMode::kToggle) {
        return applyToggle(modifier, baseValue, n);
    }

    float mapped = mapToOutput(modifier.outputRange, n);

    switch (modifier.blend) {
    case BlendMode::kAdditive:
        return applyAdditive(baseValue, mapped, modifier.outputRange.relativeToBase);
    case BlendMode::kAbsolute:
        return applyAbsolute(baseValue, mapped, modifier.outputRange.relativeToBase);
    case BlendMode::kScale:
        return applyScale(baseValue, mapped, modifier.outputRange.relativeToBase);
    case BlendMode::kClamp:
        return modifier.outputRange.relativeToBase ? applyClampRelative(baseValue, modifier.outputRange)
                                                   : applyClamp(baseValue, modifier.outputRange);
    case BlendMode::kToggle:
        break;
    }
    return baseValue;
}

inline bool evaluateBool(const Modifier& modifier, bool baseValue, float inputValue) {
    if (!modifier.enabled) {
        return baseValue;
    }
    float n = normalizeInput(modifier.inputRange, inputValue);
    if (modifier.invertInput) {
        n = 1.0f - n;
    }
    if (modifier.blend == BlendMode::kToggle) {
        return n >= 0.5f;
    }
    float result = evaluateFloat(modifier, baseValue ? 1.0f : 0.0f, inputValue);
    return result >= 0.5f;
}

}  // namespace modifier
