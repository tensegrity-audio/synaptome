#pragma once

namespace glm {
struct vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr vec2() = default;
    constexpr vec2(float scalar) : x(scalar), y(scalar) {}
    constexpr vec2(float xValue, float yValue) : x(xValue), y(yValue) {}
};

struct vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr vec3() = default;
    constexpr vec3(float scalar) : x(scalar), y(scalar), z(scalar) {}
    constexpr vec3(float xValue, float yValue, float zValue) : x(xValue), y(yValue), z(zValue) {}
};

struct vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    constexpr vec4() = default;
    constexpr vec4(float scalar) : x(scalar), y(scalar), z(scalar), w(scalar) {}
    constexpr vec4(float xValue, float yValue, float zValue, float wValue)
        : x(xValue), y(yValue), z(zValue), w(wValue) {}
};

struct ivec2 {
    int x = 0;
    int y = 0;

    constexpr ivec2() = default;
    constexpr ivec2(int xValue, int yValue) : x(xValue), y(yValue) {}
};
} // namespace glm
