#include "Game.h"
#include "GameInternal.h"
#include "shader.h"
#include "object.h"
#include "Light.h"
#include <algorithm>
#include <cmath>
#include <vector>

void Game::Update(float deltaTime) {
    time += deltaTime;
    updateExplosionParticles(deltaTime);
    if (game_internal::kEnableChapter22Collision) {
        rebuildSceneColliders();
    }
    updateBeamTarget(deltaTime);

    if (!lights.empty() && lightProjectileActive) {
        lightProjectileLifetime += deltaTime;
        float travelled = lightProjectileLifetime * lightProjectileSpeed;
        if (lightProjectileLifetime >= lightProjectileMaxLifetime || travelled >= lightProjectileMaxDistance) {
            disableLightProjectile();
            return;
        }

        const glm::vec3 currentPosition = lights[0]->position;
        const glm::vec3 targetPosition = currentPosition + lightProjectileDirection * (lightProjectileSpeed * deltaTime);
        const glm::vec3 nextPosition = targetPosition;

        // Map collision: remove projectile when leaving the playable box.
        if (nextPosition.x < -worldCollisionHalfExtent || nextPosition.x > worldCollisionHalfExtent ||
            nextPosition.y < -worldCollisionHalfExtent || nextPosition.y > worldCollisionHalfExtent ||
            nextPosition.z < -worldCollisionHalfExtent || nextPosition.z > worldCollisionHalfExtent) {
            disableLightProjectile();
            return;
        }

        const float projectileRadius = 0.06f;
        if (game_internal::kEnableChapter22Collision &&
            isSegmentCollidingWithScene(currentPosition, nextPosition, projectileRadius)) {
            disableLightProjectile();
            return;
        }

        const glm::vec3 pillarTop = centerPillarBaseCenter + glm::vec3(0.0f, centerPillarHeight, 0.0f);
        const float kCaptureRadius = 1.0f;
        if (glm::length(nextPosition - pillarTop) < kCaptureRadius) {
            lightProjectileActive = false;
            lightProjectileLifetime = 0.0f;
            lightAnchoredOnPillar = true;
            lightAnchorAnimating = true;
            lightAnchorAnimT = 0.0f;
            lightAnchorSourcePos = nextPosition;
            lightAnchorTargetPos = pillarTop + glm::vec3(0.0f, 0.18f, 0.0f);
            lights[0]->position = nextPosition;
            lightProjectileDirection = cannonDirection;
            return;
        }

        lights[0]->position = nextPosition;
    } else if (!lights.empty() && lightAnchoredOnPillar) {
        if (lightAnchorAnimating) {
            lightAnchorAnimT += deltaTime / std::max(0.0001f, lightAnchorAnimDuration);
            if (lightAnchorAnimT >= 1.0f) {
                lightAnchorAnimT = 1.0f;
                lightAnchorAnimating = false;
            }
            const float t = lightAnchorAnimT * lightAnchorAnimT * (3.0f - 2.0f * lightAnchorAnimT);
            lights[0]->position = glm::mix(lightAnchorSourcePos, lightAnchorTargetPos, t);
        } else {
            lights[0]->position = lightAnchorTargetPos;
        }
    }
}
bool Game::raySphereIntersect(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const glm::vec3& sphereCenter,
    float sphereRadius,
    float maxDistance,
    float& outDistance
) const {
    const float dirLen = glm::length(direction);
    if (dirLen < 0.0001f) {
        return false;
    }
    const glm::vec3 d = direction / dirLen;

    const glm::vec3 m = origin - sphereCenter;
    const float b = glm::dot(m, d);
    const float c = glm::dot(m, m) - sphereRadius * sphereRadius;

    // Origine en dehors et qui s'eloigne: pas de hit.
    if (c > 0.0f && b > 0.0f) {
        return false;
    }
    const float discriminant = b * b - c;
    if (discriminant < 0.0f) {
        return false;
    }

    float t = -b - std::sqrt(discriminant);
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > maxDistance) {
        return false;
    }
    outDistance = t;
    return true;
}

float Game::GetGroundHeight() const {
    return groundTopY;
}

float Game::GetSupportHeightAtPosition(const glm::vec3& cameraPosition, float cameraRadius) const {
    if (!game_internal::kEnableChapter22Collision) {
        return groundTopY;
    }

    float highestSupport = groundTopY;
    const float maxSupportY = cameraPosition.y - 0.05f;

    for (const auto& collider : sceneColliders) {
        if (!collider.collisionEnabled || !collider.canSupport) {
            continue;
        }

        if (collider.type == SceneCollider::Type::AABB) {
            float dx = std::abs(cameraPosition.x - collider.center.x);
            float dz = std::abs(cameraPosition.z - collider.center.z);
            if (dx <= collider.halfExtents.x + cameraRadius &&
                dz <= collider.halfExtents.z + cameraRadius) {
                float topY = collider.center.y + collider.halfExtents.y;
                if (topY <= maxSupportY && topY > highestSupport) {
                    highestSupport = topY;
                }
            }
        } else {
            float dx = cameraPosition.x - collider.center.x;
            float dz = cameraPosition.z - collider.center.z;
            float horizontalSq = dx * dx + dz * dz;
            float r = collider.radius + cameraRadius;
            if (horizontalSq <= r * r) {
                float topY = collider.center.y + std::sqrt(std::max(0.0f, r * r - horizontalSq));
                if (topY <= maxSupportY && topY > highestSupport) {
                    highestSupport = topY;
                }
            }
        }
    }

    return highestSupport;
}

void Game::ResolveCameraCollisions(glm::vec3& cameraPosition, float cameraRadius) const {
    if (!game_internal::kEnableChapter22Collision) {
        return;
    }

    for (const auto& collider : sceneColliders) {
        if (!collider.collisionEnabled) {
            continue;
        }

        if (collider.type == SceneCollider::Type::AABB) {
            glm::vec3 minB = collider.center - (collider.halfExtents + glm::vec3(cameraRadius));
            glm::vec3 maxB = collider.center + (collider.halfExtents + glm::vec3(cameraRadius));

            if (cameraPosition.x > minB.x && cameraPosition.x < maxB.x &&
                cameraPosition.y > minB.y && cameraPosition.y < maxB.y &&
                cameraPosition.z > minB.z && cameraPosition.z < maxB.z) {
                float dxMin = std::abs(cameraPosition.x - minB.x);
                float dxMax = std::abs(maxB.x - cameraPosition.x);
                float dzMin = std::abs(cameraPosition.z - minB.z);
                float dzMax = std::abs(maxB.z - cameraPosition.z);

                float minPen = dxMin;
                int axis = 0;
                if (dxMax < minPen) { minPen = dxMax; axis = 1; }
                if (dzMin < minPen) { minPen = dzMin; axis = 2; }
                if (dzMax < minPen) { minPen = dzMax; axis = 3; }

                if (axis == 0) cameraPosition.x = minB.x;
                if (axis == 1) cameraPosition.x = maxB.x;
                if (axis == 2) cameraPosition.z = minB.z;
                if (axis == 3) cameraPosition.z = maxB.z;
            }
        } else {
            float target = collider.radius + cameraRadius;
            glm::vec3 toCamera = cameraPosition - collider.center;
            float dist = glm::length(toCamera);
            if (dist < target) {
                if (dist < 0.0001f) {
                    toCamera = glm::vec3(1.0f, 0.0f, 0.0f);
                    dist = 1.0f;
                }
                cameraPosition = collider.center + (toCamera / dist) * target;
            }
        }
    }
}

void Game::FireLightProjectile(const glm::vec3& origin, const glm::vec3& direction) {
    if (lights.empty() || lightProjectileActive || lightAnchoredOnPillar) {
        return;
    }
    lightProjectileActive = true;
    lightProjectileLifetime = 0.0f;
    lightProjectileStart = origin + direction * 0.35f;
    lightProjectileDirection = glm::normalize(direction);
    lights[0]->position = lightProjectileStart;
    lights[0]->color = glm::vec3(0.30f, 0.60f, 1.00f);
    lights[0]->ambientStrength = 0.02f;
    lights[0]->diffuseStrength = 0.78f;
    lights[0]->specularStrength = 0.38f;
}

void Game::disableLightProjectile() {
    if (lights.empty()) {
        return;
    }
    if (lightProjectileActive) {
        spawnLightExplosion(lights[0]->position);
    }
    lightProjectileActive = false;
    lightProjectileLifetime = 0.0f;
}

void Game::syncCarriedLight(const glm::vec3& cameraPosition, const glm::vec3& cameraForward) {
    if (lights.empty() || lightProjectileActive || lightAnchoredOnPillar) {
        return;
    }

    glm::vec3 forward = glm::normalize(cameraForward);
    if (glm::length(forward) < 0.0001f) {
        forward = glm::vec3(0.0f, 0.0f, -1.0f);
    }
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    if (glm::length(right) < 0.0001f) {
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    // Carried light: slightly in front and on the right of the camera.
    lights[0]->position = cameraPosition + forward * 0.65f + right * 0.25f - glm::vec3(0.0f, 0.15f, 0.0f);
    lightProjectileDirection = forward;
}

void Game::spawnLightExplosion(const glm::vec3& position) {
    const int particleCount = 26;
    const float baseLife = 2.0f;
    const float goldenAngle = 2.39996323f;

    for (int i = 0; i < particleCount; ++i) {
        float t = (i + 0.5f) / static_cast<float>(particleCount);
        float y = 1.0f - 2.0f * t;
        float radius = std::sqrt(std::max(0.0f, 1.0f - y * y));
        float theta = goldenAngle * static_cast<float>(i);

        glm::vec3 dir(
            radius * std::cos(theta),
            y,
            radius * std::sin(theta)
        );
        dir = glm::normalize(dir);

        float speed = 1.8f + 2.8f * t;
        ExplosionParticle p;
        p.position = position;
        p.velocity = dir * speed;
        p.color = glm::vec3(0.30f, 0.55f + 0.30f * (1.0f - t), 1.0f);
        p.life = baseLife * (0.75f + 0.35f * t);
        p.maxLife = p.life;
        p.size = 0.03f + 0.06f * (1.0f - t);
        explosionParticles.push_back(p);
    }
}

void Game::updateExplosionParticles(float deltaTime) {
    for (auto& particle : explosionParticles) {
        if (particle.life <= 0.0f) {
            continue;
        }
        particle.life -= deltaTime;
        particle.position += particle.velocity * deltaTime;
        particle.velocity *= 0.96f;
    }

    explosionParticles.erase(
        std::remove_if(
            explosionParticles.begin(),
            explosionParticles.end(),
            [](const ExplosionParticle& p) { return p.life <= 0.0f; }
        ),
        explosionParticles.end()
    );
}

void Game::renderExplosionParticles(const glm::mat4& view, const glm::mat4& projection) {
    if (explosionParticles.empty()) {
        return;
    }

    lampShader->use();
    lampShader->setMat4("view", view);
    lampShader->setMat4("projection", projection);
    lampShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
    lampShader->setVec4("clipPlane", reflectionClipPlane);

    for (const auto& particle : explosionParticles) {
        float lifeRatio = std::max(0.0f, particle.life / particle.maxLife);
        lampShader->setVec3("lightColor", particle.color * lifeRatio);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, particle.position);
        model = glm::scale(model, glm::vec3(particle.size * (0.5f + 0.5f * lifeRatio)));
        lampShader->setMat4("model", model);
        lightMarker->draw();
    }
}

void Game::rebuildCenterPillarTransform() {
    const float baseZ = centerPillarOffsetZ;
    centerPillarBaseCenter = glm::vec3(0.0f, groundTopY, baseZ);

    // Boite englobante du mesh capture_pillar.obj (Blender export) — ancrage au sol,
    // mise a l'echelle sur la meme empreinte que l'ancien cube (collision inchangee).
    const float meshXMin = -1.1012f;
    const float meshXMax = 1.1012f;
    const float meshYMin = -1.0f;
    const float meshYMax = 3.693571f;
    const float meshZMin = -1.1012f;
    const float meshZMax = 1.1012f;
    const glm::vec3 meshBottomCenter(
        (meshXMin + meshXMax) * 0.5f,
        meshYMin,
        (meshZMin + meshZMax) * 0.5f
    );
    const float meshSpanX = meshXMax - meshXMin;
    const float meshSpanY = meshYMax - meshYMin;
    const float meshSpanZ = meshZMax - meshZMin;
    const float scaleX = (centerPillarHalfWidth * 2.0f) / meshSpanX;
    const float scaleY = centerPillarHeight / meshSpanY;
    const float scaleZ = (centerPillarHalfWidth * 2.0f) / meshSpanZ;
    capturePillarModelMatrix =
        glm::translate(glm::mat4(1.0f), glm::vec3(centerPillarBaseCenter.x, groundTopY, centerPillarBaseCenter.z))
        * glm::scale(glm::mat4(1.0f), glm::vec3(scaleX, scaleY, scaleZ))
        * glm::translate(glm::mat4(1.0f), -meshBottomCenter);

    if (centerPillarColliderIndex >= 0 &&
        centerPillarColliderIndex < static_cast<int>(extraCubeModels.size())) {
        const float centerY = groundTopY + centerPillarHeight * 0.5f;
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, glm::vec3(0.0f, centerY, baseZ));
        m = glm::scale(
            m,
            glm::vec3(centerPillarHalfWidth * 2.0f, centerPillarHeight, centerPillarHalfWidth * 2.0f)
        );
        extraCubeModels[centerPillarColliderIndex] = m;
    }

    if (cannonColliderIndex >= 0 &&
        cannonColliderIndex < static_cast<int>(extraCubeModels.size())) {
        const float cannonCenterX = centerPillarHalfWidth + cannonLength * 0.5f;
        const float cannonCenterY = groundTopY + centerPillarHeight - cannonHalfWidth;
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, glm::vec3(cannonCenterX, cannonCenterY, baseZ));
        m = glm::scale(
            m,
            glm::vec3(cannonLength, cannonHalfWidth * 2.0f, cannonHalfWidth * 2.0f)
        );
        extraCubeModels[cannonColliderIndex] = m;
        cannonMuzzlePos = glm::vec3(
            centerPillarHalfWidth + cannonLength,
            cannonCenterY,
            baseZ
        );
    }

    if (centerPillarShadowIndex >= 0 &&
        centerPillarShadowIndex < static_cast<int>(pillarBaseCenters.size())) {
        pillarBaseCenters[centerPillarShadowIndex] = centerPillarBaseCenter;
    }

    if (lightAnchoredOnPillar) {
        const glm::vec3 pillarTop = centerPillarBaseCenter + glm::vec3(0.0f, centerPillarHeight, 0.0f);
        lightAnchorTargetPos = pillarTop + glm::vec3(0.0f, 0.18f, 0.0f);
    }

    // Force la regeneration des shadow maps statiques: les occluders ont bouge.
    staticShadowMapsBuilt = false;
}

void Game::rebuildDeflectorPillarTransform() {
    const float baseX = deflectorPillarBase.x + deflectorPillarOffsetX;
    const float baseZ = deflectorPillarBase.z;

    if (deflectorPillarColliderIndex >= 0 &&
        deflectorPillarColliderIndex < static_cast<int>(extraCubeModels.size())) {
        const float centerY = groundTopY + deflectorPillarHeight * 0.5f;
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, glm::vec3(baseX, centerY, baseZ));
        m = glm::scale(
            m,
            glm::vec3(centerPillarHalfWidth * 2.0f, deflectorPillarHeight, centerPillarHalfWidth * 2.0f)
        );
        extraCubeModels[deflectorPillarColliderIndex] = m;
    }

    if (deflectorPillarShadowIndex >= 0 &&
        deflectorPillarShadowIndex < static_cast<int>(pillarBaseCenters.size())) {
        pillarBaseCenters[deflectorPillarShadowIndex] = glm::vec3(baseX, groundTopY, baseZ);
    }

    // La pyramide deflectrice est ancree juste au-dessus du sommet du pilier
    // deflecteur, a la hauteur exacte du rayon principal.
    prismCenter = glm::vec3(baseX, groundTopY + centerPillarHeight - cannonHalfWidth, baseZ);

    staticShadowMapsBuilt = false;
}

void Game::UpdateMovablePillar(const glm::vec3& cameraPosition, float cameraRadius) {
    if (!game_internal::kEnableChapter22Collision) {
        return;
    }

    // Hauteur de l'oeil au-dessus des pieds. Doit rester aligne sur CAMERA_EYE_HEIGHT
    // dans main.cpp. On teste l'intersection verticale [pieds, tete] avec l'AABB du
    // pilier au lieu du seul point oeil: sans ca, pour un pilier plus petit que
    // l'oeil (deflecteur a 0.6m), l'oeil passe au-dessus de l'AABB et le push ne se
    // declenche jamais alors que le corps du joueur l'intersecte bien.
    const float kCameraBodyHeight = 1.0f;
    const float playerFeetY = cameraPosition.y - kCameraBodyHeight;
    const float playerHeadY = cameraPosition.y;

    // Pilier central: rail aligne sur Z, donc on pousse sur Z quand la penetration Z
    // est dominante. Logique fermee dans son propre bloc pour que la suite teste le
    // pilier deflecteur independamment, meme si le central n'a pas ete touche.
    if (centerPillarColliderIndex >= 0) {
        const glm::vec3 cHalf(centerPillarHalfWidth, centerPillarHeight * 0.5f, centerPillarHalfWidth);
        const glm::vec3 cCenter(
            centerPillarBaseCenter.x,
            groundTopY + centerPillarHeight * 0.5f,
            centerPillarBaseCenter.z
        );
        const glm::vec3 cExp = cHalf + glm::vec3(cameraRadius);
        const glm::vec3 cMin = cCenter - cExp;
        const glm::vec3 cMax = cCenter + cExp;

        if (cameraPosition.x > cMin.x && cameraPosition.x < cMax.x &&
            playerFeetY < cMax.y && playerHeadY > cMin.y &&
            cameraPosition.z > cMin.z && cameraPosition.z < cMax.z) {
            const float dxMin = cameraPosition.x - cMin.x;
            const float dxMax = cMax.x - cameraPosition.x;
            const float dzMin = cameraPosition.z - cMin.z;
            const float dzMax = cMax.z - cameraPosition.z;
            const float minPenX = std::min(dxMin, dxMax);
            const float minPenZ = std::min(dzMin, dzMax);

            // Pousse uniquement quand l'entree est frontale sur Z.
            if (minPenZ <= minPenX) {
                float pushDelta = (dzMin <= dzMax) ? dzMin : -dzMax;
                float newOffset = centerPillarOffsetZ + pushDelta;
                newOffset = std::max(centerPillarRailMin, std::min(centerPillarRailMax, newOffset));
                pushDelta = newOffset - centerPillarOffsetZ;
                if (std::abs(pushDelta) > 0.00001f) {
                    centerPillarOffsetZ = newOffset;
                    rebuildCenterPillarTransform();
                    rebuildSceneColliders();
                }
            }
        }
    }

    // Pilier deflecteur: rail aligne sur X, on pousse sur X quand la penetration X
    // est dominante. Independant du test du pilier central.
    if (deflectorPillarColliderIndex >= 0) {
        const glm::vec3 dHalf(centerPillarHalfWidth, deflectorPillarHeight * 0.5f, centerPillarHalfWidth);
        const glm::vec3 dCenter(
            deflectorPillarBase.x + deflectorPillarOffsetX,
            groundTopY + deflectorPillarHeight * 0.5f,
            deflectorPillarBase.z
        );
        const glm::vec3 dExp = dHalf + glm::vec3(cameraRadius);
        const glm::vec3 dMin = dCenter - dExp;
        const glm::vec3 dMax = dCenter + dExp;

        if (cameraPosition.x > dMin.x && cameraPosition.x < dMax.x &&
            playerFeetY < dMax.y && playerHeadY > dMin.y &&
            cameraPosition.z > dMin.z && cameraPosition.z < dMax.z) {
            const float ddxMin = cameraPosition.x - dMin.x;
            const float ddxMax = dMax.x - cameraPosition.x;
            const float ddzMin = cameraPosition.z - dMin.z;
            const float ddzMax = dMax.z - cameraPosition.z;
            const float dMinPenX = std::min(ddxMin, ddxMax);
            const float dMinPenZ = std::min(ddzMin, ddzMax);

            if (dMinPenX <= dMinPenZ) {
                float dPushDelta = (ddxMin <= ddxMax) ? ddxMin : -ddxMax;
                float dNewOffset = deflectorPillarOffsetX + dPushDelta;
                dNewOffset = std::max(deflectorRailMin, std::min(deflectorRailMax, dNewOffset));
                dPushDelta = dNewOffset - deflectorPillarOffsetX;
                if (std::abs(dPushDelta) > 0.00001f) {
                    deflectorPillarOffsetX = dNewOffset;
                    rebuildDeflectorPillarTransform();
                    rebuildSceneColliders();
                }
            }
        }
    }
}

void Game::rebuildSceneColliders() {
    if (!game_internal::kEnableChapter22Collision) {
        return;
    }

    sceneColliders.clear();

    for (const auto& cubeModel : extraCubeModels) {
        SceneCollider extraCubeCollider;
        extraCubeCollider.type = SceneCollider::Type::AABB;
        extraCubeCollider.collisionEnabled = true;
        extraCubeCollider.center = glm::vec3(cubeModel[3]);
        extraCubeCollider.halfExtents = glm::vec3(
            std::abs(cubeModel[0][0]) * 0.5f,
            std::abs(cubeModel[1][1]) * 0.5f,
            std::abs(cubeModel[2][2]) * 0.5f
        );
        extraCubeCollider.radius = 0.0f;
        sceneColliders.push_back(extraCubeCollider);
    }

    // Les piliers mobiles ne doivent pas servir de surface de support pour la camera:
    // sinon, en s'approchant, le joueur grimpe dessus avant que UpdateMovablePillar
    // ne le pousse. On ne peut donc plus se tenir sur le pilier deflecteur (trop bas
    // pour etre une marche credible) ni sur le pilier central recepteur. Les autres
    // tests de collision (push, blocage horizontal) restent intacts.
    if (deflectorPillarColliderIndex >= 0 &&
        deflectorPillarColliderIndex < static_cast<int>(sceneColliders.size())) {
        sceneColliders[deflectorPillarColliderIndex].canSupport = false;
    }
    if (centerPillarColliderIndex >= 0 &&
        centerPillarColliderIndex < static_cast<int>(sceneColliders.size())) {
        sceneColliders[centerPillarColliderIndex].canSupport = false;
    }

    SceneCollider groundCollider;
    groundCollider.type = SceneCollider::Type::AABB;
    groundCollider.collisionEnabled = true;
    groundCollider.center = glm::vec3(groundObject->model[3]);
    groundCollider.halfExtents = glm::vec3(
        std::abs(groundObject->model[0][0]) * 0.5f,
        std::abs(groundObject->model[1][1]) * 0.5f,
        std::abs(groundObject->model[2][2]) * 0.5f
    );
    groundCollider.radius = 0.0f;
    sceneColliders.push_back(groundCollider);
}

bool Game::isCollidingWithScene(const glm::vec3& point) const {
    for (const auto& collider : sceneColliders) {
        if (!collider.collisionEnabled) {
            continue;
        }
        if (collider.type == SceneCollider::Type::AABB) {
            glm::vec3 minB = collider.center - collider.halfExtents;
            glm::vec3 maxB = collider.center + collider.halfExtents;
            if (point.x >= minB.x && point.x <= maxB.x &&
                point.y >= minB.y && point.y <= maxB.y &&
                point.z >= minB.z && point.z <= maxB.z) {
                return true;
            }
        } else {
            if (glm::distance(point, collider.center) <= collider.radius) {
                return true;
            }
        }
    }
    return false;
}

bool Game::isSegmentCollidingWithScene(const glm::vec3& start, const glm::vec3& end, float radius) const {
    const glm::vec3 direction = end - start;
    const float segmentLength = glm::length(direction);
    if (segmentLength < 0.0001f) {
        return isCollidingWithScene(start);
    }

    const glm::vec3 dir = direction / segmentLength;
    for (const auto& collider : sceneColliders) {
        if (!collider.collisionEnabled) {
            continue;
        }

        if (collider.type == SceneCollider::Type::AABB) {
            const glm::vec3 expandedHalf = collider.halfExtents + glm::vec3(radius);
            const glm::vec3 minB = collider.center - expandedHalf;
            const glm::vec3 maxB = collider.center + expandedHalf;

            float tMin = 0.0f;
            float tMax = segmentLength;
            bool miss = false;

            for (int axis = 0; axis < 3; ++axis) {
                const float origin = start[axis];
                const float rayDir = dir[axis];
                const float minVal = minB[axis];
                const float maxVal = maxB[axis];

                if (std::abs(rayDir) < 0.00001f) {
                    if (origin < minVal || origin > maxVal) {
                        miss = true;
                        break;
                    }
                    continue;
                }

                float t1 = (minVal - origin) / rayDir;
                float t2 = (maxVal - origin) / rayDir;
                if (t1 > t2) {
                    std::swap(t1, t2);
                }

                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);
                if (tMin > tMax) {
                    miss = true;
                    break;
                }
            }

            if (!miss && tMax >= 0.0f && tMin <= segmentLength) {
                return true;
            }
        } else {
            const glm::vec3 m = start - collider.center;
            const float combinedRadius = collider.radius + radius;
            const float b = glm::dot(m, dir);
            const float c = glm::dot(m, m) - combinedRadius * combinedRadius;
            if (c <= 0.0f) {
                return true;
            }
            if (b > 0.0f) {
                continue;
            }
            const float discriminant = b * b - c;
            if (discriminant < 0.0f) {
                continue;
            }
            const float t = -b - std::sqrt(discriminant);
            if (t >= 0.0f && t <= segmentLength) {
                return true;
            }
        }
    }

    return false;
}

bool Game::raycastScene(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float maxDistance,
    int excludeColliderIndex,
    glm::vec3& outHit,
    float& outDistance
) const {
    const float dirLen = glm::length(direction);
    if (dirLen < 0.0001f || maxDistance <= 0.0f) {
        return false;
    }
    const glm::vec3 d = direction / dirLen;

    float bestT = maxDistance;
    bool hitFound = false;

    for (int i = 0; i < static_cast<int>(sceneColliders.size()); ++i) {
        if (i == excludeColliderIndex) {
            continue;
        }
        const auto& collider = sceneColliders[i];
        if (!collider.collisionEnabled) {
            continue;
        }

        if (collider.type == SceneCollider::Type::AABB) {
            const glm::vec3 minB = collider.center - collider.halfExtents;
            const glm::vec3 maxB = collider.center + collider.halfExtents;
            float tMin = 0.0f;
            float tMax = bestT;
            bool miss = false;

            for (int axis = 0; axis < 3; ++axis) {
                const float o = origin[axis];
                const float dirAxis = d[axis];
                const float minVal = minB[axis];
                const float maxVal = maxB[axis];

                if (std::abs(dirAxis) < 0.00001f) {
                    if (o < minVal || o > maxVal) {
                        miss = true;
                        break;
                    }
                    continue;
                }
                float t1 = (minVal - o) / dirAxis;
                float t2 = (maxVal - o) / dirAxis;
                if (t1 > t2) {
                    std::swap(t1, t2);
                }
                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);
                if (tMin > tMax) {
                    miss = true;
                    break;
                }
            }

            if (!miss && tMin >= 0.0f && tMin < bestT) {
                bestT = tMin;
                hitFound = true;
            }
        } else {
            const glm::vec3 m = origin - collider.center;
            const float b = glm::dot(m, d);
            const float c = glm::dot(m, m) - collider.radius * collider.radius;
            if (c > 0.0f && b > 0.0f) {
                continue;
            }
            const float discriminant = b * b - c;
            if (discriminant < 0.0f) {
                continue;
            }
            const float t = -b - std::sqrt(discriminant);
            if (t >= 0.0f && t < bestT) {
                bestT = t;
                hitFound = true;
            }
        }
    }

    if (hitFound) {
        outDistance = bestT;
        outHit = origin + d * bestT;
    }
    return hitFound;
}
