#include "LightProjectileSystem.h"

#include <glm/gtc/constants.hpp>
#include <glm/geometric.hpp>

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