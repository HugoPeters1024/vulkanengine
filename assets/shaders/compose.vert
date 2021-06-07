#version 460



void main() {
    gl_Position = lol.camera * lol.mvp * vec4(vPosition, 1.0f);
    uv = vUv;
    normal = normalize((lol.mvp * vec4(vNormal, 0.0f)).xyz);
    position = (lol.mvp * vec4(vPosition, 1)).xyz;
}
