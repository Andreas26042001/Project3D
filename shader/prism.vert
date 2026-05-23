#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 v_frag_coord;
out vec3 v_normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 frag_coord = model * vec4(aPos, 1.0);
    gl_Position = projection * view * frag_coord;
    v_normal = mat3(transpose(inverse(model))) * aNormal;
    v_frag_coord = frag_coord.xyz;
}
