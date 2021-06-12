#version 460

#include "math.glsl"

layout(binding = 0) uniform sampler2D texPos;
layout(binding = 1) uniform sampler2D texAlbedo;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform Push {
    vec3 camPos;
} push;

vec3 lightPos = vec3(0, -1, 0);
vec3 lightColor = vec3(17,17, 15);

void main() {
    vec4 posRead = texture(texPos, uv);
//    vec3 normal = texture(texNormals, uv).xyz * 2.0f - 1.0f;
    vec3 normal = decodeNormal(posRead.w);
    if (dot(normal, normal) < 0.01f) return;
    normal = normalize(normal);
    vec3 position = posRead.xyz;
    vec3 albedo = texture(texAlbedo, uv).xyz;

    vec3 toLight = lightPos - position;
    float lightDist = length(toLight);
    const float a = 0.3f;
    const float b = 0.4f;
    float attenuation = 1.0f / (1.0f + a*lightDist + b*lightDist*lightDist);
    toLight /= lightDist;
    float diffuse = max(0.0f, dot(normal, toLight));

    vec3 E = normalize(position - push.camPos);
    vec3 R = normalize(reflect(toLight, normal));
    float specular = 2.6f * pow(max(0.0f, dot(E, R)), 30);


    vec3 ambient = vec3(0.01f);
    outColor = vec4(albedo * ((lightColor * (diffuse + specular)) * attenuation + ambient), 1.0f);
}
