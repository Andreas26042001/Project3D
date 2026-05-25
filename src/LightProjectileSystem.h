#ifndef LIGHT_PROJECTILE_SYSTEM_H
#define LIGHT_PROJECTILE_SYSTEM_H

#include <functional>
#include <glm/glm.hpp>

class LightProjectileSystem {
public:
    enum class UpdateResult {
        None,
        Disabled,
        Captured
    };

    bool active = false;
    bool anchoredOnPillar = false;
    bool anchorAnimating = false;

    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};

    float lifetime = 0.0f;
    float speed = 9.0f;
    float maxLifetime = 1.8f;
    float maxDistance = 14.0f;

    float anchorAnimT = 0.0f;
    float anchorAnimDuration = 0.45f;
    glm::vec3 anchorSourcePos{0.0f};
    glm::vec3 anchorTargetPos{0.0f};

    bool CanFire() const;
    void Fire(const glm::vec3& origin, const glm::vec3& dir);
    void Disable();

    UpdateResult Update(
        float deltaTime,
        float worldCollisionHalfExtent,
        const glm::vec3& centerPillarBaseCenter,
        float centerPillarHeight,
        float lightSourceOffsetY,
        const glm::vec3& beamDirection,
        const std::function<bool(const glm::vec3&, const glm::vec3&, float)>& segmentCollision
    );
};

#endif