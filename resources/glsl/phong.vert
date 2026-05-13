#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace[4];
out vec4 FragPosDynamicLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool useClipPlane;
uniform vec4 clipPlane;
uniform bool useEdgeClipPlanes;
uniform vec4 edgeClipPlanes[4];
uniform mat4 lightSpaceMatrices[4];
uniform mat4 dynamicLightSpaceMatrix;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    for (int i = 0; i < 4; ++i) {
        FragPosLightSpace[i] = lightSpaceMatrices[i] * worldPos;
    }
    FragPosDynamicLightSpace = dynamicLightSpaceMatrix * worldPos;
    gl_ClipDistance[0] = useClipPlane ? dot(worldPos, clipPlane) : 1.0;
    gl_ClipDistance[1] = useEdgeClipPlanes ? dot(worldPos, edgeClipPlanes[0]) : 1.0;
    gl_ClipDistance[2] = useEdgeClipPlanes ? dot(worldPos, edgeClipPlanes[1]) : 1.0;
    gl_ClipDistance[3] = useEdgeClipPlanes ? dot(worldPos, edgeClipPlanes[2]) : 1.0;
    gl_ClipDistance[4] = useEdgeClipPlanes ? dot(worldPos, edgeClipPlanes[3]) : 1.0;

    gl_Position = projection * view * worldPos;
}