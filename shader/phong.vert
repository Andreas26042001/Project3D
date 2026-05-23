#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;
out vec4 FragPosDynamicLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform mat4 dynamicLightSpaceMatrix;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    // Coordonnees homogenes pour l'echantillonnage de la shadow map (RTR4 §7.4 p. 234-235).
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    FragPosDynamicLightSpace = dynamicLightSpaceMatrix * worldPos;
    gl_Position = projection * view * worldPos;
}
