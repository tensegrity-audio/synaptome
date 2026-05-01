#include <cassert>
#include <cmath>
#include <iostream>

#include "../common/modifier.h"

using modifier::BlendMode;
using modifier::Modifier;
using modifier::Range;

template <typename T>
bool nearlyEqual(T a, T b, T eps = static_cast<T>(1e-5)) {
    return std::fabs(a - b) <= eps;
}

int main() {
    {
        Modifier mod;
        mod.blend = BlendMode::kAdditive;
        mod.inputRange = Range{0.0f, 127.0f, false};
        mod.outputRange = Range{-5.0f, 5.0f, false};
        float result = modifier::evaluateFloat(mod, 10.0f, 127.0f);
        assert(nearlyEqual(result, 15.0f));
    }

    {
        Modifier mod;
        mod.blend = BlendMode::kAdditive;
        mod.inputRange = Range{0.0f, 127.0f, false};
        mod.outputRange = Range{-1.0f, 1.0f, true};
        float result = modifier::evaluateFloat(mod, 100.0f, 127.0f);
        assert(nearlyEqual(result, 200.0f));
    }

    {
        Modifier mod;
        mod.blend = BlendMode::kAbsolute;
        mod.inputRange = Range{0.0f, 1.0f, false};
        mod.outputRange = Range{0.0f, 1.0f, true};
        float result = modifier::evaluateFloat(mod, 80.0f, 0.5f);
        assert(nearlyEqual(result, 40.0f));
    }

    {
        Modifier mod;
        mod.blend = BlendMode::kScale;
        mod.inputRange = Range{0.0f, 1.0f, false};
        mod.outputRange = Range{0.0f, 2.0f, false};
        float result = modifier::evaluateFloat(mod, 10.0f, 0.75f);
        assert(nearlyEqual(result, 15.0f));
    }

    {
        Modifier mod;
        mod.blend = BlendMode::kClamp;
        mod.outputRange = Range{0.0f, 1.0f, false};
        float result = modifier::evaluateFloat(mod, 1.5f, 0.0f);
        assert(nearlyEqual(result, 1.0f));
    }

    {
        Modifier mod;
        mod.blend = BlendMode::kClamp;
        mod.outputRange = Range{-0.5f, 0.5f, true};
        float result = modifier::evaluateFloat(mod, 2.0f, 0.0f);
        assert(nearlyEqual(result, 1.0f));
    }

    {
        Modifier mod;
        mod.blend = BlendMode::kToggle;
        mod.inputRange = Range{0.0f, 1.0f, false};
        mod.outputRange = Range{0.0f, 1.0f, false};
        float resultOff = modifier::evaluateFloat(mod, 0.0f, 0.2f);
        float resultOn = modifier::evaluateFloat(mod, 0.0f, 0.8f);
        assert(nearlyEqual(resultOff, 0.0f));
        assert(nearlyEqual(resultOn, 1.0f));
    }

    {
        Modifier mod;
        mod.blend = BlendMode::kToggle;
        mod.inputRange = Range{0.0f, 1.0f, false};
        mod.outputRange = Range{-1.0f, 1.0f, true};
        float base = 100.0f;
        float resultOff = modifier::evaluateFloat(mod, base, 0.1f);
        float resultOn = modifier::evaluateFloat(mod, base, 0.9f);
        assert(nearlyEqual(resultOff, 0.0f));
        assert(nearlyEqual(resultOn, 200.0f));
    }

    std::cout << "modifier evaluator tests passed\n";
    return 0;
}
