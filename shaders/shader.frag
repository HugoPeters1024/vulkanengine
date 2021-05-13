#version 460
#define IN_SHADER
#include "../ShaderTypes.h"

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}