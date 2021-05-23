#ifndef H_SHADER_TYPES
#define H_SHADER_TYPES

#ifndef IN_SHADER
#include <glm/glm.hpp>
using namespace glm;
#endif

struct PushConstant
{
    mat4 camera;
    mat4 mvp;
};

#endif