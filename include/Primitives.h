#pragma once

#include <limits>
#include <glm/glm.hpp>

struct BoundingBox {
    glm::vec3 vmin, vmax;

    void consumePoint(glm::vec3 p) {
        vmin = glm::min(vmin, p);
        vmax = glm::max(vmax, p);
    }
    static BoundingBox InsideOut() {
        return {
            .vmin = glm::vec3(std::numeric_limits<float>::infinity()),
            .vmax = glm::vec3(-std::numeric_limits<float>::infinity()),
        };
    }

    BoundingBox operator* (const glm::vec3& s) const {
        return BoundingBox {
            .vmin = vmin * s,
            .vmax = vmax * s,
        };
    }
};