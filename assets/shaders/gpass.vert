#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vUv;
layout(location = 2) in vec3 vNormal;

layout(location = 0) out vec3 position;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 normal;

layout (push_constant) uniform Lol {
    mat4 camera;
    mat4 mvp;
} lol;

void main() {
    gl_Position = lol.camera * lol.mvp * vec4(vPosition, 1.0f);
    uv = vUv;
    normal = normalize((lol.mvp * vec4(vNormal, 0.0f)).xyz);
    position = (lol.mvp * vec4(vPosition, 1)).xyz;
}
