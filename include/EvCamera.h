#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "utils.hpp"
#include "EvInputHelper.h"

class EvCamera {
public:
    glm::vec3 position{0,0,0};
    float theta = 3.1415926f/2.0f, phi = 3.1415926/2.0f;
    float fov{120};

    inline glm::vec3 getViewDir() const { return glm::sphericalToCartesian(theta, phi); }
    glm::mat4 getVPMatrix(float aspectRatio) const;

    void handleInput(const EvInputHelper& input);
};