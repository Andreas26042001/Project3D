#include "Collider.h"

#include <algorithm>
#include <cmath>

void ColliderWorld::clear() {
    colliders.clear();
}

int ColliderWorld::add(const Collider& collider) {
    colliders.push_back(collider);
    return static_cast<int>(colliders.size()) - 1;
}

float ColliderWorld::getSupportHeightAtPosition(
    const glm::vec3& position,
    float radius,
    float groundTopY
) const {
    float highestSupport = groundTopY;
    const float maxSupportY = position.y - 0.05f;

    for (const auto& collider : colliders) {
        if (!collider.canSupport) {
            continue;
        }

        const float dx = std::abs(position.x - collider.center.x);
        const float dz = std::abs(position.z - collider.center.z);
        if (dx <= collider.halfExtents.x + radius &&
            dz <= collider.halfExtents.z + radius) {
            const float topY = collider.center.y + collider.halfExtents.y;
            if (topY <= maxSupportY && topY > highestSupport) {
                highestSupport = topY;
            }
        }
    }

    return highestSupport;
}

void ColliderWorld::resolveCameraCollisions(glm::vec3& position, float radius) const {
    for (const auto& collider : colliders) {
        const glm::vec3 minB = collider.center - (collider.halfExtents + glm::vec3(radius));
        const glm::vec3 maxB = collider.center + (collider.halfExtents + glm::vec3(radius));

        if (position.x > minB.x && position.x < maxB.x &&
            position.y > minB.y && position.y < maxB.y &&
            position.z > minB.z && position.z < maxB.z) {
            const float dxMin = std::abs(position.x - minB.x);
            const float dxMax = std::abs(maxB.x - position.x);
            const float dzMin = std::abs(position.z - minB.z);
            const float dzMax = std::abs(maxB.z - position.z);

            float minPen = dxMin;
            int axis = 0;
            if (dxMax < minPen) { minPen = dxMax; axis = 1; }
            if (dzMin < minPen) { minPen = dzMin; axis = 2; }
            if (dzMax < minPen) { minPen = dzMax; axis = 3; }

            if (axis == 0) position.x = minB.x;
            if (axis == 1) position.x = maxB.x;
            if (axis == 2) position.z = minB.z;
            if (axis == 3) position.z = maxB.z;
        }
    }
}

bool ColliderWorld::isCollidingWithScene(const glm::vec3& point) const {
    for (const auto& collider : colliders) {
        const glm::vec3 minB = collider.center - collider.halfExtents;
        const glm::vec3 maxB = collider.center + collider.halfExtents;
        if (point.x >= minB.x && point.x <= maxB.x &&
            point.y >= minB.y && point.y <= maxB.y &&
            point.z >= minB.z && point.z <= maxB.z) {
            return true;
        }
    }
    return false;
}

bool ColliderWorld::isSegmentCollidingWithScene(
    const glm::vec3& start,
    const glm::vec3& end,
    float radius
) const {
    const glm::vec3 direction = end - start;
    const float segmentLength = glm::length(direction);
    if (segmentLength < 0.0001f) {
        return isCollidingWithScene(start);
    }

    const glm::vec3 dir = direction / segmentLength;
    for (const auto& collider : colliders) {
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
    }

    return false;
}

bool ColliderWorld::raycastScene(
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

    for (int i = 0; i < static_cast<int>(colliders.size()); ++i) {
        if (i == excludeColliderIndex) {
            continue;
        }
        const auto& collider = colliders[i];
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
    }

    if (hitFound) {
        outDistance = bestT;
        outHit = origin + d * bestT;
    }
    return hitFound;
}

bool ColliderWorld::raySphereIntersect(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const glm::vec3& sphereCenter,
    float sphereRadius,
    float maxDistance,
    float& outDistance
) {
    const float dirLen = glm::length(direction);
    if (dirLen < 0.0001f) {
        return false;
    }
    const glm::vec3 d = direction / dirLen;

    const glm::vec3 m = origin - sphereCenter;
    const float b = glm::dot(m, d);
    const float c = glm::dot(m, m) - sphereRadius * sphereRadius;

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
