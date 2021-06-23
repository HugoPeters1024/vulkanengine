#version 460

#include "math.glsl"
#include "structs.glsl"

layout(location = 1) in flat LightData lightData;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texPos;
layout(binding = 1) uniform sampler2D texNormal;
layout(binding = 2) uniform sampler2D texAlbedo;


layout (push_constant) uniform Push {
    mat4 camera;
    vec4 camPos;
    vec4 invScreenSizeAttenuation;
} push;

void main() {
    const vec2 uv = gl_FragCoord.xy * push.invScreenSizeAttenuation.xy;

    // xyz are position, w is an 10-10-10 uint encoded normal
    const vec3 position = texture(texPos, uv).xyz;
    const vec3 albedo = texture(texAlbedo, uv).xyz;
    const vec3 normal = decodeNormal(texture(texNormal, uv).x);

    vec3 toLight = lightData.lightPos.xyz - position;
    const float lightDist = length(toLight);
    toLight = normalize(toLight);
    const float linear = push.invScreenSizeAttenuation.z;
    const float quadratic = push.invScreenSizeAttenuation.w;

    const float attenuation = 1.0f / (1.0f + linear*lightDist + quadratic*lightDist*lightDist);
    const float diffuse = pow(max(0.0f, dot(normal, toLight)),2);

    const vec3 E = normalize(position - push.camPos.xyz);
    const vec3 R = normalize(reflect(toLight, normal));
    const float specular = 1.6f * pow(max(0.0f, dot(E, R)), 30);

    outColor = vec4(albedo * lightData.lightColor.xyz * (diffuse + specular) * attenuation, 1.0f);
}
