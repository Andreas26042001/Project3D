#version 330 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec4 center;
layout (location = 2) in vec4 col;

uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform mat4 view;
uniform mat4 projection;

out vec4 color;
out vec2 quadUV;

void main() {
    float scale = center.w;
    vec3 particleCenter = center.xyz;
    vec3 worldPos = particleCenter
        + cameraRight * vertex.x * scale
        + cameraUp * vertex.y * scale;

    gl_Position = projection * view * vec4(worldPos, 1.0);
    color = col;
    quadUV = vertex.xy * 0.5 + 0.5;
}
