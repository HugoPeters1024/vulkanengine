#version 460

#include "math.glsl"

layout(location = 0) in vec3 vPosition;

layout (push_constant) uniform Push {
    mat4 mvp;
    mat4 camera;
    vec4 camPos;
    vec4 lightPos;
    vec4 lightColor;
    vec4 invScreenSizeAttenuation;
} push;

void main() {
    vec4 worldPos = push.mvp * vec4(vPosition, 1.0f);
    gl_Position = push.camera * worldPos;
}

