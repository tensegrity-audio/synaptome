#include "../synaptome/src/visuals/EightDirectionMotion.h"

#include <array>
#include <cmath>
#include <set>
#include <stdexcept>
#include <utility>

namespace circuit_trace_contract {
namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool same(const glm::ivec2& left, const glm::ivec2& right) {
    return left.x == right.x && left.y == right.y;
}
}

bool RunEightDirectionMotionScenario() {
    using namespace synaptome::eight_direction;
    const std::array<glm::ivec2, 8> expected{{
        { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 },
        { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 },
    }};
    std::set<std::pair<int, int>> uniqueSteps;
    for (int direction = 0; direction < kDirectionCount; ++direction) {
        const glm::ivec2 actual = step(direction);
        require(same(actual, expected[static_cast<std::size_t>(direction)]),
                "direction index does not match its lattice step");
        require(actual.x != 0 || actual.y != 0, "motion emitted a zero step");
        require(std::abs(actual.x) <= 1 && std::abs(actual.y) <= 1,
                "motion escaped the one-cell lattice neighborhood");
        uniqueSteps.emplace(actual.x, actual.y);
        require(same(step(direction + 8), actual) && same(step(direction - 8), actual),
                "direction wrapping changed a step");
        require(opposite(opposite(direction)) == direction,
                "double opposite did not restore direction");
        const glm::ivec2 oppositeStep = step(opposite(direction));
        require(oppositeStep.x == -actual.x && oppositeStep.y == -actual.y,
                "opposite direction did not negate the step");
        require(quantizeVector(unitVector(direction), opposite(direction)) == direction,
                "exact direction vector did not quantize to itself");
    }
    require(uniqueSteps.size() == 8, "direction table must contain eight unique steps");

    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kEighthTurn = kPi / 4.0f;
    for (int direction = 0; direction < kDirectionCount; ++direction) {
        for (float offset : { -0.48f, 0.0f, 0.48f }) {
            const float angle = static_cast<float>(direction) * kEighthTurn +
                                offset * kEighthTurn;
            require(quantizeVector({ std::cos(angle), std::sin(angle) }) == direction,
                    "angle inside direction sector quantized incorrectly");
        }
    }
    require(quantizeVector({ 0.0f, 0.0f }, -1) == 7,
            "zero-vector fallback did not wrap");
    require(turn(7, 1) == 0 && turn(0, -1) == 7, "turn did not wrap");

    glm::ivec2 point{ 9, 4 };
    for (int direction : { 0, 1, 2, 3, 4, 5, 6, 7, 7, 0, 2, 5 }) {
        const glm::ivec2 before = point;
        const glm::ivec2 gridStep = step(direction);
        point.x += gridStep.x;
        point.y += gridStep.y;
        const glm::ivec2 delta{ point.x - before.x, point.y - before.y };
        require(uniqueSteps.count({ delta.x, delta.y }) == 1,
                "composed motion left the eight-direction lattice");
    }
    require(same(wrapPoint({ -1, 10 }, { 8, 6 }), glm::ivec2(7, 4)),
            "wrapPoint contract failed");
    require(same(clampPoint({ -2, 20 }, { 8, 6 }, 1), glm::ivec2(1, 4)),
            "clampPoint margin contract failed");
    return true;
}
}
