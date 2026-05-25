#ifndef BEAM_RENDERER_H
#define BEAM_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "shader.h"

class BeamRenderer {
public:
    void InitTargetGeometry();
    void RenderTargets(
        Shader& shader,
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::mat4& target1ModelMatrix,
        const glm::mat4& target2ModelMatrix,
        bool target1Activated,
        bool target2Activated
    );
    void Destroy();

private:
    GLuint targetVAO = 0;
    GLuint targetVBO = 0;
    int targetVertexCount = 0;
};

#endif