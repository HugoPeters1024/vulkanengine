#version 460

layout(binding = 0) uniform sampler2D texNormals;
layout(binding = 1) uniform sampler2D texPos;
layout(binding = 2) uniform sampler2D texAlbedo;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform Push {
    vec3 camPos;
} push;

vec3 lightPos = vec3(5, -4, 6);
vec3 lightColor = vec3(17,17, 15);

void main() {
    vec3 normal = texture(texNormals, uv).xyz * 2.0f - 1.0f;
    if (dot(normal, normal) < 0.01f) return;
    normal = normalize(normal);
    vec3 position = texture(texPos, uv).xyz;
    vec3 albedo = texture(texAlbedo, uv).xyz;

    vec3 toLight = lightPos - position;
    float toLightDis2 = dot(toLight, toLight);
    toLight *= inversesqrt(toLightDis2);
    float diffuse = max(0.0f, dot(normal, toLight)) / toLightDis2;

    vec3 E = normalize(push.camPos - position);
    vec3 R = reflect(-E, normal);
    float specular = 0.2f * pow(max(0.0f, dot(E, R)), 100);


    vec3 ambient = vec3(0.1f);
    outColor = vec4(albedo * (lightColor * (diffuse + specular) + ambient), 1.0f);
}
