#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

layout(binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 outColor;

vec3 lightPos = vec3(0, -2, 6);
vec3 lightColor = vec3(10,11, 4);

void main() {
    vec3 color = texture(tex, uv).xyz;
    vec3 toLight = lightPos - position;
    float toLightDis2 = dot(toLight, toLight);
    toLight *= inversesqrt(toLightDis2);
    outColor = vec4(color * lightColor * max(0.0f, dot(normal, toLight)) / toLightDis2, 1.0f);
}