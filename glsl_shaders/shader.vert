#version 460

layout(location = 0) in vec3 position;

layout (push_constant) uniform Lol {
    mat4 camera;
    mat4 mvp;
} lol;

void main() {
    gl_Position = lol.camera * lol.mvp * vec4(position, 1.0f);
}