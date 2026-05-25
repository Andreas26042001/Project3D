#include "Game.h"
#include "GameInternal.h"
#include "Player.h"
#include "Collider.h"
#include "shader.h"
#include "camera.h"
#include "object.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

namespace {

// RTR4 §7.1.1 Eq. 7.4: project a point from a point light onto plane pi:
// plane·x = 0 with plane = (nx, ny, nz, d).
glm::mat4 buildPlanarShadowMatrix(const glm::vec4& lightPos, const glm::vec4& plane) {
    const float dot = glm::dot(plane, lightPos);
    return glm::mat4(
        dot - lightPos.x * plane.x, -lightPos.y * plane.x, -lightPos.z * plane.x, -lightPos.w * plane.x,
        -lightPos.x * plane.y, dot - lightPos.y * plane.y, -lightPos.z * plane.y, -lightPos.w * plane.y,
        -lightPos.x * plane.z, -lightPos.y * plane.z, dot - lightPos.z * plane.z, -lightPos.w * plane.z,
        -lightPos.x * plane.w, -lightPos.y * plane.w, -lightPos.z * plane.w, dot - lightPos.w * plane.w
    );
}

bool isCeilingShadowCaster(const glm::mat4& modelMatrix) {
    return std::abs(modelMatrix[1][1]) >= 1.0f;
}

// §7.1.1: only pillars (narrow footprint) cast a shadow on the ground.
bool isPlanarShadowCaster(const glm::mat4& modelMatrix) {
    if (!isCeilingShadowCaster(modelMatrix)) {
        return false;
    }
    const float sx = std::abs(modelMatrix[0][0]);
    const float sz = std::abs(modelMatrix[2][2]);
    return sx <= 1.0f && sz <= 1.0f;
}

float groundTopFromModel(const glm::mat4& groundModel) {
    return groundModel[3][1] + std::abs(groundModel[1][1]) * 0.5f;
}

void disableLampEdgeClip(Shader* lampShader) {
    if (!lampShader) {
        return;
    }
    lampShader->setInt("useEdgeClipPlanes", 0);
}

} // namespace

void Game::drawMovablePillarPhong(const glm::mat4& pillarModelMatrix) {
    if (!capturePillarMesh || !phongShader) {
        return;
    }

    phongShader->setMat4("model", pillarModelMatrix);
    if (capturePillarMetalTexture.isValid()) {
        phongShader->setVec2("uvScale", glm::vec2(1.8f, 1.8f));
        capturePillarMetalTexture.bind(0);
        phongShader->setInt("useTexture", 1);
    } else if (pillarDiffuseTexture.isValid()) {
        phongShader->setInt("useTexture", 1);
        phongShader->setVec2("uvScale", glm::vec2(1.75f, 5.5f));
        pillarDiffuseTexture.bind(0);
    } else {
        phongShader->setInt("useTexture", 0);
    }
    capturePillarMesh->draw();
    if (capturePillarMetalTexture.isValid() || pillarDiffuseTexture.isValid()) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    phongShader->setInt("useTexture", 0);
    phongShader->setVec2("uvScale", glm::vec2(1.0f, 1.0f));
}

void Game::drawMovablePillarShadows() {
    // Depth pass §7.4: only objects that cast shadows (casters).
    if (!capturePillarMesh || !shadowDepthShader) {
        return;
    }

    shadowDepthShader->setMat4("model", capturePillarModelMatrix);
    capturePillarMesh->draw();
    shadowDepthShader->setMat4("model", deflectorPillarModelMatrix);
    capturePillarMesh->draw();
}

void Game::renderPlanarShadows(const glm::mat4& view, const glm::mat4& projection) {
    // RTR4 §7.1.1: geometric projection; stencil limits the receiver to the marked ground.
    if (objects.empty() || !lampShader || !groundObject) {
        return;
    }

    const glm::mat4& groundModel = groundObject->model;
    const float groundTop = groundTopFromModel(groundModel);
    const float groundHalfX = std::abs(groundModel[0][0]) * 0.5f;
    const float groundHalfZ = std::abs(groundModel[2][2]) * 0.5f;
    const glm::vec4 groundPlane(0.0f, 1.0f, 0.0f, -groundTop);
    const glm::mat4 shadowMatrix = buildPlanarShadowMatrix(
        glm::vec4(ceilingLightPosition, 1.0f),
        groundPlane
    );
    const std::array<glm::vec4, 4> groundClipPlanes = {
        glm::vec4( 1.0f, 0.0f, 0.0f, groundHalfX),
        glm::vec4(-1.0f, 0.0f, 0.0f, groundHalfX),
        glm::vec4( 0.0f, 0.0f, 1.0f, groundHalfZ),
        glm::vec4( 0.0f, 0.0f,-1.0f, groundHalfZ),
    };

    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLint previousBlendSrc = GL_ONE;
    GLint previousBlendDst = GL_ZERO;
    glGetIntegerv(GL_BLEND_SRC_RGB, &previousBlendSrc);
    glGetIntegerv(GL_BLEND_DST_RGB, &previousBlendDst);

    GLboolean depthMaskEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskEnabled);
    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLint previousDepthFunc = GL_LESS;
    glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);

    const GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
    const GLboolean polyOffsetWasEnabled = glIsEnabled(GL_POLYGON_OFFSET_FILL);
    GLfloat previousPolyOffsetFactor = 0.0f;
    GLfloat previousPolyOffsetUnits = 0.0f;
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &previousPolyOffsetFactor);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &previousPolyOffsetUnits);

    GLboolean clipEnabled[4] = {GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE};
    for (int i = 0; i < 4; ++i) {
        clipEnabled[i] = glIsEnabled(static_cast<GLenum>(GL_CLIP_DISTANCE1 + i));
        glEnable(GL_CLIP_DISTANCE1 + i);
    }

    const GLboolean stencilWasEnabled = glIsEnabled(GL_STENCIL_TEST);
    GLint previousStencilFunc = GL_EQUAL;
    GLint previousStencilRef = 0;
    GLint previousStencilMask = 0xFF;
    glGetIntegerv(GL_STENCIL_FUNC, &previousStencilFunc);
    glGetIntegerv(GL_STENCIL_REF, &previousStencilRef);
    glGetIntegerv(GL_STENCIL_VALUE_MASK, &previousStencilMask);

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0x00);
    glStencilFunc(GL_EQUAL, 1, 0xFF);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    // Depth test: shadow is at ground level; pillars (already rendered) occlude it.
    // glPolygonOffset avoids z-fighting with the ground (RTR4 §7.4 p. 236-237).
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -4.0f);
    glDisable(GL_CULL_FACE);

    lampShader->use();
    lampShader->setMat4("view", view);
    lampShader->setMat4("projection", projection);
    lampShader->setInt("useEdgeClipPlanes", 1);
    for (int i = 0; i < 4; ++i) {
        lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", groundClipPlanes[i]);
    }
    lampShader->setVec3("lightColor", glm::vec3(0.42f, 0.42f, 0.42f));

    auto drawProjectedCaster = [&](const glm::mat4& casterModel, Object& mesh) {
        lampShader->setMat4("model", shadowMatrix * casterModel);
        mesh.draw();
    };

    for (size_t ci = 0; ci < extraCubeModels.size(); ++ci) {
        if (!isPlanarShadowCaster(extraCubeModels[ci])) {
            continue;
        }
        drawProjectedCaster(extraCubeModels[ci], *objects[0]);
    }
    if (capturePillarMesh != nullptr) {
        drawProjectedCaster(capturePillarModelMatrix, *capturePillarMesh);
        drawProjectedCaster(deflectorPillarModelMatrix, *capturePillarMesh);
    }

    for (int i = 0; i < 4; ++i) {
        if (!clipEnabled[i]) {
            glDisable(GL_CLIP_DISTANCE1 + i);
        }
    }
    if (!stencilWasEnabled) {
        glDisable(GL_STENCIL_TEST);
    } else {
        glStencilFunc(previousStencilFunc, previousStencilRef, previousStencilMask);
    }
    glPolygonOffset(previousPolyOffsetFactor, previousPolyOffsetUnits);
    if (!polyOffsetWasEnabled) {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    if (cullWasEnabled) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }
    glDepthMask(depthMaskEnabled);
    glDepthFunc(previousDepthFunc);
    if (depthTestWasEnabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glBlendFunc(previousBlendSrc, previousBlendDst);
    if (!blendWasEnabled) {
        glDisable(GL_BLEND);
    }
}

Game::BeamTrace Game::traceAnchoredBeam(float maxDistance) const {
    BeamTrace result;
    const glm::vec3 beamOrigin = lightPosition;

    glm::vec3 hit(0.0f);
    float dist = 0.0f;
    const bool hitSomething = raycastScene(
        beamOrigin, beamDirection, maxDistance,
        centerPillarColliderIndex, hit, dist
    );
    if (!hitSomething) {
        dist = maxDistance;
        hit = beamOrigin + beamDirection * maxDistance;
    }

    float prismHitDist = 0.0f;
    const bool prismHit = ColliderWorld::raySphereIntersect(
        beamOrigin, beamDirection, prismCenter,
        prismRadius, dist, prismHitDist
    );

    if (prismHit) {
        result.starts[0] = beamOrigin;
        result.ends[0] = beamOrigin + beamDirection * prismHitDist;
        result.lengths[0] = prismHitDist;

        glm::vec3 dHit(0.0f);
        float dDist = 0.0f;
        const bool dGotHit = raycastScene(
            prismCenter, prismDeflectDirection, maxDistance,
            centerPillarColliderIndex, dHit, dDist
        );
        if (!dGotHit) {
            dDist = maxDistance;
            dHit = prismCenter + prismDeflectDirection * maxDistance;
        }
        result.starts[1] = prismCenter;
        result.ends[1] = dHit;
        result.lengths[1] = dDist;
        result.segmentCount = 2;
    } else {
        result.starts[0] = beamOrigin;
        result.ends[0] = hit;
        result.lengths[0] = dist;
        result.segmentCount = 1;
    }

    return result;
}

void Game::renderShadowMap() {
    if (objects.empty() || shadowDepthShader == nullptr) {
        return;
    }

    const bool dynamicActive = lightProjectileActive || lightAnchoredOnPillar;
    ShadowMap::RenderParams params;
    params.dynamicActive = dynamicActive;
    if (dynamicActive) {
        params.lightPosition = lightPosition;
        params.lightDirection = lightProjectileDirection;
    }

    shadowMap.render(params, shadowDepthShader.get(), [this](Shader* depthShader, bool ceilingCastersOnly) {
        (void)depthShader;
        if (ceilingCastersOnly) {
            for (size_t ci = 0; ci < extraCubeModels.size(); ++ci) {
                if (!isCeilingShadowCaster(extraCubeModels[ci])) {
                    continue;
                }
                shadowDepthShader->setMat4("model", extraCubeModels[ci]);
                objects[0]->draw();
            }
            drawMovablePillarShadows();
        }
    });
}

void Game::Render(const Player& player) {
    const Camera& camera = player.camera;
    syncCarriedLight(camera.Position, camera.Front);
    renderShadowMap();
    const float aspect = static_cast<float>(viewportWidth) / static_cast<float>(std::max(1, viewportHeight));
    glm::mat4 projection = camera.GetProjectionMatrix(glm::radians(camera.Zoom), aspect);
    const glm::vec3 playerWorldPosition = player.getWorldPosition();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewportWidth, viewportHeight);
    glClearColor(0.015f, 0.015f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDisable(GL_CULL_FACE);

    glm::mat4 view = camera.GetViewMatrix();
    renderSceneOpaque(view, projection, camera.Position, playerWorldPosition);
    skybox.render(view, projection);
    explosionParticles.render(view, projection);
    renderDeflectorPrism(view, projection, camera.Position);
    renderCrosshair();
}

void Game::SetViewportSize(int width, int height) {
    viewportWidth = std::max(1, width);
    viewportHeight = std::max(1, height);
}

void Game::renderSceneOpaque(
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPosition,
    const glm::vec3& playerWorldPosition
) {
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    for (int i = 0; i < 4; ++i) {
        glDisable(static_cast<GLenum>(GL_CLIP_DISTANCE1 + i));
    }

    const bool dynamicLightActive = lightProjectileActive || lightAnchoredOnPillar;
    const bool beamActive = lightAnchoredOnPillar && !lightAnchorAnimating;
    const glm::vec3 projectileBlue(0.10f, 0.28f, 1.00f);
    const glm::vec3 dynamicLightColor = lightProjectileActive
        ? projectileBlue
        : glm::vec3(0.30f, 0.60f, 1.00f);
    const float dynamicLightStrength = lightProjectileActive
        ? 4.8f
        : (lightAnchoredOnPillar ? 1.0f : 1.6f);
    const glm::vec3 beamColor(0.30f, 0.60f, 1.00f);
    const float kBeamStrength = beamActive ? 1.0f : 1.8f;
    const float kMaxBeamDistance = 40.0f;

    int beamCount = 0;
    glm::vec3 beamStarts[2] = {beamSourcePos, beamSourcePos};
    glm::vec3 beamEnds[2] = {beamSourcePos, beamSourcePos};
    float beamLengths[2] = {0.0f, 0.0f};

    if (beamActive) {
        const BeamTrace beam = traceAnchoredBeam(kMaxBeamDistance);
        beamCount = beam.segmentCount;
        for (int i = 0; i < beamCount; ++i) {
            beamStarts[i] = beam.starts[i];
            beamEnds[i] = beam.ends[i];
            beamLengths[i] = beam.lengths[i];
        }
    }

    auto applyPhongLightingUniforms = [&]() {
        phongShader->setVec3("ceilingLightPos", ceilingLightPosition);
        phongShader->setVec3("ceilingLightColor", ceilingLightColor);
        phongShader->setFloat("ceilingLightStrength", ceilingLightStrength);
        // Point attenuation: ~25% intensity at ceilingLightRange meters (isotropic, distance only).
        const float rangeFactor = std::max(1.0f, ceilingLightRange);
        phongShader->setFloat("ceilingAttLinear", 1.0f / rangeFactor);
        phongShader->setFloat("ceilingAttQuadratic", 2.0f / (rangeFactor * rangeFactor));
        phongShader->setInt("dynamicLightActive", dynamicLightActive ? 1 : 0);
        if (dynamicLightActive) {
            phongShader->setVec3("dynamicLightPos", lightPosition);
        }
        phongShader->setVec3("dynamicLightColor", dynamicLightColor);
        phongShader->setFloat("dynamicLightStrength", dynamicLightActive ? dynamicLightStrength : 0.0f);
        phongShader->setInt("dynamicShadowActive", dynamicLightActive ? 1 : 0);
        phongShader->setInt("beamLightCount", beamCount);
        for (int i = 0; i < beamCount; ++i) {
            phongShader->setVec3("beamLightStarts[" + std::to_string(i) + "]", beamStarts[i]);
            phongShader->setVec3("beamLightEnds[" + std::to_string(i) + "]", beamEnds[i]);
            phongShader->setVec3("beamLightColors[" + std::to_string(i) + "]", beamColor);
            phongShader->setFloat("beamLightStrengths[" + std::to_string(i) + "]", kBeamStrength);
        }
    };

    if (!objects.empty()) {
        phongShader->use();
        phongShader->setMat4("view", view);
        phongShader->setMat4("projection", projection);
        phongShader->setMat4("dynamicLightSpaceMatrix", shadowMap.getDynamicLightSpaceMatrix());
        phongShader->setInt("dynamicShadowMap", 5);
        phongShader->setVec3("viewPos", cameraPosition);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, shadowMap.getDynamicTexture());
        glActiveTexture(GL_TEXTURE0);
        applyPhongLightingUniforms();
        phongShader->setInt("useTexture", 0);
        phongShader->setInt("diffuseMap", 0);
        phongShader->setVec2("uvScale", glm::vec2(1.0f, 1.0f));
        phongShader->setVec3("material.ambient", glm::vec3(0.10f, 0.10f, 0.10f));
        phongShader->setVec3("material.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));

        for (size_t ci = 0; ci < extraCubeModels.size(); ++ci) {
            const glm::mat4& cubeModel = extraCubeModels[ci];
            const float sy = std::abs(cubeModel[1][1]);
            if (pillarDiffuseTexture.isValid()) {
                const float sx = std::abs(cubeModel[0][0]);
                const float sz = std::abs(cubeModel[2][2]);

                phongShader->setInt("useTexture", 1);
                if (sy < 1.0f) {
                    // Ceiling: strong tiling on both axes to avoid giant stretched bricks.
                    phongShader->setVec2("uvScale", glm::vec2(12.0f, 12.0f));
                } else if (sx > 10.0f || sz > 10.0f) {
                    // Perimeter walls: repeat a lot on length, enough on height.
                    phongShader->setVec2("uvScale", glm::vec2(12.0f, 4.0f));
                } else {
                    // Pillars: tall but much thinner.
                    phongShader->setVec2("uvScale", glm::vec2(1.2f, 8.0f));
                }
                pillarDiffuseTexture.bind(0);
            }
            phongShader->setMat4("model", cubeModel);
            objects[0]->draw();
        }
        drawMovablePillarPhong(capturePillarModelMatrix);
        drawMovablePillarPhong(deflectorPillarModelMatrix);
        if (pillarDiffuseTexture.isValid()) {
            glBindTexture(GL_TEXTURE_2D, 0);
            phongShader->setInt("useTexture", 0);
        }
    }

    phongShader->use();
    phongShader->setMat4("model", groundObject->model);
    phongShader->setMat4("view", view);
    phongShader->setMat4("projection", projection);
    phongShader->setMat4("dynamicLightSpaceMatrix", shadowMap.getDynamicLightSpaceMatrix());
    phongShader->setInt("dynamicShadowMap", 5);
    phongShader->setVec3("viewPos", cameraPosition);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowMap.getDynamicTexture());
    glActiveTexture(GL_TEXTURE0);
    applyPhongLightingUniforms();
    phongShader->setInt("useTexture", groundDiffuseTexture.isValid() ? 1 : 0);
    phongShader->setInt("diffuseMap", 0);
    phongShader->setVec2("uvScale", glm::vec2(6.0f, 6.0f));
    if (groundDiffuseTexture.isValid()) {
        groundDiffuseTexture.bind(0);
    }
    phongShader->setVec3("material.ambient", glm::vec3(0.13f, 0.13f, 0.13f));
    phongShader->setVec3("material.diffuse", glm::vec3(0.19f, 0.19f, 0.19f));

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    groundObject->draw();
    glStencilMask(0x00);
    if (groundDiffuseTexture.isValid()) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    renderPlanarShadows(view, projection);
    glDisable(GL_STENCIL_TEST);

    {
        lampShader->use();
        lampShader->setMat4("view", view);
        lampShader->setMat4("projection", projection);
        disableLampEdgeClip(lampShader.get());
        const glm::vec3 lampColor = lightProjectileActive
            ? glm::vec3(0.06f, 0.18f, 2.0f)
            : dynamicLightColor;
        const float lampScale = lightProjectileActive ? 0.22f : 0.12f;
        lampShader->setVec3("lightColor", lampColor);

        glm::mat4 lightModel = glm::mat4(1.0f);
        lightModel = glm::translate(lightModel, lightPosition);
        lightModel = glm::scale(lightModel, glm::vec3(lampScale));
        lampShader->setMat4("model", lightModel);
        lightMarker->draw();
    }

    // Ground rail that guides the center pillar.
    {
        const glm::vec3 railIdle(0.10f, 0.18f, 0.32f);
        const glm::vec3 railActive(0.30f, 0.60f, 1.00f);
        const glm::vec3 railColor = beamActive ? railActive : railIdle;
        lampShader->use();
        lampShader->setMat4("view", view);
        lampShader->setMat4("projection", projection);
        disableLampEdgeClip(lampShader.get());
        lampShader->setVec3("lightColor", railColor);
        lampShader->setMat4("model", railModel);
        lightMarker->draw();
        // Deflector pillar rail, perpendicular to the previous one.
        lampShader->setMat4("model", deflectorRailModel);
        lightMarker->draw();
    }

    // East wall target: small dot + ring, dark at rest, blue after 3 seconds of
    // continuous beam contact. Must be rendered before the beam so the beam does
    // not visually cover it.
    renderBeamTarget(view, projection);

    if (beamCount > 0) {
        const float kBeamHalfWidth = 0.035f;
        lampShader->use();
        lampShader->setMat4("view", view);
        lampShader->setMat4("projection", projection);
        disableLampEdgeClip(lampShader.get());
        lampShader->setVec3("lightColor", beamColor);

        for (int i = 0; i < beamCount; ++i) {
            if (beamLengths[i] <= 0.001f) {
                continue;
            }
            const glm::vec3 beamCenter = (beamStarts[i] + beamEnds[i]) * 0.5f;
            const glm::vec3 dir = beamEnds[i] - beamStarts[i];
            const float lenX = std::abs(dir.x) > 0.0001f ? std::abs(dir.x) : kBeamHalfWidth * 2.0f;
            const float lenY = std::abs(dir.y) > 0.0001f ? std::abs(dir.y) : kBeamHalfWidth * 2.0f;
            const float lenZ = std::abs(dir.z) > 0.0001f ? std::abs(dir.z) : kBeamHalfWidth * 2.0f;
            glm::mat4 beamModel = glm::mat4(1.0f);
            beamModel = glm::translate(beamModel, beamCenter);
            beamModel = glm::scale(beamModel, glm::vec3(lenX, lenY, lenZ));
            lampShader->setMat4("model", beamModel);
            lightMarker->draw();
        }
    }

    // Yellow ceiling light marker.
    lampShader->use();
    lampShader->setMat4("view", view);
    lampShader->setMat4("projection", projection);
    disableLampEdgeClip(lampShader.get());
    lampShader->setVec3("lightColor", ceilingLightColor);
    glm::mat4 ceilingLightModel = glm::mat4(1.0f);
    ceilingLightModel = glm::translate(ceilingLightModel, ceilingLightPosition);
    ceilingLightModel = glm::scale(ceilingLightModel, glm::vec3(0.16f));
    lampShader->setMat4("model", ceilingLightModel);
    lightMarker->draw();

    // Character proxy: a small yellow cube following the camera from behind.
    lampShader->use();
    lampShader->setMat4("view", view);
    lampShader->setMat4("projection", projection);
    disableLampEdgeClip(lampShader.get());
    lampShader->setVec3("lightColor", glm::vec3(1.0f, 0.95f, 0.25f));
    glm::mat4 playerModel = glm::mat4(1.0f);
    playerModel = glm::translate(playerModel, playerWorldPosition);
    playerModel = glm::scale(playerModel, glm::vec3(0.18f));
    lampShader->setMat4("model", playerModel);
    lightMarker->draw();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

void Game::setupCrosshair() {
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

    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &crosshairVBO);
    glBindVertexArray(crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void Game::renderCrosshair() {
    if (!crosshairVisible) {
        return;
    }

    glDisable(GL_DEPTH_TEST);
    crosshairShader->use();
    const float aspect = static_cast<float>(viewportWidth) / static_cast<float>(std::max(1, viewportHeight));
    crosshairShader->setFloat("aspect", aspect);
    crosshairShader->setVec3("crosshairColor", glm::vec3(1.0f, 1.0f, 0.2f));
    glBindVertexArray(crosshairVAO);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 8);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void Game::ToggleCrosshair() {
    crosshairVisible = !crosshairVisible;
}

void Game::setupBeamTarget() {
    const float kBeamY = groundTopY + centerPillarHeight + kLightSourceOffsetY;
    const float kAvoidZFightingOffset = 0.012f;
    const float kTargetWallThickness = 0.6f;

    // Inner face of the small wall sections (center at mapHalfExtent - thickness/2).
    const float kWallInnerX = worldCollisionHalfExtent - kTargetWallThickness;
    const float kTarget1Z = 0.8f;
    targetPosition = glm::vec3(kWallInnerX - kAvoidZFightingOffset, kBeamY, kTarget1Z);
    target1ModelMatrix = glm::translate(glm::mat4(1.0f), targetPosition);

    const float kWallInnerZ = worldCollisionHalfExtent - kTargetWallThickness;
    const float kTarget2X = deflectorPillarBase.x - 1.0f;
    target2Position = glm::vec3(kTarget2X, kBeamY, kWallInnerZ - kAvoidZFightingOffset);
    target2ModelMatrix = glm::translate(glm::mat4(1.0f), target2Position);
    // Mesh is generated in the local YZ plane (normal +X): to align the target on the
    // south wall, apply a 90° rotation around Y.
    target2ModelMatrix = glm::rotate(target2ModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Solid center disk + ring, in the local YZ plane (X = 0). Coordinates are
    // transformed via the model uniform at render time.
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

        // Center disk: triangle (center, segment i, segment i+1).
        verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(0.0f);
        verts.push_back(0.0f); verts.push_back(kPointRadius * c0); verts.push_back(kPointRadius * s0);
        verts.push_back(0.0f); verts.push_back(kPointRadius * c1); verts.push_back(kPointRadius * s1);

        // Ring: two triangles forming a trapezoid between inner/outer radii.
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
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
    glBindVertexArray(0);
}

void Game::updateBeamTarget(float deltaTime) {
    const bool beamCurrentlyActive = lightAnchoredOnPillar && !lightAnchorAnimating;
    bool target1Hit = false;
    bool target2Hit = false;

    if (beamCurrentlyActive) {
        const float kMax = 60.0f;
        const BeamTrace beam = traceAnchoredBeam(kMax);
        const float beamLen = beam.lengths[0];

        if (beam.segmentCount >= 2) {
            const float deflectedBeamLen = beam.lengths[1];
            const glm::vec3 toTarget2 = target2Position - prismCenter;
            const float t2 = glm::dot(toTarget2, prismDeflectDirection);
            if (t2 >= 0.0f && t2 <= deflectedBeamLen + 0.05f) {
                const glm::vec3 closest = prismCenter + prismDeflectDirection * t2;
                if (glm::length(target2Position - closest) < targetHitTolerance) {
                    target2Hit = true;
                }
            }
        }

        const glm::vec3 toTarget = targetPosition - lightPosition;
        const float t = glm::dot(toTarget, beamDirection);
        if (t >= 0.0f && t <= beamLen + 0.05f) {
            const glm::vec3 closest = lightPosition + beamDirection * t;
            if (glm::length(targetPosition - closest) < targetHitTolerance) {
                target1Hit = true;
            }
        }
    }

    auto updateActivation = [&](bool isHit, float& timer, bool& activated) {
        if (isHit) {
            if (!activated) {
                timer += deltaTime;
                if (timer >= targetActivationDuration) {
                    activated = true;
                }
            }
        } else {
            activated = false;
            timer = 0.0f;
        }
    };

    updateActivation(target1Hit, targetActivationTimer, targetActivated);
    updateActivation(target2Hit, target2ActivationTimer, target2Activated);
}

void Game::renderBeamTarget(const glm::mat4& view, const glm::mat4& projection) {
    if (targetVAO == 0 || targetVertexCount <= 0) {
        return;
    }

    const glm::vec3 colorIdle(0.08f, 0.14f, 0.22f);
    const glm::vec3 colorActive(0.30f, 0.60f, 1.00f);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);

    lampShader->use();
    lampShader->setMat4("view", view);
    lampShader->setMat4("projection", projection);
    disableLampEdgeClip(lampShader.get());
    glBindVertexArray(targetVAO);

    // Target 1 (east wall, main beam).
    lampShader->setVec3("lightColor", targetActivated ? colorActive : colorIdle);
    lampShader->setMat4("model", target1ModelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, targetVertexCount);

    // Target 2 (south wall, deflected beam).
    lampShader->setVec3("lightColor", target2Activated ? colorActive : colorIdle);
    lampShader->setMat4("model", target2ModelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, targetVertexCount);

    glBindVertexArray(0);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void Game::setupDeflectorPrism() {
    prismShader = std::make_unique<Shader>(
        game_internal::shaderPath("prism.vert").c_str(),
        game_internal::shaderPath("prism.frag").c_str()
    );

    // Regular octahedron (6 vertices at axis extremes, 8 triangular faces).
    const float r = prismRadius;
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
        const glm::vec3 faceNormal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
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
    prismVertexCount = static_cast<int>(data.size() / 6);

    glGenVertexArrays(1, &prismVAO);
    glGenBuffers(1, &prismVBO);
    glBindVertexArray(prismVAO);
    glBindBuffer(GL_ARRAY_BUFFER, prismVBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glBindVertexArray(0);
}

void Game::renderDeflectorPrism(
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPosition
) {
    if (prismVAO == 0 || prismShader == nullptr || prismVertexCount <= 0) {
        return;
    }

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, prismCenter);

    prismShader->use();
    prismShader->setMat4("view", view);
    prismShader->setMat4("projection", projection);
    prismShader->setMat4("model", model);
    prismShader->setVec3("u_view_pos", cameraPosition);
    prismShader->setFloat("refractionIndice", 1.52f);
    glActiveTexture(GL_TEXTURE6);
    skybox.getCubemapTexture().bind(6);
    prismShader->setInt("cubemapSampler", 6);

    // Like LAB03 ex10: depth test on + depth write (no depth mask off).
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(prismVAO);
    glDrawArrays(GL_TRIANGLES, 0, prismVertexCount);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glDisable(GL_BLEND);
}
