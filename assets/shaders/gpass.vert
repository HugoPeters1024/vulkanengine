#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vUv;
layout(location = 2) in vec3 vNormal;
layout(location = 3) in vec3 vTangent;

layout(location = 0) out vec3 position;
layout(location = 1) out vec2 uv;
layout(location = 2) out mat3 TBN;

layout (push_constant) uniform Lol {
    mat4 camera;
    mat4 mvp;
} lol;

void main() {

    vec4 worldPos = lol.mvp * vec4(vPosition, 1.0f);

    gl_Position = lol.camera * worldPos;

    uv = vUv;
    position = worldPos.xyz;

    vec3 N = normalize((lol.mvp * vec4(vNormal, 0.0f)).xyz);
    vec3 T = normalize((lol.mvp * vec4(vTangent, 0.0f)).xyz);
    vec3 B = cross(T, N);
    TBN = mat3(T, B, N);
}
