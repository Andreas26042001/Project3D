#ifndef COLLIDER_H
#define COLLIDER_H

#include <vector>
#include <glm/glm.hpp>

struct Collider {
    glm::vec3 center = glm::vec3(0.0f);
    glm::vec3 halfExtents = glm::vec3(0.0f);
    bool canSupport = true;
};

class ColliderWorld {
public:
    std::vector<Collider> colliders;

    void clear();
    int add(const Collider& collider);

    float getSupportHeightAtPosition(
        const glm::vec3& position,
        float radius,
        float groundTopY
    ) const;

    void resolveCameraCollisions(glm::vec3& position, float radius) const;

    bool isSegmentCollidingWithScene(
        const glm::vec3& start,
        const glm::vec3& end,
        float radius
    ) const;

    bool raycastScene(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance,
        int excludeColliderIndex,
        glm::vec3& outHit,
        float& outDistance
    ) const;

    static bool raySphereIntersect(
        const glm::vec3& origin,
        const glm::vec3& direction,
        const glm::vec3& sphereCenter,
        float sphereRadius,
        float maxDistance,
        float& outDistance
    );

private:
    bool isCollidingWithScene(const glm::vec3& point) const;
};

#endif
