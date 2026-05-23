#include "ShadowMap.h"
#include "shader.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

ShadowMap::ShadowMap()
    : mapSize(0),
      fbo(0),
      dynamicTexture(0),
      dynamicLightSpaceMatrix(1.0f) {}

ShadowMap::~ShadowMap() {
    if (dynamicTexture != 0) {
        glDeleteTextures(1, &dynamicTexture);
    }
    if (fbo != 0) {
        glDeleteFramebuffers(1, &fbo);
    }
}

void ShadowMap::init(int shadowMapSize) {
    mapSize = shadowMapSize;

    glGenFramebuffers(1, &fbo);

    glGenTextures(1, &dynamicTexture);
    glBindTexture(GL_TEXTURE_2D, dynamicTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, mapSize, mapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ShadowMap::render(
    const RenderParams& params,
    Shader* depthShader,
    const CasterDrawFn& drawCasters
) {
    if (depthShader == nullptr || fbo == 0) {
        return;
    }

    const GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
    GLint previousCullFace = GL_BACK;
    glGetIntegerv(GL_CULL_FACE_MODE, &previousCullFace);

    const GLboolean polyOffsetWasEnabled = glIsEnabled(GL_POLYGON_OFFSET_FILL);
    GLfloat previousPolyOffsetFactor = 0.0f;
    GLfloat previousPolyOffsetUnits = 0.0f;
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &previousPolyOffsetFactor);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &previousPolyOffsetUnits);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 4.0f);

    GLint previousViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    if (params.dynamicActive) {
        glm::vec3 forward = params.lightDirection;
        if (glm::length(forward) < 0.0001f) {
            forward = glm::vec3(0.0f, -0.2f, -1.0f);
        }
        forward = glm::normalize(forward);

        auto snapToMillimeter = [](float v) { return std::round(v * 1000.0f) / 1000.0f; };
        const glm::vec3 shadowEye(
            snapToMillimeter(params.lightPosition.x),
            snapToMillimeter(params.lightPosition.y),
            snapToMillimeter(params.lightPosition.z)
        );
        const glm::vec3 shadowTarget = shadowEye + forward * 8.0f;

        const glm::mat4 dynamicProjection = glm::perspective(glm::radians(72.0f), 1.0f, 0.15f, 32.0f);
        const glm::mat4 dynamicView = glm::lookAt(shadowEye, shadowTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 viewProj = dynamicProjection * dynamicView;

        const glm::vec4 anchorClip = viewProj * glm::vec4(shadowTarget, 1.0f);
        if (std::abs(anchorClip.w) > 1.0e-5f) {
            const glm::vec2 anchorNDC(anchorClip.x / anchorClip.w, anchorClip.y / anchorClip.w);
            const float halfMap = static_cast<float>(mapSize) * 0.5f;
            const glm::vec2 anchorTexel = anchorNDC * halfMap;
            const glm::vec2 rounded(std::round(anchorTexel.x), std::round(anchorTexel.y));
            const glm::vec2 deltaNDC = (rounded - anchorTexel) / halfMap;
            glm::mat4 snap(1.0f);
            snap[3][0] = deltaNDC.x;
            snap[3][1] = deltaNDC.y;
            viewProj = snap * viewProj;
        }
        dynamicLightSpaceMatrix = viewProj;

        depthShader->use();
        depthShader->setMat4("lightSpaceMatrix", dynamicLightSpaceMatrix);
        glViewport(0, 0, mapSize, mapSize);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dynamicTexture, 0);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glPolygonOffset(1.5f, 4.0f);

        drawCasters(depthShader, true);
    } else {
        dynamicLightSpaceMatrix = glm::mat4(1.0f);
    }

    glPolygonOffset(previousPolyOffsetFactor, previousPolyOffsetUnits);
    if (!polyOffsetWasEnabled) {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    glCullFace(previousCullFace);
    if (!cullWasEnabled) {
        glDisable(GL_CULL_FACE);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(
        previousViewport[0],
        previousViewport[1],
        previousViewport[2],
        previousViewport[3]
    );
}
