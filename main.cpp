#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "import/stb/stb_image.h"

#include "src/Game.h"
#include "src/Player.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Player player(glm::vec3(1.6f, 4.5f, 4.8f));
bool g_isPaused = false;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
Game* g_game = nullptr;
float g_groundHeight = -1.0f;

void updateViewport(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void window_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void setPauseState(GLFWwindow* window, bool paused);

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SpacePuzzle", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Game game;
    g_game = &game;
    updateViewport(window);
    g_groundHeight = game.GetGroundHeight();
    player.camera.Position.x = 1.6f;
    player.camera.Position.z = 4.8f;
    player.camera.Position.y = g_groundHeight + Player::kEyeHeight;
    std::cout << "Controls:" << std::endl;
    std::cout << "  F   -> fire a light projectile" << std::endl;
    std::cout << "  P   -> pause/resume (release/recapture mouse)" << std::endl;
    std::cout << "  C   -> show/hide crosshair" << std::endl;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        player.updatePhysics(game, g_groundHeight, g_isPaused);
        updateViewport(window);

        if (!g_isPaused) {
            game.Update(deltaTime);
        }
        game.Render(player);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    static bool keyFPressedLastFrame = false;
    static bool keyPPressedLastFrame = false;
    static bool keyCPressedLastFrame = false;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (!g_isPaused) {
        player.processKeyboard(window, deltaTime);
    }

    bool keyFPressed = (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS);
    bool keyPPressed = (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS);
    bool keyCPressed = (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);

    if (keyPPressed && !keyPPressedLastFrame) {
        setPauseState(window, !g_isPaused);
    }
    if (keyCPressed && !keyCPressedLastFrame && g_game != nullptr) {
        g_game->ToggleCrosshair();
    }
    keyPPressedLastFrame = keyPPressed;
    keyCPressedLastFrame = keyCPressed;

    if (g_isPaused) {
        return;
    }

    if (g_game != nullptr) {
        if (keyFPressed && !keyFPressedLastFrame) {
            g_game->FireLightProjectile(player.camera.Position, player.camera.Front);
        }
    }

    keyFPressedLastFrame = keyFPressed;
}

void updateViewport(GLFWwindow* window) {
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        return;
    }

    glViewport(0, 0, framebufferWidth, framebufferHeight);
    if (g_game != nullptr) {
        g_game->SetViewportSize(framebufferWidth, framebufferHeight);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)width;
    (void)height;
    updateViewport(window);
}

void window_size_callback(GLFWwindow* window, int width, int height) {
    (void)width;
    (void)height;
    updateViewport(window);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    (void)window;
    if (g_isPaused) {
        return;
    }
    player.processMouseMovement(xpos, ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)window;
    if (g_isPaused) {
        return;
    }
    player.processMouseScroll(yoffset);
}

void setPauseState(GLFWwindow* window, bool paused) {
    g_isPaused = paused;
    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    player.resetMouseState(
        static_cast<double>(windowWidth) / 2.0,
        static_cast<double>(windowHeight) / 2.0
    );

    if (g_isPaused) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        std::cout << "Game paused." << std::endl;
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        std::cout << "Game resumed." << std::endl;
    }
}
