#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;

layout(location = 0) out vec3 normal;

layout (push_constant) uniform Lol {
    mat4 camera;
    mat4 mvp;
} lol;

void main() {
    gl_Position = lol.camera * lol.mvp * vec4(vPosition, 1.0f);
    normal = (lol.mvp * vec4(vNormal, 0.0f)).xyz;
}