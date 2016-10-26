#pragma once

#include <cstdint>
#include <array>

namespace mbgl {

class Size {
public:
    Size() : width(0), height(0) {
    }

    Size(const uint32_t width_, const uint32_t height_) : width(width_), height(height_) {
    }

    operator bool() const {
        return width > 0 && height > 0;
    }

    uint32_t width;
    uint32_t height;
};

inline bool operator==(const Size& a, const Size& b) {
    return a.width == b.width && a.height == b.height;
}

inline bool operator!=(const Size& a, const Size& b) {
    return !(a == b);
}

} // namespace mbgl
