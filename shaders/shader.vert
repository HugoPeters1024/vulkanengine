#version 460

layout(location = 0) in vec3 position;

vec2 positions[3] = vec2[](
    vec2(0.0f, -0.5f),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

layout (push_constant) uniform Lol {
    vec2 offset;
    vec3 color;
} lol;

void main() {
    //gl_Position = vec4(positions[gl_VertexIndex] + lol.offset, 0.0f, 1.0f);
    gl_Position = vec4(position + vec3(lol.offset,0), 1.0f);
}