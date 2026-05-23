#include "Game.h"
#include "GameInternal.h"
#include "shader.h"
#include "camera.h"
#include "object.h"
#include "Light.h"
#include <cmath>
#include <iostream>
#include <vector>

Game::Game() : time(0.0f),
               lightProjectileActive(false),
               lightProjectileStart(glm::vec3(0.0f)),
               lightProjectileDirection(glm::vec3(0.0f, 0.0f, -1.0f)),
               lightProjectileLifetime(0.0f),
               lightProjectileSpeed(9.0f),
               lightProjectileMaxLifetime(1.8f),
               lightProjectileMaxDistance(14.0f),
               lightAnchoredOnPillar(false),
               lightAnchorAnimating(false),
               lightAnchorAnimT(0.0f),
               lightAnchorAnimDuration(0.45f),
               lightAnchorSourcePos(glm::vec3(0.0f)),
               lightAnchorTargetPos(glm::vec3(0.0f)),
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
               ceilingLightStrength(2.4f),
               ceilingLightRange(18.0f),
               ceilingLightColor(1.0f, 0.88f, 0.38f),
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
               capturePillarMesh(nullptr),
               capturePillarModelMatrix(glm::mat4(1.0f)),
               capturePillarMetalTexture(0),
               capturePillarMetalTextureLoaded(false),
               shadowMapFBO(0),
               shadowMapTexture(0),
               lightSpaceMatrix(1.0f),
               staticShadowMapsBuilt(false),
               dynamicShadowMapTexture(0),
               dynamicLightSpaceMatrix(1.0f),
               ceilingLightPosition(glm::vec3(0.0f)),
               viewportWidth(800),
               viewportHeight(600) {
    // Initialize shaders
    phongShader = new Shader(game_internal::shaderPath("phong.vert").c_str(), game_internal::shaderPath("phong.frag").c_str());
    lampShader = new Shader(game_internal::shaderPath("lamp.vert").c_str(), game_internal::shaderPath("lamp.frag").c_str());
    cubemapShader = new Shader(game_internal::shaderPath("cubemap.vert").c_str(), game_internal::shaderPath("cubemap.frag").c_str());
    shadowDepthShader = new Shader(game_internal::shaderPath("shadow_depth.vert").c_str(), game_internal::shaderPath("shadow_depth.frag").c_str());
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
    Object* cube = new Object(game_internal::objectPath("cube.obj").c_str());
    cube->makeObject(*phongShader);
    objects.push_back(cube);

    capturePillarMesh = new Object(game_internal::objectPath("capture_pillar.obj").c_str());
    if (!capturePillarMesh->vertices.empty()) {
        capturePillarMesh->makeObject(*phongShader);
        std::cout << "Maillage OBJ du pilier recepteur charge (" << capturePillarMesh->vertices.size()
                  << " sommets).\n";
    } else {
        delete capturePillarMesh;
        capturePillarMesh = nullptr;
        std::cout << "AVERTISSEMENT: capture_pillar.obj absent ou illisible — cube conserve pour le pilier central.\n";
    }

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

    // Perimetre : rangees de piliers a la place des murs, avec un petit pan de mur
    // entre deux piliers pour accueillir chaque cible bleue.
    const float edgeInset = mapHalfExtent - pillarHalfWidth;
    const float kTarget1Z = 0.8f;
    const float kTarget2X = deflectorPillarBase.x - 1.0f;
    const float kTargetWallSpan = pillarSpacing * 0.75f;
    auto addStandardPillar = [&](float x, float z) {
        glm::mat4 pillarModel = glm::mat4(1.0f);
        pillarModel = glm::translate(pillarModel, glm::vec3(x, pillarCenterY, z));
        pillarModel = glm::scale(
            pillarModel,
            glm::vec3(pillarHalfWidth * 2.0f, pillarHeight, pillarHalfWidth * 2.0f)
        );
        extraCubeModels.push_back(pillarModel);
        pillarBaseCenters.push_back(glm::vec3(x, groundTopY, z));
    };

    auto shouldSkipForEastTarget = [&](float z) {
        return std::abs(z - kTarget1Z) < pillarSpacing * 0.55f;
    };
    auto shouldSkipForSouthTarget = [&](float x) {
        return std::abs(x - kTarget2X) < pillarSpacing * 0.55f;
    };

    for (float x = -edgeInset; x <= edgeInset + 0.001f; x += pillarSpacing) {
        addStandardPillar(x, -edgeInset);
    }
    for (float x = -edgeInset; x <= edgeInset + 0.001f; x += pillarSpacing) {
        if (shouldSkipForSouthTarget(x)) {
            continue;
        }
        addStandardPillar(x, edgeInset);
    }
    for (float z = -edgeInset + pillarSpacing; z <= edgeInset - pillarSpacing + 0.001f; z += pillarSpacing) {
        if (shouldSkipForEastTarget(z)) {
            continue;
        }
        addStandardPillar(edgeInset, z);
    }
    for (float z = -edgeInset + pillarSpacing; z <= edgeInset - pillarSpacing + 0.001f; z += pillarSpacing) {
        addStandardPillar(-edgeInset, z);
    }

    glm::mat4 eastTargetWall = glm::mat4(1.0f);
    eastTargetWall = glm::translate(
        eastTargetWall,
        glm::vec3(mapHalfExtent - wallThickness * 0.5f, wallCenterY, kTarget1Z)
    );
    eastTargetWall = glm::scale(eastTargetWall, glm::vec3(wallThickness, wallHeight, kTargetWallSpan));
    extraCubeModels.push_back(eastTargetWall);

    glm::mat4 southTargetWall = glm::mat4(1.0f);
    southTargetWall = glm::translate(
        southTargetWall,
        glm::vec3(kTarget2X, wallCenterY, mapHalfExtent - wallThickness * 0.5f)
    );
    southTargetWall = glm::scale(southTargetWall, glm::vec3(kTargetWallSpan, wallHeight, wallThickness));
    extraCubeModels.push_back(southTargetWall);

    ceilingLightPosition = glm::vec3(
        0.0f,
        groundTopY + pillarHeight - ceilingThickness + 0.02f,
        0.0f
    );

    lightMarker = new Object(game_internal::objectPath("cube.obj").c_str());
    lightMarker->makeObject(*lampShader, false);
    groundObject = new Object(game_internal::objectPath("cube.obj").c_str());
    groundObject->makeObject(*phongShader);
    groundObject->model = glm::translate(groundObject->model, glm::vec3(0.0f, -1.25f, 0.0f));
    const float groundSpan = (worldCollisionHalfExtent * 2.0f) + 0.6f;
    groundObject->model = glm::scale(groundObject->model, glm::vec3(groundSpan, 0.5f, groundSpan));

    const std::vector<std::string> diffuseCandidates = {
        game_internal::texturePath("ground/Ground081_1K-JPG/Ground081_1K-JPG_Color.jpg"),
        game_internal::texturePath("ground/Ground081_1K-JPG_Color.jpg")
    };
    for (const auto& texturePath : diffuseCandidates) {
        if (!game_internal::fileExists(texturePath)) {
            continue;
        }
        groundDiffuseTexture = game_internal::loadTexture2D(texturePath);
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
        game_internal::texturePath("pillars/Bricks075B_2K-JPG/Bricks075B_2K-JPG_Color.jpg"),
        game_internal::texturePath("pillars/Bricks075B_2K-JPG_Color.jpg")
    };
    for (const auto& texturePath : pillarDiffuseCandidates) {
        if (!game_internal::fileExists(texturePath)) {
            continue;
        }
        pillarDiffuseTexture = game_internal::loadTexture2D(texturePath);
        if (pillarDiffuseTexture != 0) {
            pillarDiffuseTextureLoaded = true;
            std::cout << "Texture des piliers chargee: " << texturePath << std::endl;
            break;
        }
    }
    if (!pillarDiffuseTextureLoaded) {
        std::cout << "INFO: aucune image diffuse de piliers trouvee, rendu couleur utilise." << std::endl;
    }

    const std::vector<std::string> captureMetalCandidates = {
        game_internal::texturePath("Metal055A_2K-JPG/Metal055A_2K-JPG_Color.jpg"),
        game_internal::texturePath("Metal055A_2K-JPG_Color.jpg")
    };
    for (const auto& texturePath : captureMetalCandidates) {
        if (!game_internal::fileExists(texturePath)) {
            continue;
        }
        capturePillarMetalTexture = game_internal::loadTexture2D(texturePath);
        if (capturePillarMetalTexture != 0) {
            capturePillarMetalTextureLoaded = true;
            std::cout << "Texture metal du pilier recepteur chargee: " << texturePath << std::endl;
            break;
        }
    }
    if (!capturePillarMetalTextureLoaded) {
        std::cout << "INFO: texture metal Metal055 pour le pilier recepteur introuvable." << std::endl;
    }

    // Shadow map statique pour la lumiere du plafond (RTR4 §7.4 "Shadow Maps", p. 234).
    // CLAMP_TO_BORDER + white border: out-of-frustum samples read as fully lit (§7.4).
    glGenFramebuffers(1, &shadowMapFBO);
    glGenTextures(1, &shadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, game_internal::kShadowMapSize, game_internal::kShadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Dynamic shadow map for the player-carried light (re-rendered every frame).
    glGenTextures(1, &dynamicShadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, dynamicShadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, game_internal::kShadowMapSize, game_internal::kShadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float dynamicBorderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, dynamicBorderColor);

    setupSkybox();
    setupCrosshair();
    setupBeamTarget();
    if (game_internal::kEnableChapter14Translucency) {
        setupDeflectorPrism();
    }

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
    if (game_internal::kEnableChapter22Collision) {
        rebuildSceneColliders();
    }
}

Game::~Game() {
    for (auto obj : objects) {
        delete obj;
    }
    for (auto light : lights) delete light;
    delete phongShader;
    delete lampShader;
    delete cubemapShader;
    delete crosshairShader;
    delete shadowDepthShader;
    delete lightMarker;
    delete groundObject;
    if (capturePillarMesh != nullptr) {
        delete capturePillarMesh;
    }
    if (capturePillarMetalTexture != 0) {
        glDeleteTextures(1, &capturePillarMetalTexture);
    }
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
    if (shadowMapTexture != 0) {
        glDeleteTextures(1, &shadowMapTexture);
    }
    if (dynamicShadowMapTexture != 0) {
        glDeleteTextures(1, &dynamicShadowMapTexture);
    }
    if (shadowMapFBO != 0) {
        glDeleteFramebuffers(1, &shadowMapFBO);
    }
}
