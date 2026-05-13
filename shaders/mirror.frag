#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D mirrorTexture;
uniform bool reflectiveBackFace;

void main() {
    if (!gl_FrontFacing && !reflectiveBackFace) {
        FragColor = vec4(vec3(0.45), 1.0);
        return;
    }

    vec3 reflected = texture(mirrorTexture, TexCoords).rgb;
    vec3 tint = vec3(0.97, 0.98, 1.0);
    FragColor = vec4(reflected * tint, 1.0);
}
