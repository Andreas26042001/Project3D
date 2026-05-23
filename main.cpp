#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "import/stb/stb_image.h"

#include "src/shader.h"
#include "src/camera.h"
#include "src/object.h"
#include "src/Game.h"
#include "src/GameInternal.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(1.6f, 4.5f, 4.8f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool g_isPaused = false;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
Game* g_game = nullptr;
float g_groundHeight = -1.0f;
const float CAMERA_EYE_HEIGHT = 1.0f;
const float CAMERA_COLLISION_RADIUS = 0.35f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void updateCameraPhysics();
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

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Mon Projet OpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
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
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    game.SetViewportSize(framebufferWidth, framebufferHeight);
    g_groundHeight = game.GetGroundHeight();
    camera.Position.x = 1.6f;
    camera.Position.z = 4.8f;
    camera.Position.y = g_groundHeight + CAMERA_EYE_HEIGHT;
    std::cout << "Controles:" << std::endl;
    std::cout << "  F   -> tirer un projectile de lumiere" << std::endl;
    std::cout << "  P   -> pause/reprendre (libere/reprend la souris)" << std::endl;
    std::cout << "  C   -> afficher/masquer le curseur de visee" << std::endl;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        updateCameraPhysics();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!g_isPaused) {
            game.Update(deltaTime);
        }
        game.Render(camera);

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

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

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
            g_game->FireLightProjectile(camera.Position, camera.Front);
        }
    }

    keyFPressedLastFrame = keyFPressed;
}

void updateCameraPhysics() {
    if (g_isPaused) {
        return;
    }

    float supportY = g_groundHeight;
    if (g_game != nullptr) {
        supportY = g_game->GetSupportHeightAtPosition(camera.Position, CAMERA_COLLISION_RADIUS);
    }
    camera.Position.y = supportY + CAMERA_EYE_HEIGHT;

    if (g_game != nullptr && game_internal::kEnableChapter22Collision) {
        g_game->UpdateMovablePillar(camera.Position, CAMERA_COLLISION_RADIUS);
        g_game->ResolveCameraCollisions(camera.Position, CAMERA_COLLISION_RADIUS);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    if (g_game != nullptr) {
        g_game->SetViewportSize(width, height);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (g_isPaused) {
        return;
    }

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (g_isPaused) {
        return;
    }
    camera.ProcessMouseScroll(yoffset);
}

void setPauseState(GLFWwindow* window, bool paused) {
    g_isPaused = paused;
    firstMouse = true;

    if (g_isPaused) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        std::cout << "Jeu en pause." << std::endl;
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        std::cout << "Jeu repris." << std::endl;
    }
}
