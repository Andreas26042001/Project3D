#include "Game.h"
#include "shader.h"
#include "camera.h"
#include "object.h"
#include "Light.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <vector>
#include "stb_image.h"

#ifndef GAME_RESOURCE_DIR
#define GAME_RESOURCE_DIR "../resources"
#endif

namespace {
std::string resourcePath(const char* relativePath) {
    const std::string root(GAME_RESOURCE_DIR);
    if (root.empty()) {
        return relativePath;
    }
    const char last = root.back();
    if (last == '/' || last == '\\') {
        return root + relativePath;
    }
    return root + "/" + relativePath;
}

constexpr int SHADOW_MAP_SIZE = 2048;

bool fileExists(const std::string& path) {
    std::ifstream file(path.c_str());
    return file.good();
}

GLuint loadTexture2D(const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (data == nullptr) {
        std::cout << "WARNING: Impossible de charger la texture " << path << std::endl;
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 1) {
        format = GL_RED;
    } else if (channels == 4) {
        format = GL_RGBA;
    }

    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    return textureId;
}
}

Game::Game() : time(0.0f),
               lightProjectileActive(false),
               lightProjectileStart(glm::vec3(0.0f)),
               lightProjectileDirection(glm::vec3(0.0f, 0.0f, -1.0f)),
               lightProjectileLifetime(0.0f),
               lightProjectileSpeed(9.0f),
               lightProjectileMaxLifetime(1.8f),
               lightProjectileMaxDistance(14.0f),
               placedLightActive(false),
               placedLightTimer(0.0f),
               placedLightDuration(15.0f),
               placedLightVelocity(glm::vec3(0.0f)),
               placedLightGravity(-9.81f),
               placedLightRadius(0.12f),
               lightLockedOnPillar(false),
               lightLockAnimating(false),
               lightLockAnimT(0.0f),
               lightLockAnimDuration(0.45f),
               lightLockSourcePos(glm::vec3(0.0f)),
               lightLockTargetPos(glm::vec3(0.0f)),
               centerPillarBaseCenter(glm::vec3(0.0f)),
               centerPillarHeight(1.0f),
               centerPillarHalfWidth(0.2f),
               cannonMuzzlePos(glm::vec3(0.0f)),
               cannonDirection(glm::vec3(1.0f, 0.0f, 0.0f)),
               cannonLength(0.4f),
               cannonHalfWidth(0.07f),
               cannonColliderIndex(-1),
               centerPillarColliderIndex(-1),
               centerPillarShadowIndex(-1),
               centerPillarOffsetZ(0.0f),
               centerPillarRailMin(0.0f),
               centerPillarRailMax(0.0f),
               railModel(glm::mat4(1.0f)),
               targetVAO(0),
               targetVBO(0),
               targetVertexCount(0),
               targetPosition(glm::vec3(0.0f)),
               targetHitTolerance(0.18f),
               targetActivationTimer(0.0f),
               targetActivationDuration(3.0f),
               targetActivated(false),
               prismVAO(0),
               prismVBO(0),
               prismVertexCount(0),
               prismShader(nullptr),
               prismCenter(glm::vec3(0.0f)),
               prismRadius(0.33f),
               prismDeflectDirection(glm::vec3(0.0f, 0.0f, 1.0f)),
               deflectorPillarColliderIndex(-1),
               deflectorPillarShadowIndex(-1),
               deflectorPillarOffsetX(0.0f),
               deflectorRailMin(0.0f),
               deflectorRailMax(0.0f),
               deflectorPillarHeight(0.6f),
               deflectorRailModel(glm::mat4(1.0f)),
               deflectorPillarBase(glm::vec3(0.0f)),
               target2Position(glm::vec3(0.0f)),
               target2ModelMatrix(glm::mat4(1.0f)),
               target2ActivationTimer(0.0f),
               target2Activated(false),
               target1ModelMatrix(glm::mat4(1.0f)),
               worldCollisionHalfExtent(14.0f),
               groundTopY(-1.0f),
               topLightStrength(1.0f),
               cornerLightRange(12.0f),
               topLightColor(0.9f, 0.92f, 1.0f),
               reflectionClipEnabled(false),
               reflectionClipPlane(0.0f, 0.0f, 1.0f, 0.0f),
               edgeClipEnabled(false),
               edgeClipPlanes{
                   glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
                   glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
                   glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
                   glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)
               },
               crosshairVisible(true),
               groundDiffuseTexture(0),
               groundDiffuseTextureLoaded(false),
               pillarDiffuseTexture(0),
               pillarDiffuseTextureLoaded(false),
               shadowMapFBO(0),
               shadowMapTextures{0, 0, 0, 0},
               lightSpaceMatrices{
                   glm::mat4(1.0f),
                   glm::mat4(1.0f),
                   glm::mat4(1.0f),
                   glm::mat4(1.0f)
               },
               staticShadowMapsBuilt(false),
               dynamicShadowMapTexture(0),
               dynamicLightSpaceMatrix(1.0f),
               viewportWidth(800),
               viewportHeight(600) {
    // Initialize shaders
    phongShader = new Shader(resourcePath("glsl/phong.vert").c_str(), resourcePath("glsl/phong.frag").c_str());
    lampShader = new Shader(resourcePath("glsl/lamp.vert").c_str(), resourcePath("glsl/lamp.frag").c_str());
    cubemapShader = new Shader(resourcePath("glsl/cubemap.vert").c_str(), resourcePath("glsl/cubemap.frag").c_str());
    shadowDepthShader = new Shader(resourcePath("glsl/shadow_depth.vert").c_str(), resourcePath("glsl/shadow_depth.frag").c_str());
    const std::string crosshairVertexCode = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";
    const std::string crosshairFragmentCode = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 crosshairColor;
        void main() {
            FragColor = vec4(crosshairColor, 1.0);
        }
    )";
    crosshairShader = new Shader(crosshairVertexCode, crosshairFragmentCode);

    // Load objects
    Object* cube = new Object(resourcePath("models/cube.obj").c_str());
    cube->makeObject(*phongShader);
    objects.push_back(cube);

    // Rectangular pillars arranged as a full scene grid.
    const float pillarSpacing = 3.2f;
    const float pillarHalfWidth = 0.35f;
    const float pillarHeight = 5.8f;
    const float pillarCenterY = groundTopY + (pillarHeight * 0.5f);
    const int gridRadius = 3;
    for (int gx = -gridRadius; gx <= gridRadius; ++gx) {
        for (int gz = -gridRadius; gz <= gridRadius; ++gz) {
            // Le centre est reserve pour le petit pilier "receptacle" de la lampe.
            if (gx == 0 && gz == 0) {
                continue;
            }
            glm::mat4 pillarModel = glm::mat4(1.0f);
            pillarModel = glm::translate(
                pillarModel,
                glm::vec3(gx * pillarSpacing, pillarCenterY, gz * pillarSpacing)
            );
            pillarModel = glm::scale(
                pillarModel,
                glm::vec3(pillarHalfWidth * 2.0f, pillarHeight, pillarHalfWidth * 2.0f)
            );
            extraCubeModels.push_back(pillarModel);
            pillarBaseCenters.push_back(glm::vec3(gx * pillarSpacing, groundTopY, gz * pillarSpacing));
        }
    }

    // Petit pilier "receptacle" au centre, hauteur egale a l'oeil de la camera
    // (= groundTopY + CAMERA_EYE_HEIGHT). Sert de socle pour aspirer la lampe.
    // Il est mobile sur l'axe X grace au rail; on memorise son index pour
    // pouvoir mettre a jour sa matrice de transformation chaque fois qu'il glisse.
    centerPillarBaseCenter = glm::vec3(0.0f, groundTopY, 0.0f);
    centerPillarColliderIndex = static_cast<int>(extraCubeModels.size());
    extraCubeModels.push_back(glm::mat4(1.0f));
    centerPillarShadowIndex = static_cast<int>(pillarBaseCenters.size());
    pillarBaseCenters.push_back(centerPillarBaseCenter);

    // Canon horizontal accole a la face +X du petit pilier, au niveau du sommet.
    // Sert de "bouche" d'ou part le rayon lumineux quand la lampe est ancree.
    cannonDirection = glm::vec3(1.0f, 0.0f, 0.0f);
    cannonColliderIndex = static_cast<int>(extraCubeModels.size());
    extraCubeModels.push_back(glm::mat4(1.0f));

    // Rail au sol qui guide visuellement le deplacement du petit pilier.
    // Il est aligne sur Z pour etre perpendiculaire a la direction du canon (qui sort sur +X).
    // Le rail n'a ni collider ni ombre: il est rendu separement dans renderSceneOpaque.
    const float railLength = 4.0f;
    const float railHalfWidthX = 0.08f;
    const float railHalfHeightY = 0.015f;
    centerPillarRailMin = -(railLength * 0.5f) + centerPillarHalfWidth;
    centerPillarRailMax =  (railLength * 0.5f) - centerPillarHalfWidth;
    railModel = glm::mat4(1.0f);
    railModel = glm::translate(railModel, glm::vec3(0.0f, groundTopY + railHalfHeightY, 0.0f));
    railModel = glm::scale(railModel, glm::vec3(railHalfWidthX * 2.0f, railHalfHeightY * 2.0f, railLength));

    // Initialise la matrice du petit pilier et du canon a partir des parametres
    // partages, et synchronise toutes les donnees derivees (muzzle, ombre, cible
    // d'ancrage de la lampe).
    rebuildCenterPillarTransform();

    // 2eme pilier qui porte la pyramide deflectrice. Il est plus court que le pilier
    // central pour que la pyramide (plus grosse) soit clairement visible "posee"
    // dessus. Place sur la trajectoire balayee par le canon (+X), et lui-meme mobile
    // sur un rail aligne sur X (perpendiculaire a la direction du rayon devie +Z).
    const float kDeflectorPillarX = 5.5f;
    const float kDeflectorPillarZ = -1.5f;
    prismDeflectDirection = glm::vec3(0.0f, 0.0f, 1.0f);
    deflectorPillarBase = glm::vec3(kDeflectorPillarX, groundTopY, kDeflectorPillarZ);
    deflectorPillarColliderIndex = static_cast<int>(extraCubeModels.size());
    extraCubeModels.push_back(glm::mat4(1.0f));
    deflectorPillarShadowIndex = static_cast<int>(pillarBaseCenters.size());
    pillarBaseCenters.push_back(deflectorPillarBase);

    // Rail au sol qui guide le pilier deflecteur. Aligne sur X (perpendiculaire au
    // rayon devie). Comme pour le rail central, il est purement decoratif: pas de
    // collider ni d'ombre.
    const float kDeflectorRailLength = 4.0f;
    const float kDeflectorRailHalfWidthZ = 0.08f;
    const float kDeflectorRailHalfHeightY = 0.015f;
    deflectorRailMin = -(kDeflectorRailLength * 0.5f) + centerPillarHalfWidth;
    deflectorRailMax =  (kDeflectorRailLength * 0.5f) - centerPillarHalfWidth;
    deflectorRailModel = glm::mat4(1.0f);
    deflectorRailModel = glm::translate(
        deflectorRailModel,
        glm::vec3(kDeflectorPillarX, groundTopY + kDeflectorRailHalfHeightY, kDeflectorPillarZ)
    );
    deflectorRailModel = glm::scale(
        deflectorRailModel,
        glm::vec3(kDeflectorRailLength, kDeflectorRailHalfHeightY * 2.0f, kDeflectorRailHalfWidthZ * 2.0f)
    );

    // Initialise pilier deflecteur + position de la pyramide a partir des parametres.
    rebuildDeflectorPillarTransform();

    // Add a ceiling at pillar height, using the same textured cube pass.
    const float mapHalfExtent = worldCollisionHalfExtent;
    const float wallThickness = 0.6f;
    const float ceilingThickness = 0.6f;
    const float wallHeight = pillarHeight;
    const float ceilingCenterY = groundTopY + pillarHeight - (ceilingThickness * 0.5f);
    const float wallCenterY = groundTopY + (wallHeight * 0.5f);

    glm::mat4 ceilingModel = glm::mat4(1.0f);
    ceilingModel = glm::translate(ceilingModel, glm::vec3(0.0f, ceilingCenterY, 0.0f));
    ceilingModel = glm::scale(
        ceilingModel,
        glm::vec3(mapHalfExtent * 2.0f, ceilingThickness, mapHalfExtent * 2.0f)
    );
    extraCubeModels.push_back(ceilingModel);

    // Perimeter walls around the playable area.
    glm::mat4 wallNorth = glm::mat4(1.0f);
    wallNorth = glm::translate(wallNorth, glm::vec3(0.0f, wallCenterY, -mapHalfExtent));
    wallNorth = glm::scale(
        wallNorth,
        glm::vec3((mapHalfExtent * 2.0f) + wallThickness, wallHeight, wallThickness)
    );
    extraCubeModels.push_back(wallNorth);

    glm::mat4 wallSouth = glm::mat4(1.0f);
    wallSouth = glm::translate(wallSouth, glm::vec3(0.0f, wallCenterY, mapHalfExtent));
    wallSouth = glm::scale(
        wallSouth,
        glm::vec3((mapHalfExtent * 2.0f) + wallThickness, wallHeight, wallThickness)
    );
    extraCubeModels.push_back(wallSouth);

    glm::mat4 wallWest = glm::mat4(1.0f);
    wallWest = glm::translate(wallWest, glm::vec3(-mapHalfExtent, wallCenterY, 0.0f));
    wallWest = glm::scale(
        wallWest,
        glm::vec3(wallThickness, wallHeight, (mapHalfExtent * 2.0f) + wallThickness)
    );
    extraCubeModels.push_back(wallWest);

    glm::mat4 wallEast = glm::mat4(1.0f);
    wallEast = glm::translate(wallEast, glm::vec3(mapHalfExtent, wallCenterY, 0.0f));
    wallEast = glm::scale(
        wallEast,
        glm::vec3(wallThickness, wallHeight, (mapHalfExtent * 2.0f) + wallThickness)
    );
    extraCubeModels.push_back(wallEast);

    lightMarker = new Object(resourcePath("models/cube.obj").c_str());
    lightMarker->makeObject(*lampShader, false);
    groundObject = new Object(resourcePath("models/cube.obj").c_str());
    groundObject->makeObject(*phongShader);
    groundObject->model = glm::translate(groundObject->model, glm::vec3(0.0f, -1.25f, 0.0f));
    const float groundSpan = (worldCollisionHalfExtent * 2.0f) + 0.6f;
    groundObject->model = glm::scale(groundObject->model, glm::vec3(groundSpan, 0.5f, groundSpan));

    const std::vector<std::string> diffuseCandidates = {
        resourcePath("textures/ground/Ground081_1K-JPG/Ground081_1K-JPG_Color.jpg"),
        resourcePath("textures/ground/Ground081_1K-JPG_Color.jpg")
    };
    for (const auto& texturePath : diffuseCandidates) {
        if (!fileExists(texturePath)) {
            continue;
        }
        groundDiffuseTexture = loadTexture2D(texturePath);
        if (groundDiffuseTexture != 0) {
            groundDiffuseTextureLoaded = true;
            std::cout << "Texture du sol chargee: " << texturePath << std::endl;
            break;
        }
    }
    if (!groundDiffuseTextureLoaded) {
        std::cout << "INFO: aucune image diffuse de sol trouvee, rendu couleur utilise." << std::endl;
    }

    const std::vector<std::string> pillarDiffuseCandidates = {
        resourcePath("textures/pillars/Bricks075B_2K-JPG/Bricks075B_2K-JPG_Color.jpg"),
        resourcePath("textures/pillars/Bricks075B_2K-JPG_Color.jpg")
    };
    for (const auto& texturePath : pillarDiffuseCandidates) {
        if (!fileExists(texturePath)) {
            continue;
        }
        pillarDiffuseTexture = loadTexture2D(texturePath);
        if (pillarDiffuseTexture != 0) {
            pillarDiffuseTextureLoaded = true;
            std::cout << "Texture des piliers chargee: " << texturePath << std::endl;
            break;
        }
    }
    if (!pillarDiffuseTextureLoaded) {
        std::cout << "INFO: aucune image diffuse de piliers trouvee, rendu couleur utilise." << std::endl;
    }

    // Shadow maps: one depth texture per "corner" light source (Real-Time Rendering 4e,
    // §7.4 "Shadow Maps", p. 234). The textures are configured for hardware PCF (RTR4 §7.5
    // "Percentage-Closer Filtering", p. 248-249): GL_LINEAR + COMPARE_REF_TO_TEXTURE lets
    // the GPU return a bilinearly filtered comparison result in a single instruction (DX10+
    // / GLSL sampler2DShadow). CLAMP_TO_BORDER with a white (==1.0) border makes any sample
    // outside the light's frustum read as "fully lit", which is the safe choice described
    // in RTR4 §7.4 (out-of-frustum receivers should not be considered shadowed).
    glGenFramebuffers(1, &shadowMapFBO);
    glGenTextures(4, shadowMapTextures.data());
    for (int i = 0; i < 4; ++i) {
        glBindTexture(GL_TEXTURE_2D, shadowMapTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        const float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        // Hardware depth comparison: when sampled through sampler2DShadow the texel depth
        // is compared (GL_LEQUAL) against the provided reference and the 0..1 result is
        // bilinearly filtered for free — RTR4 §7.5 (p. 248-249).
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Dynamic shadow map for the player-carried light (re-rendered every frame).
    // Same hardware-PCF setup as the static corner maps — RTR4 §7.4 / §7.5.
    glGenTextures(1, &dynamicShadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, dynamicShadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float dynamicBorderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, dynamicBorderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    setupSkybox();
    setupCrosshair();
    setupBeamTarget();
    setupDeflectorPrism();

    // Setup lights
    Light* mainLight = new Light(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.30f, 0.60f, 1.00f),
        0.02f,
        0.0f,
        0.0f
    );
    lights.push_back(mainLight);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    rebuildSceneColliders();
}

Game::~Game() {
    for (auto obj : objects) delete obj;
    for (auto light : lights) delete light;
    delete phongShader;
    delete lampShader;
    delete cubemapShader;
    delete crosshairShader;
    delete shadowDepthShader;
    delete lightMarker;
    delete groundObject;
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteTextures(1, &cubemapTexture);
    glDeleteVertexArrays(1, &crosshairVAO);
    glDeleteBuffers(1, &crosshairVBO);
    if (targetVAO != 0) {
        glDeleteVertexArrays(1, &targetVAO);
    }
    if (targetVBO != 0) {
        glDeleteBuffers(1, &targetVBO);
    }
    if (prismVAO != 0) {
        glDeleteVertexArrays(1, &prismVAO);
    }
    if (prismVBO != 0) {
        glDeleteBuffers(1, &prismVBO);
    }
    delete prismShader;
    if (groundDiffuseTexture != 0) {
        glDeleteTextures(1, &groundDiffuseTexture);
    }
    if (pillarDiffuseTexture != 0) {
        glDeleteTextures(1, &pillarDiffuseTexture);
    }
    glDeleteTextures(4, shadowMapTextures.data());
    if (dynamicShadowMapTexture != 0) {
        glDeleteTextures(1, &dynamicShadowMapTexture);
    }
    if (shadowMapFBO != 0) {
        glDeleteFramebuffers(1, &shadowMapFBO);
    }
}

void Game::Update(float deltaTime) {
    time += deltaTime;
    updateExplosionParticles(deltaTime);
    rebuildSceneColliders();
    updateBeamTarget(deltaTime);

    if (!lights.empty() && lightProjectileActive) {
        lightProjectileLifetime += deltaTime;
        float travelled = lightProjectileLifetime * lightProjectileSpeed;
        if (lightProjectileLifetime >= lightProjectileMaxLifetime || travelled >= lightProjectileMaxDistance) {
            disableLightProjectile();
            return;
        }

        const glm::vec3 currentPosition = lights[0]->position;
        const glm::vec3 targetPosition = currentPosition + lightProjectileDirection * (lightProjectileSpeed * deltaTime);
        const glm::vec3 nextPosition = targetPosition;

        // Map collision: remove projectile when leaving the playable box.
        if (nextPosition.x < -worldCollisionHalfExtent || nextPosition.x > worldCollisionHalfExtent ||
            nextPosition.y < -worldCollisionHalfExtent || nextPosition.y > worldCollisionHalfExtent ||
            nextPosition.z < -worldCollisionHalfExtent || nextPosition.z > worldCollisionHalfExtent) {
            disableLightProjectile();
            return;
        }

        const float projectileRadius = 0.06f;
        if (isSegmentCollidingWithScene(currentPosition, nextPosition, projectileRadius)) {
            disableLightProjectile();
            return;
        }

        // Capture: si le projectile passe a proximite du sommet du pilier central,
        // le pilier l'aspire et l'ancre au-dessus, comme un appui sur B en visant.
        const glm::vec3 pillarTop = centerPillarBaseCenter + glm::vec3(0.0f, centerPillarHeight, 0.0f);
        const float kCaptureRadius = 1.0f;
        if (glm::length(nextPosition - pillarTop) < kCaptureRadius) {
            lightProjectileActive = false;
            lightProjectileLifetime = 0.0f;
            placedLightActive = false;
            placedLightTimer = 0.0f;
            placedLightVelocity = glm::vec3(0.0f);

            lightLockedOnPillar = true;
            lightLockAnimating = true;
            lightLockAnimT = 0.0f;
            lightLockSourcePos = nextPosition;
            lightLockTargetPos = pillarTop + glm::vec3(0.0f, 0.18f, 0.0f);
            lights[0]->position = nextPosition;
            return;
        }

        lights[0]->position = nextPosition;
    } else if (!lights.empty() && placedLightActive) {
        placedLightVelocity.y += placedLightGravity * deltaTime;
        glm::vec3 nextPosition = lights[0]->position + placedLightVelocity * deltaTime;
        resolvePlacedLightCollisions(nextPosition, placedLightVelocity);

        if (nextPosition.x < -worldCollisionHalfExtent || nextPosition.x > worldCollisionHalfExtent ||
            nextPosition.y < -worldCollisionHalfExtent || nextPosition.y > worldCollisionHalfExtent ||
            nextPosition.z < -worldCollisionHalfExtent || nextPosition.z > worldCollisionHalfExtent) {
            disablePlacedLight();
            return;
        }

        lights[0]->position = nextPosition;
    } else if (!lights.empty() && lightLockedOnPillar) {
        if (lightLockAnimating) {
            lightLockAnimT += deltaTime / std::max(0.0001f, lightLockAnimDuration);
            if (lightLockAnimT >= 1.0f) {
                lightLockAnimT = 1.0f;
                lightLockAnimating = false;
            }
            // smoothstep pour donner un effet d'aspiration progressif.
            const float t = lightLockAnimT * lightLockAnimT * (3.0f - 2.0f * lightLockAnimT);
            lights[0]->position = glm::mix(lightLockSourcePos, lightLockTargetPos, t);
        } else {
            lights[0]->position = lightLockTargetPos;
        }
    }

    if (!lights.empty()) {
        // Balance the dynamic light to reduce floor-vs-pillar contrast.
        lights[0]->color = glm::vec3(0.30f, 0.60f, 1.00f);
        lights[0]->ambientStrength = 0.02f;
        lights[0]->diffuseStrength = 0.78f;
        lights[0]->specularStrength = 0.38f;
    }
}

void Game::renderShadowMap() {
    // Standard shadow-map generation pass: render the scene from each light into a depth
    // buffer (Williams 1978, see Real-Time Rendering 4e §7.4 "Shadow Maps", p. 234).
    if (objects.empty() || shadowDepthShader == nullptr) {
        return;
    }

    const GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
    GLint previousCullFace = GL_BACK;
    glGetIntegerv(GL_CULL_FACE_MODE, &previousCullFace);

    // Canonical slope-scale + constant depth bias to fight "shadow acne". RTR4 §7.4
    // (p. 236-237) explicitly recommends OpenGL's glPolygonOffset for this: the offset is
    // added at depth-write time, scaled by the polygon's slope w.r.t. the light, exactly
    // matching the slope-scale bias described in the book. A clamped maximum (the second
    // parameter, here 4 units) avoids the runaway tangent values that happen for nearly
    // edge-on triangles (also discussed at p. 237).
    const GLboolean polyOffsetWasEnabled = glIsEnabled(GL_POLYGON_OFFSET_FILL);
    GLfloat previousPolyOffsetFactor = 0.0f;
    GLfloat previousPolyOffsetUnits = 0.0f;
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &previousPolyOffsetFactor);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &previousPolyOffsetUnits);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 4.0f);

    const glm::vec3 topCornerLights[4] = {
        glm::vec3(-12.5f, groundTopY + 3.6f, -12.5f),
        glm::vec3( 12.5f, groundTopY + 3.6f, -12.5f),
        glm::vec3(-12.5f, groundTopY + 3.6f,  12.5f),
        glm::vec3( 12.5f, groundTopY + 3.6f,  12.5f)
    };

    // The four corner torches and their occluders (pillars, walls, ceiling) never move,
    // so the depth maps are built once at startup and reused for the rest of the run.
    // This is the frame-to-frame coherence optimization described in RTR4 §7.4 p. 235
    // ("If the shadow situation does not change from frame to frame, i.e., the light and
    // shadow casters do not move relative to each other, this texture can be reused").
    if (!staticShadowMapsBuilt) {
        for (int i = 0; i < 4; ++i) {
            // Light frustum tightened to the actual receiver volume of the cave (~25x25x5).
            // RTR4 §7.4 Figure 7.11 (p. 236) shows that pulling near/far in and reducing
            // the x/y extent around the visible receivers increases the effective
            // shadow-map resolution and the z-buffer precision (cf. §4.7.2 cited at p. 236).
            // Previous frustum was a loose 48x48x60 cube wasting ~4x of the texel budget.
            const glm::vec3 shadowTarget(0.0f, groundTopY + 1.2f, 0.0f);
            const glm::mat4 lightProjection = glm::ortho(-18.0f, 18.0f, -18.0f, 18.0f, 0.5f, 45.0f);
            const glm::mat4 lightView = glm::lookAt(topCornerLights[i], shadowTarget, glm::vec3(0.0f, 1.0f, 0.0f));
            lightSpaceMatrices[i] = lightProjection * lightView;

            shadowDepthShader->use();
            shadowDepthShader->setMat4("lightSpaceMatrix", lightSpaceMatrices[i]);
            glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
            glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTextures[i], 0);
            glClear(GL_DEPTH_BUFFER_BIT);
            // Second-depth shadow mapping (Wang [1845], RTR4 §7.4 p. 238 and Figure 7.14):
            // by culling front faces and writing the back faces' depth into the shadow map,
            // self-shadow acne on lit surfaces is essentially eliminated at the cost of a
            // small risk of light leaks near silhouette edges. This works well for the closed
            // pillar cubes in this scene (they are "watertight" — see RTR4 §7.4 p. 238).
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);

            for (const auto& cubeModel : extraCubeModels) {
                shadowDepthShader->setMat4("model", cubeModel);
                objects[0]->draw();
            }

            shadowDepthShader->setMat4("model", groundObject->model);
            groundObject->draw();
        }
        staticShadowMapsBuilt = true;
    }

    if (!lights.empty()) {
        // Player-carried torch: re-rendered every frame, modeled as a spotlight with a
        // perspective frustum (RTR4 §7.4 p. 234, "if the local light is a spotlight, it
        // has a natural frustum associated with it").
        //
        // === Stable shadow maps (RTR4 §7.4 end of section, p. 239) ===
        //
        // As the carried torch moves with the camera, the projection samples a slightly
        // different set of world-space directions each frame. Texels no longer cover the
        // same world footprints and shadow edges visibly "swim" pixel by pixel between
        // frames. RTR4 §7.4 p. 239 prescribes "[forcing] each succeeding shadow map
        // generated to maintain the same relative texel beam locations in world space"
        // [Valient 1810, Tuft 1792]. The canonical recipe is written for directional
        // / orthographic lights (typically the sun in a CSM); we adapt it here to a
        // perspective spotlight via two cooperating measures:
        //
        //   (a) Snap the light position to a fine world-space grid (1 mm). Removes
        //       sub-millimeter floating-point jitter of the spotlight origin and forces
        //       identical views on still frames, which is otherwise impossible when the
        //       camera position varies by a few ULPs every frame.
        //
        //   (b) Project a fixed world-space anchor (here the cone-axis target ~8 m in
        //       front of the torch) into clip space, compute its fractional sub-texel
        //       offset on the SHADOW_MAP_SIZE grid, and apply that delta as a constant
        //       NDC translation by left-multiplying the projection by a translation
        //       matrix. Because the translation row multiplies the clip-space w, the
        //       offset survives the perspective divide as a uniform shift across the
        //       entire frustum — so the snap is exact for every fragment, not just for
        //       the anchor itself.
        //
        // Limitation explicitly inherited from §7.4: a perspective spotlight does not
        // have a constant world-space texel grid (the texel "beams" diverge from the
        // light origin), so a rotation of the spotlight inevitably re-shuffles which
        // world surfaces sit in which texel. The snap therefore stabilizes translation
        // and sub-pixel jitter, but cannot eliminate rotation-induced reshuffling.
        const glm::vec3 rawEye = lights[0]->position;
        glm::vec3 forward = lightProjectileDirection;
        if (glm::length(forward) < 0.0001f) {
            forward = glm::vec3(0.0f, -0.2f, -1.0f);
        }
        forward = glm::normalize(forward);

        // (a) Position snapping on a 1 mm world grid.
        auto snapToMillimeter = [](float v) { return std::round(v * 1000.0f) / 1000.0f; };
        const glm::vec3 shadowEye(snapToMillimeter(rawEye.x),
                                  snapToMillimeter(rawEye.y),
                                  snapToMillimeter(rawEye.z));
        const glm::vec3 shadowTarget = shadowEye + forward * 8.0f;

        const glm::mat4 dynamicProjection = glm::perspective(glm::radians(72.0f), 1.0f, 0.15f, 32.0f);
        const glm::mat4 dynamicView = glm::lookAt(shadowEye, shadowTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 viewProj = dynamicProjection * dynamicView;

        // (b) Anchor-based texel snap of the principal point in NDC.
        const glm::vec4 anchorClip = viewProj * glm::vec4(shadowTarget, 1.0f);
        if (std::abs(anchorClip.w) > 1.0e-5f) {
            const glm::vec2 anchorNDC(anchorClip.x / anchorClip.w,
                                      anchorClip.y / anchorClip.w);
            const float halfMap = static_cast<float>(SHADOW_MAP_SIZE) * 0.5f;
            const glm::vec2 anchorTexel = anchorNDC * halfMap;
            const glm::vec2 rounded(std::round(anchorTexel.x),
                                    std::round(anchorTexel.y));
            const glm::vec2 deltaNDC = (rounded - anchorTexel) / halfMap;
            glm::mat4 snap(1.0f);
            snap[3][0] = deltaNDC.x;
            snap[3][1] = deltaNDC.y;
            viewProj = snap * viewProj;
        }
        dynamicLightSpaceMatrix = viewProj;

        shadowDepthShader->use();
        shadowDepthShader->setMat4("lightSpaceMatrix", dynamicLightSpaceMatrix);
        glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dynamicShadowMapTexture, 0);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        // Only shadow-casters are rendered here: the ground is a pure receiver and is
        // intentionally omitted to save work (RTR4 §7.4 p. 234 — "only objects that can
        // cast shadows need to be rendered" into the light's view).
        for (const auto& cubeModel : extraCubeModels) {
            shadowDepthShader->setMat4("model", cubeModel);
            objects[0]->draw();
        }
    } else {
        dynamicLightSpaceMatrix = glm::mat4(1.0f);
    }

    // Restore previous render state.
    glPolygonOffset(previousPolyOffsetFactor, previousPolyOffsetUnits);
    if (!polyOffsetWasEnabled) {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    glCullFace(previousCullFace);
    if (!cullWasEnabled) {
        glDisable(GL_CULL_FACE);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Game::Render(Camera& camera) {
    syncCarriedLight(camera.Position, camera.Front);
    renderShadowMap();
    glm::mat4 projection = camera.GetProjectionMatrix();
    const glm::vec3 playerWorldPosition = camera.Position - glm::normalize(camera.Front) * 0.5f + glm::vec3(0.0f, -0.55f, 0.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewportWidth, viewportHeight);
    glClearColor(0.015f, 0.015f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDisable(GL_CULL_FACE);

    glm::mat4 view = camera.GetViewMatrix();
    renderSceneOpaque(view, projection, camera.Position, playerWorldPosition);
    renderSkybox(view, projection);
    renderCrosshair();
}

void Game::SetViewportSize(int width, int height) {
    viewportWidth = std::max(1, width);
    viewportHeight = std::max(1, height);
}

void Game::renderSceneOpaque(
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPosition,
    const glm::vec3& playerWorldPosition
) {
    // Rayons bleus: on calcule jusqu'a deux segments. Le rayon principal sort du canon
    // sur +X; s'il traverse la pyramide deflectrice, il est stoppe au point d'entree
    // et un second rayon part du centre de la pyramide sur la direction perpendiculaire.
    // Tout est calcule ici une fois pour etre reutilise par les passes phong (eclairage)
    // et le rendu des cubes lumineux.
    const glm::vec3 beamColor(0.30f, 0.60f, 1.00f);
    const float kBeamStrength = 1.8f;
    const float kMaxBeamDistance = 40.0f;
    const bool beamActive = !lights.empty() && lightLockedOnPillar && !lightLockAnimating;

    int beamCount = 0;
    glm::vec3 beamStarts[2] = {cannonMuzzlePos, cannonMuzzlePos};
    glm::vec3 beamEnds[2] = {cannonMuzzlePos, cannonMuzzlePos};
    float beamLengths[2] = {0.0f, 0.0f};

    if (beamActive) {
        glm::vec3 hit(0.0f);
        float dist = 0.0f;
        const bool hitSomething = raycastScene(
            cannonMuzzlePos, cannonDirection, kMaxBeamDistance,
            cannonColliderIndex, hit, dist
        );
        if (!hitSomething) {
            dist = kMaxBeamDistance;
            hit = cannonMuzzlePos + cannonDirection * kMaxBeamDistance;
        }

        // Intersection rayon principal <-> sphere englobante de la pyramide.
        float prismHitDist = 0.0f;
        const bool prismHit = raySphereIntersect(
            cannonMuzzlePos, cannonDirection, prismCenter,
            prismRadius, dist, prismHitDist
        );

        if (prismHit) {
            // Rayon principal coupe au point d'entree dans la pyramide.
            beamStarts[0] = cannonMuzzlePos;
            beamEnds[0]   = cannonMuzzlePos + cannonDirection * prismHitDist;
            beamLengths[0] = prismHitDist;
            beamCount = 1;

            // Rayon devie depuis le centre de la pyramide jusqu'au premier obstacle.
            glm::vec3 dHit(0.0f);
            float dDist = 0.0f;
            const bool dGotHit = raycastScene(
                prismCenter, prismDeflectDirection, kMaxBeamDistance,
                cannonColliderIndex, dHit, dDist
            );
            if (!dGotHit) {
                dDist = kMaxBeamDistance;
                dHit = prismCenter + prismDeflectDirection * kMaxBeamDistance;
            }
            beamStarts[1] = prismCenter;
            beamEnds[1]   = dHit;
            beamLengths[1] = dDist;
            beamCount = 2;
        } else {
            beamStarts[0] = cannonMuzzlePos;
            beamEnds[0]   = hit;
            beamLengths[0] = dist;
            beamCount = 1;
        }
    }

    const float topLightY = groundTopY + 3.6f;
    const glm::vec3 cornerWarmBase(1.0f, 0.82f, 0.30f);
    const glm::vec3 cornerWarmTip(1.0f, 0.92f, 0.45f);
    const float cornerFlickerA[4] = {
        std::sin(time * 2.1f + 0.2f) * 0.5f + 0.5f,
        std::sin(time * 2.9f + 1.4f) * 0.5f + 0.5f,
        std::sin(time * 1.7f + 2.5f) * 0.5f + 0.5f,
        std::sin(time * 3.2f + 3.8f) * 0.5f + 0.5f
    };
    const float cornerFlickerB[4] = {
        std::sin(time * 5.7f + 0.8f) * 0.5f + 0.5f,
        std::sin(time * 4.6f + 2.1f) * 0.5f + 0.5f,
        std::sin(time * 6.1f + 1.3f) * 0.5f + 0.5f,
        std::sin(time * 4.9f + 2.9f) * 0.5f + 0.5f
    };
    float cornerFlicker[4];
    for (int i = 0; i < 4; ++i) {
        // Random-like but deterministic and desynchronized flicker per corner.
        cornerFlicker[i] = 0.78f + 0.22f * (0.58f * cornerFlickerA[i] + 0.42f * cornerFlickerB[i]);
    }
    const float cornerFlickerMean = (cornerFlicker[0] + cornerFlicker[1] + cornerFlicker[2] + cornerFlicker[3]) * 0.25f;
    const glm::vec3 cornerLightColor = glm::mix(cornerWarmBase, cornerWarmTip, 0.3f + 0.5f * cornerFlickerMean);
    const float cornerLightStrength = topLightStrength * 2.2f;
    const float placedCornerLikeFlicker = 0.88f + 0.12f * (std::sin(time * 6.2f + 1.3f) * 0.5f + 0.5f);
    const glm::vec3 topCornerLights[4] = {
        glm::vec3(-12.5f, topLightY, -12.5f),
        glm::vec3( 12.5f, topLightY, -12.5f),
        glm::vec3(-12.5f, topLightY,  12.5f),
        glm::vec3( 12.5f, topLightY,  12.5f)
    };

    if (!objects.empty()) {
        phongShader->use();
        phongShader->setMat4("view", view);
        phongShader->setMat4("projection", projection);
        for (int i = 0; i < 4; ++i) {
            phongShader->setMat4("lightSpaceMatrices[" + std::to_string(i) + "]", lightSpaceMatrices[i]);
        }
        phongShader->setMat4("dynamicLightSpaceMatrix", dynamicLightSpaceMatrix);
        phongShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
        phongShader->setVec4("clipPlane", reflectionClipPlane);
        phongShader->setInt("useShadowMap", 1);
        for (int i = 0; i < 4; ++i) {
            phongShader->setInt("shadowMaps[" + std::to_string(i) + "]", 1 + i);
            glActiveTexture(GL_TEXTURE1 + i);
            glBindTexture(GL_TEXTURE_2D, shadowMapTextures[i]);
        }
        phongShader->setInt("dynamicShadowMap", 5);
        phongShader->setInt("dynamicShadowActive", !lights.empty() ? 1 : 0);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, dynamicShadowMapTexture);
        glActiveTexture(GL_TEXTURE0);
        phongShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
        for (int i = 0; i < 4; ++i) {
            phongShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
        }
        for (int i = 0; i < 4; ++i) {
            phongShader->setVec3("topCornerLights[" + std::to_string(i) + "]", topCornerLights[i]);
            phongShader->setFloat("topCornerFlicker[" + std::to_string(i) + "]", cornerFlicker[i]);
        }
        phongShader->setVec3("topLightColor", cornerLightColor);
        phongShader->setFloat("topLightStrength", cornerLightStrength);
        const float rangeFactor = std::max(0.2f, cornerLightRange);
        phongShader->setFloat("cornerAttLinear", 0.11f / rangeFactor);
        phongShader->setFloat("cornerAttQuadratic", 0.15f / (rangeFactor * rangeFactor));
        const bool dynamicCornerLikeActive = !lights.empty();
        phongShader->setInt("placedCornerLikeActive", dynamicCornerLikeActive ? 1 : 0);
        phongShader->setVec3("placedCornerLikePos", lights[0]->position);
        phongShader->setVec3("placedCornerLikeColor", lights[0]->color);
        phongShader->setFloat("placedCornerLikeStrength", dynamicCornerLikeActive ? 1.6f : 0.0f);
        phongShader->setFloat("placedCornerLikeFlicker", placedCornerLikeFlicker);
        phongShader->setInt("beamLightCount", beamCount);
        for (int i = 0; i < beamCount; ++i) {
            phongShader->setVec3("beamLightStarts[" + std::to_string(i) + "]", beamStarts[i]);
            phongShader->setVec3("beamLightEnds[" + std::to_string(i) + "]", beamEnds[i]);
            phongShader->setVec3("beamLightColors[" + std::to_string(i) + "]", beamColor);
            phongShader->setFloat("beamLightStrengths[" + std::to_string(i) + "]", kBeamStrength);
        }
        phongShader->setInt("useTexture", 0);
        phongShader->setInt("diffuseMap", 0);
        phongShader->setInt("isGroundPass", 0);
        phongShader->setInt("pillarShadowCount", 0);
        glUniform2f(glGetUniformLocation(phongShader->ID, "uvScale"), 1.0f, 1.0f);
        phongShader->setVec3("material.ambient", glm::vec3(0.10f, 0.10f, 0.10f));
        phongShader->setVec3("material.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));
        phongShader->setVec3("material.specular", glm::vec3(0.14f, 0.14f, 0.14f));
        phongShader->setFloat("material.shininess", 18.0f);
        lights[0]->setUniforms(*phongShader);
        if (dynamicCornerLikeActive) {
            phongShader->setFloat("light.ambient", 0.0f);
            phongShader->setFloat("light.diffuse", 0.0f);
            phongShader->setFloat("light.specular", 0.0f);
        }
        phongShader->setVec3("viewPos", cameraPosition);
        for (const auto& cubeModel : extraCubeModels) {
            if (pillarDiffuseTextureLoaded) {
                const float sx = std::abs(cubeModel[0][0]);
                const float sy = std::abs(cubeModel[1][1]);
                const float sz = std::abs(cubeModel[2][2]);

                phongShader->setInt("useTexture", 1);
                if (sy < 1.0f) {
                    // Ceiling: strong tiling on both axes to avoid giant stretched bricks.
                    glUniform2f(glGetUniformLocation(phongShader->ID, "uvScale"), 12.0f, 12.0f);
                } else if (sx > 10.0f || sz > 10.0f) {
                    // Perimeter walls: repeat a lot on length, enough on height.
                    glUniform2f(glGetUniformLocation(phongShader->ID, "uvScale"), 12.0f, 4.0f);
                } else {
                    // Pillars: tall but much thinner.
                    glUniform2f(glGetUniformLocation(phongShader->ID, "uvScale"), 1.2f, 8.0f);
                }
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, pillarDiffuseTexture);
            }
            phongShader->setMat4("model", cubeModel);
            objects[0]->draw();
        }
        if (pillarDiffuseTextureLoaded) {
            glBindTexture(GL_TEXTURE_2D, 0);
            phongShader->setInt("useTexture", 0);
        }
    }

    phongShader->use();
    phongShader->setMat4("model", groundObject->model);
    phongShader->setMat4("view", view);
    phongShader->setMat4("projection", projection);
    for (int i = 0; i < 4; ++i) {
        phongShader->setMat4("lightSpaceMatrices[" + std::to_string(i) + "]", lightSpaceMatrices[i]);
    }
    phongShader->setMat4("dynamicLightSpaceMatrix", dynamicLightSpaceMatrix);
    phongShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
    phongShader->setVec4("clipPlane", reflectionClipPlane);
    phongShader->setInt("useShadowMap", 1);
    for (int i = 0; i < 4; ++i) {
        phongShader->setInt("shadowMaps[" + std::to_string(i) + "]", 1 + i);
        glActiveTexture(GL_TEXTURE1 + i);
        glBindTexture(GL_TEXTURE_2D, shadowMapTextures[i]);
    }
    phongShader->setInt("dynamicShadowMap", 5);
    phongShader->setInt("dynamicShadowActive", !lights.empty() ? 1 : 0);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, dynamicShadowMapTexture);
    glActiveTexture(GL_TEXTURE0);
    phongShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
    for (int i = 0; i < 4; ++i) {
        phongShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
    }
    for (int i = 0; i < 4; ++i) {
        phongShader->setVec3("topCornerLights[" + std::to_string(i) + "]", topCornerLights[i]);
        phongShader->setFloat("topCornerFlicker[" + std::to_string(i) + "]", cornerFlicker[i]);
    }
    phongShader->setVec3("topLightColor", cornerLightColor);
    phongShader->setFloat("topLightStrength", cornerLightStrength);
    const float rangeFactor = std::max(0.2f, cornerLightRange);
    phongShader->setFloat("cornerAttLinear", 0.11f / rangeFactor);
    phongShader->setFloat("cornerAttQuadratic", 0.15f / (rangeFactor * rangeFactor));
    const bool dynamicCornerLikeActive = !lights.empty();
    phongShader->setInt("placedCornerLikeActive", dynamicCornerLikeActive ? 1 : 0);
    phongShader->setVec3("placedCornerLikePos", lights[0]->position);
    phongShader->setVec3("placedCornerLikeColor", lights[0]->color);
    phongShader->setFloat("placedCornerLikeStrength", dynamicCornerLikeActive ? 1.6f : 0.0f);
    phongShader->setFloat("placedCornerLikeFlicker", placedCornerLikeFlicker);
    phongShader->setInt("beamLightCount", beamCount);
    for (int i = 0; i < beamCount; ++i) {
        phongShader->setVec3("beamLightStarts[" + std::to_string(i) + "]", beamStarts[i]);
        phongShader->setVec3("beamLightEnds[" + std::to_string(i) + "]", beamEnds[i]);
        phongShader->setVec3("beamLightColors[" + std::to_string(i) + "]", beamColor);
        phongShader->setFloat("beamLightStrengths[" + std::to_string(i) + "]", kBeamStrength);
    }
    phongShader->setInt("useTexture", groundDiffuseTextureLoaded ? 1 : 0);
    phongShader->setInt("diffuseMap", 0);
    phongShader->setInt("isGroundPass", 1);
    const int maxShadowPillars = 64;
    const int shadowPillarCount = std::min(static_cast<int>(pillarBaseCenters.size()), maxShadowPillars);
    phongShader->setInt("pillarShadowCount", shadowPillarCount);
    if (shadowPillarCount > 0) {
        glUniform3fv(
            glGetUniformLocation(phongShader->ID, "pillarShadowCenters[0]"),
            shadowPillarCount,
            &pillarBaseCenters[0].x
        );
    }
    phongShader->setFloat("pillarShadowRadius", 0.85f);
    phongShader->setFloat("pillarShadowStrength", 0.16f);
    glUniform2f(glGetUniformLocation(phongShader->ID, "uvScale"), 6.0f, 6.0f);
    if (groundDiffuseTextureLoaded) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, groundDiffuseTexture);
    }
    phongShader->setVec3("material.ambient", glm::vec3(0.13f, 0.13f, 0.13f));
    phongShader->setVec3("material.diffuse", glm::vec3(0.19f, 0.19f, 0.19f));
    phongShader->setVec3("material.specular", glm::vec3(0.11f, 0.11f, 0.11f));
    phongShader->setFloat("material.shininess", 9.0f);
    lights[0]->setUniforms(*phongShader);
    if (dynamicCornerLikeActive) {
        phongShader->setFloat("light.ambient", 0.0f);
        phongShader->setFloat("light.diffuse", 0.0f);
        phongShader->setFloat("light.specular", 0.0f);
    }
    phongShader->setVec3("viewPos", cameraPosition);
    groundObject->draw();
    if (groundDiffuseTextureLoaded) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (!lights.empty()) {
        lampShader->use();
        lampShader->setMat4("view", view);
        lampShader->setMat4("projection", projection);
        lampShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
        lampShader->setVec4("clipPlane", reflectionClipPlane);
        lampShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
        for (int i = 0; i < 4; ++i) {
            lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
        }
        lampShader->setVec3("lightColor", lights[0]->color);

        glm::mat4 lightModel = glm::mat4(1.0f);
        lightModel = glm::translate(lightModel, lights[0]->position);
        lightModel = glm::scale(lightModel, glm::vec3(0.12f));
        lampShader->setMat4("model", lightModel);
        lightMarker->draw();
    }

    // Rail au sol qui guide le pilier central. Plus lumineux quand la lampe est ancree.
    {
        const glm::vec3 railIdle(0.10f, 0.18f, 0.32f);
        const glm::vec3 railActive(0.30f, 0.60f, 1.00f);
        const bool beamActive = lightLockedOnPillar && !lightLockAnimating;
        const glm::vec3 railColor = beamActive ? railActive : railIdle;
        lampShader->use();
        lampShader->setMat4("view", view);
        lampShader->setMat4("projection", projection);
        lampShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
        lampShader->setVec4("clipPlane", reflectionClipPlane);
        lampShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
        for (int i = 0; i < 4; ++i) {
            lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
        }
        lampShader->setVec3("lightColor", railColor);
        lampShader->setMat4("model", railModel);
        lightMarker->draw();
        // Rail du pilier deflecteur, perpendiculaire au precedent.
        lampShader->setMat4("model", deflectorRailModel);
        lightMarker->draw();
    }

    // Cible sur le mur Est: petit point + anneau, noir au repos, bleu apres 3 secondes
    // de touche continue par le rayon. Doit etre rendue avant le faisceau pour eviter
    // qu'il la masque visuellement.
    renderBeamTarget(view, projection);

    // Cubes fins lumineux materialisant le rayon principal et son rayon devie.
    if (beamCount > 0) {
        const float kBeamHalfWidth = 0.035f;
        lampShader->use();
        lampShader->setMat4("view", view);
        lampShader->setMat4("projection", projection);
        lampShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
        lampShader->setVec4("clipPlane", reflectionClipPlane);
        lampShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
        for (int i = 0; i < 4; ++i) {
            lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
        }
        lampShader->setVec3("lightColor", beamColor);

        for (int i = 0; i < beamCount; ++i) {
            if (beamLengths[i] <= 0.001f) {
                continue;
            }
            const glm::vec3 beamCenter = (beamStarts[i] + beamEnds[i]) * 0.5f;
            const glm::vec3 dir = beamEnds[i] - beamStarts[i];
            // Les deux rayons utilises sont alignes sur un axe canonique (X pour le
            // rayon principal, Z pour le rayon devie). On peut donc construire la
            // matrice avec un simple scale composant par composant.
            const float lenX = std::abs(dir.x) > 0.0001f ? std::abs(dir.x) : kBeamHalfWidth * 2.0f;
            const float lenY = std::abs(dir.y) > 0.0001f ? std::abs(dir.y) : kBeamHalfWidth * 2.0f;
            const float lenZ = std::abs(dir.z) > 0.0001f ? std::abs(dir.z) : kBeamHalfWidth * 2.0f;
            glm::mat4 beamModel = glm::mat4(1.0f);
            beamModel = glm::translate(beamModel, beamCenter);
            beamModel = glm::scale(beamModel, glm::vec3(lenX, lenY, lenZ));
            lampShader->setMat4("model", beamModel);
            lightMarker->draw();
        }
    }

    // Visual markers for the four fixed ceiling lights.
    lampShader->use();
    lampShader->setMat4("view", view);
    lampShader->setMat4("projection", projection);
    lampShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
    lampShader->setVec4("clipPlane", reflectionClipPlane);
    lampShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
    for (int i = 0; i < 4; ++i) {
        lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
    }
    lampShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
    for (int i = 0; i < 4; ++i) {
        lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
    }
    for (int i = 0; i < 4; ++i) {
        lampShader->setVec3("lightColor", cornerLightColor * (0.95f + 0.35f * cornerFlicker[i]));
        glm::mat4 topLightModel = glm::mat4(1.0f);
        topLightModel = glm::translate(topLightModel, topCornerLights[i]);
        topLightModel = glm::scale(topLightModel, glm::vec3(0.14f));
        lampShader->setMat4("model", topLightModel);
        lightMarker->draw();
    }

    // Character proxy: a small yellow cube following the camera from behind.
    lampShader->use();
    lampShader->setMat4("view", view);
    lampShader->setMat4("projection", projection);
    lampShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
    lampShader->setVec4("clipPlane", reflectionClipPlane);
    lampShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
    for (int i = 0; i < 4; ++i) {
        lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
    }
    lampShader->setVec3("lightColor", glm::vec3(1.0f, 0.95f, 0.25f));
    glm::mat4 playerModel = glm::mat4(1.0f);
    playerModel = glm::translate(playerModel, playerWorldPosition);
    playerModel = glm::scale(playerModel, glm::vec3(0.18f));
    lampShader->setMat4("model", playerModel);
    lightMarker->draw();

    renderExplosionParticles(view, projection);

    // Pyramide bleue translucide rendue en dernier (apres tout l'opaque + les rayons),
    // pour que le blending alpha la combine proprement avec le reste de la scene.
    renderDeflectorPrism(view, projection);
}

void Game::setupCrosshair() {
    const float crosshairSize = 0.018f;
    const float crosshairGap = 0.008f;
    const float crosshairVertices[] = {
        -crosshairSize, 0.0f,
        -crosshairGap, 0.0f,
         crosshairGap, 0.0f,
         crosshairSize, 0.0f,
         0.0f, -crosshairSize,
         0.0f, -crosshairGap,
         0.0f,  crosshairGap,
         0.0f,  crosshairSize
    };

    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &crosshairVBO);
    glBindVertexArray(crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void Game::renderCrosshair() {
    if (!crosshairVisible) {
        return;
    }

    glDisable(GL_DEPTH_TEST);
    crosshairShader->use();
    crosshairShader->setVec3("crosshairColor", glm::vec3(1.0f, 1.0f, 0.2f));
    glBindVertexArray(crosshairVAO);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 8);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void Game::ToggleCrosshair() {
    crosshairVisible = !crosshairVisible;
}

void Game::setupBeamTarget() {
    const float kCannonY = groundTopY + centerPillarHeight - cannonHalfWidth;
    const float kAvoidZFightingOffset = 0.012f;

    // Cible 1: sur la face interieure du mur Est (normale -X), atteignable par le
    // rayon principal en glissant le pilier central sur son rail Z.
    const float kWallInnerX = worldCollisionHalfExtent - 0.3f;  // mapHalfExtent - wallThickness/2
    const float kTarget1Z = 0.8f;
    targetPosition = glm::vec3(kWallInnerX - kAvoidZFightingOffset, kCannonY, kTarget1Z);
    target1ModelMatrix = glm::translate(glm::mat4(1.0f), targetPosition);

    // Cible 2: sur la face interieure du mur Sud (normale -Z), atteignable par le
    // rayon devie en glissant le pilier deflecteur sur son rail X. On vise un x dans
    // le couloir vide entre les rangees de piliers gx=1 (x in [2.85, 3.55]) et gx=2
    // (x in [6.05, 6.75]). deflectorPillarBase.x - 1.0 = 4.5 est central dans ce
    // couloir et accessible (offset = -1.0 in [-1.8, 1.8]).
    const float kWallInnerZ = worldCollisionHalfExtent - 0.3f;
    const float kTarget2X = deflectorPillarBase.x - 1.0f;
    target2Position = glm::vec3(kTarget2X, kCannonY, kWallInnerZ - kAvoidZFightingOffset);
    target2ModelMatrix = glm::translate(glm::mat4(1.0f), target2Position);
    // Le mesh est genere dans le plan local YZ (normale +X): pour aligner la cible
    // sur le mur Sud, on lui fait subir une rotation de 90 deg autour de Y.
    target2ModelMatrix = glm::rotate(target2ModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Disque central plein + anneau, dans le plan local YZ (X = 0). Les coordonnees
    // sont transformees via le model uniform au moment du rendu.
    const int N = 48;
    const float kPointRadius = 0.11f;
    const float kRingInner = 0.24f;
    const float kRingOuter = 0.30f;

    std::vector<float> verts;
    verts.reserve(static_cast<size_t>(N) * (3 + 6) * 3);

    const float kTwoPi = 6.28318530718f;
    for (int i = 0; i < N; ++i) {
        const float a0 = kTwoPi * static_cast<float>(i) / static_cast<float>(N);
        const float a1 = kTwoPi * static_cast<float>(i + 1) / static_cast<float>(N);
        const float c0 = std::cos(a0);
        const float s0 = std::sin(a0);
        const float c1 = std::cos(a1);
        const float s1 = std::sin(a1);

        // Disque central: triangle (centre, segment i, segment i+1).
        verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(0.0f);
        verts.push_back(0.0f); verts.push_back(kPointRadius * c0); verts.push_back(kPointRadius * s0);
        verts.push_back(0.0f); verts.push_back(kPointRadius * c1); verts.push_back(kPointRadius * s1);

        // Anneau: deux triangles formant un trapeze entre rayons interieur/exterieur.
        const float pIn0[3] = {0.0f, kRingInner * c0, kRingInner * s0};
        const float pOut0[3] = {0.0f, kRingOuter * c0, kRingOuter * s0};
        const float pOut1[3] = {0.0f, kRingOuter * c1, kRingOuter * s1};
        const float pIn1[3] = {0.0f, kRingInner * c1, kRingInner * s1};

        for (int k = 0; k < 3; ++k) verts.push_back(pIn0[k]);
        for (int k = 0; k < 3; ++k) verts.push_back(pOut0[k]);
        for (int k = 0; k < 3; ++k) verts.push_back(pOut1[k]);

        for (int k = 0; k < 3; ++k) verts.push_back(pIn0[k]);
        for (int k = 0; k < 3; ++k) verts.push_back(pOut1[k]);
        for (int k = 0; k < 3; ++k) verts.push_back(pIn1[k]);
    }

    targetVertexCount = static_cast<int>(verts.size() / 3);

    glGenVertexArrays(1, &targetVAO);
    glGenBuffers(1, &targetVBO);
    glBindVertexArray(targetVAO);
    glBindBuffer(GL_ARRAY_BUFFER, targetVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
    glBindVertexArray(0);
}

void Game::updateBeamTarget(float deltaTime) {
    const bool beamCurrentlyActive = !lights.empty() && lightLockedOnPillar && !lightLockAnimating;
    bool target1Hit = false;
    bool target2Hit = false;

    if (beamCurrentlyActive) {
        // Rayon principal (canon vers +X).
        glm::vec3 hit(0.0f);
        float dist = 0.0f;
        const float kMax = 60.0f;
        const bool hitSomething = raycastScene(
            cannonMuzzlePos, cannonDirection, kMax,
            cannonColliderIndex, hit, dist
        );
        const float beamLenFull = hitSomething ? dist : kMax;

        // Si la pyramide intercepte, le rayon principal s'arrete au point d'entree;
        // un rayon devie part alors du centre de la pyramide.
        float prismDist = 0.0f;
        const bool prismIntercept = raySphereIntersect(
            cannonMuzzlePos, cannonDirection, prismCenter,
            prismRadius, beamLenFull, prismDist
        );
        const float beamLen = prismIntercept ? prismDist : beamLenFull;

        // Cible 1 sur le rayon principal.
        {
            const glm::vec3 toTarget = targetPosition - cannonMuzzlePos;
            const float t = glm::dot(toTarget, cannonDirection);
            if (t >= 0.0f && t <= beamLen + 0.05f) {
                const glm::vec3 closest = cannonMuzzlePos + cannonDirection * t;
                if (glm::length(targetPosition - closest) < targetHitTolerance) {
                    target1Hit = true;
                }
            }
        }

        // Cible 2 sur le rayon devie (si la pyramide intercepte).
        if (prismIntercept) {
            glm::vec3 dHit(0.0f);
            float dDist = 0.0f;
            const bool dGotHit = raycastScene(
                prismCenter, prismDeflectDirection, kMax,
                cannonColliderIndex, dHit, dDist
            );
            const float deflectedBeamLen = dGotHit ? dDist : kMax;

            const glm::vec3 toTarget2 = target2Position - prismCenter;
            const float t2 = glm::dot(toTarget2, prismDeflectDirection);
            if (t2 >= 0.0f && t2 <= deflectedBeamLen + 0.05f) {
                const glm::vec3 closest = prismCenter + prismDeflectDirection * t2;
                if (glm::length(target2Position - closest) < targetHitTolerance) {
                    target2Hit = true;
                }
            }
        }
    }

    auto updateActivation = [&](bool isHit, float& timer, bool& activated) {
        if (isHit) {
            if (!activated) {
                timer += deltaTime;
                if (timer >= targetActivationDuration) {
                    activated = true;
                }
            }
        } else {
            activated = false;
            timer = 0.0f;
        }
    };

    updateActivation(target1Hit, targetActivationTimer, targetActivated);
    updateActivation(target2Hit, target2ActivationTimer, target2Activated);
}

void Game::renderBeamTarget(const glm::mat4& view, const glm::mat4& projection) {
    if (targetVAO == 0 || targetVertexCount <= 0) {
        return;
    }

    const glm::vec3 colorIdle(0.0f, 0.0f, 0.0f);
    const glm::vec3 colorActive(0.30f, 0.60f, 1.00f);

    lampShader->use();
    lampShader->setMat4("view", view);
    lampShader->setMat4("projection", projection);
    lampShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
    lampShader->setVec4("clipPlane", reflectionClipPlane);
    lampShader->setInt("useEdgeClipPlanes", edgeClipEnabled ? 1 : 0);
    for (int i = 0; i < 4; ++i) {
        lampShader->setVec4("edgeClipPlanes[" + std::to_string(i) + "]", edgeClipPlanes[i]);
    }
    glBindVertexArray(targetVAO);

    // Cible 1 (mur Est, rayon principal).
    lampShader->setVec3("lightColor", targetActivated ? colorActive : colorIdle);
    lampShader->setMat4("model", target1ModelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, targetVertexCount);

    // Cible 2 (mur Sud, rayon devie).
    lampShader->setVec3("lightColor", target2Activated ? colorActive : colorIdle);
    lampShader->setMat4("model", target2ModelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, targetVertexCount);

    glBindVertexArray(0);
}

void Game::setupDeflectorPrism() {
    // Shader minimal avec une couleur RGBA uniforme pour rendre la pyramide en blending.
    const std::string vert = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";
    const std::string frag = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec4 color;
        void main() {
            FragColor = color;
        }
    )";
    prismShader = new Shader(vert, frag);

    // Octaedre regulier (6 sommets aux extremites des axes, 8 faces triangulaires).
    const float r = prismRadius;
    const glm::vec3 v[6] = {
        glm::vec3(+r, 0.0f, 0.0f),
        glm::vec3(-r, 0.0f, 0.0f),
        glm::vec3(0.0f, +r, 0.0f),
        glm::vec3(0.0f, -r, 0.0f),
        glm::vec3(0.0f, 0.0f, +r),
        glm::vec3(0.0f, 0.0f, -r)
    };
    const int faces[8][3] = {
        {0, 2, 4}, {0, 4, 3}, {0, 3, 5}, {0, 5, 2},
        {1, 4, 2}, {1, 3, 4}, {1, 5, 3}, {1, 2, 5}
    };

    std::vector<float> data;
    data.reserve(8 * 3 * 3);
    for (int f = 0; f < 8; ++f) {
        for (int k = 0; k < 3; ++k) {
            const glm::vec3& vert = v[faces[f][k]];
            data.push_back(vert.x);
            data.push_back(vert.y);
            data.push_back(vert.z);
        }
    }
    prismVertexCount = static_cast<int>(data.size() / 3);

    glGenVertexArrays(1, &prismVAO);
    glGenBuffers(1, &prismVBO);
    glBindVertexArray(prismVAO);
    glBindBuffer(GL_ARRAY_BUFFER, prismVBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
    glBindVertexArray(0);
}

void Game::renderDeflectorPrism(const glm::mat4& view, const glm::mat4& projection) {
    if (prismVAO == 0 || prismShader == nullptr || prismVertexCount <= 0) {
        return;
    }

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, prismCenter);

    prismShader->use();
    prismShader->setMat4("view", view);
    prismShader->setMat4("projection", projection);
    prismShader->setMat4("model", model);
    glUniform4f(glGetUniformLocation(prismShader->ID, "color"), 0.30f, 0.60f, 1.0f, 0.5f);

    // Rendu en blending classique (alpha-blend). On ne reecrit pas le depth pour que
    // les fragments derriere la pyramide restent visibles a travers elle.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBindVertexArray(prismVAO);
    glDrawArrays(GL_TRIANGLES, 0, prismVertexCount);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

bool Game::raySphereIntersect(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const glm::vec3& sphereCenter,
    float sphereRadius,
    float maxDistance,
    float& outDistance
) const {
    const float dirLen = glm::length(direction);
    if (dirLen < 0.0001f) {
        return false;
    }
    const glm::vec3 d = direction / dirLen;

    const glm::vec3 m = origin - sphereCenter;
    const float b = glm::dot(m, d);
    const float c = glm::dot(m, m) - sphereRadius * sphereRadius;

    // Origine en dehors et qui s'eloigne: pas de hit.
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

float Game::GetGroundHeight() const {
    return groundTopY;
}

float Game::GetSupportHeightAtPosition(const glm::vec3& cameraPosition, float cameraRadius) const {
    float highestSupport = groundTopY;
    const float maxSupportY = cameraPosition.y - 0.05f;

    for (const auto& collider : sceneColliders) {
        if (!collider.collisionEnabled || !collider.canSupport) {
            continue;
        }

        if (collider.type == SceneCollider::Type::AABB) {
            float dx = std::abs(cameraPosition.x - collider.center.x);
            float dz = std::abs(cameraPosition.z - collider.center.z);
            if (dx <= collider.halfExtents.x + cameraRadius &&
                dz <= collider.halfExtents.z + cameraRadius) {
                float topY = collider.center.y + collider.halfExtents.y;
                if (topY <= maxSupportY && topY > highestSupport) {
                    highestSupport = topY;
                }
            }
        } else {
            float dx = cameraPosition.x - collider.center.x;
            float dz = cameraPosition.z - collider.center.z;
            float horizontalSq = dx * dx + dz * dz;
            float r = collider.radius + cameraRadius;
            if (horizontalSq <= r * r) {
                float topY = collider.center.y + std::sqrt(std::max(0.0f, r * r - horizontalSq));
                if (topY <= maxSupportY && topY > highestSupport) {
                    highestSupport = topY;
                }
            }
        }
    }

    return highestSupport;
}

void Game::ResolveCameraCollisions(glm::vec3& cameraPosition, float cameraRadius) const {
    for (const auto& collider : sceneColliders) {
        if (!collider.collisionEnabled) {
            continue;
        }

        if (collider.type == SceneCollider::Type::AABB) {
            glm::vec3 minB = collider.center - (collider.halfExtents + glm::vec3(cameraRadius));
            glm::vec3 maxB = collider.center + (collider.halfExtents + glm::vec3(cameraRadius));

            if (cameraPosition.x > minB.x && cameraPosition.x < maxB.x &&
                cameraPosition.y > minB.y && cameraPosition.y < maxB.y &&
                cameraPosition.z > minB.z && cameraPosition.z < maxB.z) {
                float dxMin = std::abs(cameraPosition.x - minB.x);
                float dxMax = std::abs(maxB.x - cameraPosition.x);
                float dzMin = std::abs(cameraPosition.z - minB.z);
                float dzMax = std::abs(maxB.z - cameraPosition.z);

                float minPen = dxMin;
                int axis = 0;
                if (dxMax < minPen) { minPen = dxMax; axis = 1; }
                if (dzMin < minPen) { minPen = dzMin; axis = 2; }
                if (dzMax < minPen) { minPen = dzMax; axis = 3; }

                if (axis == 0) cameraPosition.x = minB.x;
                if (axis == 1) cameraPosition.x = maxB.x;
                if (axis == 2) cameraPosition.z = minB.z;
                if (axis == 3) cameraPosition.z = maxB.z;
            }
        } else {
            float target = collider.radius + cameraRadius;
            glm::vec3 toCamera = cameraPosition - collider.center;
            float dist = glm::length(toCamera);
            if (dist < target) {
                if (dist < 0.0001f) {
                    toCamera = glm::vec3(1.0f, 0.0f, 0.0f);
                    dist = 1.0f;
                }
                cameraPosition = collider.center + (toCamera / dist) * target;
            }
        }
    }
}

void Game::FireLightProjectile(const glm::vec3& origin, const glm::vec3& direction) {
    if (lights.empty() || lightProjectileActive || placedLightActive || lightLockedOnPillar) {
        return;
    }
    lightProjectileActive = true;
    lightProjectileLifetime = 0.0f;
    lightProjectileStart = origin + direction * 0.35f;
    lightProjectileDirection = glm::normalize(direction);
    lights[0]->position = lightProjectileStart;
    lights[0]->color = glm::vec3(0.30f, 0.60f, 1.00f);
    lights[0]->ambientStrength = 0.02f;
    lights[0]->diffuseStrength = 0.78f;
    lights[0]->specularStrength = 0.38f;
}

void Game::PlaceTemporaryLight(const glm::vec3& cameraPosition, const glm::vec3& cameraForward) {
    if (lights.empty()) {
        return;
    }
    if (lightProjectileActive) {
        return;
    }

    // Si la lampe est deja ancree sur le pilier, on la relache (retour en main).
    if (lightLockedOnPillar) {
        lightLockedOnPillar = false;
        lightLockAnimating = false;
        lightLockAnimT = 0.0f;
        return;
    }

    // Detection: proche du pilier central ET on le regarde -> aspirer la lampe.
    const glm::vec3 pillarTop = centerPillarBaseCenter + glm::vec3(0.0f, centerPillarHeight, 0.0f);
    const float dx = cameraPosition.x - centerPillarBaseCenter.x;
    const float dz = cameraPosition.z - centerPillarBaseCenter.z;
    const float horizontalDist = std::sqrt(dx * dx + dz * dz);
    const float kMaxInteractDistance = 2.5f;
    const float kMinAimDot = 0.78f;

    if (horizontalDist < kMaxInteractDistance) {
        glm::vec3 toPillarTop = pillarTop - cameraPosition;
        const float lenToTop = glm::length(toPillarTop);
        if (lenToTop > 0.0001f) {
            const glm::vec3 dirToPillar = toPillarTop / lenToTop;
            glm::vec3 forward = cameraForward;
            if (glm::length(forward) > 0.0001f) {
                forward = glm::normalize(forward);
                if (glm::dot(forward, dirToPillar) > kMinAimDot) {
                    placedLightActive = false;
                    placedLightTimer = 0.0f;
                    placedLightVelocity = glm::vec3(0.0f);

                    lightLockedOnPillar = true;
                    lightLockAnimating = true;
                    lightLockAnimT = 0.0f;
                    lightLockSourcePos = lights[0]->position;
                    lightLockTargetPos = pillarTop + glm::vec3(0.0f, 0.18f, 0.0f);
                    lights[0]->color = glm::vec3(0.30f, 0.60f, 1.00f);
                    lights[0]->ambientStrength = 0.02f;
                    lights[0]->diffuseStrength = 0.78f;
                    lights[0]->specularStrength = 0.38f;
                    return;
                }
            }
        }
    }

    // Comportement par defaut: poser la lampe au sol / la recuperer.
    if (placedLightActive) {
        disablePlacedLight();
        return;
    }

    placedLightActive = true;
    placedLightTimer = 0.0f;
    placedLightVelocity = glm::vec3(0.0f);
    glm::vec3 forwardFlat(cameraForward.x, 0.0f, cameraForward.z);
    if (glm::length(forwardFlat) > 0.0001f) {
        forwardFlat = glm::normalize(forwardFlat);
    } else {
        forwardFlat = glm::vec3(0.0f, 0.0f, -1.0f);
    }
    const glm::vec3 spawnForward = forwardFlat;
    glm::vec3 placedPosition = cameraPosition + spawnForward * 0.9f;
    placedPosition.y = groundTopY + placedLightRadius;
    lights[0]->position = placedPosition;
    lights[0]->color = glm::vec3(0.30f, 0.60f, 1.00f);
    lights[0]->ambientStrength = 0.02f;
    lights[0]->diffuseStrength = 0.78f;
    lights[0]->specularStrength = 0.38f;
}

void Game::disableLightProjectile() {
    if (lights.empty()) {
        return;
    }
    if (lightProjectileActive) {
        spawnLightExplosion(lights[0]->position);
    }
    lightProjectileActive = false;
    lightProjectileLifetime = 0.0f;
}

void Game::disablePlacedLight() {
    if (lights.empty()) {
        return;
    }
    placedLightActive = false;
    placedLightTimer = 0.0f;
    placedLightVelocity = glm::vec3(0.0f);
}

void Game::syncCarriedLight(const glm::vec3& cameraPosition, const glm::vec3& cameraForward) {
    if (lights.empty() || lightProjectileActive || placedLightActive || lightLockedOnPillar) {
        return;
    }

    glm::vec3 forward = glm::normalize(cameraForward);
    if (glm::length(forward) < 0.0001f) {
        forward = glm::vec3(0.0f, 0.0f, -1.0f);
    }
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    if (glm::length(right) < 0.0001f) {
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    // Carried light: slightly in front and on the right of the camera.
    lights[0]->position = cameraPosition + forward * 0.65f + right * 0.25f - glm::vec3(0.0f, 0.15f, 0.0f);
    lightProjectileDirection = forward;
}

void Game::resolvePlacedLightCollisions(glm::vec3& position, glm::vec3& velocity) {
    for (const auto& collider : sceneColliders) {
        if (!collider.collisionEnabled) {
            continue;
        }

        if (collider.type == SceneCollider::Type::AABB) {
            glm::vec3 minB = collider.center - collider.halfExtents;
            glm::vec3 maxB = collider.center + collider.halfExtents;
            glm::vec3 closest = glm::clamp(position, minB, maxB);
            glm::vec3 delta = position - closest;
            float dist = glm::length(delta);

            if (dist < placedLightRadius) {
                glm::vec3 normal(0.0f, 1.0f, 0.0f);
                if (dist > 0.0001f) {
                    normal = delta / dist;
                }
                float penetration = placedLightRadius - dist;
                position += normal * penetration;

                if (normal.y > 0.4f && velocity.y < 0.0f) velocity.y = 0.0f;
                if (std::abs(normal.x) > 0.4f) velocity.x = 0.0f;
                if (std::abs(normal.z) > 0.4f) velocity.z = 0.0f;
            }
        } else {
            float target = collider.radius + placedLightRadius;
            glm::vec3 delta = position - collider.center;
            float dist = glm::length(delta);
            if (dist < target) {
                glm::vec3 normal(1.0f, 0.0f, 0.0f);
                if (dist > 0.0001f) {
                    normal = delta / dist;
                }
                position = collider.center + normal * target;

                float vn = glm::dot(velocity, normal);
                if (vn < 0.0f) {
                    velocity -= normal * vn;
                }
            }
        }
    }
}

void Game::spawnLightExplosion(const glm::vec3& position) {
    const int particleCount = 26;
    const float baseLife = 2.0f;
    const float goldenAngle = 2.39996323f;

    for (int i = 0; i < particleCount; ++i) {
        float t = (i + 0.5f) / static_cast<float>(particleCount);
        float y = 1.0f - 2.0f * t;
        float radius = std::sqrt(std::max(0.0f, 1.0f - y * y));
        float theta = goldenAngle * static_cast<float>(i);

        glm::vec3 dir(
            radius * std::cos(theta),
            y,
            radius * std::sin(theta)
        );
        dir = glm::normalize(dir);

        float speed = 1.8f + 2.8f * t;
        ExplosionParticle p;
        p.position = position;
        p.velocity = dir * speed;
        p.color = glm::vec3(0.30f, 0.55f + 0.30f * (1.0f - t), 1.0f);
        p.life = baseLife * (0.75f + 0.35f * t);
        p.maxLife = p.life;
        p.size = 0.03f + 0.06f * (1.0f - t);
        explosionParticles.push_back(p);
    }
}

void Game::updateExplosionParticles(float deltaTime) {
    for (auto& particle : explosionParticles) {
        if (particle.life <= 0.0f) {
            continue;
        }
        particle.life -= deltaTime;
        particle.position += particle.velocity * deltaTime;
        particle.velocity *= 0.96f;
    }

    explosionParticles.erase(
        std::remove_if(
            explosionParticles.begin(),
            explosionParticles.end(),
            [](const ExplosionParticle& p) { return p.life <= 0.0f; }
        ),
        explosionParticles.end()
    );
}

void Game::renderExplosionParticles(const glm::mat4& view, const glm::mat4& projection) {
    if (explosionParticles.empty()) {
        return;
    }

    lampShader->use();
    lampShader->setMat4("view", view);
    lampShader->setMat4("projection", projection);
    lampShader->setInt("useClipPlane", reflectionClipEnabled ? 1 : 0);
    lampShader->setVec4("clipPlane", reflectionClipPlane);

    for (const auto& particle : explosionParticles) {
        float lifeRatio = std::max(0.0f, particle.life / particle.maxLife);
        lampShader->setVec3("lightColor", particle.color * lifeRatio);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, particle.position);
        model = glm::scale(model, glm::vec3(particle.size * (0.5f + 0.5f * lifeRatio)));
        lampShader->setMat4("model", model);
        lightMarker->draw();
    }
}

void Game::rebuildCenterPillarTransform() {
    const float baseZ = centerPillarOffsetZ;
    centerPillarBaseCenter = glm::vec3(0.0f, groundTopY, baseZ);

    if (centerPillarColliderIndex >= 0 &&
        centerPillarColliderIndex < static_cast<int>(extraCubeModels.size())) {
        const float centerY = groundTopY + centerPillarHeight * 0.5f;
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, glm::vec3(0.0f, centerY, baseZ));
        m = glm::scale(
            m,
            glm::vec3(centerPillarHalfWidth * 2.0f, centerPillarHeight, centerPillarHalfWidth * 2.0f)
        );
        extraCubeModels[centerPillarColliderIndex] = m;
    }

    if (cannonColliderIndex >= 0 &&
        cannonColliderIndex < static_cast<int>(extraCubeModels.size())) {
        const float cannonCenterX = centerPillarHalfWidth + cannonLength * 0.5f;
        const float cannonCenterY = groundTopY + centerPillarHeight - cannonHalfWidth;
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, glm::vec3(cannonCenterX, cannonCenterY, baseZ));
        m = glm::scale(
            m,
            glm::vec3(cannonLength, cannonHalfWidth * 2.0f, cannonHalfWidth * 2.0f)
        );
        extraCubeModels[cannonColliderIndex] = m;
        cannonMuzzlePos = glm::vec3(
            centerPillarHalfWidth + cannonLength,
            cannonCenterY,
            baseZ
        );
    }

    if (centerPillarShadowIndex >= 0 &&
        centerPillarShadowIndex < static_cast<int>(pillarBaseCenters.size())) {
        pillarBaseCenters[centerPillarShadowIndex] = centerPillarBaseCenter;
    }

    if (lightLockedOnPillar) {
        const glm::vec3 pillarTop = centerPillarBaseCenter + glm::vec3(0.0f, centerPillarHeight, 0.0f);
        lightLockTargetPos = pillarTop + glm::vec3(0.0f, 0.18f, 0.0f);
    }

    // Force la regeneration des shadow maps statiques: les occluders ont bouge.
    staticShadowMapsBuilt = false;
}

void Game::rebuildDeflectorPillarTransform() {
    const float baseX = deflectorPillarBase.x + deflectorPillarOffsetX;
    const float baseZ = deflectorPillarBase.z;

    if (deflectorPillarColliderIndex >= 0 &&
        deflectorPillarColliderIndex < static_cast<int>(extraCubeModels.size())) {
        const float centerY = groundTopY + deflectorPillarHeight * 0.5f;
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, glm::vec3(baseX, centerY, baseZ));
        m = glm::scale(
            m,
            glm::vec3(centerPillarHalfWidth * 2.0f, deflectorPillarHeight, centerPillarHalfWidth * 2.0f)
        );
        extraCubeModels[deflectorPillarColliderIndex] = m;
    }

    if (deflectorPillarShadowIndex >= 0 &&
        deflectorPillarShadowIndex < static_cast<int>(pillarBaseCenters.size())) {
        pillarBaseCenters[deflectorPillarShadowIndex] = glm::vec3(baseX, groundTopY, baseZ);
    }

    // La pyramide deflectrice est ancree juste au-dessus du sommet du pilier
    // deflecteur, a la hauteur exacte du rayon principal.
    prismCenter = glm::vec3(baseX, groundTopY + centerPillarHeight - cannonHalfWidth, baseZ);

    staticShadowMapsBuilt = false;
}

void Game::UpdateMovablePillar(const glm::vec3& cameraPosition, float cameraRadius) {
    // Hauteur de l'oeil au-dessus des pieds. Doit rester aligne sur CAMERA_EYE_HEIGHT
    // dans main.cpp. On teste l'intersection verticale [pieds, tete] avec l'AABB du
    // pilier au lieu du seul point oeil: sans ca, pour un pilier plus petit que
    // l'oeil (deflecteur a 0.6m), l'oeil passe au-dessus de l'AABB et le push ne se
    // declenche jamais alors que le corps du joueur l'intersecte bien.
    const float kCameraBodyHeight = 1.0f;
    const float playerFeetY = cameraPosition.y - kCameraBodyHeight;
    const float playerHeadY = cameraPosition.y;

    // Pilier central: rail aligne sur Z, donc on pousse sur Z quand la penetration Z
    // est dominante. Logique fermee dans son propre bloc pour que la suite teste le
    // pilier deflecteur independamment, meme si le central n'a pas ete touche.
    if (centerPillarColliderIndex >= 0) {
        const glm::vec3 cHalf(centerPillarHalfWidth, centerPillarHeight * 0.5f, centerPillarHalfWidth);
        const glm::vec3 cCenter(
            centerPillarBaseCenter.x,
            groundTopY + centerPillarHeight * 0.5f,
            centerPillarBaseCenter.z
        );
        const glm::vec3 cExp = cHalf + glm::vec3(cameraRadius);
        const glm::vec3 cMin = cCenter - cExp;
        const glm::vec3 cMax = cCenter + cExp;

        if (cameraPosition.x > cMin.x && cameraPosition.x < cMax.x &&
            playerFeetY < cMax.y && playerHeadY > cMin.y &&
            cameraPosition.z > cMin.z && cameraPosition.z < cMax.z) {
            const float dxMin = cameraPosition.x - cMin.x;
            const float dxMax = cMax.x - cameraPosition.x;
            const float dzMin = cameraPosition.z - cMin.z;
            const float dzMax = cMax.z - cameraPosition.z;
            const float minPenX = std::min(dxMin, dxMax);
            const float minPenZ = std::min(dzMin, dzMax);

            // Pousse uniquement quand l'entree est frontale sur Z.
            if (minPenZ <= minPenX) {
                float pushDelta = (dzMin <= dzMax) ? dzMin : -dzMax;
                float newOffset = centerPillarOffsetZ + pushDelta;
                newOffset = std::max(centerPillarRailMin, std::min(centerPillarRailMax, newOffset));
                pushDelta = newOffset - centerPillarOffsetZ;
                if (std::abs(pushDelta) > 0.00001f) {
                    centerPillarOffsetZ = newOffset;
                    rebuildCenterPillarTransform();
                    rebuildSceneColliders();
                }
            }
        }
    }

    // Pilier deflecteur: rail aligne sur X, on pousse sur X quand la penetration X
    // est dominante. Independant du test du pilier central.
    if (deflectorPillarColliderIndex >= 0) {
        const glm::vec3 dHalf(centerPillarHalfWidth, deflectorPillarHeight * 0.5f, centerPillarHalfWidth);
        const glm::vec3 dCenter(
            deflectorPillarBase.x + deflectorPillarOffsetX,
            groundTopY + deflectorPillarHeight * 0.5f,
            deflectorPillarBase.z
        );
        const glm::vec3 dExp = dHalf + glm::vec3(cameraRadius);
        const glm::vec3 dMin = dCenter - dExp;
        const glm::vec3 dMax = dCenter + dExp;

        if (cameraPosition.x > dMin.x && cameraPosition.x < dMax.x &&
            playerFeetY < dMax.y && playerHeadY > dMin.y &&
            cameraPosition.z > dMin.z && cameraPosition.z < dMax.z) {
            const float ddxMin = cameraPosition.x - dMin.x;
            const float ddxMax = dMax.x - cameraPosition.x;
            const float ddzMin = cameraPosition.z - dMin.z;
            const float ddzMax = dMax.z - cameraPosition.z;
            const float dMinPenX = std::min(ddxMin, ddxMax);
            const float dMinPenZ = std::min(ddzMin, ddzMax);

            if (dMinPenX <= dMinPenZ) {
                float dPushDelta = (ddxMin <= ddxMax) ? ddxMin : -ddxMax;
                float dNewOffset = deflectorPillarOffsetX + dPushDelta;
                dNewOffset = std::max(deflectorRailMin, std::min(deflectorRailMax, dNewOffset));
                dPushDelta = dNewOffset - deflectorPillarOffsetX;
                if (std::abs(dPushDelta) > 0.00001f) {
                    deflectorPillarOffsetX = dNewOffset;
                    rebuildDeflectorPillarTransform();
                    rebuildSceneColliders();
                }
            }
        }
    }
}

void Game::rebuildSceneColliders() {
    sceneColliders.clear();

    for (const auto& cubeModel : extraCubeModels) {
        SceneCollider extraCubeCollider;
        extraCubeCollider.type = SceneCollider::Type::AABB;
        extraCubeCollider.collisionEnabled = true;
        extraCubeCollider.center = glm::vec3(cubeModel[3]);
        extraCubeCollider.halfExtents = glm::vec3(
            std::abs(cubeModel[0][0]) * 0.5f,
            std::abs(cubeModel[1][1]) * 0.5f,
            std::abs(cubeModel[2][2]) * 0.5f
        );
        extraCubeCollider.radius = 0.0f;
        sceneColliders.push_back(extraCubeCollider);
    }

    // Les piliers mobiles ne doivent pas servir de surface de support pour la camera:
    // sinon, en s'approchant, le joueur grimpe dessus avant que UpdateMovablePillar
    // ne le pousse. On ne peut donc plus se tenir sur le pilier deflecteur (trop bas
    // pour etre une marche credible) ni sur le pilier central recepteur. Les autres
    // tests de collision (push, blocage horizontal) restent intacts.
    if (deflectorPillarColliderIndex >= 0 &&
        deflectorPillarColliderIndex < static_cast<int>(sceneColliders.size())) {
        sceneColliders[deflectorPillarColliderIndex].canSupport = false;
    }
    if (centerPillarColliderIndex >= 0 &&
        centerPillarColliderIndex < static_cast<int>(sceneColliders.size())) {
        sceneColliders[centerPillarColliderIndex].canSupport = false;
    }

    SceneCollider groundCollider;
    groundCollider.type = SceneCollider::Type::AABB;
    groundCollider.collisionEnabled = true;
    groundCollider.center = glm::vec3(groundObject->model[3]);
    groundCollider.halfExtents = glm::vec3(
        std::abs(groundObject->model[0][0]) * 0.5f,
        std::abs(groundObject->model[1][1]) * 0.5f,
        std::abs(groundObject->model[2][2]) * 0.5f
    );
    groundCollider.radius = 0.0f;
    sceneColliders.push_back(groundCollider);
}

bool Game::isCollidingWithScene(const glm::vec3& point) const {
    for (const auto& collider : sceneColliders) {
        if (!collider.collisionEnabled) {
            continue;
        }
        if (collider.type == SceneCollider::Type::AABB) {
            glm::vec3 minB = collider.center - collider.halfExtents;
            glm::vec3 maxB = collider.center + collider.halfExtents;
            if (point.x >= minB.x && point.x <= maxB.x &&
                point.y >= minB.y && point.y <= maxB.y &&
                point.z >= minB.z && point.z <= maxB.z) {
                return true;
            }
        } else {
            if (glm::distance(point, collider.center) <= collider.radius) {
                return true;
            }
        }
    }
    return false;
}

bool Game::isSegmentCollidingWithScene(const glm::vec3& start, const glm::vec3& end, float radius) const {
    const glm::vec3 direction = end - start;
    const float segmentLength = glm::length(direction);
    if (segmentLength < 0.0001f) {
        return isCollidingWithScene(start);
    }

    const glm::vec3 dir = direction / segmentLength;
    for (const auto& collider : sceneColliders) {
        if (!collider.collisionEnabled) {
            continue;
        }

        if (collider.type == SceneCollider::Type::AABB) {
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
        } else {
            const glm::vec3 m = start - collider.center;
            const float combinedRadius = collider.radius + radius;
            const float b = glm::dot(m, dir);
            const float c = glm::dot(m, m) - combinedRadius * combinedRadius;
            if (c <= 0.0f) {
                return true;
            }
            if (b > 0.0f) {
                continue;
            }
            const float discriminant = b * b - c;
            if (discriminant < 0.0f) {
                continue;
            }
            const float t = -b - std::sqrt(discriminant);
            if (t >= 0.0f && t <= segmentLength) {
                return true;
            }
        }
    }

    return false;
}

bool Game::raycastScene(
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

    for (int i = 0; i < static_cast<int>(sceneColliders.size()); ++i) {
        if (i == excludeColliderIndex) {
            continue;
        }
        const auto& collider = sceneColliders[i];
        if (!collider.collisionEnabled) {
            continue;
        }

        if (collider.type == SceneCollider::Type::AABB) {
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
        } else {
            const glm::vec3 m = origin - collider.center;
            const float b = glm::dot(m, d);
            const float c = glm::dot(m, m) - collider.radius * collider.radius;
            if (c > 0.0f && b > 0.0f) {
                continue;
            }
            const float discriminant = b * b - c;
            if (discriminant < 0.0f) {
                continue;
            }
            const float t = -b - std::sqrt(discriminant);
            if (t >= 0.0f && t < bestT) {
                bestT = t;
                hitFound = true;
            }
        }
    }

    if (hitFound) {
        outDistance = bestT;
        outHit = origin + d * bestT;
    }
    return hitFound;
}

void Game::setupSkybox() {
    const float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    const unsigned char faceColors[6][3] = {
        {  12,  18,  30 }, // +X
        {  10,  16,  26 }, // -X
        {  16,  22,  34 }, // +Y
        {   4,   4,   7 }, // -Y
        {  11,  17,  28 }, // +Z
        {   9,  14,  24 }  // -Z
    };

    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0,
            GL_RGB,
            1,
            1,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            faceColors[i]
        );
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Game::renderSkybox(const glm::mat4& view, const glm::mat4& projection) {
    glDepthFunc(GL_LEQUAL);

    cubemapShader->use();
    cubemapShader->setMat4("projection", projection);
    cubemapShader->setMat4("view", glm::mat4(glm::mat3(view)));

    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    cubemapShader->setInt("skybox", 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}