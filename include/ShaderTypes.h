#pragma once

#include "core.h"

struct PushConstant
{
    glm::mat4 camera;
    glm::mat4 mvp;
    glm::vec3 camPos;
};
