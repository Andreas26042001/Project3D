#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "shader.h"
#include "camera.h"
#include "object.h"
#include "Game.h"

#include <unistd.h>
#include <libgen.h>
#include <string.h>

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
float g_cameraVerticalVelocity = 0.0f;
const float CAMERA_EYE_HEIGHT = 1.0f;
const float CAMERA_GRAVITY = -14.0f;
const float CAMERA_JUMP_SPEED = 6.0f;
const float CAMERA_JUMP_HOLD_ACCEL = 22.0f;
const float CAMERA_MAX_JUMP_HOLD_TIME = 0.22f;
const float CAMERA_COLLISION_RADIUS = 0.35f;
bool g_jumpHeld = false;
float g_jumpHoldTimer = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void updateCameraPhysics();
void setPauseState(GLFWwindow* window, bool paused);

int main(int argc, char* argv[])
{
    // Set working directory to the executable's directory
    if (argc > 0) {
        char* exePath = strdup(argv[0]);
        char* exeDir = dirname(exePath);
        chdir(exeDir);
        free(exePath);
    }

    // Initialize GLFW
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

    // Create game instance
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
    // Spawn under the ceiling with enough headroom.
    camera.Position.y = g_groundHeight + 3.0f;
    std::cout << "Controles:" << std::endl;
    std::cout << "  I/K -> augmenter/diminuer la reflectivite de la sphere" << std::endl;
    std::cout << "  O/L -> augmenter/diminuer la refractivite du cube" << std::endl;
    std::cout << "  F   -> tirer un projectile de lumiere" << std::endl;
    std::cout << "  B   -> poser la lumiere au sol / la recuperer" << std::endl;
    std::cout << "  U/J -> augmenter/diminuer la lumiere du haut" << std::endl;
    std::cout << "  H/N -> augmenter/diminuer la portee des lumieres de coin" << std::endl;
    std::cout << "  SPACE -> saut (gravite active)" << std::endl;
    std::cout << "  P   -> pause/reprendre (libere/reprend la souris)" << std::endl;
    std::cout << "  C   -> afficher/masquer le curseur de visee" << std::endl;

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        updateCameraPhysics();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update and render game
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
    static bool keyIPressedLastFrame = false;
    static bool keyKPressedLastFrame = false;
    static bool keyOPressedLastFrame = false;
    static bool keyLPressedLastFrame = false;
    static bool keyFPressedLastFrame = false;
    static bool keyBPressedLastFrame = false;
    static bool keyUPressedLastFrame = false;
    static bool keyJPressedLastFrame = false;
    static bool keyHPressedLastFrame = false;
    static bool keyNPressedLastFrame = false;
    static bool keySpacePressedLastFrame = false;
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

    bool keyIPressed = (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS);
    bool keyKPressed = (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS);
    bool keyOPressed = (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS);
    bool keyLPressed = (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS);
    bool keyFPressed = (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS);
    bool keyBPressed = (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS);
    bool keyUPressed = (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS);
    bool keyJPressed = (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS);
    bool keyHPressed = (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS);
    bool keyNPressed = (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS);
    bool keySpacePressed = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
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
        if (keyIPressed && !keyIPressedLastFrame) {
            g_game->IncreaseSphereReflectivity(0.05f);
            std::cout << "Sphere reflectivite: " << g_game->GetSphereReflectivity() << std::endl;
        }
        if (keyKPressed && !keyKPressedLastFrame) {
            g_game->DecreaseSphereReflectivity(0.05f);
            std::cout << "Sphere reflectivite: " << g_game->GetSphereReflectivity() << std::endl;
        }
        if (keyOPressed && !keyOPressedLastFrame) {
            g_game->IncreaseCubeRefractivity(0.05f);
            std::cout << "Cube refractive index: " << g_game->GetCubeRefractiveIndex() << std::endl;
        }
        if (keyLPressed && !keyLPressedLastFrame) {
            g_game->DecreaseCubeRefractivity(0.05f);
            std::cout << "Cube refractive index: " << g_game->GetCubeRefractiveIndex() << std::endl;
        }
        if (keyFPressed && !keyFPressedLastFrame) {
            g_game->FireLightProjectile(camera.Position, camera.Front);
        }
        if (keyBPressed && !keyBPressedLastFrame) {
            g_game->PlaceTemporaryLight(camera.Position, camera.Front);
        }
        if (keyUPressed && !keyUPressedLastFrame) {
            g_game->IncreaseTopLight(0.03f);
            std::cout << "Top light strength: " << g_game->GetTopLightStrength() << std::endl;
        }
        if (keyJPressed && !keyJPressedLastFrame) {
            g_game->DecreaseTopLight(0.03f);
            std::cout << "Top light strength: " << g_game->GetTopLightStrength() << std::endl;
        }
        if (keyHPressed && !keyHPressedLastFrame) {
            g_game->IncreaseCornerLightRange(0.1f);
            std::cout << "Corner light range: " << g_game->GetCornerLightRange() << std::endl;
        }
        if (keyNPressed && !keyNPressedLastFrame) {
            g_game->DecreaseCornerLightRange(0.1f);
            std::cout << "Corner light range: " << g_game->GetCornerLightRange() << std::endl;
        }
    }

    float supportY = g_groundHeight;
    if (g_game != nullptr) {
        supportY = g_game->GetSupportHeightAtPosition(camera.Position, CAMERA_COLLISION_RADIUS);
    }
    float cameraFloorY = supportY + CAMERA_EYE_HEIGHT;
    bool onGround = (camera.Position.y <= cameraFloorY + 0.0001f);
    if (keySpacePressed && !keySpacePressedLastFrame && onGround) {
        g_cameraVerticalVelocity = CAMERA_JUMP_SPEED;
        g_jumpHeld = true;
        g_jumpHoldTimer = 0.0f;
    }
    if (!keySpacePressed) {
        g_jumpHeld = false;
    }

    keyIPressedLastFrame = keyIPressed;
    keyKPressedLastFrame = keyKPressed;
    keyOPressedLastFrame = keyOPressed;
    keyLPressedLastFrame = keyLPressed;
    keyFPressedLastFrame = keyFPressed;
    keyBPressedLastFrame = keyBPressed;
    keyUPressedLastFrame = keyUPressed;
    keyJPressedLastFrame = keyJPressed;
    keyHPressedLastFrame = keyHPressed;
    keyNPressedLastFrame = keyNPressed;
    keySpacePressedLastFrame = keySpacePressed;
}

void updateCameraPhysics() {
    if (g_isPaused) {
        return;
    }

    if (g_jumpHeld && g_jumpHoldTimer < CAMERA_MAX_JUMP_HOLD_TIME) {
        g_cameraVerticalVelocity += CAMERA_JUMP_HOLD_ACCEL * deltaTime;
        g_jumpHoldTimer += deltaTime;
    }

    g_cameraVerticalVelocity += CAMERA_GRAVITY * deltaTime;
    camera.Position.y += g_cameraVerticalVelocity * deltaTime;

    float supportY = g_groundHeight;
    if (g_game != nullptr) {
        supportY = g_game->GetSupportHeightAtPosition(camera.Position, CAMERA_COLLISION_RADIUS);
    }
    float cameraFloorY = supportY + CAMERA_EYE_HEIGHT;
    if (camera.Position.y < cameraFloorY) {
        camera.Position.y = cameraFloorY;
        g_cameraVerticalVelocity = 0.0f;
        g_jumpHeld = false;
        g_jumpHoldTimer = 0.0f;
    }

    if (g_game != nullptr) {
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