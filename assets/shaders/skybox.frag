#version 460

layout(location = 0) in vec3 uv;

layout(binding = 0) uniform samplerCube skybox;

layout(location = 0) out vec4 color;


void main() {
    color = texture(skybox, uv);
}
