#version 460
#define IN_SHADER
//#include "../ShaderTypes.h"


vec2 positions[3] = vec2[](
    vec2(0.0f, -0.5f),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

layout (push_constant) uniform Lol {
    vec2 offset;
    vec3 color;
} lol;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex] + lol.offset, 0.0f, 1.0f);
}