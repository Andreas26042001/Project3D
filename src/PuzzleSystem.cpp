#include "PuzzleSystem.h"
#include <glm/geometric.hpp>

void PuzzleSystem::UpdateActivation(bool target1Hit, bool target2Hit, float deltaTime) {
    auto updateOneTarget = [&](bool isHit, float& timer, bool& activated) {
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

    updateOneTarget(target1Hit, targetActivationTimer, targetActivated);
    updateOneTarget(target2Hit, target2ActivationTimer, target2Activated);
}

void PuzzleSystem::Update(
    float deltaTime,
    const LightProjectileSystem& lightProjectile,
    const BeamTrace& beam,
    const glm::vec3& targetPosition,
    const glm::vec3& target2Position,
    const glm::vec3& prismCenter,
    const glm::vec3& prismDeflectDirection,
    const glm::vec3& beamDirection,
    float targetHitTolerance
) {
    bool target1Hit = false;
    bool target2Hit = false;

    const bool beamCurrentlyActive =
        lightProjectile.anchoredOnPillar &&
        !lightProjectile.anchorAnimating;

    if (beamCurrentlyActive) {
        const float beamLen = beam.lengths[0];

        if (beam.segmentCount >= 2) {
            const float deflectedBeamLen = beam.lengths[1];

            const glm::vec3 toTarget2 =
                target2Position - prismCenter;

            const float t2 =
                glm::dot(toTarget2, prismDeflectDirection);

            if (t2 >= 0.0f && t2 <= deflectedBeamLen + 0.05f) {
                const glm::vec3 closest =
                    prismCenter + prismDeflectDirection * t2;

                if (glm::length(target2Position - closest) < targetHitTolerance) {
                    target2Hit = true;
                }
            }
        }

        const glm::vec3 toTarget =
            targetPosition - lightProjectile.position;

        const float t =
            glm::dot(toTarget, beamDirection);

        if (t >= 0.0f && t <= beamLen + 0.05f) {
            const glm::vec3 closest =
                lightProjectile.position + beamDirection * t;

            if (glm::length(targetPosition - closest) < targetHitTolerance) {
                target1Hit = true;
            }
        }
    }

    UpdateActivation(target1Hit, target2Hit, deltaTime);
}