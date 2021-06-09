#version 460

layout(binding = 0) uniform sampler2D texInput;

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform Push {
    vec2 invScreenSize;
} push;

void main() {
    vec2 texCoords = gl_FragCoord.xy * push.invScreenSize;
    outColor = texture(texInput, texCoords);
}
