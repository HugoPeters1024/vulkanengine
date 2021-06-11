#ifndef H_SHADER_TYPES
#define H_SHADER_TYPES

#ifndef IN_SHADER
#include "core.h"
using namespace glm;
#endif

struct PushConstant
{
    mat4 camera;
    mat4 mvp;
    vec4 texScale;
};

#endif