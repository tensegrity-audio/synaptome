#pragma once

#ifdef OF_SDK_AVAILABLE
#include <utils/ofLog.h>
#else

#include <iostream>
#include <sstream>
#include <string>

class OfLogStream {
public:
    explicit OfLogStream(const std::string& channel, const std::string& level)
        : channel_(channel), level_(level) {}
    ~OfLogStream() {
        if (!buffer_.str().empty()) {
            std::cerr << "[" << level_ << ":" << channel_ << "] " << buffer_.str() << std::endl;
        }
    }

    template <typename T>
    OfLogStream& operator<<(const T& value) {
        buffer_ << value;
        return *this;
    }

private:
    std::string channel_;
    std::string level_;
    std::ostringstream buffer_;
};

inline OfLogStream ofLogWarning(const std::string& channel) {
    return OfLogStream(channel, "warning");
}

inline OfLogStream ofLogNotice(const std::string& channel) {
    return OfLogStream(channel, "notice");
}

inline OfLogStream ofLogVerbose(const std::string& channel) {
    return OfLogStream(channel, "verbose");
}

inline OfLogStream ofLogError(const std::string& channel) {
    return OfLogStream(channel, "error");
}

#endif
