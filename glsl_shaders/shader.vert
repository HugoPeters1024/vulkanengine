#version 460

layout(location = 0) in vec3 position;

vec2 positions[3] = vec2[](
    vec2(0.0f, -0.5f),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

layout (push_constant) uniform Lol {
    mat4 transform;
} lol;

void main() {
    gl_Position = lol.transform * vec4(position, 1.0f);
}