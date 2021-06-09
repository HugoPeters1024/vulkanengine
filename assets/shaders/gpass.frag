#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

layout(binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 outNormal;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outAlbedo;

void main() {
    outNormal = vec4((normal * 0.5f) + 0.5f, 0);
    outPosition = vec4(position, 0);
    outAlbedo = texture(tex, uv);
}
