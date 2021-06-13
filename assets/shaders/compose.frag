#version 460

#include "math.glsl"

layout(binding = 0) uniform sampler2D texPos;
layout(binding = 1) uniform sampler2D texAlbedo;

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform Push {
    mat4 mvp;
    mat4 camera;
    vec4 camPos;
    vec4 lightPos;
    vec4 lightColor;
    vec4 invScreenSizeAttenuation;
} push;

void main() {
    vec2 uv = gl_FragCoord.xy * push.invScreenSizeAttenuation.xy;

    // xyz are position, w is an 10-10-10 uint encoded normal
    vec4 positionNormal = texture(texPos, uv);

    vec3 normal = decodeNormal(positionNormal.w);
    normal = normalize(normal);

    vec3 position = positionNormal.xyz;
    vec3 albedo = texture(texAlbedo, uv).xyz;

    vec3 toLight = push.lightPos.xyz - position;
    float lightDist = length(toLight);
    const float a = 0.4f;
    const float b = 0.2f;

    const float linear = push.invScreenSizeAttenuation.z;
    const float quadratic = push.invScreenSizeAttenuation.w;
    float attenuation = 1.0f / (1.0f + a*lightDist + b*lightDist*lightDist);
    toLight /= lightDist;
    float diffuse = pow(max(0.0f, dot(normal, toLight)),2);

    vec3 E = normalize(position - push.camPos.xyz);
    vec3 R = normalize(reflect(toLight, normal));
    float specular = 2.6f * pow(max(0.0f, dot(E, R)), 30);


    vec3 ambient = vec3(0.00f);
    outColor = vec4(albedo * push.lightColor.xyz * (diffuse + specular) * attenuation + ambient, 1.0f);
}
