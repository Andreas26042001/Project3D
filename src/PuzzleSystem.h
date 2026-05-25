#ifndef PUZZLE_SYSTEM_H
#define PUZZLE_SYSTEM_H

#include <glm/glm.hpp>
#include "LightProjectileSystem.h"

struct BeamTrace {
    int segmentCount = 0;
    glm::vec3 starts[2];
    glm::vec3 ends[2];
    float lengths[2] = {0.0f, 0.0f};
};

class PuzzleSystem {
public:
    bool targetActivated = false;
    bool target2Activated = false;

    float targetActivationTimer = 0.0f;
    float target2ActivationTimer = 0.0f;
    float targetActivationDuration = 1.5f;

    void UpdateActivation(bool target1Hit, bool target2Hit, float deltaTime);

    void Update(
        float deltaTime,
        const LightProjectileSystem& lightProjectile,
        const BeamTrace& beam,
        const glm::vec3& targetPosition,
        const glm::vec3& target2Position,
        const glm::vec3& prismCenter,
        const glm::vec3& prismDeflectDirection,
        const glm::vec3& beamDirection,
        float targetHitTolerance
    );
};

#endif