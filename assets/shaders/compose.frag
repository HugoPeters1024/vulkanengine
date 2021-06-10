#version 460

layout(binding = 0) uniform sampler2D texNormals;
layout(binding = 1) uniform sampler2D texPos;
layout(binding = 2) uniform sampler2D texAlbedo;

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform Push {
    vec2 invScreenSize;
} push;

vec3 lightPos = vec3(0, -4, 6);
vec3 lightColor = vec3(10,11, 4);
vec3 lightDir = vec3(0, 1, 0);

void main() {
    vec2 texCoords = gl_FragCoord.xy * push.invScreenSize;
    vec3 normal = texture(texNormals, texCoords).xyz * 2.0f - 1.0f;
    if (dot(normal, normal) < 0.01f) return;
    normal = normalize(normal);
    vec3 position = texture(texPos, texCoords).xyz;
    vec3 albedo = texture(texAlbedo, texCoords).xyz;

    vec3 toLight = lightPos - position;
    float toLightDis2 = dot(toLight, toLight);
    toLight *= inversesqrt(toLightDis2);
    float spotFactor = max(0.0f, pow(dot(-lightDir, toLight), 20));
    vec3 light = lightColor * max(0.0f, dot(normal, toLight)) * spotFactor / toLightDis2;
    vec3 ambient = vec3(0.1f);
    outColor = vec4(albedo * (light + ambient), 1.0f);
}
