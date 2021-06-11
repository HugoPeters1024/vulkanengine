#version 460

#include "math.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

layout(binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outAlbedo;

void main() {
    outPosition = vec4(position, encodeNormal(normal));
    outAlbedo = texture(tex, uv);
}
