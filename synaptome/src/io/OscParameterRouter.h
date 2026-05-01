#pragma once
#include "ofMain.h"
#include <functional>
#include <regex>
#include <string>
#include <vector>

class OscParameterRouter {
public:
    struct FloatRouteConfig {
        std::string pattern;
        float* target = nullptr;
        std::function<void(float)> targetSetter;
        float inMin = 0.0f;
        float inMax = 1.0f;
        float outMin = 0.0f;
        float outMax = 1.0f;
        float smooth = 0.2f;
        float deadband = 0.0f;
        std::function<float()> baseValueProvider;
        bool relativeToBase = false;
        bool clampEnabled = false;
        float clampMin = 0.0f;
        float clampMax = 0.0f;
        std::function<bool()> writeGuard;
        bool useMappedValue = true;
    };

    struct BoolRouteConfig {
        std::string pattern;
        bool* target = nullptr;
        float threshold = 0.5f;
        std::function<bool()> writeGuard;
    };

    void addFloatRoute(const FloatRouteConfig& cfg);
    void addBoolRoute(const std::string& pattern, bool* target, float threshold = 0.5f);
    void addBoolRoute(const BoolRouteConfig& cfg);
    void onMessage(const std::string& address, float value);
    void updateBaseValues();

private:
    struct FloatRoute {
        std::regex matcher;
        float* target = nullptr;
        std::function<void(float)> targetSetter;
        float inMin = 0.0f;
        float inMax = 1.0f;
        float outMin = 0.0f;
        float outMax = 1.0f;
        float smooth = 0.2f;
        float deadband = 0.0f;
        float lastValue = 0.0f;
        bool hasLast = false;
        std::function<float()> baseValueProvider;
        bool relativeToBase = false;
        bool clampEnabled = false;
        float clampMin = 0.0f;
        float clampMax = 0.0f;
        float cachedBase = 0.0f;
        bool cachedBaseValid = false;
        float lastApplied = 0.0f;
        bool hasLastApplied = false;
        std::function<bool()> writeGuard;
        bool useMappedValue = true;
    };

    struct BoolRoute {
        std::regex matcher;
        bool* target = nullptr;
        float threshold = 0.5f;
        std::function<bool()> writeGuard;
    };

    std::vector<FloatRoute> floatRoutes;
    std::vector<BoolRoute> boolRoutes;

    void refreshRouteBase(FloatRoute& route);

    static std::regex makeRegex(const std::string& pattern);
};
