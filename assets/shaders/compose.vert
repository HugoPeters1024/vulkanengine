#version 460

#include "math.glsl"
#include "structs.glsl"

layout(location = 0) in vec3 vPosition;

layout(location = 1) out LightData lightDataOut;

layout(binding = 2) readonly buffer LightDataBuffer {
    LightData data[];
} lightData;

layout (push_constant) uniform Push {
    mat4 camera;
    vec4 camPos;
    vec4 invScreenSizeAttenuation;
} push;

void main() {
    lightDataOut = lightData.data[gl_InstanceIndex];
    float prec = 255.0f / 5.0f;
    const float linear = push.invScreenSizeAttenuation.z;
    const float quadratic = push.invScreenSizeAttenuation.w;
    const vec3 lightColor = lightDataOut.lightColor.xyz;
    const float maxI = max(lightColor.x, max(lightColor.y, lightColor.z));
    float radius = (-linear + sqrt(linear*linear - 4.0f * quadratic * (1.0f - maxI * prec))) / (2.0f * quadratic);

    mat4 transform = mat4(
        vec4(radius, 0, 0, 0),
        vec4(0, radius, 0, 0),
        vec4(0, 0, radius, 0),
        vec4(lightDataOut.lightPos.xyz, 1)
    );
    vec4 worldPos = transform * vec4(vPosition, 1.0f);
    gl_Position = push.camera * worldPos;
}

