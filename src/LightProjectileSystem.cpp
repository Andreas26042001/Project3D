#include "LightProjectileSystem.h"

#include <glm/gtc/constants.hpp>
#include <glm/geometric.hpp>
#include <algorithm>
#include <glm/common.hpp>

bool LightProjectileSystem::CanFire() const {
    return !active && !anchoredOnPillar;
}

void LightProjectileSystem::Fire(const glm::vec3& origin, const glm::vec3& dir) {
    if (!CanFire()) {
        return;
    }

    active = true;
    lifetime = 0.0f;
    direction = glm::normalize(dir);
    position = origin + direction * 0.35f;
}

void LightProjectileSystem::Disable() {
    active = false;
    lifetime = 0.0f;
}

LightProjectileSystem::UpdateResult LightProjectileSystem::Update(
    float deltaTime,
    float worldCollisionHalfExtent,
    const glm::vec3& centerPillarBaseCenter,
    float centerPillarHeight,
    float lightSourceOffsetY,
    const glm::vec3& beamDirection,
    const std::function<bool(const glm::vec3&, const glm::vec3&, float)>& segmentCollision
) {
    if (active) {
        lifetime += deltaTime;

        float travelled = lifetime * speed;
        if (lifetime >= maxLifetime || travelled >= maxDistance) {
            Disable();
            return UpdateResult::Disabled;
        }

        const glm::vec3 currentPosition = position;
        const glm::vec3 nextPosition =
            currentPosition + direction * (speed * deltaTime);

        if (nextPosition.x < -worldCollisionHalfExtent || nextPosition.x > worldCollisionHalfExtent ||
            nextPosition.y < -worldCollisionHalfExtent || nextPosition.y > worldCollisionHalfExtent ||
            nextPosition.z < -worldCollisionHalfExtent || nextPosition.z > worldCollisionHalfExtent) {
            Disable();
            return UpdateResult::Disabled;
        }

        const float projectileRadius = 0.06f;
        if (segmentCollision(currentPosition, nextPosition, projectileRadius)) {
            Disable();
            return UpdateResult::Disabled;
        }

        const glm::vec3 pillarTop =
            centerPillarBaseCenter + glm::vec3(0.0f, centerPillarHeight, 0.0f);

        const float kCaptureRadius = 1.0f;
        if (glm::length(nextPosition - pillarTop) < kCaptureRadius) {
            active = false;
            lifetime = 0.0f;
            anchoredOnPillar = true;
            anchorAnimating = true;
            anchorAnimT = 0.0f;
            anchorSourcePos = nextPosition;
            anchorTargetPos = pillarTop + glm::vec3(0.0f, lightSourceOffsetY, 0.0f);
            position = nextPosition;
            direction = beamDirection;
            return UpdateResult::Captured;
        }

        position = nextPosition;
        return UpdateResult::None;
    }

    if (anchoredOnPillar) {
        if (anchorAnimating) {
            anchorAnimT += deltaTime / std::max(0.0001f, anchorAnimDuration);

            if (anchorAnimT >= 1.0f) {
                anchorAnimT = 1.0f;
                anchorAnimating = false;
            }

            const float t = anchorAnimT * anchorAnimT * (3.0f - 2.0f * anchorAnimT);
            position = glm::mix(anchorSourcePos, anchorTargetPos, t);
        } else {
            position = anchorTargetPos;
        }
    }

    return UpdateResult::None;
}