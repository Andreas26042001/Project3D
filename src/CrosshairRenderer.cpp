#include "CrosshairRenderer.h"

#include <algorithm>
#include <glm/glm.hpp>

void CrosshairRenderer::Init() {
    const float crosshairSize = 0.018f;
    const float crosshairGap = 0.008f;

    const float crosshairVertices[] = {
        -crosshairSize, 0.0f,
        -crosshairGap, 0.0f,
         crosshairGap, 0.0f,
         crosshairSize, 0.0f,
         0.0f, -crosshairSize,
         0.0f, -crosshairGap,
         0.0f,  crosshairGap,
         0.0f,  crosshairSize
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(crosshairVertices),
        crosshairVertices,
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        (void*)0
    );

    glBindVertexArray(0);
}

void CrosshairRenderer::Render(Shader& shader, int viewportWidth, int viewportHeight) {
    if (!visible) {
        return;
    }

    glDisable(GL_DEPTH_TEST);

    shader.use();

    const float aspect =
        static_cast<float>(viewportWidth) /
        static_cast<float>(std::max(1, viewportHeight));

    shader.setFloat("aspect", aspect);
    shader.setVec3("crosshairColor", glm::vec3(1.0f, 1.0f, 0.2f));

    glBindVertexArray(VAO);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 8);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

void CrosshairRenderer::Destroy() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }

    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
}

void CrosshairRenderer::Toggle() {
    visible = !visible;
}

bool CrosshairRenderer::IsVisible() const {
    return visible;
}