#version 460

#include "utils.glsl"

layout(location = 0) in vec3 uv;

layout(binding = 0) uniform samplerCube skybox;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 bloom;


void main() {
    color = texture(skybox, uv);
    bloom = vec4(bloomFilter(color.xyz), 0.0f);
}
