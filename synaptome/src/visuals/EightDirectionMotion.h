#pragma once

#include "ofMain.h"
#include <array>
#include <cmath>

namespace synaptome::eight_direction {

inline constexpr int kDirectionCount = 8;
inline constexpr std::array<glm::ivec2, kDirectionCount> kSteps{{
    { 1, 0 },
    { 1, 1 },
    { 0, 1 },
    { -1, 1 },
    { -1, 0 },
    { -1, -1 },
    { 0, -1 },
    { 1, -1 }
}};

inline int wrapIndex(int direction) {
    direction %= kDirectionCount;
    return direction < 0 ? direction + kDirectionCount : direction;
}

inline int turn(int direction, int eighthTurns) {
    return wrapIndex(direction + eighthTurns);
}

inline int opposite(int direction) {
    return turn(direction, 4);
}

inline glm::ivec2 step(int direction) {
    return kSteps[static_cast<std::size_t>(wrapIndex(direction))];
}

inline glm::vec2 unitVector(int direction) {
    const glm::ivec2 gridStep = step(direction);
    const float x = static_cast<float>(gridStep.x);
    const float y = static_cast<float>(gridStep.y);
    const float length = std::sqrt(x * x + y * y);
    return { x / length, y / length };
}

inline int quantizeVector(const glm::vec2& value, int fallbackDirection = 0) {
    if (value.x * value.x + value.y * value.y <= 0.000001f) {
        return wrapIndex(fallbackDirection);
    }
    constexpr float eighthTurn = 0.7853981633974483f;
    return wrapIndex(static_cast<int>(std::round(std::atan2(value.y, value.x) / eighthTurn)));
}

inline glm::ivec2 wrapPoint(glm::ivec2 point, glm::ivec2 size) {
    if (size.x > 0) {
        point.x = (point.x % size.x + size.x) % size.x;
    }
    if (size.y > 0) {
        point.y = (point.y % size.y + size.y) % size.y;
    }
    return point;
}

inline glm::ivec2 clampPoint(glm::ivec2 point, glm::ivec2 size, int margin = 0) {
    const int maxX = std::max(margin, size.x - 1 - margin);
    const int maxY = std::max(margin, size.y - 1 - margin);
    point.x = ofClamp(point.x, margin, maxX);
    point.y = ofClamp(point.y, margin, maxY);
    return point;
}

} // namespace synaptome::eight_direction
