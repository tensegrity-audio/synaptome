#pragma once

#ifdef OF_SDK_AVAILABLE
#include <utils/ofUtils.h>
#else

#include <algorithm>
#include <chrono>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

inline uint64_t ofGetElapsedTimeMillis() {
    using clock = std::chrono::steady_clock;
    static const auto start = clock::now();
    auto diff = clock::now() - start;
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(diff).count());
}

inline float ofGetElapsedTimef() {
    return static_cast<float>(ofGetElapsedTimeMillis()) / 1000.0f;
}

template <typename T>
inline T ofClamp(T value, T minValue, T maxValue) {
    return std::max(minValue, std::min(maxValue, value));
}

inline float ofLerp(float start, float stop, float amt) {
    return start + (stop - start) * amt;
}

inline float ofMap(float value,
                   float inputMin,
                   float inputMax,
                   float outputMin,
                   float outputMax,
                   bool clamp = false) {
    if (inputMax - inputMin == 0.0f) {
        return outputMin;
    }
    float t = (value - inputMin) / (inputMax - inputMin);
    if (clamp) {
        t = ofClamp(t, 0.0f, 1.0f);
    }
    return outputMin + (outputMax - outputMin) * t;
}

inline std::string ofToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

inline float ofToFloat(const std::string& value) {
    return std::stof(value);
}

inline int ofToInt(const std::string& value) {
    return std::stoi(value);
}

inline std::string ofToUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

inline bool ofIsStringInString(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    return haystack.find(needle) != std::string::npos;
}

template <typename T>
inline std::string ofToString(const T& value, int precision = -1) {
    std::ostringstream ss;
    if (precision >= 0) {
        ss << std::fixed << std::setprecision(precision);
    }
    ss << value;
    return ss.str();
}

inline std::string ofTrim(const std::string& value) {
    auto begin = value.begin();
    auto end = value.end();
    while (begin != end && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    while (end != begin && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    return std::string(begin, end);
}

inline std::vector<std::string> ofSplitString(const std::string& value,
                                              const std::string& delimiter,
                                              bool ignoreEmpty = true,
                                              bool trim = false) {
    std::vector<std::string> result;
    if (delimiter.empty()) {
        result.push_back(value);
        return result;
    }
    std::size_t start = 0;
    while (start <= value.size()) {
        auto pos = value.find(delimiter, start);
        std::string token = pos == std::string::npos ?
            value.substr(start) :
            value.substr(start, pos - start);
        if (trim) {
            token = ofTrim(token);
        }
        if (!token.empty() || !ignoreEmpty) {
            result.push_back(token);
        }
        if (pos == std::string::npos) {
            break;
        }
        start = pos + delimiter.size();
    }
    return result;
}

inline std::string ofJoinString(const std::vector<std::string>& values,
                                const std::string& delimiter) {
    if (values.empty()) {
        return "";
    }
    std::ostringstream ss;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            ss << delimiter;
        }
        ss << values[i];
    }
    return ss.str();
}

#endif
