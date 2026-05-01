#pragma once

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "../common/modifier.h"

class ParameterRegistry {
public:
    struct Range {
        float min = std::numeric_limits<float>::quiet_NaN();
        float max = std::numeric_limits<float>::quiet_NaN();
        float step = 0.0f;
    };

    struct Descriptor {
        std::string id;
        std::string label;
        std::string group;
        std::string units;
        std::string description;
        Range range;
        bool quickAccess = false;
        int quickAccessOrder = 0;
    };

    struct RuntimeModifier {
        modifier::Modifier descriptor;
        std::string ownerTag;
        float inputValue = 0.0f;
        bool active = false;
        float normalizedInput = 0.0f;
        float valueBefore = 0.0f;
        float valueAfter = 0.0f;
        bool applied = false;
        bool inputClamped = false;
        bool outputClamped = false;
        bool conflict = false;
    };

    struct FloatParam {
        Descriptor meta;
        float* value = nullptr;
        float defaultValue = 0.0f;
        float baseValue = 0.0f;
        std::vector<RuntimeModifier> modifiers;

        void applyBaseToLive() const {
            if (value) {
                *value = baseValue;
            }
        }
    };

    struct BoolParam {
        Descriptor meta;
        bool* value = nullptr;
        bool defaultValue = false;
        bool baseValue = false;
        std::vector<RuntimeModifier> modifiers;

        void applyBaseToLive() const {
            if (value) {
                *value = baseValue;
            }
        }
    };

    struct StringParam {
        Descriptor meta;
        std::string* value = nullptr;
        std::string defaultValue;
        std::string baseValue;

        void applyBaseToLive() const {
            if (value) {
                *value = baseValue;
            }
        }
    };

    FloatParam& addFloat(const std::string& id,
                         float* valuePtr,
                         float defaultValue,
                         const Descriptor& metaTemplate);

    BoolParam& addBool(const std::string& id,
                       bool* valuePtr,
                       bool defaultValue,
                       const Descriptor& metaTemplate);

    StringParam& addString(const std::string& id,
                           std::string* valuePtr,
                           const std::string& defaultValue,
                           const Descriptor& metaTemplate);

    FloatParam* findFloat(const std::string& id);
    const FloatParam* findFloat(const std::string& id) const;

    BoolParam* findBool(const std::string& id);
    const BoolParam* findBool(const std::string& id) const;

    StringParam* findString(const std::string& id);
    const StringParam* findString(const std::string& id) const;

    float getFloatBase(const std::string& id) const;
    void setFloatBase(const std::string& id, float value, bool applyToLive = false);

    bool getBoolBase(const std::string& id) const;
    void setBoolBase(const std::string& id, bool value, bool applyToLive = false);

    std::string getStringBase(const std::string& id) const;
    void setStringBase(const std::string& id, const std::string& value, bool applyToLive = false);

    RuntimeModifier& addFloatModifier(const std::string& id, const modifier::Modifier& modifier);
    void clearFloatModifiers(const std::string& id);
    std::size_t clearFloatModifiersMatching(const std::string& id,
                                           const std::function<bool(const RuntimeModifier&)>& predicate);
    void setFloatModifierInput(const std::string& id, std::size_t modifierIndex, float inputValue, bool active = true);
    std::vector<RuntimeModifier>* floatModifiers(const std::string& id);
    const std::vector<RuntimeModifier>* floatModifiers(const std::string& id) const;
    void setFloatModifierEnabled(const std::string& id, std::size_t modifierIndex, bool enabled);
    bool reorderFloatModifier(const std::string& id, std::size_t from, std::size_t to);

    RuntimeModifier& addBoolModifier(const std::string& id, const modifier::Modifier& modifier);
    void clearBoolModifiers(const std::string& id);
    std::size_t clearBoolModifiersMatching(const std::string& id,
                                          const std::function<bool(const RuntimeModifier&)>& predicate);
    void setBoolModifierInput(const std::string& id, std::size_t modifierIndex, float inputValue, bool active = true);
    std::vector<RuntimeModifier>* boolModifiers(const std::string& id);
    const std::vector<RuntimeModifier>* boolModifiers(const std::string& id) const;
    void setBoolModifierEnabled(const std::string& id, std::size_t modifierIndex, bool enabled);
    bool reorderBoolModifier(const std::string& id, std::size_t from, std::size_t to);

    void evaluateAllModifiers();

    void resetFloatToBase(const std::string& id);
    void resetBoolToBase(const std::string& id);
    void resetAllToBase();

    std::vector<std::pair<std::string, float>> snapshotFloatBases() const;
    std::vector<std::pair<std::string, bool>> snapshotBoolBases() const;
    std::vector<std::pair<std::string, std::string>> snapshotStringBases() const;

    const std::vector<FloatParam>& floats() const { return floats_; }
    const std::vector<BoolParam>& bools() const { return bools_; }
    const std::vector<StringParam>& strings() const { return strings_; }

    std::vector<FloatParam*> orderedQuickFloat() const;
    void removeByPrefix(const std::string& prefix);

private:
    static bool floatsNearlyEqual(float a, float b, float epsilon = 1e-4f);

    std::vector<FloatParam> floats_;
    std::vector<BoolParam> bools_;
    std::vector<StringParam> strings_;
};

inline ParameterRegistry::Descriptor mergeDescriptor(const std::string& id,
                                                     const ParameterRegistry::Descriptor& metaTemplate) {
    ParameterRegistry::Descriptor desc = metaTemplate;
    if (desc.id.empty()) desc.id = id;
    if (desc.label.empty()) desc.label = id;
    return desc;
}

inline ParameterRegistry::FloatParam& ParameterRegistry::addFloat(const std::string& id,
                                                                  float* valuePtr,
                                                                  float defaultValue,
                                                                  const Descriptor& metaTemplate) {
    if (!valuePtr) {
        throw std::invalid_argument("ParameterRegistry::addFloat requires non-null value pointer");
    }
    Descriptor descriptor = mergeDescriptor(id, metaTemplate);
    if (findFloat(descriptor.id)) {
        throw std::logic_error("ParameterRegistry::addFloat duplicate id: " + descriptor.id);
    }
    FloatParam param{ descriptor, valuePtr, defaultValue, defaultValue };
    if (param.value) {
        *param.value = defaultValue;
    }
    floats_.push_back(param);
    return floats_.back();
}

inline ParameterRegistry::StringParam& ParameterRegistry::addString(const std::string& id,
                                                                  std::string* valuePtr,
                                                                  const std::string& defaultValue,
                                                                  const Descriptor& metaTemplate) {
    if (!valuePtr) {
        throw std::invalid_argument("ParameterRegistry::addString requires non-null value pointer");
    }
    Descriptor descriptor = mergeDescriptor(id, metaTemplate);
    // ensure no duplicate among floats/bools/strings
    if (findFloat(descriptor.id) || findBool(descriptor.id) || findString(descriptor.id)) {
        throw std::logic_error("ParameterRegistry::addString duplicate id: " + descriptor.id);
    }
    StringParam param{ descriptor, valuePtr, defaultValue, defaultValue };
    if (param.value) {
        *param.value = defaultValue;
    }
    strings_.push_back(param);
    return strings_.back();
}

inline ParameterRegistry::StringParam* ParameterRegistry::findString(const std::string& id) {
    auto it = std::find_if(strings_.begin(), strings_.end(), [&](const StringParam& entry) {
        return entry.meta.id == id;
    });
    return it != strings_.end() ? &(*it) : nullptr;
}

inline const ParameterRegistry::StringParam* ParameterRegistry::findString(const std::string& id) const {
    auto it = std::find_if(strings_.begin(), strings_.end(), [&](const StringParam& entry) {
        return entry.meta.id == id;
    });
    return it != strings_.end() ? &(*it) : nullptr;
}

inline std::string ParameterRegistry::getStringBase(const std::string& id) const {
    const auto* param = findString(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::getStringBase missing id: " + id);
    }
    return param->baseValue;
}

inline void ParameterRegistry::setStringBase(const std::string& id, const std::string& value, bool applyToLive) {
    auto* param = const_cast<ParameterRegistry::StringParam*>(findString(id));
    if (!param) {
        throw std::out_of_range("ParameterRegistry::setStringBase missing id: " + id);
    }
    param->baseValue = value;
    if (applyToLive) {
        param->applyBaseToLive();
    }
}

inline ParameterRegistry::BoolParam& ParameterRegistry::addBool(const std::string& id,
                                                                bool* valuePtr,
                                                                bool defaultValue,
                                                                const Descriptor& metaTemplate) {
    if (!valuePtr) {
        throw std::invalid_argument("ParameterRegistry::addBool requires non-null value pointer");
    }
    Descriptor descriptor = mergeDescriptor(id, metaTemplate);
    if (findBool(descriptor.id)) {
        throw std::logic_error("ParameterRegistry::addBool duplicate id: " + descriptor.id);
    }
    BoolParam param{ descriptor, valuePtr, defaultValue, defaultValue };
    if (param.value) {
        *param.value = defaultValue;
    }
    bools_.push_back(param);
    return bools_.back();
}

inline ParameterRegistry::FloatParam* ParameterRegistry::findFloat(const std::string& id) {
    auto it = std::find_if(floats_.begin(), floats_.end(), [&](const FloatParam& entry) {
        return entry.meta.id == id;
    });
    return it != floats_.end() ? &(*it) : nullptr;
}

inline const ParameterRegistry::FloatParam* ParameterRegistry::findFloat(const std::string& id) const {
    auto it = std::find_if(floats_.begin(), floats_.end(), [&](const FloatParam& entry) {
        return entry.meta.id == id;
    });
    return it != floats_.end() ? &(*it) : nullptr;
}

inline ParameterRegistry::BoolParam* ParameterRegistry::findBool(const std::string& id) {
    auto it = std::find_if(bools_.begin(), bools_.end(), [&](const BoolParam& entry) {
        return entry.meta.id == id;
    });
    return it != bools_.end() ? &(*it) : nullptr;
}

inline const ParameterRegistry::BoolParam* ParameterRegistry::findBool(const std::string& id) const {
    auto it = std::find_if(bools_.begin(), bools_.end(), [&](const BoolParam& entry) {
        return entry.meta.id == id;
    });
    return it != bools_.end() ? &(*it) : nullptr;
}

inline float ParameterRegistry::getFloatBase(const std::string& id) const {
    const auto* param = findFloat(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::getFloatBase missing id: " + id);
    }
    return param->baseValue;
}

inline void ParameterRegistry::setFloatBase(const std::string& id, float value, bool applyToLive) {
    auto* param = findFloat(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::setFloatBase missing id: " + id);
    }
    param->baseValue = value;
    if (applyToLive) {
        param->applyBaseToLive();
    }
}

inline bool ParameterRegistry::getBoolBase(const std::string& id) const {
    const auto* param = findBool(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::getBoolBase missing id: " + id);
    }
    return param->baseValue;
}

inline void ParameterRegistry::setBoolBase(const std::string& id, bool value, bool applyToLive) {
    auto* param = findBool(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::setBoolBase missing id: " + id);
    }
    param->baseValue = value;
    if (applyToLive) {
        param->applyBaseToLive();
    }
}

inline void ParameterRegistry::resetFloatToBase(const std::string& id) {
    auto* param = findFloat(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::resetFloatToBase missing id: " + id);
    }
    param->applyBaseToLive();
}

inline void ParameterRegistry::resetBoolToBase(const std::string& id) {
    auto* param = findBool(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::resetBoolToBase missing id: " + id);
    }
    param->applyBaseToLive();
}

inline void ParameterRegistry::resetAllToBase() {
    for (const auto& entry : floats_) {
        entry.applyBaseToLive();
    }
    for (const auto& entry : bools_) {
        entry.applyBaseToLive();
    }
    for (const auto& entry : strings_) {
        entry.applyBaseToLive();
    }
}

inline std::vector<std::pair<std::string, float>> ParameterRegistry::snapshotFloatBases() const {
    std::vector<std::pair<std::string, float>> result;
    result.reserve(floats_.size());
    for (const auto& entry : floats_) {
        result.emplace_back(entry.meta.id, entry.baseValue);
    }
    return result;
}

inline std::vector<std::pair<std::string, bool>> ParameterRegistry::snapshotBoolBases() const {
    std::vector<std::pair<std::string, bool>> result;
    result.reserve(bools_.size());
    for (const auto& entry : bools_) {
        result.emplace_back(entry.meta.id, entry.baseValue);
    }
    return result;
}
inline std::vector<std::pair<std::string, std::string>> ParameterRegistry::snapshotStringBases() const {
    std::vector<std::pair<std::string, std::string>> result;
    result.reserve(strings_.size());
    for (const auto& entry : strings_) {
        result.emplace_back(entry.meta.id, entry.baseValue);
    }
    return result;
}
inline ParameterRegistry::RuntimeModifier& ParameterRegistry::addFloatModifier(const std::string& id, const modifier::Modifier& modifier) {
    auto* param = findFloat(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::addFloatModifier missing id: " + id);
    }
    param->modifiers.emplace_back();
    auto& runtime = param->modifiers.back();
    runtime.descriptor = modifier;
    runtime.inputValue = 0.0f;
    runtime.active = false;
    runtime.normalizedInput = 0.0f;
    runtime.valueBefore = 0.0f;
    runtime.valueAfter = 0.0f;
    runtime.applied = false;
    runtime.inputClamped = false;
    runtime.outputClamped = false;
    runtime.conflict = false;
    return runtime;
}

inline void ParameterRegistry::clearFloatModifiers(const std::string& id) {
    auto* param = findFloat(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::clearFloatModifiers missing id: " + id);
    }
    param->modifiers.clear();
    if (param->value) {
        *param->value = param->baseValue;
    }
}

inline std::size_t ParameterRegistry::clearFloatModifiersMatching(
    const std::string& id,
    const std::function<bool(const RuntimeModifier&)>& predicate) {
    auto* param = findFloat(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::clearFloatModifiersMatching missing id: " + id);
    }
    auto oldSize = param->modifiers.size();
    param->modifiers.erase(
        std::remove_if(param->modifiers.begin(), param->modifiers.end(), [&](const RuntimeModifier& runtime) {
            return predicate && predicate(runtime);
        }),
        param->modifiers.end());
    if (param->value) {
        *param->value = param->baseValue;
    }
    return oldSize - param->modifiers.size();
}

inline void ParameterRegistry::setFloatModifierInput(const std::string& id, std::size_t modifierIndex, float inputValue, bool active) {
    auto* param = findFloat(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::setFloatModifierInput missing id: " + id);
    }
    if (modifierIndex >= param->modifiers.size()) {
        throw std::out_of_range("ParameterRegistry::setFloatModifierInput index out of range");
    }
    auto& runtime = param->modifiers[modifierIndex];
    runtime.inputValue = inputValue;
    runtime.active = active;
}

inline void ParameterRegistry::setFloatModifierEnabled(const std::string& id, std::size_t modifierIndex, bool enabled) {
    auto* param = findFloat(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::setFloatModifierEnabled missing id: " + id);
    }
    if (modifierIndex >= param->modifiers.size()) {
        throw std::out_of_range("ParameterRegistry::setFloatModifierEnabled index out of range");
    }
    auto& runtime = param->modifiers[modifierIndex];
    runtime.descriptor.enabled = enabled;
    if (!enabled) {
        runtime.active = false;
    }
}

inline bool ParameterRegistry::reorderFloatModifier(const std::string& id, std::size_t from, std::size_t to) {
    auto* param = findFloat(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::reorderFloatModifier missing id: " + id);
    }
    auto& mods = param->modifiers;
    if (from >= mods.size() || to >= mods.size()) {
        throw std::out_of_range("ParameterRegistry::reorderFloatModifier index out of range");
    }
    if (from == to) {
        return false;
    }
    auto fromIdx = static_cast<std::ptrdiff_t>(from);
    auto toIdx = static_cast<std::ptrdiff_t>(to);
    RuntimeModifier moved = std::move(mods[fromIdx]);
    mods.erase(mods.begin() + fromIdx);
    if (fromIdx < toIdx) {
        toIdx -= 1;
    }
    mods.insert(mods.begin() + toIdx, std::move(moved));
    return true;
}

inline std::vector<ParameterRegistry::RuntimeModifier>* ParameterRegistry::floatModifiers(const std::string& id) {
    auto* param = findFloat(id);
    return param ? &param->modifiers : nullptr;
}

inline const std::vector<ParameterRegistry::RuntimeModifier>* ParameterRegistry::floatModifiers(const std::string& id) const {
    const auto* param = findFloat(id);
    return param ? &param->modifiers : nullptr;
}

inline ParameterRegistry::RuntimeModifier& ParameterRegistry::addBoolModifier(const std::string& id, const modifier::Modifier& modifier) {
    auto* param = findBool(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::addBoolModifier missing id: " + id);
    }
    param->modifiers.emplace_back();
    auto& runtime = param->modifiers.back();
    runtime.descriptor = modifier;
    runtime.inputValue = 0.0f;
    runtime.active = false;
    runtime.normalizedInput = 0.0f;
    runtime.valueBefore = 0.0f;
    runtime.valueAfter = 0.0f;
    runtime.applied = false;
    runtime.inputClamped = false;
    runtime.outputClamped = false;
    runtime.conflict = false;
    return runtime;
}

inline void ParameterRegistry::clearBoolModifiers(const std::string& id) {
    auto* param = findBool(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::clearBoolModifiers missing id: " + id);
    }
    param->modifiers.clear();
    if (param->value) {
        *param->value = param->baseValue;
    }
}

inline std::size_t ParameterRegistry::clearBoolModifiersMatching(
    const std::string& id,
    const std::function<bool(const RuntimeModifier&)>& predicate) {
    auto* param = findBool(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::clearBoolModifiersMatching missing id: " + id);
    }
    auto oldSize = param->modifiers.size();
    param->modifiers.erase(
        std::remove_if(param->modifiers.begin(), param->modifiers.end(), [&](const RuntimeModifier& runtime) {
            return predicate && predicate(runtime);
        }),
        param->modifiers.end());
    if (param->value) {
        *param->value = param->baseValue;
    }
    return oldSize - param->modifiers.size();
}

inline void ParameterRegistry::setBoolModifierInput(const std::string& id, std::size_t modifierIndex, float inputValue, bool active) {
    auto* param = findBool(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::setBoolModifierInput missing id: " + id);
    }
    if (modifierIndex >= param->modifiers.size()) {
        throw std::out_of_range("ParameterRegistry::setBoolModifierInput index out of range");
    }
    auto& runtime = param->modifiers[modifierIndex];
    runtime.inputValue = inputValue;
    runtime.active = active;
}

inline void ParameterRegistry::setBoolModifierEnabled(const std::string& id, std::size_t modifierIndex, bool enabled) {
    auto* param = findBool(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::setBoolModifierEnabled missing id: " + id);
    }
    if (modifierIndex >= param->modifiers.size()) {
        throw std::out_of_range("ParameterRegistry::setBoolModifierEnabled index out of range");
    }
    auto& runtime = param->modifiers[modifierIndex];
    runtime.descriptor.enabled = enabled;
    if (!enabled) {
        runtime.active = false;
    }
}

inline bool ParameterRegistry::reorderBoolModifier(const std::string& id, std::size_t from, std::size_t to) {
    auto* param = findBool(id);
    if (!param) {
        throw std::out_of_range("ParameterRegistry::reorderBoolModifier missing id: " + id);
    }
    auto& mods = param->modifiers;
    if (from >= mods.size() || to >= mods.size()) {
        throw std::out_of_range("ParameterRegistry::reorderBoolModifier index out of range");
    }
    if (from == to) {
        return false;
    }
    auto fromIdx = static_cast<std::ptrdiff_t>(from);
    auto toIdx = static_cast<std::ptrdiff_t>(to);
    RuntimeModifier moved = std::move(mods[fromIdx]);
    mods.erase(mods.begin() + fromIdx);
    if (fromIdx < toIdx) {
        toIdx -= 1;
    }
    mods.insert(mods.begin() + toIdx, std::move(moved));
    return true;
}

inline std::vector<ParameterRegistry::RuntimeModifier>* ParameterRegistry::boolModifiers(const std::string& id) {
    auto* param = findBool(id);
    return param ? &param->modifiers : nullptr;
}

inline const std::vector<ParameterRegistry::RuntimeModifier>* ParameterRegistry::boolModifiers(const std::string& id) const {
    const auto* param = findBool(id);
    return param ? &param->modifiers : nullptr;
}

inline void ParameterRegistry::evaluateAllModifiers() {
    for (auto& entry : floats_) {
        if (entry.modifiers.empty()) {
            continue;
        }
        float value = entry.baseValue;
        bool touched = false;
        std::vector<RuntimeModifier*> hardSetModifiers;
        hardSetModifiers.reserve(entry.modifiers.size());
        for (auto& mod : entry.modifiers) {
            mod.valueBefore = value;
            mod.valueAfter = value;
            mod.applied = false;
            mod.inputClamped = false;
            mod.outputClamped = false;
            mod.conflict = false;
            mod.normalizedInput = 0.0f;

            if (!mod.active || !mod.descriptor.enabled) {
                continue;
            }

            touched = true;
            mod.applied = true;

            if (mod.descriptor.inputRange.isValid()) {
                float lo = std::min(mod.descriptor.inputRange.min, mod.descriptor.inputRange.max);
                float hi = std::max(mod.descriptor.inputRange.min, mod.descriptor.inputRange.max);
                mod.inputClamped = (mod.inputValue < lo) || (mod.inputValue > hi);
            }

            float normalized = modifier::normalizeInput(mod.descriptor.inputRange, mod.inputValue);
            if (mod.descriptor.invertInput) {
                normalized = 1.0f - normalized;
            }
            mod.normalizedInput = normalized;

            float newValue = value;
            switch (mod.descriptor.blend) {
            case modifier::BlendMode::kAdditive: {
                float mapped = modifier::mapToOutput(mod.descriptor.outputRange, normalized);
                newValue = modifier::applyAdditive(value, mapped, mod.descriptor.outputRange.relativeToBase);
                break;
            }
            case modifier::BlendMode::kAbsolute: {
                float mapped = modifier::mapToOutput(mod.descriptor.outputRange, normalized);
                newValue = modifier::applyAbsolute(value, mapped, mod.descriptor.outputRange.relativeToBase);
                break;
            }
            case modifier::BlendMode::kScale: {
                float mapped = modifier::mapToOutput(mod.descriptor.outputRange, normalized);
                newValue = modifier::applyScale(value, mapped, mod.descriptor.outputRange.relativeToBase);
                break;
            }
            case modifier::BlendMode::kClamp: {
                float before = value;
                if (mod.descriptor.outputRange.relativeToBase) {
                    newValue = modifier::applyClampRelative(value, mod.descriptor.outputRange);
                } else {
                    newValue = modifier::applyClamp(value, mod.descriptor.outputRange);
                }
                mod.outputClamped = !floatsNearlyEqual(before, newValue);
                break;
            }
            case modifier::BlendMode::kToggle: {
                newValue = modifier::applyToggle(mod.descriptor, value, normalized);
                break;
            }
            }

            mod.valueAfter = newValue;
            value = newValue;

            bool hardSet = mod.descriptor.blend == modifier::BlendMode::kAbsolute ||
                           mod.descriptor.blend == modifier::BlendMode::kToggle;
            if (hardSet) {
                for (auto* previous : hardSetModifiers) {
                    if (!floatsNearlyEqual(previous->valueAfter, mod.valueAfter)) {
                        previous->conflict = true;
                        mod.conflict = true;
                    }
                }
                hardSetModifiers.push_back(&mod);
            }
        }

        float finalValue = touched ? value : entry.baseValue;
        if (std::isfinite(entry.meta.range.min) && std::isfinite(entry.meta.range.max)) {
            float lo = std::min(entry.meta.range.min, entry.meta.range.max);
            float hi = std::max(entry.meta.range.min, entry.meta.range.max);
            finalValue = std::clamp(finalValue, lo, hi);
        }
        if (entry.value) {
            *entry.value = finalValue;
        }
    }

    for (auto& entry : bools_) {
        if (entry.modifiers.empty()) {
            continue;
        }
        bool current = entry.baseValue;
        bool touched = false;
        std::vector<RuntimeModifier*> hardSetModifiers;
        hardSetModifiers.reserve(entry.modifiers.size());

        for (auto& mod : entry.modifiers) {
            float baseNumeric = current ? 1.0f : 0.0f;
            mod.valueBefore = baseNumeric;
            mod.valueAfter = baseNumeric;
            mod.applied = false;
            mod.inputClamped = false;
            mod.outputClamped = false;
            mod.conflict = false;
            mod.normalizedInput = 0.0f;

            if (!mod.active || !mod.descriptor.enabled) {
                continue;
            }

            touched = true;
            mod.applied = true;

            if (mod.descriptor.inputRange.isValid()) {
                float lo = std::min(mod.descriptor.inputRange.min, mod.descriptor.inputRange.max);
                float hi = std::max(mod.descriptor.inputRange.min, mod.descriptor.inputRange.max);
                mod.inputClamped = (mod.inputValue < lo) || (mod.inputValue > hi);
            }

            float normalized = modifier::normalizeInput(mod.descriptor.inputRange, mod.inputValue);
            if (mod.descriptor.invertInput) {
                normalized = 1.0f - normalized;
            }
            mod.normalizedInput = normalized;

            float numericValue = baseNumeric;
            switch (mod.descriptor.blend) {
            case modifier::BlendMode::kAdditive: {
                float mapped = modifier::mapToOutput(mod.descriptor.outputRange, normalized);
                numericValue = modifier::applyAdditive(baseNumeric, mapped, mod.descriptor.outputRange.relativeToBase);
                break;
            }
            case modifier::BlendMode::kAbsolute: {
                float mapped = modifier::mapToOutput(mod.descriptor.outputRange, normalized);
                numericValue = modifier::applyAbsolute(baseNumeric, mapped, mod.descriptor.outputRange.relativeToBase);
                break;
            }
            case modifier::BlendMode::kScale: {
                float mapped = modifier::mapToOutput(mod.descriptor.outputRange, normalized);
                numericValue = modifier::applyScale(baseNumeric, mapped, mod.descriptor.outputRange.relativeToBase);
                break;
            }
            case modifier::BlendMode::kClamp: {
                float before = baseNumeric;
                if (mod.descriptor.outputRange.relativeToBase) {
                    numericValue = modifier::applyClampRelative(baseNumeric, mod.descriptor.outputRange);
                } else {
                    numericValue = modifier::applyClamp(baseNumeric, mod.descriptor.outputRange);
                }
                mod.outputClamped = !floatsNearlyEqual(before, numericValue);
                break;
            }
            case modifier::BlendMode::kToggle: {
                numericValue = modifier::applyToggle(mod.descriptor, baseNumeric, normalized);
                break;
            }
            }

            mod.valueAfter = numericValue;
            current = numericValue >= 0.5f;

            bool hardSet = mod.descriptor.blend == modifier::BlendMode::kAbsolute ||
                           mod.descriptor.blend == modifier::BlendMode::kToggle;
            if (hardSet) {
                for (auto* previous : hardSetModifiers) {
                    bool previousValue = previous->valueAfter >= 0.5f;
                    bool newValue = mod.valueAfter >= 0.5f;
                    if (previousValue != newValue) {
                        previous->conflict = true;
                        mod.conflict = true;
                    }
                }
                hardSetModifiers.push_back(&mod);
            }
        }

        if (entry.value) {
            *entry.value = touched ? current : entry.baseValue;
        }
    }
}


inline bool ParameterRegistry::floatsNearlyEqual(float a, float b, float epsilon) {
    if (!std::isfinite(a) || !std::isfinite(b)) {
        return a == b;
    }
    return std::fabs(a - b) <= epsilon;
}

inline std::vector<ParameterRegistry::FloatParam*> ParameterRegistry::orderedQuickFloat() const {
    std::vector<FloatParam*> results;
    results.reserve(floats_.size());
    for (const auto& entry : floats_) {
        if (!entry.meta.quickAccess) continue;
        results.push_back(const_cast<FloatParam*>(&entry));
    }
    std::sort(results.begin(), results.end(), [](const FloatParam* a, const FloatParam* b) {
        if (a->meta.quickAccessOrder != b->meta.quickAccessOrder) {
            return a->meta.quickAccessOrder < b->meta.quickAccessOrder;
        }
        return a->meta.id < b->meta.id;
    });
    return results;
}

inline void ParameterRegistry::removeByPrefix(const std::string& prefix) {
    auto starts = [&](const std::string& id) {
        return !prefix.empty() && id.compare(0, prefix.size(), prefix) == 0;
    };
    floats_.erase(std::remove_if(floats_.begin(), floats_.end(), [&](const FloatParam& entry) {
        return starts(entry.meta.id);
    }), floats_.end());
    bools_.erase(std::remove_if(bools_.begin(), bools_.end(), [&](const BoolParam& entry) {
        return starts(entry.meta.id);
    }), bools_.end());
    strings_.erase(std::remove_if(strings_.begin(), strings_.end(), [&](const StringParam& entry) {
        return starts(entry.meta.id);
    }), strings_.end());
}
