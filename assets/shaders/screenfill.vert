#version 460

vec2 positions[3] = vec2[](
    vec2(-1.0f, -1.0f),
    vec2(3.0f, -1.0f),
    vec2(-1.0f, 3.0f)
);

layout(location = 0) out vec2 uv;

void main() {
    uv = positions[gl_VertexIndex];
    gl_Position = vec4(uv, 0.0f, 1.0f);
    uv = (uv + 1.0f) * 0.5f;
}
