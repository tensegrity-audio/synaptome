#pragma once

#if defined(SYNAPTOME_OF_STUB_TRACE)

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ofstub {

enum class EventKind {
    Allocate,
    AllocateFailed,
    Begin,
    End,
    Release,
    Draw,
    ClearColor,
    ClearBits,
    PushStyle,
    PopStyle,
    EnableBlend,
    DisableBlend,
    SetColor,
};

struct FboSettings {
    int width = 0;
    int height = 0;
    bool useDepth = false;
    bool useStencil = false;
    int internalFormat = 0;
    int textureTarget = 0;
    int minFilter = 0;
    int maxFilter = 0;
    int wrapHorizontal = 0;
    int wrapVertical = 0;
};

struct Event {
    EventKind kind = EventKind::Allocate;
    std::uint64_t objectId = 0;
    std::uint64_t activeTargetId = 0;
    FboSettings settings;
    float value0 = 0.0f;
    float value1 = 0.0f;
    float value2 = 0.0f;
    float value3 = 0.0f;
    unsigned int bits = 0;
    bool allocated = false;
};

inline std::vector<Event> recordedEvents;
inline std::vector<std::uint64_t> targetStack;
inline std::uint64_t nextObjectId = 1;
inline std::size_t allocationAttemptCount = 0;
inline std::size_t failedAllocationAttempt = 0;

inline void reset() {
    recordedEvents.clear();
    targetStack.clear();
    allocationAttemptCount = 0;
    failedAllocationAttempt = 0;
}

inline const std::vector<Event>& events() {
    return recordedEvents;
}

inline std::uint64_t nextId() {
    return nextObjectId++;
}

inline std::uint64_t activeTargetId() {
    return targetStack.empty() ? 0 : targetStack.back();
}

inline void record(Event event) {
    if (event.activeTargetId == 0) {
        event.activeTargetId = activeTargetId();
    }
    recordedEvents.push_back(event);
}

inline void failAllocationOnAttempt(std::size_t oneBasedAttempt) {
    failedAllocationAttempt = oneBasedAttempt;
}

inline void clearAllocationFailure() {
    failedAllocationAttempt = 0;
}

inline std::size_t allocationAttempts() {
    return allocationAttemptCount;
}

inline bool beginAllocationAttempt() {
    ++allocationAttemptCount;
    return failedAllocationAttempt != 0 &&
        allocationAttemptCount == failedAllocationAttempt;
}

inline void beginTarget(std::uint64_t objectId, bool allocated) {
    record({
        EventKind::Begin,
        objectId,
        activeTargetId(),
        {},
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0,
        allocated,
    });
    if (allocated) {
        targetStack.push_back(objectId);
    }
}

inline void endTarget(std::uint64_t objectId, bool allocated) {
    record({
        EventKind::End,
        objectId,
        activeTargetId(),
        {},
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0,
        allocated,
    });
    if (allocated &&
        !targetStack.empty() &&
        targetStack.back() == objectId) {
        targetStack.pop_back();
    }
}

} // namespace ofstub

#endif
