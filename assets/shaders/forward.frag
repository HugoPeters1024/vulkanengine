#version 460

#include "structs.glsl"
#include "utils.glsl"

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 uv;
layout(location = 2) in mat3 TBN;

layout(set = 0, binding = 0) uniform sampler2D diffuseTex;
layout(set = 0, binding = 1) uniform sampler2D normalTex;

layout(set = 1, binding = 2) uniform LightBuffer {
    LightData lightData[2000];
    uint lightCount;
    float falloffConstant;
    float falloffLinear;
    float falloffQuadratic;
};

layout (push_constant) uniform Lol {
    mat4 camera;
    mat4 mvp;
    vec3 camPos;
} lol;


layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outBloom;

void main() {
    vec3 normal = texture(normalTex, uv).xyz * 2.0f - 1.0f;
    normal = normalize(TBN * normal);
    vec3 albedo = texture(diffuseTex, uv).xyz;

    vec3 totalLight = vec3(0);

    for(uint lightIdx = 0; lightIdx < lightCount; lightIdx++) {
        const vec3 lightPos = lightData[lightIdx].lightPos.xyz;
        const vec3 lightColor = lightData[lightIdx].lightColor.xyz;
        const vec3 toLight = normalize(lightPos - fragPos);
        const float lightDist = distance(fragPos, lightPos);
        const float attenuation = 1.0f / (falloffConstant + falloffLinear * lightDist + falloffQuadratic * lightDist * lightDist);

        const float diffuse = max(0.0f, dot(toLight, normal));
        const vec3 E = normalize(lol.camPos - fragPos);
        const vec3 R = reflect(-toLight, normal);
        const float specular = pow(max(0.0f, dot(R, E)), 25);

        totalLight += lightColor * (diffuse + specular) * attenuation;
    }
    const vec3 ambient = vec3(0.01) + 0.01 * max(0.0f, dot(normal, vec3(0, 1, 0)));
    outColor = vec4(albedo * (totalLight + ambient), 1.0f);
    outBloom = vec4(bloomFilter(outColor.xyz), 0.0f);
}
