#include "Collider.h"
#include "Game.h"
#include "GameInternal.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

namespace {

glm::mat4 buildCapturePillarModelMatrix(
    const glm::vec3& baseCenter,
    float groundY,
    float halfWidth,
    float height
) {
    // Bounding box of capture_pillar.obj mesh (Blender export) — anchored to the ground
    // and scaled to the movable pillar collision footprint.
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
    const float scaleX = (halfWidth * 2.0f) / meshSpanX;
    const float scaleY = height / meshSpanY;
    const float scaleZ = (halfWidth * 2.0f) / meshSpanZ;
    return glm::translate(glm::mat4(1.0f), glm::vec3(baseCenter.x, groundY, baseCenter.z))
        * glm::scale(glm::mat4(1.0f), glm::vec3(scaleX, scaleY, scaleZ))
        * glm::translate(glm::mat4(1.0f), -meshBottomCenter);
}

} // namespace

void Game::Update(float deltaTime) {
    explosionParticles.update(deltaTime);
    rebuildSceneColliders();
    updateBeamTarget(deltaTime);

    if (lightProjectileActive) {
        lightProjectileLifetime += deltaTime;
        float travelled = lightProjectileLifetime * lightProjectileSpeed;
        if (lightProjectileLifetime >= lightProjectileMaxLifetime || travelled >= lightProjectileMaxDistance) {
            disableLightProjectile();
            return;
        }

        const glm::vec3 currentPosition = lightPosition;
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
        if (isSegmentCollidingWithScene(currentPosition, nextPosition, projectileRadius)) {
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
            lightAnchorTargetPos = pillarTop + glm::vec3(0.0f, kLightSourceOffsetY, 0.0f);
            lightPosition = nextPosition;
            lightProjectileDirection = beamDirection;
            return;
        }

        lightPosition = nextPosition;
    } else if (lightAnchoredOnPillar) {
        if (lightAnchorAnimating) {
            lightAnchorAnimT += deltaTime / std::max(0.0001f, lightAnchorAnimDuration);
            if (lightAnchorAnimT >= 1.0f) {
                lightAnchorAnimT = 1.0f;
                lightAnchorAnimating = false;
            }
            const float t = lightAnchorAnimT * lightAnchorAnimT * (3.0f - 2.0f * lightAnchorAnimT);
            lightPosition = glm::mix(lightAnchorSourcePos, lightAnchorTargetPos, t);
        } else {
            lightPosition = lightAnchorTargetPos;
        }
    }
}
bool Game::raycastScene(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float maxDistance,
    int excludeColliderIndex,
    glm::vec3& outHit,
    float& outDistance
) const {
    return colliderWorld.raycastScene(origin, direction, maxDistance, excludeColliderIndex, outHit, outDistance);
}

float Game::GetGroundHeight() const {
    return groundTopY;
}

float Game::GetSupportHeightAtPosition(const glm::vec3& cameraPosition, float cameraRadius) const {
    return colliderWorld.getSupportHeightAtPosition(cameraPosition, cameraRadius, groundTopY);
}

void Game::ResolveCameraCollisions(glm::vec3& cameraPosition, float cameraRadius) const {
    colliderWorld.resolveCameraCollisions(cameraPosition, cameraRadius);
}

void Game::FireLightProjectile(const glm::vec3& origin, const glm::vec3& direction) {
    if (lightProjectileActive || lightAnchoredOnPillar) {
        return;
    }
    lightProjectileActive = true;
    lightProjectileLifetime = 0.0f;
    lightProjectileDirection = glm::normalize(direction);
    lightPosition = origin + direction * 0.35f;
}

void Game::disableLightProjectile() {
    if (lightProjectileActive) {
        explosionParticles.spawnExplosion(lightPosition);
    }
    lightProjectileActive = false;
    lightProjectileLifetime = 0.0f;
}

void Game::syncCarriedLight(const glm::vec3& cameraPosition, const glm::vec3& cameraForward) {
    if (lightProjectileActive || lightAnchoredOnPillar) {
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
    lightPosition = cameraPosition + forward * 0.65f + right * 0.25f - glm::vec3(0.0f, 0.15f, 0.0f);
    lightProjectileDirection = forward;
}

void Game::rebuildCenterPillarTransform() {
    const float baseZ = centerPillarOffsetZ;
    centerPillarBaseCenter = glm::vec3(0.0f, groundTopY, baseZ);
    capturePillarModelMatrix = buildCapturePillarModelMatrix(
        centerPillarBaseCenter,
        groundTopY,
        centerPillarHalfWidth,
        centerPillarHeight
    );

    beamSourcePos = centerPillarBaseCenter + glm::vec3(0.0f, centerPillarHeight + kLightSourceOffsetY, 0.0f);

    lightAnchorTargetPos = beamSourcePos;
}

void Game::rebuildDeflectorPillarTransform() {
    const float baseX = deflectorPillarBase.x + deflectorPillarOffsetX;
    const float baseZ = deflectorPillarBase.z;
    const glm::vec3 deflectorBaseCenter(baseX, groundTopY, baseZ);
    deflectorPillarModelMatrix = buildCapturePillarModelMatrix(
        deflectorBaseCenter,
        groundTopY,
        centerPillarHalfWidth,
        centerPillarHeight
    );

    // Prism at beam height, above the top of the deflector pillar.
    prismCenter = glm::vec3(baseX, groundTopY + centerPillarHeight + kLightSourceOffsetY, baseZ);
}

void Game::UpdateMovablePillar(const glm::vec3& cameraPosition, float cameraRadius) {
    // Eye height above the feet. Must stay aligned with CAMERA_EYE_HEIGHT in main.cpp.
    // Test vertical intersection [feet, head] with the pillar AABB instead of the eye
    // point alone: otherwise the eye passes above the AABB and the push never triggers
    // even though the player body intersects the pillar.
    const float kCameraBodyHeight = 1.0f;
    const float playerFeetY = cameraPosition.y - kCameraBodyHeight;
    const float playerHeadY = cameraPosition.y;

    // Center pillar: rail aligned on Z, so push on Z when Z penetration is dominant.
    // Logic is self-contained so the deflector pillar is tested independently,
    // even if the center pillar was not touched.
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

            // Push only when entry is frontal on Z.
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

    // Deflector pillar: rail aligned on X, push on X when X penetration is dominant.
    // Independent from the center pillar test.
    if (deflectorPillarColliderIndex >= 0) {
        const glm::vec3 dHalf(centerPillarHalfWidth, centerPillarHeight * 0.5f, centerPillarHalfWidth);
        const glm::vec3 dCenter(
            deflectorPillarBase.x + deflectorPillarOffsetX,
            groundTopY + centerPillarHeight * 0.5f,
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
    colliderWorld.clear();
    centerPillarColliderIndex = -1;
    deflectorPillarColliderIndex = -1;

    for (const auto& cubeModel : extraCubeModels) {
        Collider extraCubeCollider;
        extraCubeCollider.center = glm::vec3(cubeModel[3]);
        extraCubeCollider.halfExtents = glm::vec3(
            std::abs(cubeModel[0][0]) * 0.5f,
            std::abs(cubeModel[1][1]) * 0.5f,
            std::abs(cubeModel[2][2]) * 0.5f
        );
        colliderWorld.add(extraCubeCollider);
    }

    auto addMovablePillarCollider = [&](const glm::vec3& center, float halfWidth, float height, int& outIndex) {
        Collider pillarCollider;
        pillarCollider.center = center;
        pillarCollider.halfExtents = glm::vec3(halfWidth, height * 0.5f, halfWidth);
        pillarCollider.canSupport = false;
        outIndex = colliderWorld.add(pillarCollider);
    };

    addMovablePillarCollider(
        glm::vec3(
            centerPillarBaseCenter.x,
            groundTopY + centerPillarHeight * 0.5f,
            centerPillarBaseCenter.z
        ),
        centerPillarHalfWidth,
        centerPillarHeight,
        centerPillarColliderIndex
    );
    addMovablePillarCollider(
        glm::vec3(
            deflectorPillarBase.x + deflectorPillarOffsetX,
            groundTopY + centerPillarHeight * 0.5f,
            deflectorPillarBase.z
        ),
        centerPillarHalfWidth,
        centerPillarHeight,
        deflectorPillarColliderIndex
    );

    Collider groundCollider;
    groundCollider.center = glm::vec3(groundObject->model[3]);
    groundCollider.halfExtents = glm::vec3(
        std::abs(groundObject->model[0][0]) * 0.5f,
        std::abs(groundObject->model[1][1]) * 0.5f,
        std::abs(groundObject->model[2][2]) * 0.5f
    );
    colliderWorld.add(groundCollider);
}

bool Game::isSegmentCollidingWithScene(const glm::vec3& start, const glm::vec3& end, float radius) const {
    return colliderWorld.isSegmentCollidingWithScene(start, end, radius);
}
