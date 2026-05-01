#include "OscParameterRouter.h"

#include <algorithm>

namespace {
    float clamp01(float v) {
        return ofClamp(v, 0.0f, 1.0f);
    }
}

void OscParameterRouter::addFloatRoute(const FloatRouteConfig& cfg) {
    if (!cfg.target && !cfg.targetSetter) return;
    FloatRoute route;
    route.matcher = makeRegex(cfg.pattern);
    route.target = cfg.target;
    route.targetSetter = cfg.targetSetter;
    route.inMin = cfg.inMin;
    route.inMax = cfg.inMax;
    route.outMin = cfg.outMin;
    route.outMax = cfg.outMax;
    route.smooth = cfg.smooth;
    route.deadband = cfg.deadband;
    route.baseValueProvider = cfg.baseValueProvider;
    route.relativeToBase = cfg.relativeToBase;
    route.clampEnabled = cfg.clampEnabled;
    route.clampMin = cfg.clampMin;
    route.clampMax = cfg.clampMax;
    route.writeGuard = cfg.writeGuard;
    route.useMappedValue = cfg.useMappedValue;
    refreshRouteBase(route);
    route.hasLast = false;
    route.lastValue = route.relativeToBase && route.cachedBaseValid ? route.cachedBase : 0.0f;
    floatRoutes.push_back(route);
}

void OscParameterRouter::addBoolRoute(const std::string& pattern, bool* target, float threshold) {
    BoolRouteConfig cfg;
    cfg.pattern = pattern;
    cfg.target = target;
    cfg.threshold = threshold;
    addBoolRoute(cfg);
}

void OscParameterRouter::addBoolRoute(const BoolRouteConfig& cfg) {
    if (!cfg.target) return;
    BoolRoute route;
    route.matcher = makeRegex(cfg.pattern);
    route.target = cfg.target;
    route.threshold = cfg.threshold;
    route.writeGuard = cfg.writeGuard;
    boolRoutes.push_back(route);
}

void OscParameterRouter::onMessage(const std::string& address, float value) {
    for (auto& route : floatRoutes) {
        if (!route.target && !route.targetSetter) continue;
        if (!std::regex_match(address, route.matcher)) continue;
        float inLo = std::min(route.inMin, route.inMax);
        float inHi = std::max(route.inMin, route.inMax);
        float clampedInput = route.useMappedValue ? value : ofClamp(value, inLo, inHi);
        float norm = 0.0f;
        if (route.inMax - route.inMin != 0.0f) {
            norm = ofMap(value, route.inMin, route.inMax, 0.0f, 1.0f, true);
        }
        float mapped = ofLerp(route.outMin, route.outMax, clamp01(norm));
        float desired = route.useMappedValue ? mapped : clampedInput;
        if (route.relativeToBase) {
            float baseValue = 0.0f;
            if (route.baseValueProvider) {
                baseValue = route.baseValueProvider();
            } else if (route.cachedBaseValid) {
                baseValue = route.cachedBase;
            } else if (route.target) {
                baseValue = *route.target;
            }
            desired = baseValue * mapped;
        }
        if (route.useMappedValue && route.clampEnabled) {
            float lo = std::min(route.clampMin, route.clampMax);
            float hi = std::max(route.clampMin, route.clampMax);
            desired = ofClamp(desired, lo, hi);
        }
        if (!route.hasLast) {
            if (route.relativeToBase && route.baseValueProvider) {
                route.lastValue = route.baseValueProvider();
            } else if (route.target && route.useMappedValue) {
                route.lastValue = *route.target;
            } else {
                route.lastValue = desired;
            }
            route.hasLast = true;
        }
        route.lastValue += route.smooth * (desired - route.lastValue);
        if (route.writeGuard && !route.writeGuard()) {
            continue;
        }
        bool shouldApply = !route.hasLastApplied || std::fabs(route.lastValue - route.lastApplied) >= route.deadband;
        if (!shouldApply && !route.targetSetter && route.target) {
            // legacy pointer path used deadband relative to current target value
            shouldApply = std::fabs(route.lastValue - *route.target) >= route.deadband;
        }
        if (!shouldApply) {
            continue;
        }
        if (route.targetSetter) {
            route.targetSetter(route.lastValue);
        }
        if (route.target) {
            *route.target = route.lastValue;
        }
        route.lastApplied = route.lastValue;
        route.hasLastApplied = true;
    }

    for (auto& route : boolRoutes) {
        if (!route.target) continue;
        if (!std::regex_match(address, route.matcher)) continue;
        if (route.writeGuard && !route.writeGuard()) continue;
        *route.target = (value >= route.threshold);
    }
}

std::regex OscParameterRouter::makeRegex(const std::string& pattern) {
    std::string expr;
    expr.reserve(pattern.size() * 2 + 4);
    expr += '^';
    for (char c : pattern) {
        if (c == '*') {
            expr += ".*";
        } else if (c == '?') {
            expr += '.';
        } else {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '/' || c == '_' || c == '-') {
                expr += c;
            } else {
                expr += '\\';
                expr += c;
            }
        }
    }
    expr += '$';
    return std::regex(expr);
}

void OscParameterRouter::updateBaseValues() {
    for (auto& route : floatRoutes) {
        if (!route.relativeToBase) {
            continue;
        }
        refreshRouteBase(route);
    }
}

void OscParameterRouter::refreshRouteBase(FloatRoute& route) {
    if (!route.relativeToBase) {
        route.cachedBase = 0.0f;
        route.cachedBaseValid = false;
        return;
    }
    if (route.baseValueProvider) {
        route.cachedBase = route.baseValueProvider();
        route.cachedBaseValid = true;
        return;
    }
    if (route.target) {
        route.cachedBase = *route.target;
        route.cachedBaseValid = true;
        return;
    }
    route.cachedBase = 0.0f;
    route.cachedBaseValid = false;
}
