#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vUv;
layout(location = 2) in vec3 vNormal;
layout(location = 3) in vec3 vTangent;

layout(location = 0) out vec3 position;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 normal;
layout(location = 3) out mat3 TBN;

layout (push_constant) uniform Lol {
    mat4 camera;
    mat4 mvp;
    vec4 texScale;
} lol;

void main() {
    gl_Position = lol.camera * lol.mvp * vec4(vPosition, 1.0f);
    uv = vUv * lol.texScale.xy;
    normal = normalize((lol.mvp * vec4(vNormal, 0.0f)).xyz);
    position = (lol.mvp * vec4(vPosition, 1)).xyz;

    vec3 vBitangent = cross(vTangent, vNormal);

    vec3 T = normalize((lol.mvp * vec4(vTangent, 0.0f)).xyz);
    vec3 B = normalize((lol.mvp * vec4(vBitangent, 0.0f)).xyz);
    vec3 N = normalize((lol.mvp * vec4(vNormal, 0.0f)).xyz);
    TBN = transpose(mat3(B, -T, N));
}
