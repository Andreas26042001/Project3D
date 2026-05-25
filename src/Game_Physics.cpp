#include "Collider.h"
#include "Game.h"
#include "GameInternal.h"
#include <algorithm>
#include <cmath>
#include <functional>
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


static Collider buildColliderFromModel(const glm::mat4& model, bool canSupport = true) {
    Collider c;
    c.center = glm::vec3(model[3]);
    c.halfExtents = glm::vec3(
        std::abs(model[0][0]) * 0.5f,
        std::abs(model[1][1]) * 0.5f,
        std::abs(model[2][2]) * 0.5f
    );
    c.canSupport = canSupport;
    return c;
}

static Collider buildPillarCollider(const glm::vec3& center, float halfWidth, float height, bool canSupport = false) {
    Collider c;
    c.center = center;
    c.halfExtents = glm::vec3(halfWidth, height * 0.5f, halfWidth);
    c.canSupport = canSupport;
    return c;
}

static bool tryPushPillarOnRail(
    int colliderIndex,
    const glm::vec3& pillarCenter,
    float halfWidth,
    float height,
    float& offset,
    float railMin,
    float railMax,
    bool pushAlongX,
    const glm::vec3& cameraPosition,
    float cameraRadius,
    float playerFeetY,
    float playerHeadY,
    const std::function<void()>& onMoved
) {
    if (colliderIndex < 0) {
        return false;
    }

    const glm::vec3 half(halfWidth, height * 0.5f, halfWidth);
    const glm::vec3 center = pillarCenter;
    const glm::vec3 expanded = half + glm::vec3(cameraRadius);
    const glm::vec3 min = center - expanded;
    const glm::vec3 max = center + expanded;

    if (!(cameraPosition.x > min.x && cameraPosition.x < max.x &&
          playerFeetY < max.y && playerHeadY > min.y &&
          cameraPosition.z > min.z && cameraPosition.z < max.z)) {
        return false;
    }

    const float dxMin = cameraPosition.x - min.x;
    const float dxMax = max.x - cameraPosition.x;
    const float dzMin = cameraPosition.z - min.z;
    const float dzMax = max.z - cameraPosition.z;
    const float minPenX = std::min(dxMin, dxMax);
    const float minPenZ = std::min(dzMin, dzMax);

    const bool shouldPush = pushAlongX ? (minPenX <= minPenZ) : (minPenZ <= minPenX);
    if (!shouldPush) {
        return false;
    }

    float pushDelta = 0.0f;
    if (pushAlongX) {
        pushDelta = (dxMin <= dxMax) ? dxMin : -dxMax;
    } else {
        pushDelta = (dzMin <= dzMax) ? dzMin : -dzMax;
    }

    float newOffset = offset + pushDelta;
    newOffset = std::max(railMin, std::min(railMax, newOffset));
    const float actualDelta = newOffset - offset;
    if (std::abs(actualDelta) > 0.00001f) {
        offset = newOffset;
        onMoved();
        return true;
    }
    return false;
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

    const glm::vec3 centerPillarCollisionCenter(
        centerPillarBaseCenter.x,
        groundTopY + centerPillarHeight * 0.5f,
        centerPillarBaseCenter.z
    );
    tryPushPillarOnRail(
        centerPillarColliderIndex,
        centerPillarCollisionCenter,
        centerPillarHalfWidth,
        centerPillarHeight,
        centerPillarOffsetZ,
        centerPillarRailMin,
        centerPillarRailMax,
        false,
        cameraPosition,
        cameraRadius,
        playerFeetY,
        playerHeadY,
        [&]() {
            rebuildCenterPillarTransform();
            rebuildSceneColliders();
        }
    );

    const glm::vec3 deflectorPillarCollisionCenter(
        deflectorPillarBase.x + deflectorPillarOffsetX,
        groundTopY + centerPillarHeight * 0.5f,
        deflectorPillarBase.z
    );
    tryPushPillarOnRail(
        deflectorPillarColliderIndex,
        deflectorPillarCollisionCenter,
        centerPillarHalfWidth,
        centerPillarHeight,
        deflectorPillarOffsetX,
        deflectorRailMin,
        deflectorRailMax,
        true,
        cameraPosition,
        cameraRadius,
        playerFeetY,
        playerHeadY,
        [&]() {
            rebuildDeflectorPillarTransform();
            rebuildSceneColliders();
        }
    );
}

void Game::rebuildSceneColliders() {
    colliderWorld.clear();
    centerPillarColliderIndex = -1;
    deflectorPillarColliderIndex = -1;

    for (const auto& cubeModel : extraCubeModels) {
        colliderWorld.add(buildColliderFromModel(cubeModel));
    }

    centerPillarColliderIndex = colliderWorld.add(buildPillarCollider(
        glm::vec3(
            centerPillarBaseCenter.x,
            groundTopY + centerPillarHeight * 0.5f,
            centerPillarBaseCenter.z
        ),
        centerPillarHalfWidth,
        centerPillarHeight,
        false
    ));

    deflectorPillarColliderIndex = colliderWorld.add(buildPillarCollider(
        glm::vec3(
            deflectorPillarBase.x + deflectorPillarOffsetX,
            groundTopY + centerPillarHeight * 0.5f,
            deflectorPillarBase.z
        ),
        centerPillarHalfWidth,
        centerPillarHeight,
        false
    ));

    colliderWorld.add(buildColliderFromModel(groundObject->model));
}

bool Game::isSegmentCollidingWithScene(const glm::vec3& start, const glm::vec3& end, float radius) const {
    return colliderWorld.isSegmentCollidingWithScene(start, end, radius);
}
