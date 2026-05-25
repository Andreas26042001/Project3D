#include "PrismRenderer.h"

#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/geometric.hpp>

void PrismRenderer::InitGeometry(float radius) {
    const float r = radius;

    const glm::vec3 v[6] = {
        glm::vec3(+r, 0.0f, 0.0f),
        glm::vec3(-r, 0.0f, 0.0f),
        glm::vec3(0.0f, +r, 0.0f),
        glm::vec3(0.0f, -r, 0.0f),
        glm::vec3(0.0f, 0.0f, +r),
        glm::vec3(0.0f, 0.0f, -r)
    };

    const int faces[8][3] = {
        {0, 2, 4}, {0, 4, 3}, {0, 3, 5}, {0, 5, 2},
        {1, 4, 2}, {1, 3, 4}, {1, 5, 3}, {1, 2, 5}
    };

    std::vector<float> data;
    data.reserve(8 * 3 * 6);

    for (int f = 0; f < 8; ++f) {
        const glm::vec3 p0 = v[faces[f][0]];
        const glm::vec3 p1 = v[faces[f][1]];
        const glm::vec3 p2 = v[faces[f][2]];

        const glm::vec3 faceNormal =
            glm::normalize(glm::cross(p1 - p0, p2 - p0));

        for (int k = 0; k < 3; ++k) {
            const glm::vec3& position = v[faces[f][k]];

            data.push_back(position.x);
            data.push_back(position.y);
            data.push_back(position.z);

            data.push_back(faceNormal.x);
            data.push_back(faceNormal.y);
            data.push_back(faceNormal.z);
        }
    }

    vertexCount = static_cast<int>(data.size() / 6);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        data.size() * sizeof(float),
        data.data(),
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        reinterpret_cast<void*>(0)
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        reinterpret_cast<void*>(3 * sizeof(float))
    );

    glBindVertexArray(0);
}

void PrismRenderer::Render(
    Shader& shader,
    const Texture& cubemapTexture,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPosition,
    const glm::vec3& prismCenter
) {
    if (VAO == 0 || vertexCount <= 0) {
        return;
    }

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, prismCenter);

    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setMat4("model", model);
    shader.setVec3("u_view_pos", cameraPosition);
    shader.setFloat("refractionIndice", 1.52f);

    glActiveTexture(GL_TEXTURE6);
    cubemapTexture.bind(6);
    shader.setInt("cubemapSampler", 6);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glDisable(GL_BLEND);
}

void PrismRenderer::Destroy() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }

    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }

    vertexCount = 0;
}