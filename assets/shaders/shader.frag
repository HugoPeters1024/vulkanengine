#version 460

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;

layout(binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(vec3(1,0,2));
    vec4 color = texture(tex, uv);
    outColor = color * max(0.0f, dot(normal, -lightDir));
}