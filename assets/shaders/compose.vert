#version 460

#include "math.glsl"

layout(location = 0) in vec3 vPosition;

layout(location = 1) out flat uint instanceID;

struct LightData {
    vec4 lightPos;
    vec4 lightColor;
};

layout(binding = 2) readonly buffer LightDataBuffer {
    LightData data[];
} lightData;

layout (push_constant) uniform Push {
    mat4 camera;
    vec4 camPos;
    vec4 invScreenSizeAttenuation;
} push;

void main() {
    float prec = 255.0f / 5.0f;
    const float linear = push.invScreenSizeAttenuation.z;
    const float quadratic = push.invScreenSizeAttenuation.w;
    const vec3 lightColor = lightData.data[gl_InstanceIndex].lightColor.xyz;
    const float maxI = max(lightColor.x, max(lightColor.y, lightColor.z));
    float radius = (-linear + sqrt(linear*linear - 4 * quadratic * (1.0f - maxI * prec))) / (2 * quadratic);

    mat4 transform = mat4(
        vec4(radius, 0, 0, 0),
        vec4(0, radius, 0, 0),
        vec4(0, 0, radius, 0),
        vec4(lightData.data[gl_InstanceIndex].lightPos.xyz, 1)
    );
    vec4 worldPos = transform * vec4(vPosition, 1.0f);
    gl_Position = push.camera * worldPos;
    instanceID = gl_InstanceIndex;
}

