#version 460

#include "math.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in mat3 TBN;

layout(binding = 0) uniform sampler2D diffuseTex;
layout(binding = 1) uniform sampler2D normalTex;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outAlbedo;

void main() {
    vec3 newNormal = texture(normalTex, uv).xyz * 2.0f - 1.0f;
    newNormal = normalize(TBN * newNormal);
    outPosition = vec4(position, encodeNormal(newNormal));
    outAlbedo = texture(diffuseTex, uv);
}
