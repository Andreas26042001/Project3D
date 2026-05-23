#ifndef PLAYER_H
#define PLAYER_H

#include <glm/glm.hpp>
#include "camera.h"

struct GLFWwindow;

class Game;

class Player {
public:
    static constexpr float kEyeHeight = 1.0f;
    static constexpr float kCollisionRadius = 0.35f;
    static constexpr float kBodyForwardOffset = 0.5f;
    static constexpr float kBodyVerticalOffset = -0.55f;

    Camera camera;

    Player(const glm::vec3& startPosition);

    void resetMouseState(double xpos, double ypos);
    void processKeyboard(GLFWwindow* window, float deltaTime);
    void processMouseMovement(double xpos, double ypos);
    void processMouseScroll(double yoffset);

    void updatePhysics(Game& game, float groundHeight, bool paused);

    glm::vec3 getWorldPosition() const;

private:
    float lastMouseX;
    float lastMouseY;
    bool firstMouse;
};

#endif
