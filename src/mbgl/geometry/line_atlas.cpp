#include <mbgl/geometry/line_atlas.hpp>
#include <mbgl/gl/gl.hpp>
#include <mbgl/gl/context.hpp>
#include <mbgl/platform/log.hpp>
#include <mbgl/platform/platform.hpp>

#include <boost/functional/hash.hpp>

#include <sstream>
#include <cmath>

namespace mbgl {

LineAtlas::LineAtlas(uint16_t w, uint16_t h)
    : width(w),
      height(h),
      image(width, height),
      dirty(true) {
}

LineAtlas::~LineAtlas() = default;

LinePatternPos LineAtlas::getDashPosition(const std::vector<float>& dasharray,
                                          LinePatternCap patternCap) {
    size_t key = patternCap == LinePatternCap::Round ? std::numeric_limits<size_t>::min()
                                                     : std::numeric_limits<size_t>::max();
    for (const float part : dasharray) {
        boost::hash_combine<float>(key, part);
    }

    // Note: We're not handling hash collisions here.
    const auto it = positions.find(key);
    if (it == positions.end()) {
        auto inserted = positions.emplace(key, addDash(dasharray, patternCap));
        assert(inserted.second);
        return inserted.first->second;
    } else {
        return it->second;
    }
}

LinePatternPos LineAtlas::addDash(const std::vector<float>& dasharray, LinePatternCap patternCap) {
    int n = patternCap == LinePatternCap::Round ? 7 : 0;
    int dashheight = 2 * n + 1;
    const uint8_t offset = 128;

    if (nextRow + dashheight > height) {
        Log::Warning(Event::OpenGL, "line atlas bitmap overflow");
        return LinePatternPos();
    }

    float length = 0;
    for (const float part : dasharray) {
        length += part;
    }

    float stretch = width / length;
    float halfWidth = stretch * 0.5;
    // If dasharray has an odd length, both the first and last parts
    // are dashes and should be joined seamlessly.
    bool oddLength = dasharray.size() % 2 == 1;

    for (int y = -n; y <= n; y++) {
        int row = nextRow + n + y;
        int index = width * row;

        float left = 0;
        float right = dasharray[0];
        unsigned int partIndex = 1;

        if (oddLength) {
            left -= dasharray.back();
        }

        for (int x = 0; x < width; x++) {

            while (right < x / stretch) {
                left = right;
                right = right + dasharray[partIndex];

                if (oddLength && partIndex == dasharray.size() - 1) {
                    right += dasharray.front();
                }

                partIndex++;
            }

            float distLeft = fabs(x - left * stretch);
            float distRight = fabs(x - right * stretch);
            float dist = fmin(distLeft, distRight);
            bool inside = (partIndex % 2) == 1;
            int signedDistance;

            if (patternCap == LinePatternCap::Round) {
                float distMiddle = n ? (float)y / n * (halfWidth + 1) : 0;
                if (inside) {
                    float distEdge = halfWidth - fabs(distMiddle);
                    signedDistance = sqrt(dist * dist + distEdge * distEdge);
                } else {
                    signedDistance = halfWidth - sqrt(dist * dist + distMiddle * distMiddle);
                }

            } else {
                signedDistance = int((inside ? 1 : -1) * dist);
            }

            image.data[index + x] = fmax(0, fmin(255, signedDistance + offset));
        }
    }

    LinePatternPos position;
    position.y = (0.5 + nextRow + n) / height;
    position.height = (2.0 * n) / height;
    position.width = length;

    nextRow += dashheight;

    dirty = true;

    return position;
}

void LineAtlas::upload(gl::Context& context, gl::TextureUnit unit) {
    if (dirty) {
        bind(context, unit);
    }
}

void LineAtlas::bind(gl::Context& context, gl::TextureUnit unit) {
    if (!texture) {
        texture = context.createTexture(image, unit);
        dirty = false;
    }

    if (dirty) {
        context.updateTexture(*texture, image, unit);
        dirty = false;
    }

    context.bindTexture(*texture, unit, gl::TextureFilter::Linear, gl::TextureMipMap::No,
                        gl::TextureWrap::Repeat, gl::TextureWrap::Clamp);
}

} // namespace mbgl
