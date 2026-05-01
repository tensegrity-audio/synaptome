#pragma once

#ifdef OF_SDK_AVAILABLE
#include <utils/ofFileUtils.h>
#else

#include "../../synaptome/src/ofJson.h"
#include "SynaptomeTestPaths.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

std::string ofToDataPath(const std::string& path, bool makeAbsolute = false);

class ofBuffer {
public:
    ofBuffer() = default;
    explicit ofBuffer(std::string text) : data_(std::move(text)) {}

    const std::string& getText() const { return data_; }
    bool empty() const { return data_.empty(); }

private:
    std::string data_;
};

namespace ofFilePath {
inline std::string getEnclosingDirectory(const std::string& path, bool /*relative*/ = false) {
    std::filesystem::path p(path);
    auto parent = p.has_parent_path() ? p.parent_path() : std::filesystem::path();
    auto result = parent.string();
    if (!result.empty()) {
        result += std::filesystem::path::preferred_separator;
    }
    return result;
}

inline std::string join(const std::string& base, const std::string& path) {
    return (std::filesystem::path(base) / path).string();
}

inline std::string getAbsolutePath(const std::string& path) {
    std::error_code ec;
    auto resolved = std::filesystem::absolute(path, ec);
    if (ec) {
        return path;
    }
    return resolved.string();
}

inline std::string getFileName(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}
} // namespace ofFilePath

class ofFile {
public:
    enum Mode {
        Reference,
        ReadOnly,
        WriteOnly
    };

    ofFile() = default;
    explicit ofFile(std::string path, Mode mode = Reference) {
        open(std::move(path), mode);
    }

    bool open(std::string path, Mode mode) {
        path_ = std::move(path);
        mode_ = mode;
        if (mode == WriteOnly) {
            std::ofstream out(path_);
            return out.is_open();
        }
        if (mode == ReadOnly) {
            std::ifstream in(path_);
            return in.is_open();
        }
        return true;
    }

    const std::string& path() const { return path_; }
    std::string getAbsolutePath() const { return ofFilePath::getAbsolutePath(path_); }
    std::string getBaseName() const {
        std::filesystem::path p(path_);
        return p.stem().string();
    }
    std::string getFileName() const {
        std::filesystem::path p(path_);
        return p.filename().string();
    }
    std::string getExtension() const {
        std::filesystem::path p(path_);
        auto ext = p.extension().string();
        if (!ext.empty() && ext.front() == '.') {
            ext.erase(0, 1);
        }
        return ext;
    }
    bool isDirectory() const { return isDirectory_; }
    void setIsDirectory(bool value) { isDirectory_ = value; }

    static bool doesFileExist(const std::string& path) {
        std::error_code ec;
        return std::filesystem::exists(path, ec);
    }

    static bool removeFile(const std::string& path, bool recursive = false) {
        std::error_code ec;
        if (recursive) {
            std::filesystem::remove_all(path, ec);
        } else {
            std::filesystem::remove(path, ec);
        }
        return ec.value() == 0 || !std::filesystem::exists(path, ec);
    }

    static bool moveFromTo(const std::string& src, const std::string& dst, bool overwrite = true, bool relativeToData = false) {
        namespace fs = std::filesystem;
        std::error_code ec;
        fs::path from = relativeToData ? fs::path(ofToDataPath(src, true)) : fs::path(src);
        fs::path to = relativeToData ? fs::path(ofToDataPath(dst, true)) : fs::path(dst);
        if (overwrite && fs::exists(to, ec)) {
            fs::remove_all(to, ec);
        }
        fs::create_directories(to.parent_path(), ec);
        fs::rename(from, to, ec);
        return !ec;
    }

private:
    std::string path_;
    Mode mode_ = Reference;
    bool isDirectory_ = false;
};

class ofDirectory {
public:
    ofDirectory() = default;
    explicit ofDirectory(std::string path) : path_(std::move(path)) {}

    bool exists() const {
        std::error_code ec;
        return std::filesystem::exists(path_, ec);
    }

    void allowExt(const std::string& ext) { extension_ = ext; }

    std::size_t listDir() {
        files_.clear();
        std::error_code ec;
        if (!exists()) {
            return 0;
        }
        for (auto& entry : std::filesystem::directory_iterator(path_, ec)) {
            if (ec) break;
            ofFile file(entry.path().string());
            file.setIsDirectory(entry.is_directory());
            if (!file.isDirectory() && !extension_.empty()) {
                if (file.getExtension() != extension_) {
                    continue;
                }
            }
            files_.push_back(std::move(file));
        }
        return files_.size();
    }

    const std::vector<ofFile>& getFiles() const { return files_; }
    std::size_t size() const { return files_.size(); }
    const ofFile& getFile(std::size_t index) const { return files_.at(index); }

    static bool createDirectory(const std::string& path, bool recursive = true, bool allowExisting = true) {
        std::error_code ec;
        if (recursive) {
            std::filesystem::create_directories(path, ec);
        } else {
            std::filesystem::create_directory(path, ec);
        }
        if (!ec) return true;
        return allowExisting && std::filesystem::exists(path);
    }

private:
    std::filesystem::path path_;
    std::string extension_;
    std::vector<ofFile> files_;
};

inline ofBuffer ofBufferFromFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return ofBuffer();
    }
    std::string data((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
    return ofBuffer(std::move(data));
}

inline std::string ofToDataPath(const std::string& path, bool makeAbsolute) {
    namespace fs = std::filesystem;
    fs::path dataRoot = synaptome_test_paths::dataRoot();
    fs::path target = path;
    if (!target.is_absolute()) {
        target = dataRoot / target;
    }
    target = target.lexically_normal();
    if (makeAbsolute) {
        std::error_code ec;
        auto canonical = fs::weakly_canonical(target, ec);
        if (!ec) {
            target = canonical;
        }
    }
    return target.string();
}

#endif
