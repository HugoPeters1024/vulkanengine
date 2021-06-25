#version 460

layout(location = 0) in vec3 vPosition;

layout (push_constant) uniform Lol {
    mat4 camera;
    mat4 mvp;
} lol;

void main() {
    vec4 worldPos = lol.mvp * vec4(vPosition, 1.0f);
    gl_Position = lol.camera * worldPos;
}
