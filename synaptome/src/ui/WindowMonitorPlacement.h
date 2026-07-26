#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

namespace window_monitor_placement {

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

inline std::int64_t overlapArea(const Rect& left, const Rect& right) {
    const std::int64_t width = std::max<std::int64_t>(
        0,
        std::min<std::int64_t>(
            static_cast<std::int64_t>(left.x) + left.width,
            static_cast<std::int64_t>(right.x) + right.width) -
            std::max<std::int64_t>(left.x, right.x));
    const std::int64_t height = std::max<std::int64_t>(
        0,
        std::min<std::int64_t>(
            static_cast<std::int64_t>(left.y) + left.height,
            static_cast<std::int64_t>(right.y) + right.height) -
            std::max<std::int64_t>(left.y, right.y));
    return width * height;
}

inline std::int64_t centerDistanceSquared(const Rect& left,
                                          const Rect& right) {
    const std::int64_t leftCenterX =
        static_cast<std::int64_t>(left.x) * 2 + left.width;
    const std::int64_t leftCenterY =
        static_cast<std::int64_t>(left.y) * 2 + left.height;
    const std::int64_t rightCenterX =
        static_cast<std::int64_t>(right.x) * 2 + right.width;
    const std::int64_t rightCenterY =
        static_cast<std::int64_t>(right.y) * 2 + right.height;
    const std::int64_t dx = leftCenterX - rightCenterX;
    const std::int64_t dy = leftCenterY - rightCenterY;
    return dx * dx + dy * dy;
}

inline int selectMonitorForWindow(const Rect& window,
                                  const std::vector<Rect>& monitors) {
    if (monitors.empty()) {
        return -1;
    }
    int selected = 0;
    std::int64_t bestOverlap = -1;
    std::int64_t bestDistance = std::numeric_limits<std::int64_t>::max();
    for (std::size_t i = 0; i < monitors.size(); ++i) {
        const std::int64_t overlap = overlapArea(window, monitors[i]);
        const std::int64_t distance =
            centerDistanceSquared(window, monitors[i]);
        if (overlap > bestOverlap ||
            (overlap == bestOverlap && distance < bestDistance)) {
            selected = static_cast<int>(i);
            bestOverlap = overlap;
            bestDistance = distance;
        }
    }
    return selected;
}

}  // namespace window_monitor_placement
