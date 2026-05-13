#include "Game.h"
#include "GameInternal.h"
#include "shader.h"
#include "camera.h"
#include "object.h"
#include "Light.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

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

    const glm::vec3 topCornerLights[4] = {
        glm::vec3(-12.5f, groundTopY + 3.6f, -12.5f),
        glm::vec3( 12.5f, groundTopY + 3.6f, -12.5f),
        glm::vec3(-12.5f, groundTopY + 3.6f,  12.5f),
        glm::vec3( 12.5f, groundTopY + 3.6f,  12.5f)
    };

    // The four corner torches and their occluders (pillars, walls, ceiling) never move,
    // so the depth maps are built once at startup and reused for the rest of the run.
    // This is the frame-to-frame coherence optimization described in RTR4 §7.4 p. 235
    // ("If the shadow situation does not change from frame to frame, i.e., the light and
    // shadow casters do not move relative to each other, this texture can be reused").
    if (!staticShadowMapsBuilt) {
        for (int i = 0; i < 4; ++i) {
            // Light frustum tightened to the actual receiver volume of the cave (~25x25x5).
            // RTR4 §7.4 Figure 7.11 (p. 236) shows that pulling near/far in and reducing
            // the x/y extent around the visible receivers increases the effective
            // shadow-map resolution and the z-buffer precision (cf. §4.7.2 cited at p. 236).
            // Previous frustum was a loose 48x48x60 cube wasting ~4x of the texel budget.
            const glm::vec3 shadowTarget(0.0f, groundTopY + 1.2f, 0.0f);
            const glm::mat4 lightProjection = glm::ortho(-18.0f, 18.0f, -18.0f, 18.0f, 0.5f, 45.0f);
            const glm::mat4 lightView = glm::lookAt(topCornerLights[i], shadowTarget, glm::vec3(0.0f, 1.0f, 0.0f));
            lightSpaceMatrices[i] = lightProjection * lightView;

            shadowDepthShader->use();
            shadowDepthShader->setMat4("lightSpaceMatrix", lightSpaceMatrices[i]);
            glViewport(0, 0, game_internal::kShadowMapSize, game_internal::kShadowMapSize);
            glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTextures[i], 0);
            glClear(GL_DEPTH_BUFFER_BIT);
            // Second-depth shadow mapping (Wang [1845], RTR4 §7.4 p. 238 and Figure 7.14):
            // by culling front faces and writing the back faces' depth into the shadow map,
            // self-shadow acne on lit surfaces is essentially eliminated at the cost of a
            // small risk of light leaks near silhouette edges. This works well for the closed
            // pillar cubes in this scene (they are "watertight" — see RTR4 §7.4 p. 238).
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);

            for (const auto& cubeModel : extraCubeModels) {
                shadowDepthShader->setMat4("model", cubeModel);
                objects[0]->draw();
            }

            shadowDepthShader->setMat4("model", groundObject->model);
            groundObject->draw();
        }
        staticShadowMapsBuilt = true;
    }

    if (!lights.empty()) {
        // Player-carried torch: re-rendered every frame, modeled as a spotlight with a
        // perspective frustum (RTR4 §7.4 p. 234, "if the local light is a spotlight, it
        // has a natural frustum associated with it").
        //
        // === Stable shadow maps (RTR4 §7.4 end of section, p. 239) ===
        //
        // As the carried torch moves with the camera, the projection samples a slightly
        // different set of world-space directions each frame. Texels no longer cover the
        // same world footprints and shadow edges visibly "swim" pixel by pixel between
        // frames. RTR4 §7.4 p. 239 prescribes "[forcing] each succeeding shadow map
        // generated to maintain the same relative texel beam locations in world space"
        // [Valient 1810, Tuft 1792]. The canonical recipe is written for directional
        // / orthographic lights (typically the sun in a CSM); we adapt it here to a
        // perspective spotlight via two cooperating measures:
        //
        //   (a) Snap the light position to a fine world-space grid (1 mm). Removes
        //       sub-millimeter floating-point jitter of the spotlight origin and forces
        //       identical views on still frames, which is otherwise impossible when the
        //       camera position varies by a few ULPs every frame.
        //
        //   (b) Project a fixed world-space anchor (here the cone-axis target ~8 m in
        //       front of the torch) into clip space, compute its fractional sub-texel
        //       offset on the shadow map (kShadowMapSize) grid, and apply that delta as a constant
        //       NDC translation by left-multiplying the projection by a translation
        //       matrix. Because the translation row multiplies the clip-space w, the
        //       offset survives the perspective divide as a uniform shift across the
        //       entire frustum — so the snap is exact for every fragment, not just for
        //       the anchor itself.
        //
        // Limitation explicitly inherited from §7.4: a perspective spotlight does not
        // have a constant world-space texel grid (the texel "beams" diverge from the
        // light origin), so a rotation of the spotlight inevitably re-shuffles which
        // world surfaces sit in which texel. The snap therefore stabilizes translation
        // and sub-pixel jitter, but cannot eliminate rotation-induced reshuffling.
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

        // (b) Anchor-based texel snap of the principal point in NDC.
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

        // Only shadow-casters are rendered here: the ground is a pure receiver and is
        // intentionally omitted to save work (RTR4 §7.4 p. 234 — "only objects that can
        // cast shadows need to be rendered" into the light's view).
        for (const auto& cubeModel : extraCubeModels) {
            shadowDepthShader->setMat4("model", cubeModel);
            objects[0]->draw();
        }
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
    glm::mat4 projection = camera.GetProjectionMatrix();
    const glm::vec3 playerWorldPosition = camera.Position - glm::normalize(camera.Front) * 0.5f + glm::vec3(0.0f, -0.55f, 0.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewportWidth, viewportHeight);
    glClearColor(0.015f, 0.015f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDisable(GL_CULL_FACE);

    glm::mat4 view = camera.GetViewMatrix();
    renderSceneOpaque(view, projection, camera.Position, playerWorldPosition);
    renderSkybox(view, projection);
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
    // Rayons bleus: on calcule jusqu'a deux segments. Le rayon principal sort du canon
    // sur +X; s'il traverse la pyramide deflectrice, il est stoppe au point d'entree
    // et un second rayon part du centre de la pyramide sur la direction perpendiculaire.
    // Tout est calcule ici une fois pour etre reutilise par les passes phong (eclairage)
    // et le rendu des cubes lumineux.
    const glm::vec3 beamColor(0.30f, 0.60f, 1.00f);
    const float kBeamStrength = 1.8f;
    const float kMaxBeamDistance = 40.0f;
    const bool beamActive = !lights.empty() && lightLockedOnPillar && !lightLockAnimating;

    int beamCount = 0;
    glm::vec3 beamStarts[2] = {cannonMuzzlePos, cannonMuzzlePos};
    glm::vec3 beamEnds[2] = {cannonMuzzlePos, cannonMuzzlePos};
    float beamLengths[2] = {0.0f, 0.0f};

    if (beamActive) {
        glm::vec3 hit(0.0f);
        float dist = 0.0f;
        const bool hitSomething = raycastScene(
            cannonMuzzlePos, cannonDirection, kMaxBeamDistance,
            cannonColliderIndex, hit, dist
        );
        if (!hitSomething) {
            dist = kMaxBeamDistance;
            hit = cannonMuzzlePos + cannonDirection * kMaxBeamDistance;
        }

        // Intersection rayon principal <-> sphere englobante de la pyramide.
        float prismHitDist = 0.0f;
        const bool prismHit = raySphereIntersect(
            cannonMuzzlePos, cannonDirection, prismCenter,
            prismRadius, dist, prismHitDist
        );

        if (prismHit) {
            // Rayon principal coupe au point d'entree dans la pyramide.
            beamStarts[0] = cannonMuzzlePos;
            beamEnds[0]   = cannonMuzzlePos + cannonDirection * prismHitDist;
            beamLengths[0] = prismHitDist;
            beamCount = 1;

            // Rayon devie depuis le centre de la pyramide jusqu'au premier obstacle.
            glm::vec3 dHit(0.0f);
            float dDist = 0.0f;
            const bool dGotHit = raycastScene(
                prismCenter, prismDeflectDirection, kMaxBeamDistance,
                cannonColliderIndex, dHit, dDist
            );
            if (!dGotHit) {
                dDist = kMaxBeamDistance;
                dHit = prismCenter + prismDeflectDirection * kMaxBeamDistance;
            }
            beamStarts[1] = prismCenter;
            beamEnds[1]   = dHit;
            beamLengths[1] = dDist;
            beamCount = 2;
        } else {
            beamStarts[0] = cannonMuzzlePos;
            beamEnds[0]   = hit;
            beamLengths[0] = dist;
            beamCount = 1;
        }
    }

    const float topLightY = groundTopY + 3.6f;
    const glm::vec3 cornerWarmBase(1.0f, 0.82f, 0.30f);
    const glm::vec3 cornerWarmTip(1.0f, 0.92f, 0.45f);
    const float cornerFlickerA[4] = {
        std::sin(time * 2.1f + 0.2f) * 0.5f + 0.5f,
        std::sin(time * 2.9f + 1.4f) * 0.5f + 0.5f,
        std::sin(time * 1.7f + 2.5f) * 0.5f + 0.5f,
        std::sin(time * 3.2f + 3.8f) * 0.5f + 0.5f
    };
    const float cornerFlickerB[4] = {
        std::sin(time * 5.7f + 0.8f) * 0.5f + 0.5f,
        std::sin(time * 4.6f + 2.1f) * 0.5f + 0.5f,
        std::sin(time * 6.1f + 1.3f) * 0.5f + 0.5f,
        std::sin(time * 4.9f + 2.9f) * 0.5f + 0.5f
    };
    float cornerFlicker[4];
    for (int i = 0; i < 4; ++i) {
        // Random-like but deterministic and desynchronized flicker per corner.
        cornerFlicker[i] = 0.78f + 0.22f * (0.58f * cornerFlickerA[i] + 0.42f * cornerFlickerB[i]);
    }
    const float cornerFlickerMean = (cornerFlicker[0] + cornerFlicker[1] + cornerFlicker[2] + cornerFlicker[3]) * 0.25f;
    const glm::vec3 cornerLightColor = glm::mix(cornerWarmBase, cornerWarmTip, 0.3f + 0.5f * cornerFlickerMean);
    const float cornerLightStrength = topLightStrength * 2.2f;
    const float placedCornerLikeFlicker = 0.88f + 0.12f * (std::sin(time * 6.2f + 1.3f) * 0.5f + 0.5f);
    const glm::vec3 topCornerLights[4] = {
        glm::vec3(-12.5f, topLightY, -12.5f),
        glm::vec3( 12.5f, topLightY, -12.5f),
        glm::vec3(-12.5f, topLightY,  12.5f),
        glm::vec3( 12.5f, topLightY,  12.5f)
    };

    if (!objects.empty()) {
        phongShader->use();
        phongShader->setMat4("view", view);
        phongShader->setMat4("projection", projection);
        for (int i = 0; i < 4; ++i) {
            phongShader->setMat4("lightSpaceMatrices[" + std::to_string(i) + "]", lightSpaceMatrices[i]);
        }
        phongShader->setMat4("dynamicLightSpaceMatrix", dynamicLightSpaceMatrix);
        phongShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
        phongShader->setVec4("clipPlane", reflectionClipPlane);
        phongShader->setInt("useShadowMap", 1);
        for (int i = 0; i < 4; ++i) {
            phongShader->setInt("shadowMaps[" + std::to_string(i) + "]", 1 + i);
            glActiveTexture(GL_TEXTURE1 + i);
            glBindTexture(GL_TEXTURE_2D, shadowMapTextures[i]);
        }
        phongShader->setInt("dynamicShadowMap", 5);
        phongShader->setInt("dynamicShadowActive", !lights.empty() ? 1 : 0);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, dynamicShadowMapTexture);
        glActiveTexture(GL_TEXTURE0);
        phongShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
        for (int i = 0; i < 4; ++i) {
            phongShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
        }
        for (int i = 0; i < 4; ++i) {
            phongShader->setVec3("topCornerLights[" + std::to_string(i) + "]", topCornerLights[i]);
            phongShader->setFloat("topCornerFlicker[" + std::to_string(i) + "]", cornerFlicker[i]);
        }
        phongShader->setVec3("topLightColor", cornerLightColor);
        phongShader->setFloat("topLightStrength", cornerLightStrength);
        const float rangeFactor = std::max(0.2f, cornerLightRange);
        phongShader->setFloat("cornerAttLinear", 0.11f / rangeFactor);
        phongShader->setFloat("cornerAttQuadratic", 0.15f / (rangeFactor * rangeFactor));
        const bool dynamicCornerLikeActive = !lights.empty();
        phongShader->setInt("placedCornerLikeActive", dynamicCornerLikeActive ? 1 : 0);
        phongShader->setVec3("placedCornerLikePos", lights[0]->position);
        phongShader->setVec3("placedCornerLikeColor", lights[0]->color);
        phongShader->setFloat("placedCornerLikeStrength", dynamicCornerLikeActive ? 1.6f : 0.0f);
        phongShader->setFloat("placedCornerLikeFlicker", placedCornerLikeFlicker);
        phongShader->setInt("beamLightCount", beamCount);
        for (int i = 0; i < beamCount; ++i) {
            phongShader->setVec3("beamLightStarts[" + std::to_string(i) + "]", beamStarts[i]);
            phongShader->setVec3("beamLightEnds[" + std::to_string(i) + "]", beamEnds[i]);
            phongShader->setVec3("beamLightColors[" + std::to_string(i) + "]", beamColor);
            phongShader->setFloat("beamLightStrengths[" + std::to_string(i) + "]", kBeamStrength);
        }
        phongShader->setInt("useTexture", 0);
        phongShader->setInt("diffuseMap", 0);
        phongShader->setInt("isGroundPass", 0);
        phongShader->setInt("pillarShadowCount", 0);
        glUniform2f(glGetUniformLocation(phongShader->ID, "uvScale"), 1.0f, 1.0f);
        phongShader->setVec3("material.ambient", glm::vec3(0.10f, 0.10f, 0.10f));
        phongShader->setVec3("material.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));
        phongShader->setVec3("material.specular", glm::vec3(0.14f, 0.14f, 0.14f));
        phongShader->setFloat("material.shininess", 18.0f);
        lights[0]->setUniforms(*phongShader);
        if (dynamicCornerLikeActive) {
            phongShader->setFloat("light.ambient", 0.0f);
            phongShader->setFloat("light.diffuse", 0.0f);
            phongShader->setFloat("light.specular", 0.0f);
        }
        phongShader->setVec3("viewPos", cameraPosition);
        for (const auto& cubeModel : extraCubeModels) {
            if (pillarDiffuseTextureLoaded) {
                const float sx = std::abs(cubeModel[0][0]);
                const float sy = std::abs(cubeModel[1][1]);
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
            phongShader->setMat4("model", cubeModel);
            objects[0]->draw();
        }
        if (pillarDiffuseTextureLoaded) {
            glBindTexture(GL_TEXTURE_2D, 0);
            phongShader->setInt("useTexture", 0);
        }
    }

    phongShader->use();
    phongShader->setMat4("model", groundObject->model);
    phongShader->setMat4("view", view);
    phongShader->setMat4("projection", projection);
    for (int i = 0; i < 4; ++i) {
        phongShader->setMat4("lightSpaceMatrices[" + std::to_string(i) + "]", lightSpaceMatrices[i]);
    }
    phongShader->setMat4("dynamicLightSpaceMatrix", dynamicLightSpaceMatrix);
    phongShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
    phongShader->setVec4("clipPlane", reflectionClipPlane);
    phongShader->setInt("useShadowMap", 1);
    for (int i = 0; i < 4; ++i) {
        phongShader->setInt("shadowMaps[" + std::to_string(i) + "]", 1 + i);
        glActiveTexture(GL_TEXTURE1 + i);
        glBindTexture(GL_TEXTURE_2D, shadowMapTextures[i]);
    }
    phongShader->setInt("dynamicShadowMap", 5);
    phongShader->setInt("dynamicShadowActive", !lights.empty() ? 1 : 0);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, dynamicShadowMapTexture);
    glActiveTexture(GL_TEXTURE0);
    phongShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
    for (int i = 0; i < 4; ++i) {
        phongShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
    }
    for (int i = 0; i < 4; ++i) {
        phongShader->setVec3("topCornerLights[" + std::to_string(i) + "]", topCornerLights[i]);
        phongShader->setFloat("topCornerFlicker[" + std::to_string(i) + "]", cornerFlicker[i]);
    }
    phongShader->setVec3("topLightColor", cornerLightColor);
    phongShader->setFloat("topLightStrength", cornerLightStrength);
    const float rangeFactor = std::max(0.2f, cornerLightRange);
    phongShader->setFloat("cornerAttLinear", 0.11f / rangeFactor);
    phongShader->setFloat("cornerAttQuadratic", 0.15f / (rangeFactor * rangeFactor));
    const bool dynamicCornerLikeActive = !lights.empty();
    phongShader->setInt("placedCornerLikeActive", dynamicCornerLikeActive ? 1 : 0);
    phongShader->setVec3("placedCornerLikePos", lights[0]->position);
    phongShader->setVec3("placedCornerLikeColor", lights[0]->color);
    phongShader->setFloat("placedCornerLikeStrength", dynamicCornerLikeActive ? 1.6f : 0.0f);
    phongShader->setFloat("placedCornerLikeFlicker", placedCornerLikeFlicker);
    phongShader->setInt("beamLightCount", beamCount);
    for (int i = 0; i < beamCount; ++i) {
        phongShader->setVec3("beamLightStarts[" + std::to_string(i) + "]", beamStarts[i]);
        phongShader->setVec3("beamLightEnds[" + std::to_string(i) + "]", beamEnds[i]);
        phongShader->setVec3("beamLightColors[" + std::to_string(i) + "]", beamColor);
        phongShader->setFloat("beamLightStrengths[" + std::to_string(i) + "]", kBeamStrength);
    }
    phongShader->setInt("useTexture", groundDiffuseTextureLoaded ? 1 : 0);
    phongShader->setInt("diffuseMap", 0);
    phongShader->setInt("isGroundPass", 1);
    const int maxShadowPillars = 64;
    const int shadowPillarCount = std::min(static_cast<int>(pillarBaseCenters.size()), maxShadowPillars);
    phongShader->setInt("pillarShadowCount", shadowPillarCount);
    if (shadowPillarCount > 0) {
        glUniform3fv(
            glGetUniformLocation(phongShader->ID, "pillarShadowCenters[0]"),
            shadowPillarCount,
            &pillarBaseCenters[0].x
        );
    }
    phongShader->setFloat("pillarShadowRadius", 0.85f);
    phongShader->setFloat("pillarShadowStrength", 0.16f);
    glUniform2f(glGetUniformLocation(phongShader->ID, "uvScale"), 6.0f, 6.0f);
    if (groundDiffuseTextureLoaded) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, groundDiffuseTexture);
    }
    phongShader->setVec3("material.ambient", glm::vec3(0.13f, 0.13f, 0.13f));
    phongShader->setVec3("material.diffuse", glm::vec3(0.19f, 0.19f, 0.19f));
    phongShader->setVec3("material.specular", glm::vec3(0.11f, 0.11f, 0.11f));
    phongShader->setFloat("material.shininess", 9.0f);
    lights[0]->setUniforms(*phongShader);
    if (dynamicCornerLikeActive) {
        phongShader->setFloat("light.ambient", 0.0f);
        phongShader->setFloat("light.diffuse", 0.0f);
        phongShader->setFloat("light.specular", 0.0f);
    }
    phongShader->setVec3("viewPos", cameraPosition);
    groundObject->draw();
    if (groundDiffuseTextureLoaded) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

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
        lampShader->setVec3("lightColor", lights[0]->color);

        glm::mat4 lightModel = glm::mat4(1.0f);
        lightModel = glm::translate(lightModel, lights[0]->position);
        lightModel = glm::scale(lightModel, glm::vec3(0.12f));
        lampShader->setMat4("model", lightModel);
        lightMarker->draw();
    }

    // Rail au sol qui guide le pilier central. Plus lumineux quand la lampe est ancree.
    {
        const glm::vec3 railIdle(0.10f, 0.18f, 0.32f);
        const glm::vec3 railActive(0.30f, 0.60f, 1.00f);
        const bool beamActive = lightLockedOnPillar && !lightLockAnimating;
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

    // Cubes fins lumineux materialisant le rayon principal et son rayon devie.
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
            // Les deux rayons utilises sont alignes sur un axe canonique (X pour le
            // rayon principal, Z pour le rayon devie). On peut donc construire la
            // matrice avec un simple scale composant par composant.
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

    // Visual markers for the four fixed ceiling lights.
    lampShader->use();
    lampShader->setMat4("view", view);
    lampShader->setMat4("projection", projection);
    lampShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
    lampShader->setVec4("clipPlane", reflectionClipPlane);
    lampShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
    for (int i = 0; i < 4; ++i) {
        lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
    }
    lampShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
    for (int i = 0; i < 4; ++i) {
        lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
    }
    for (int i = 0; i < 4; ++i) {
        lampShader->setVec3("lightColor", cornerLightColor * (0.95f + 0.35f * cornerFlicker[i]));
        glm::mat4 topLightModel = glm::mat4(1.0f);
        topLightModel = glm::translate(topLightModel, topCornerLights[i]);
        topLightModel = glm::scale(topLightModel, glm::vec3(0.14f));
        lampShader->setMat4("model", topLightModel);
        lightMarker->draw();
    }

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

    // Pyramide bleue translucide rendue en dernier (apres tout l'opaque + les rayons),
    // pour que le blending alpha la combine proprement avec le reste de la scene.
    renderDeflectorPrism(view, projection);
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
    const float kCannonY = groundTopY + centerPillarHeight - cannonHalfWidth;
    const float kAvoidZFightingOffset = 0.012f;

    // Cible 1: sur la face interieure du mur Est (normale -X), atteignable par le
    // rayon principal en glissant le pilier central sur son rail Z.
    const float kWallInnerX = worldCollisionHalfExtent - 0.3f;  // mapHalfExtent - wallThickness/2
    const float kTarget1Z = 0.8f;
    targetPosition = glm::vec3(kWallInnerX - kAvoidZFightingOffset, kCannonY, kTarget1Z);
    target1ModelMatrix = glm::translate(glm::mat4(1.0f), targetPosition);

    // Cible 2: sur la face interieure du mur Sud (normale -Z), atteignable par le
    // rayon devie en glissant le pilier deflecteur sur son rail X. On vise un x dans
    // le couloir vide entre les rangees de piliers gx=1 (x in [2.85, 3.55]) et gx=2
    // (x in [6.05, 6.75]). deflectorPillarBase.x - 1.0 = 4.5 est central dans ce
    // couloir et accessible (offset = -1.0 in [-1.8, 1.8]).
    const float kWallInnerZ = worldCollisionHalfExtent - 0.3f;
    const float kTarget2X = deflectorPillarBase.x - 1.0f;
    target2Position = glm::vec3(kTarget2X, kCannonY, kWallInnerZ - kAvoidZFightingOffset);
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
    const bool beamCurrentlyActive = !lights.empty() && lightLockedOnPillar && !lightLockAnimating;
    bool target1Hit = false;
    bool target2Hit = false;

    if (beamCurrentlyActive) {
        // Rayon principal (canon vers +X).
        glm::vec3 hit(0.0f);
        float dist = 0.0f;
        const float kMax = 60.0f;
        const bool hitSomething = raycastScene(
            cannonMuzzlePos, cannonDirection, kMax,
            cannonColliderIndex, hit, dist
        );
        const float beamLenFull = hitSomething ? dist : kMax;

        // Si la pyramide intercepte, le rayon principal s'arrete au point d'entree;
        // un rayon devie part alors du centre de la pyramide.
        float prismDist = 0.0f;
        const bool prismIntercept = raySphereIntersect(
            cannonMuzzlePos, cannonDirection, prismCenter,
            prismRadius, beamLenFull, prismDist
        );
        const float beamLen = prismIntercept ? prismDist : beamLenFull;

        // Cible 1 sur le rayon principal.
        {
            const glm::vec3 toTarget = targetPosition - cannonMuzzlePos;
            const float t = glm::dot(toTarget, cannonDirection);
            if (t >= 0.0f && t <= beamLen + 0.05f) {
                const glm::vec3 closest = cannonMuzzlePos + cannonDirection * t;
                if (glm::length(targetPosition - closest) < targetHitTolerance) {
                    target1Hit = true;
                }
            }
        }

        // Cible 2 sur le rayon devie (si la pyramide intercepte).
        if (prismIntercept) {
            glm::vec3 dHit(0.0f);
            float dDist = 0.0f;
            const bool dGotHit = raycastScene(
                prismCenter, prismDeflectDirection, kMax,
                cannonColliderIndex, dHit, dDist
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

    const glm::vec3 colorIdle(0.0f, 0.0f, 0.0f);
    const glm::vec3 colorActive(0.30f, 0.60f, 1.00f);

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
}

void Game::setupDeflectorPrism() {
    // Shader minimal avec une couleur RGBA uniforme pour rendre la pyramide en blending.
    const std::string vert = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";
    const std::string frag = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec4 color;
        void main() {
            FragColor = color;
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
    data.reserve(8 * 3 * 3);
    for (int f = 0; f < 8; ++f) {
        for (int k = 0; k < 3; ++k) {
            const glm::vec3& vert = v[faces[f][k]];
            data.push_back(vert.x);
            data.push_back(vert.y);
            data.push_back(vert.z);
        }
    }
    prismVertexCount = static_cast<int>(data.size() / 3);

    glGenVertexArrays(1, &prismVAO);
    glGenBuffers(1, &prismVBO);
    glBindVertexArray(prismVAO);
    glBindBuffer(GL_ARRAY_BUFFER, prismVBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
    glBindVertexArray(0);
}

void Game::renderDeflectorPrism(const glm::mat4& view, const glm::mat4& projection) {
    if (prismVAO == 0 || prismShader == nullptr || prismVertexCount <= 0) {
        return;
    }

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, prismCenter);

    prismShader->use();
    prismShader->setMat4("view", view);
    prismShader->setMat4("projection", projection);
    prismShader->setMat4("model", model);
    glUniform4f(glGetUniformLocation(prismShader->ID, "color"), 0.30f, 0.60f, 1.0f, 0.5f);

    // Rendu en blending classique (alpha-blend). On ne reecrit pas le depth pour que
    // les fragments derriere la pyramide restent visibles a travers elle.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBindVertexArray(prismVAO);
    glDrawArrays(GL_TRIANGLES, 0, prismVertexCount);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
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

    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    const unsigned char faceColors[6][3] = {
        {  12,  18,  30 }, // +X
        {  10,  16,  26 }, // -X
        {  16,  22,  34 }, // +Y
        {   4,   4,   7 }, // -Y
        {  11,  17,  28 }, // +Z
        {   9,  14,  24 }  // -Z
    };

    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0,
            GL_RGB,
            1,
            1,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            faceColors[i]
        );
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Game::renderSkybox(const glm::mat4& view, const glm::mat4& projection) {
    glDepthFunc(GL_LEQUAL);

    cubemapShader->use();
    cubemapShader->setMat4("projection", projection);
    cubemapShader->setMat4("view", glm::mat4(glm::mat3(view)));

    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    cubemapShader->setInt("skybox", 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}