#version 460

layout(binding = 0) uniform sampler2D texNormals;

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform Push {
    vec2 invScreenSize;
} push;

void main() {
    vec3 normal = texture(texNormals, gl_FragCoord.xy * push.invScreenSize).xyz;
    outColor = vec4(abs(normal), 0);
}
