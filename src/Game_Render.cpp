#include "Game.h"
#include "GameInternal.h"
#include "shader.h"
#include "camera.h"
#include "object.h"
#include "Light.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

// RTR4 §7.1.1 Eq. 7.4 : projection d'un point depuis une lumiere ponctuelle
// sur le plan pi : plane·x = 0 avec plane = (nx, ny, nz, d).
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

// §7.1.1 : seuls les piliers (empreinte etroite) projettent une ombre sur le sol.
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

} // namespace

void Game::drawMovablePillarPhong(const glm::mat4& pillarModelMatrix) {
    if (capturePillarMesh == nullptr || phongShader == nullptr) {
        return;
    }

    phongShader->setMat4("model", pillarModelMatrix);
    if (capturePillarMetalTextureLoaded) {
        glUniform2f(glGetUniformLocation(phongShader->ID, "uvScale"), 1.8f, 1.8f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, capturePillarMetalTexture);
        phongShader->setInt("useTexture", 1);
        phongShader->setVec3("material.specular", glm::vec3(0.72f, 0.74f, 0.78f));
        phongShader->setFloat("material.shininess", 112.0f);
    } else if (pillarDiffuseTextureLoaded) {
        phongShader->setInt("useTexture", 1);
        glUniform2f(glGetUniformLocation(phongShader->ID, "uvScale"), 1.75f, 5.5f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pillarDiffuseTexture);
        phongShader->setVec3("material.specular", glm::vec3(0.14f, 0.14f, 0.14f));
        phongShader->setFloat("material.shininess", 18.0f);
    } else {
        phongShader->setInt("useTexture", 0);
        phongShader->setVec3("material.specular", glm::vec3(0.14f, 0.14f, 0.14f));
        phongShader->setFloat("material.shininess", 18.0f);
    }
    // Mesh Blender : pas de shadow map §7.4 (auto-ombre ortho).
    phongShader->setInt("useShadowMap", 0);
    capturePillarMesh->draw();
    phongShader->setVec3("material.specular", glm::vec3(0.14f, 0.14f, 0.14f));
    phongShader->setFloat("material.shininess", 18.0f);
    if (pillarDiffuseTextureLoaded || capturePillarMetalTextureLoaded) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    phongShader->setInt("useTexture", 0);
    glUniform2f(glGetUniformLocation(phongShader->ID, "uvScale"), 1.0f, 1.0f);
}

void Game::drawMovablePillarShadows() {
    // Passe profondeur §7.4 : seuls les objets qui projettent une ombre (casters).
    if (capturePillarMesh == nullptr || shadowDepthShader == nullptr) {
        return;
    }

    shadowDepthShader->setMat4("model", capturePillarModelMatrix);
    capturePillarMesh->draw();
    shadowDepthShader->setMat4("model", deflectorPillarModelMatrix);
    capturePillarMesh->draw();
}

void Game::renderPlanarShadows(const glm::mat4& view, const glm::mat4& projection) {
    // RTR4 §7.1.1 : projection geometrique ; stencil limite le recepteur au sol marque.
    if (objects.empty() || lampShader == nullptr || groundObject == nullptr) {
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
    // Depth test : l'ombre est au niveau du sol ; les piliers (deja rendus) la masquent.
    // glPolygonOffset evite le z-fighting avec le sol (RTR4 §7.4 p. 236-237).
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -4.0f);
    glDisable(GL_CULL_FACE);

    lampShader->use();
    lampShader->setMat4("view", view);
    lampShader->setMat4("projection", projection);
    lampShader->setInt("useClipPlane", 0);
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

void Game::renderShadowMap() {
    // Standard shadow-map generation pass: render the scene from each light into a depth
    // buffer (Williams 1978, see Real-Time Rendering 4e §7.4 "Shadow Maps", p. 234).
    if (objects.empty() || shadowDepthShader == nullptr) {
        return;
    }

    const GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
    GLint previousCullFace = GL_BACK;
    glGetIntegerv(GL_CULL_FACE_MODE, &previousCullFace);

    // Canonical slope-scale + constant depth bias to fight "shadow acne". RTR4 §7.4
    // (p. 236-237) explicitly recommends OpenGL's glPolygonOffset for this: the offset is
    // added at depth-write time, scaled by the polygon's slope w.r.t. the light, exactly
    // matching the slope-scale bias described in the book. A clamped maximum (the second
    // parameter, here 4 units) avoids the runaway tangent values that happen for nearly
    // edge-on triangles (also discussed at p. 237).
    const GLboolean polyOffsetWasEnabled = glIsEnabled(GL_POLYGON_OFFSET_FILL);
    GLfloat previousPolyOffsetFactor = 0.0f;
    GLfloat previousPolyOffsetUnits = 0.0f;
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &previousPolyOffsetFactor);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &previousPolyOffsetUnits);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 4.0f);

    // Pas de shadow map statique pour la lumiere plafond : ombres planaires sur le sol (§7.1.1).
    lightSpaceMatrix = glm::mat4(1.0f);

    if (!lights.empty() && (lightProjectileActive || lightAnchoredOnPillar)) {
        // Shadow map dynamique : lumiere ponctuelle mobile, recalculee chaque frame (§7.4 p. 234).
        const glm::vec3 rawEye = lights[0]->position;
        glm::vec3 forward = lightProjectileDirection;
        if (glm::length(forward) < 0.0001f) {
            forward = glm::vec3(0.0f, -0.2f, -1.0f);
        }
        forward = glm::normalize(forward);

        // (a) Position snapping on a 1 mm world grid.
        auto snapToMillimeter = [](float v) { return std::round(v * 1000.0f) / 1000.0f; };
        const glm::vec3 shadowEye(snapToMillimeter(rawEye.x),
                                  snapToMillimeter(rawEye.y),
                                  snapToMillimeter(rawEye.z));
        const glm::vec3 shadowTarget = shadowEye + forward * 8.0f;

        const glm::mat4 dynamicProjection = glm::perspective(glm::radians(72.0f), 1.0f, 0.15f, 32.0f);
        const glm::mat4 dynamicView = glm::lookAt(shadowEye, shadowTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 viewProj = dynamicProjection * dynamicView;

        // (b) Snap texel NDC pour stabiliser l'echantillonnage frame a frame (RTR4 §7.4 p. 239).
        const glm::vec4 anchorClip = viewProj * glm::vec4(shadowTarget, 1.0f);
        if (std::abs(anchorClip.w) > 1.0e-5f) {
            const glm::vec2 anchorNDC(anchorClip.x / anchorClip.w,
                                      anchorClip.y / anchorClip.w);
            const float halfMap = static_cast<float>(game_internal::kShadowMapSize) * 0.5f;
            const glm::vec2 anchorTexel = anchorNDC * halfMap;
            const glm::vec2 rounded(std::round(anchorTexel.x),
                                    std::round(anchorTexel.y));
            const glm::vec2 deltaNDC = (rounded - anchorTexel) / halfMap;
            glm::mat4 snap(1.0f);
            snap[3][0] = deltaNDC.x;
            snap[3][1] = deltaNDC.y;
            viewProj = snap * viewProj;
        }
        dynamicLightSpaceMatrix = viewProj;

        shadowDepthShader->use();
        shadowDepthShader->setMat4("lightSpaceMatrix", dynamicLightSpaceMatrix);
        glViewport(0, 0, game_internal::kShadowMapSize, game_internal::kShadowMapSize);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dynamicShadowMapTexture, 0);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glPolygonOffset(1.5f, 4.0f);

        // Recepteurs omis ; seuls les casters alimentent la carte (RTR4 §7.4 p. 234-235).
        for (size_t ci = 0; ci < extraCubeModels.size(); ++ci) {
            if (!isCeilingShadowCaster(extraCubeModels[ci])) {
                continue;
            }
            shadowDepthShader->setMat4("model", extraCubeModels[ci]);
            objects[0]->draw();
        }
        drawMovablePillarShadows();
    } else {
        dynamicLightSpaceMatrix = glm::mat4(1.0f);
    }

    // Restore previous render state.
    glPolygonOffset(previousPolyOffsetFactor, previousPolyOffsetUnits);
    if (!polyOffsetWasEnabled) {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    glCullFace(previousCullFace);
    if (!cullWasEnabled) {
        glDisable(GL_CULL_FACE);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Game::Render(Camera& camera) {
    syncCarriedLight(camera.Position, camera.Front);
    renderShadowMap();
    const float aspect = static_cast<float>(viewportWidth) / static_cast<float>(std::max(1, viewportHeight));
    glm::mat4 projection = camera.GetProjectionMatrix(glm::radians(camera.Zoom), aspect);
    const glm::vec3 playerWorldPosition = camera.Position - glm::normalize(camera.Front) * 0.5f + glm::vec3(0.0f, -0.55f, 0.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewportWidth, viewportHeight);
    glClearColor(0.015f, 0.015f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDisable(GL_CULL_FACE);

    glm::mat4 view = camera.GetViewMatrix();
    renderSceneOpaque(view, projection, camera.Position, playerWorldPosition);
    // Skybox en dernier : GL_LEQUAL + depth mask off, ne remplit que le fond (RTR4 / LAB03 ex08).
    renderSkybox(view, projection);
    // LAB03 ex09/ex10 : prisme cubemap apres la scene opaque.
    if (game_internal::kEnableChapter14Translucency) {
        renderDeflectorPrism(view, projection, camera.Position);
    }
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

    const bool dynamicLightActive = !lights.empty() && (lightProjectileActive || lightAnchoredOnPillar);
    const bool beamActive = !lights.empty() && lightAnchoredOnPillar && !lightAnchorAnimating;
    const glm::vec3 dynamicLightColor(0.30f, 0.60f, 1.00f);
    const float dynamicLightStrength = lightAnchoredOnPillar ? 1.0f : 1.6f;
    const glm::vec3 beamColor(0.30f, 0.60f, 1.00f);
    const float kBeamStrength = beamActive ? 1.0f : 1.8f;
    const float kMaxBeamDistance = 40.0f;

    int beamCount = 0;
    glm::vec3 beamStarts[2] = {beamSourcePos, beamSourcePos};
    glm::vec3 beamEnds[2] = {beamSourcePos, beamSourcePos};
    float beamLengths[2] = {0.0f, 0.0f};

    if (beamActive) {
        const glm::vec3 beamOrigin = lights[0]->position;
        glm::vec3 hit(0.0f);
        float dist = 0.0f;
        const bool hitSomething = game_internal::kEnableChapter22Collision && raycastScene(
            beamOrigin, beamDirection, kMaxBeamDistance,
            centerPillarColliderIndex, hit, dist
        );
        if (!hitSomething) {
            dist = kMaxBeamDistance;
            hit = beamOrigin + beamDirection * kMaxBeamDistance;
        }

        float prismHitDist = 0.0f;
        const bool prismHit = game_internal::kEnableChapter14Translucency && raySphereIntersect(
            beamOrigin, beamDirection, prismCenter,
            prismRadius, dist, prismHitDist
        );

        if (prismHit) {
            beamStarts[0] = beamOrigin;
            beamEnds[0] = beamOrigin + beamDirection * prismHitDist;
            beamLengths[0] = prismHitDist;
            beamCount = 1;

            glm::vec3 dHit(0.0f);
            float dDist = 0.0f;
            const bool dGotHit = game_internal::kEnableChapter22Collision && raycastScene(
                prismCenter, prismDeflectDirection, kMaxBeamDistance,
                centerPillarColliderIndex, dHit, dDist
            );
            if (!dGotHit) {
                dDist = kMaxBeamDistance;
                dHit = prismCenter + prismDeflectDirection * kMaxBeamDistance;
            }
            beamStarts[1] = prismCenter;
            beamEnds[1] = dHit;
            beamLengths[1] = dDist;
            beamCount = 2;
        } else {
            beamStarts[0] = beamOrigin;
            beamEnds[0] = hit;
            beamLengths[0] = dist;
            beamCount = 1;
        }
    }

    auto applyPhongLightingUniforms = [&](bool groundPass) {
        phongShader->setVec3("ceilingLightPos", ceilingLightPosition);
        phongShader->setVec3("ceilingLightColor", ceilingLightColor);
        phongShader->setFloat("ceilingLightStrength", ceilingLightStrength);
        // Attenuation ponctuelle : ~25 % de l'intensite a ceilingLightRange metres (isotrope, distance seule).
        const float rangeFactor = std::max(1.0f, ceilingLightRange);
        phongShader->setFloat("ceilingAttLinear", 1.0f / rangeFactor);
        phongShader->setFloat("ceilingAttQuadratic", 2.0f / (rangeFactor * rangeFactor));
        phongShader->setInt("dynamicLightActive", dynamicLightActive ? 1 : 0);
        if (dynamicLightActive) {
            phongShader->setVec3("dynamicLightPos", lights[0]->position);
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
        phongShader->setInt("isGroundPass", groundPass ? 1 : 0);
        lights[0]->setUniforms(*phongShader);
        phongShader->setFloat("light.ambient", 0.0f);
        phongShader->setFloat("light.diffuse", 0.0f);
        phongShader->setFloat("light.specular", 0.0f);
    };

    if (!objects.empty()) {
        phongShader->use();
        phongShader->setMat4("view", view);
        phongShader->setMat4("projection", projection);
        phongShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
        phongShader->setMat4("dynamicLightSpaceMatrix", dynamicLightSpaceMatrix);
        // Piliers/murs/plafond : eclairage plafond sans shadow map ; sol : §7.1.1.
        phongShader->setInt("useShadowMap", 0);
        phongShader->setInt("shadowMap", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
        phongShader->setInt("dynamicShadowMap", 5);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, dynamicShadowMapTexture);
        glActiveTexture(GL_TEXTURE0);
        applyPhongLightingUniforms(false);
        phongShader->setInt("useTexture", 0);
        phongShader->setInt("diffuseMap", 0);
        glUniform2f(glGetUniformLocation(phongShader->ID, "uvScale"), 1.0f, 1.0f);
        phongShader->setVec3("material.ambient", glm::vec3(0.10f, 0.10f, 0.10f));
        phongShader->setVec3("material.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));
        phongShader->setVec3("material.specular", glm::vec3(0.14f, 0.14f, 0.14f));
        phongShader->setFloat("material.shininess", 18.0f);
        phongShader->setVec3("viewPos", cameraPosition);

        for (size_t ci = 0; ci < extraCubeModels.size(); ++ci) {
            const glm::mat4& cubeModel = extraCubeModels[ci];
            const float sy = std::abs(cubeModel[1][1]);
            if (pillarDiffuseTextureLoaded) {
                const float sx = std::abs(cubeModel[0][0]);
                const float sz = std::abs(cubeModel[2][2]);

                phongShader->setInt("useTexture", 1);
                if (sy < 1.0f) {
                    // Ceiling: strong tiling on both axes to avoid giant stretched bricks.
                    glUniform2f(glGetUniformLocation(phongShader->ID, "uvScale"), 12.0f, 12.0f);
                } else if (sx > 10.0f || sz > 10.0f) {
                    // Perimeter walls: repeat a lot on length, enough on height.
                    glUniform2f(glGetUniformLocation(phongShader->ID, "uvScale"), 12.0f, 4.0f);
                } else {
                    // Pillars: tall but much thinner.
                    glUniform2f(glGetUniformLocation(phongShader->ID, "uvScale"), 1.2f, 8.0f);
                }
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, pillarDiffuseTexture);
            }
            phongShader->setInt("useShadowMap", 0);
            phongShader->setMat4("model", cubeModel);
            objects[0]->draw();
        }
        drawMovablePillarPhong(capturePillarModelMatrix);
        drawMovablePillarPhong(deflectorPillarModelMatrix);
        if (pillarDiffuseTextureLoaded) {
            glBindTexture(GL_TEXTURE_2D, 0);
            phongShader->setInt("useTexture", 0);
        }
    }

    phongShader->use();
    phongShader->setMat4("model", groundObject->model);
    phongShader->setMat4("view", view);
    phongShader->setMat4("projection", projection);
    phongShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    phongShader->setMat4("dynamicLightSpaceMatrix", dynamicLightSpaceMatrix);
    phongShader->setInt("useShadowMap", 0);
    phongShader->setInt("shadowMap", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    phongShader->setInt("dynamicShadowMap", 5);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, dynamicShadowMapTexture);
    glActiveTexture(GL_TEXTURE0);
    // Sol : pas d'echantillonnage shadow map plafond (§7.4) ; ombres via §7.1.1 juste apres.
    applyPhongLightingUniforms(true);
    phongShader->setInt("useTexture", groundDiffuseTextureLoaded ? 1 : 0);
    phongShader->setInt("diffuseMap", 0);
    glUniform2f(glGetUniformLocation(phongShader->ID, "uvScale"), 6.0f, 6.0f);
    if (groundDiffuseTextureLoaded) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, groundDiffuseTexture);
    }
    phongShader->setVec3("material.ambient", glm::vec3(0.13f, 0.13f, 0.13f));
    phongShader->setVec3("material.diffuse", glm::vec3(0.19f, 0.19f, 0.19f));
    phongShader->setVec3("material.specular", glm::vec3(0.11f, 0.11f, 0.11f));
    phongShader->setFloat("material.shininess", 9.0f);
    phongShader->setVec3("viewPos", cameraPosition);

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    groundObject->draw();
    glStencilMask(0x00);
    if (groundDiffuseTextureLoaded) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    renderPlanarShadows(view, projection);
    glDisable(GL_STENCIL_TEST);

    if (!lights.empty()) {
        lampShader->use();
        lampShader->setMat4("view", view);
        lampShader->setMat4("projection", projection);
        lampShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
        lampShader->setVec4("clipPlane", reflectionClipPlane);
        lampShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
        for (int i = 0; i < 4; ++i) {
            lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
        }
        lampShader->setVec3("lightColor", dynamicLightColor);

        glm::mat4 lightModel = glm::mat4(1.0f);
        lightModel = glm::translate(lightModel, lights[0]->position);
        lightModel = glm::scale(lightModel, glm::vec3(0.12f));
        lampShader->setMat4("model", lightModel);
        lightMarker->draw();
    }

    // Rail au sol qui guide le pilier central.
    {
        const glm::vec3 railIdle(0.10f, 0.18f, 0.32f);
        const glm::vec3 railActive(0.30f, 0.60f, 1.00f);
        const glm::vec3 railColor = beamActive ? railActive : railIdle;
        lampShader->use();
        lampShader->setMat4("view", view);
        lampShader->setMat4("projection", projection);
        lampShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
        lampShader->setVec4("clipPlane", reflectionClipPlane);
        lampShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
        for (int i = 0; i < 4; ++i) {
            lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
        }
        lampShader->setVec3("lightColor", railColor);
        lampShader->setMat4("model", railModel);
        lightMarker->draw();
        // Rail du pilier deflecteur, perpendiculaire au precedent.
        lampShader->setMat4("model", deflectorRailModel);
        lightMarker->draw();
    }

    // Cible sur le mur Est: petit point + anneau, noir au repos, bleu apres 3 secondes
    // de touche continue par le rayon. Doit etre rendue avant le faisceau pour eviter
    // qu'il la masque visuellement.
    renderBeamTarget(view, projection);

    if (beamCount > 0) {
        const float kBeamHalfWidth = 0.035f;
        lampShader->use();
        lampShader->setMat4("view", view);
        lampShader->setMat4("projection", projection);
        lampShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
        lampShader->setVec4("clipPlane", reflectionClipPlane);
        lampShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
        for (int i = 0; i < 4; ++i) {
            lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
        }
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

    // Marqueur de la lumiere jaune au plafond.
    lampShader->use();
    lampShader->setMat4("view", view);
    lampShader->setMat4("projection", projection);
    lampShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
    lampShader->setVec4("clipPlane", reflectionClipPlane);
    lampShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
    for (int i = 0; i < 4; ++i) {
        lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
    }
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
    lampShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
    lampShader->setVec4("clipPlane", reflectionClipPlane);
    lampShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
    for (int i = 0; i < 4; ++i) {
        lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
    }
    lampShader->setVec3("lightColor", glm::vec3(1.0f, 0.95f, 0.25f));
    glm::mat4 playerModel = glm::mat4(1.0f);
    playerModel = glm::translate(playerModel, playerWorldPosition);
    playerModel = glm::scale(playerModel, glm::vec3(0.18f));
    lampShader->setMat4("model", playerModel);
    lightMarker->draw();

    renderExplosionParticles(view, projection);

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

    // Face interieure des petits pans de mur (centre a mapHalfExtent - epaisseur/2).
    const float kWallInnerX = worldCollisionHalfExtent - kTargetWallThickness;
    const float kTarget1Z = 0.8f;
    targetPosition = glm::vec3(kWallInnerX - kAvoidZFightingOffset, kBeamY, kTarget1Z);
    target1ModelMatrix = glm::translate(glm::mat4(1.0f), targetPosition);

    const float kWallInnerZ = worldCollisionHalfExtent - kTargetWallThickness;
    const float kTarget2X = deflectorPillarBase.x - 1.0f;
    target2Position = glm::vec3(kTarget2X, kBeamY, kWallInnerZ - kAvoidZFightingOffset);
    target2ModelMatrix = glm::translate(glm::mat4(1.0f), target2Position);
    // Le mesh est genere dans le plan local YZ (normale +X): pour aligner la cible
    // sur le mur Sud, on lui fait subir une rotation de 90 deg autour de Y.
    target2ModelMatrix = glm::rotate(target2ModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Disque central plein + anneau, dans le plan local YZ (X = 0). Les coordonnees
    // sont transformees via le model uniform au moment du rendu.
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

        // Disque central: triangle (centre, segment i, segment i+1).
        verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(0.0f);
        verts.push_back(0.0f); verts.push_back(kPointRadius * c0); verts.push_back(kPointRadius * s0);
        verts.push_back(0.0f); verts.push_back(kPointRadius * c1); verts.push_back(kPointRadius * s1);

        // Anneau: deux triangles formant un trapeze entre rayons interieur/exterieur.
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
    const bool beamCurrentlyActive = !lights.empty() && lightAnchoredOnPillar && !lightAnchorAnimating;
    bool target1Hit = false;
    bool target2Hit = false;

    if (beamCurrentlyActive) {
        const float kMax = 60.0f;
        float beamLen = kMax;
        const glm::vec3 beamOrigin = lights[0]->position;

        glm::vec3 hit(0.0f);
        float dist = 0.0f;
        const bool hitSomething = game_internal::kEnableChapter22Collision && raycastScene(
            beamOrigin, beamDirection, kMax,
            centerPillarColliderIndex, hit, dist
        );
        const float beamLenFull = hitSomething ? dist : kMax;

        float prismDist = 0.0f;
        const bool prismIntercept = game_internal::kEnableChapter14Translucency && raySphereIntersect(
            beamOrigin, beamDirection, prismCenter,
            prismRadius, beamLenFull, prismDist
        );
        beamLen = prismIntercept ? prismDist : beamLenFull;

        if (prismIntercept && game_internal::kEnableChapter14Translucency) {
            glm::vec3 dHit(0.0f);
            float dDist = 0.0f;
            const bool dGotHit = game_internal::kEnableChapter22Collision && raycastScene(
                prismCenter, prismDeflectDirection, kMax,
                centerPillarColliderIndex, dHit, dDist
            );
            const float deflectedBeamLen = dGotHit ? dDist : kMax;

            const glm::vec3 toTarget2 = target2Position - prismCenter;
            const float t2 = glm::dot(toTarget2, prismDeflectDirection);
            if (t2 >= 0.0f && t2 <= deflectedBeamLen + 0.05f) {
                const glm::vec3 closest = prismCenter + prismDeflectDirection * t2;
                if (glm::length(target2Position - closest) < targetHitTolerance) {
                    target2Hit = true;
                }
            }
        }

        const glm::vec3 toTarget = targetPosition - beamOrigin;
        const float t = glm::dot(toTarget, beamDirection);
        if (t >= 0.0f && t <= beamLen + 0.05f) {
            const glm::vec3 closest = beamOrigin + beamDirection * t;
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
    lampShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
    lampShader->setVec4("clipPlane", reflectionClipPlane);
    lampShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
    for (int i = 0; i < 4; ++i) {
        lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
    }
    glBindVertexArray(targetVAO);

    // Cible 1 (mur Est, rayon principal).
    lampShader->setVec3("lightColor", targetActivated ? colorActive : colorIdle);
    lampShader->setMat4("model", target1ModelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, targetVertexCount);

    // Cible 2 (mur Sud, rayon devie).
    lampShader->setVec3("lightColor", target2Activated ? colorActive : colorIdle);
    lampShader->setMat4("model", target2ModelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, targetVertexCount);

    glBindVertexArray(0);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void Game::setupDeflectorPrism() {
    // Exo 9 + 10 LAB03 : reflexion (reflect) et refraction (refract) sur la cubemap.
    const std::string vert = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        out vec3 v_frag_coord;
        out vec3 v_normal;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            vec4 frag_coord = model * vec4(aPos, 1.0);
            gl_Position = projection * view * frag_coord;
            v_normal = mat3(transpose(inverse(model))) * aNormal;
            v_frag_coord = frag_coord.xyz;
        }
    )";
    const std::string frag = R"(
        #version 330 core
        out vec4 FragColor;
        in vec3 v_frag_coord;
        in vec3 v_normal;
        uniform vec3 u_view_pos;
        uniform samplerCube cubemapSampler;
        uniform float refractionIndice;
        void main() {
            vec3 N = normalize(v_normal);
            vec3 V = normalize(u_view_pos - v_frag_coord);
            vec3 R = reflect(-V, N);
            vec3 reflColor = texture(cubemapSampler, R).rgb;
            float ratio = 1.0 / refractionIndice;
            vec3 T = refract(-V, N, ratio);
            vec3 refrColor = length(T) > 0.001 ? texture(cubemapSampler, T).rgb : reflColor;
            float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
            vec3 env = mix(refrColor, reflColor, fresnel);
            // Leger assombrissement + teinte pour distinguer le cristal du ciel derriere.
            env *= 0.88;
            env = mix(env, vec3(0.30, 0.60, 1.00), 0.22);
            float alpha = 0.88 + 0.12 * fresnel;
            FragColor = vec4(env, alpha);
        }
    )";
    prismShader = new Shader(vert, frag);

    // Octaedre regulier (6 sommets aux extremites des axes, 8 faces triangulaires).
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
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    prismShader->setInt("cubemapSampler", 6);

    // Comme LAB03 ex10 : depth test actif + ecriture profondeur (pas de depth mask off).
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
void Game::setupSkybox() {
    const float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    cubemapTexture = game_internal::loadVoidSpaceCubemap();
    if (cubemapTexture == 0) {
        std::cout << "ERREUR: impossible de charger la cubemap void space.\n";
    }
}

void Game::renderSkybox(const glm::mat4& view, const glm::mat4& projection) {
    if (cubemapTexture == 0) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    cubemapShader->use();
    cubemapShader->setMat4("projection", projection);
    cubemapShader->setMat4("view", glm::mat4(glm::mat3(view)));

    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    cubemapShader->setInt("skybox", 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}