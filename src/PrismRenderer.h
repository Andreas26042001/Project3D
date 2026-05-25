#ifndef PRISM_RENDERER_H
#define PRISM_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "shader.h"
#include "Texture.h"

class PrismRenderer {
public:
    void InitGeometry(float radius);

    void Render(
        Shader& shader,
        const Texture& cubemapTexture,
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPosition,
        const glm::vec3& prismCenter
    );

    void Destroy();

private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    int vertexCount = 0;
};

#endif