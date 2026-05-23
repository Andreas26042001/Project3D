#include "Player.h"
#include "Game.h"

#include <GLFW/glfw3.h>

Player::Player(const glm::vec3& startPosition)
    : camera(startPosition),
      lastMouseX(0.0f),
      lastMouseY(0.0f),
      firstMouse(true) {}

void Player::resetMouseState(double xpos, double ypos) {
    lastMouseX = static_cast<float>(xpos);
    lastMouseY = static_cast<float>(ypos);
    firstMouse = true;
}

void Player::processKeyboard(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }
}

void Player::processMouseMovement(double xpos, double ypos) {
    if (firstMouse) {
        lastMouseX = static_cast<float>(xpos);
        lastMouseY = static_cast<float>(ypos);
        firstMouse = false;
    }

    const float xoffset = static_cast<float>(xpos) - lastMouseX;
    const float yoffset = lastMouseY - static_cast<float>(ypos);

    lastMouseX = static_cast<float>(xpos);
    lastMouseY = static_cast<float>(ypos);

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void Player::processMouseScroll(double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void Player::updatePhysics(Game& game, float groundHeight, bool paused) {
    if (paused) {
        return;
    }

    float supportY = groundHeight;
    supportY = game.GetSupportHeightAtPosition(camera.Position, kCollisionRadius);
    camera.Position.y = supportY + kEyeHeight;

    game.UpdateMovablePillar(camera.Position, kCollisionRadius);
    game.ResolveCameraCollisions(camera.Position, kCollisionRadius);
}

glm::vec3 Player::getWorldPosition() const {
    return camera.Position
        - glm::normalize(camera.Front) * kBodyForwardOffset
        + glm::vec3(0.0f, kBodyVerticalOffset, 0.0f);
}
