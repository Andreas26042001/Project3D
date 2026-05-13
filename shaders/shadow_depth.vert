#version 330 core
// Depth-only vertex shader used to populate each light's shadow map.
// Real-Time Rendering 4e §7.4 "Shadow Maps" p. 234: the scene is rendered from the
// position of the light with only z-buffering active (lighting/texturing/color writes
// are disabled by the framebuffer setup). The constant + slope-scale bias described at
// p. 236-237 is applied via glPolygonOffset on the C++ side.
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main() {
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
