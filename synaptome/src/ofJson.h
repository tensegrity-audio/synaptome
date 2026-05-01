#pragma once

#include <nlohmann/json.hpp>
#include <fstream>
#include <string>

using ofJson = nlohmann::json;

inline ofJson ofLoadJson(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return ofJson();
    }
    ofJson json;
    try {
        in >> json;
    } catch (...) {
        return ofJson();
    }
    return json;
}

inline bool ofSavePrettyJson(const std::string& path, const ofJson& json) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }
    try {
        out << json.dump(2);
    } catch (...) {
        return false;
    }
    return true;
}
