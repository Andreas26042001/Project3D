#version 330 core
in vec4 color;
in vec2 quadUV;
out vec4 FragColor;

void main() {
    float dist = length(quadUV - vec2(0.5));
    float alpha = color.a * (1.0 - smoothstep(0.35, 0.5, dist));
    FragColor = vec4(color.rgb, alpha);
}
