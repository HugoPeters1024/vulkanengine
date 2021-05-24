#version 460

layout(location = 0) in vec3 normal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(vec3(1,0,2));
    outColor = vec4(1.0f) * max(0.0f, dot(normal, -lightDir));
}