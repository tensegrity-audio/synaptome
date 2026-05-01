#pragma once

#ifdef OF_SDK_AVAILABLE
#include <graphics/ofPixels.h>
#else

#include <algorithm>
#include <cstddef>
#include <vector>

enum ofPixelFormat {
    OF_PIXELS_GRAY = 1,
    OF_PIXELS_RGB = 3,
    OF_PIXELS_RGBA = 4,
};

class ofPixels {
public:
    void allocate(int width, int height, ofPixelFormat format = OF_PIXELS_RGBA) {
        width_ = std::max(0, width);
        height_ = std::max(0, height);
        channels_ = static_cast<int>(format);
        data_.assign(static_cast<std::size_t>(width_) * height_ * channels_, 0);
    }

    void set(unsigned char value) {
        std::fill(data_.begin(), data_.end(), value);
    }

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    int getNumChannels() const { return channels_; }
    std::size_t size() const { return data_.size(); }

    unsigned char* getData() { return data_.data(); }
    const unsigned char* getData() const { return data_.data(); }

    unsigned char& operator[](std::size_t index) { return data_[index]; }
    const unsigned char& operator[](std::size_t index) const { return data_[index]; }

private:
    int width_ = 0;
    int height_ = 0;
    int channels_ = 4;
    std::vector<unsigned char> data_;
};

#endif
