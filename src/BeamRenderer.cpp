#include "BeamRenderer.h"
#include <cstdlib>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <vector>

void BeamRenderer::InitTargetGeometry() {
    const int N = 48;
    const float kPointRadius = 0.11f;
    const float kRingInner = 0.24f;
    const float kRingOuter = 0.30f;

    std::vector<float> verts;
    verts.reserve(static_cast<size_t>(N) * (3 + 6) * 3);

    const float kTwoPi = 6.28318530718f;

    for (int i = 0; i < N; ++i) {
        const float a0 = kTwoPi * static_cast<float>(i) / static_cast<float>(N);
        const float a1 = kTwoPi * static_cast<float>(i + 1) / static_cast<float>(N);

        const float c0 = std::cos(a0);
        const float s0 = std::sin(a0);
        const float c1 = std::cos(a1);
        const float s1 = std::sin(a1);

        verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(0.0f);
        verts.push_back(0.0f); verts.push_back(kPointRadius * c0); verts.push_back(kPointRadius * s0);
        verts.push_back(0.0f); verts.push_back(kPointRadius * c1); verts.push_back(kPointRadius * s1);

        const float pIn0[3] = {0.0f, kRingInner * c0, kRingInner * s0};
        const float pOut0[3] = {0.0f, kRingOuter * c0, kRingOuter * s0};
        const float pOut1[3] = {0.0f, kRingOuter * c1, kRingOuter * s1};
        const float pIn1[3] = {0.0f, kRingInner * c1, kRingInner * s1};

        for (int k = 0; k < 3; ++k) verts.push_back(pIn0[k]);
        for (int k = 0; k < 3; ++k) verts.push_back(pOut0[k]);
        for (int k = 0; k < 3; ++k) verts.push_back(pOut1[k]);

        for (int k = 0; k < 3; ++k) verts.push_back(pIn0[k]);
        for (int k = 0; k < 3; ++k) verts.push_back(pOut1[k]);
        for (int k = 0; k < 3; ++k) verts.push_back(pIn1[k]);
    }

    targetVertexCount = static_cast<int>(verts.size() / 3);

    glGenVertexArrays(1, &targetVAO);
    glGenBuffers(1, &targetVBO);

    glBindVertexArray(targetVAO);
    glBindBuffer(GL_ARRAY_BUFFER, targetVBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        verts.size() * sizeof(float),
        verts.data(),
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        reinterpret_cast<void*>(0)
    );

    glBindVertexArray(0);
}

void BeamRenderer::RenderTargets(
    Shader& shader,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::mat4& target1ModelMatrix,
    const glm::mat4& target2ModelMatrix,
    bool target1Activated,
    bool target2Activated
) {
    if (targetVAO == 0 || targetVertexCount <= 0) {
        return;
    }

    const glm::vec3 colorIdle(0.08f, 0.14f, 0.22f);
    const glm::vec3 colorActive(0.30f, 0.60f, 1.00f);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);

    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    glBindVertexArray(targetVAO);

    shader.setVec3("lightColor", target1Activated ? colorActive : colorIdle);
    shader.setMat4("model", target1ModelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, targetVertexCount);

    shader.setVec3("lightColor", target2Activated ? colorActive : colorIdle);
    shader.setMat4("model", target2ModelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, targetVertexCount);

    glBindVertexArray(0);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void BeamRenderer::Destroy() {
    if (targetVAO != 0) {
        glDeleteVertexArrays(1, &targetVAO);
        targetVAO = 0;
    }

    if (targetVBO != 0) {
        glDeleteBuffers(1, &targetVBO);
        targetVBO = 0;
    }

    targetVertexCount = 0;
}

void BeamRenderer::RenderBeam(
    Shader& shader,
    Object& beamMesh,
    const glm::mat4& view,
    const glm::mat4& projection,
    const BeamTrace& beam,
    const glm::vec3& beamColor
) {
    if (beam.segmentCount <= 0) {
        return;
    }

    const float kBeamHalfWidth = 0.035f;

    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setVec3("lightColor", beamColor);

    for (int i = 0; i < beam.segmentCount; ++i) {
        if (beam.lengths[i] <= 0.001f) {
            continue;
        }

        const glm::vec3 beamCenter = (beam.starts[i] + beam.ends[i]) * 0.5f;
        const glm::vec3 dir = beam.ends[i] - beam.starts[i];

        const float lenX = std::abs(dir.x) > 0.0001f
            ? std::abs(dir.x)
            : kBeamHalfWidth * 2.0f;

        const float lenY = std::abs(dir.y) > 0.0001f
            ? std::abs(dir.y)
            : kBeamHalfWidth * 2.0f;

        const float lenZ = std::abs(dir.z) > 0.0001f
            ? std::abs(dir.z)
            : kBeamHalfWidth * 2.0f;

        glm::mat4 beamModel = glm::mat4(1.0f);
        beamModel = glm::translate(beamModel, beamCenter);
        beamModel = glm::scale(beamModel, glm::vec3(lenX, lenY, lenZ));

        shader.setMat4("model", beamModel);
        beamMesh.draw();
    }
}