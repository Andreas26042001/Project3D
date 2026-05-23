#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int useClipPlane;
uniform vec4 clipPlane;
uniform int useEdgeClipPlanes;
uniform vec4 edgeClipPlanes[4];

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_ClipDistance[0] = useClipPlane != 0 ? dot(worldPos, clipPlane) : 1.0;
    gl_ClipDistance[1] = useEdgeClipPlanes != 0 ? dot(worldPos, edgeClipPlanes[0]) : 1.0;
    gl_ClipDistance[2] = useEdgeClipPlanes != 0 ? dot(worldPos, edgeClipPlanes[1]) : 1.0;
    gl_ClipDistance[3] = useEdgeClipPlanes != 0 ? dot(worldPos, edgeClipPlanes[2]) : 1.0;
    gl_ClipDistance[4] = useEdgeClipPlanes != 0 ? dot(worldPos, edgeClipPlanes[3]) : 1.0;
    gl_Position = projection * view * worldPos;
}
