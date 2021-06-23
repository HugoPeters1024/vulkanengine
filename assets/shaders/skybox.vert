#version 460

layout(location = 0) in vec3 vPosition;

layout(location = 0) out vec3 uv;

layout (push_constant) uniform Lol {
    mat4 camera;
} push;

void main() {
    uv = vPosition;
    vec4 position = push.camera * vec4(vPosition, 0.0f);
    gl_Position = position.xyww;
}