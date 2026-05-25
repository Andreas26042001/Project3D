#include "Game.h"
#include "GameInternal.h"
#include "shader.h"
#include "object.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

Game::Game() {
    initShaders();
    loadMeshes();
    buildSceneLayout();
    initTextures();
    initRenderingResources();

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    rebuildSceneColliders();
}

void Game::initShaders() {
    phongShader = std::make_unique<Shader>(
        game_internal::shaderPath("phong.vert").c_str(),
        game_internal::shaderPath("phong.frag").c_str()
    );

    lampShader = std::make_unique<Shader>(
        game_internal::shaderPath("lamp.vert").c_str(),
        game_internal::shaderPath("lamp.frag").c_str()
    );

    particleShader = std::make_unique<Shader>(
        game_internal::shaderPath("particle.vert").c_str(),
        game_internal::shaderPath("particle.frag").c_str()
    );

    cubemapShader = std::make_unique<Shader>(
        game_internal::shaderPath("cubemap.vert").c_str(),
        game_internal::shaderPath("cubemap.frag").c_str()
    );

    shadowDepthShader = std::make_unique<Shader>(
        game_internal::shaderPath("shadow_depth.vert").c_str(),
        game_internal::shaderPath("shadow_depth.frag").c_str()
    );

    crosshairShader = std::make_unique<Shader>(
        game_internal::shaderPath("crosshair.vert").c_str(),
        game_internal::shaderPath("crosshair.frag").c_str()
    );
}

void Game::loadMeshes() {
    auto cube = std::make_unique<Object>(game_internal::objectPath("cube.obj").c_str());
    cube->makeObject(*phongShader);
    objects.push_back(std::move(cube));

    capturePillarMesh = std::make_unique<Object>(
        game_internal::objectPath("capture_pillar.obj").c_str()
    );

    if (!capturePillarMesh->vertices.empty()) {
        capturePillarMesh->makeObject(*phongShader);
        std::cout << "Loaded movable pillar OBJ mesh ("
                  << capturePillarMesh->vertices.size()
                  << " vertices).\n";
    } else {
        capturePillarMesh.reset();
        std::cerr << "ERROR: capture_pillar.obj missing or unreadable — movable pillars will not render.\n";
    }
}

void Game::buildSceneLayout() {
    const float pillarSpacing = 3.2f;
    const float pillarHalfWidth = 0.35f;
    const float pillarHeight = 5.8f;
    scenePillarHeight = pillarHeight;
    const float pillarCenterY = groundTopY + (pillarHeight * 0.5f);
    const int gridRadius = 3;

    for (int gx = -gridRadius; gx <= gridRadius; ++gx) {
        for (int gz = -gridRadius; gz <= gridRadius; ++gz) {
            if (gx == 0 && gz == 0) {
                continue;
            }
            glm::mat4 pillarModel = glm::mat4(1.0f);
            pillarModel = glm::translate(pillarModel, glm::vec3(gx * pillarSpacing, pillarCenterY, gz * pillarSpacing));
            pillarModel = glm::scale(pillarModel, glm::vec3(pillarHalfWidth * 2.0f, pillarHeight, pillarHalfWidth * 2.0f));
            extraCubeModels.push_back(pillarModel);
        }
    }

    centerPillarBaseCenter = glm::vec3(0.0f, groundTopY, 0.0f);
    beamDirection = glm::vec3(1.0f, 0.0f, 0.0f);

    const float railLength = 4.0f;
    const float railHalfWidthX = 0.08f;
    const float railHalfHeightY = 0.015f;
    centerPillarRailMin = -(railLength * 0.5f) + centerPillarHalfWidth;
    centerPillarRailMax =  (railLength * 0.5f) - centerPillarHalfWidth;
    railModel = glm::mat4(1.0f);
    railModel = glm::translate(railModel, glm::vec3(0.0f, groundTopY + railHalfHeightY, 0.0f));
    railModel = glm::scale(railModel, glm::vec3(railHalfWidthX * 2.0f, railHalfHeightY * 2.0f, railLength));
    rebuildCenterPillarTransform();

    const float kDeflectorPillarX = 5.5f;
    const float kDeflectorPillarZ = -1.5f;
    prismDeflectDirection = glm::vec3(0.0f, 0.0f, 1.0f);
    deflectorPillarBase = glm::vec3(kDeflectorPillarX, groundTopY, kDeflectorPillarZ);

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
    rebuildDeflectorPillarTransform();

    const float mapHalfExtent = worldCollisionHalfExtent;
    const float wallThickness = 0.6f;
    const float ceilingThickness = 0.6f;
    sceneCeilingThickness = ceilingThickness;
    const float wallHeight = pillarHeight;
    const float ceilingCenterY = groundTopY + pillarHeight - (ceilingThickness * 0.5f);
    const float wallCenterY = groundTopY + (wallHeight * 0.5f);

    glm::mat4 ceilingModel = glm::mat4(1.0f);
    ceilingModel = glm::translate(ceilingModel, glm::vec3(0.0f, ceilingCenterY, 0.0f));
    ceilingModel = glm::scale(ceilingModel, glm::vec3(mapHalfExtent * 2.0f, ceilingThickness, mapHalfExtent * 2.0f));
    extraCubeModels.push_back(ceilingModel);

    const float edgeInset = mapHalfExtent - pillarHalfWidth;
    const float kTarget1Z = 0.8f;
    const float kTarget2X = deflectorPillarBase.x - 1.0f;
    const float kTargetWallSpan = pillarSpacing * 0.75f;

    auto addStandardPillar = [&](float x, float z) {
        glm::mat4 pillarModel = glm::mat4(1.0f);
        pillarModel = glm::translate(pillarModel, glm::vec3(x, pillarCenterY, z));
        pillarModel = glm::scale(pillarModel, glm::vec3(pillarHalfWidth * 2.0f, pillarHeight, pillarHalfWidth * 2.0f));
        extraCubeModels.push_back(pillarModel);
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
    eastTargetWall = glm::translate(eastTargetWall, glm::vec3(mapHalfExtent - wallThickness * 0.5f, wallCenterY, kTarget1Z));
    eastTargetWall = glm::scale(eastTargetWall, glm::vec3(wallThickness, wallHeight, kTargetWallSpan));
    extraCubeModels.push_back(eastTargetWall);

    glm::mat4 southTargetWall = glm::mat4(1.0f);
    southTargetWall = glm::translate(southTargetWall, glm::vec3(kTarget2X, wallCenterY, mapHalfExtent - wallThickness * 0.5f));
    southTargetWall = glm::scale(southTargetWall, glm::vec3(kTargetWallSpan, wallHeight, wallThickness));
    extraCubeModels.push_back(southTargetWall);

    ceilingLightPosition = glm::vec3(
        0.0f,
        groundTopY + scenePillarHeight - sceneCeilingThickness - 0.12f,
        0.0f
    );

    lightMarker = std::make_unique<Object>(game_internal::objectPath("cube.obj").c_str());
    lightMarker->makeObject(*lampShader, false);

    groundObject = std::make_unique<Object>(game_internal::objectPath("cube.obj").c_str());
    groundObject->makeObject(*phongShader);
    groundObject->model = glm::translate(groundObject->model, glm::vec3(0.0f, -1.25f, 0.0f));
    const float groundSpan = (worldCollisionHalfExtent * 2.0f) + 0.6f;
    groundObject->model = glm::scale(groundObject->model, glm::vec3(groundSpan, 0.5f, groundSpan));
}

void Game::initTextures() {
    groundDiffuseTexture = Texture::loadFromFile2D(
        game_internal::texturePath("ground/Ground081_1K-JPG/Ground081_1K-JPG_Color.jpg")
    );
    if (groundDiffuseTexture.isValid()) {
        std::cout << "Ground texture loaded: " << groundDiffuseTexture.path() << std::endl;
    } else {
        std::cout << "INFO: no ground diffuse map found, using flat color rendering." << std::endl;
    }

    pillarDiffuseTexture = Texture::loadFromFile2D(
        game_internal::texturePath("pillars/Bricks075B_2K-JPG/Bricks075B_2K-JPG_Color.jpg")
    );
    if (pillarDiffuseTexture.isValid()) {
        std::cout << "Pillar texture loaded: " << pillarDiffuseTexture.path() << std::endl;
    } else {
        std::cout << "INFO: no pillar diffuse map found, using flat color rendering." << std::endl;
    }

    capturePillarMetalTexture = Texture::loadFromFile2D(
        game_internal::texturePath("Metal055A_2K-JPG/Metal055A_2K-JPG_Color.jpg")
    );
    if (capturePillarMetalTexture.isValid()) {
        std::cout << "Receiver pillar metal texture loaded: " << capturePillarMetalTexture.path() << std::endl;
    } else {
        std::cout << "INFO: Metal055 metal texture for receiver pillar not found." << std::endl;
    }
}

void Game::initRenderingResources() {
    shadowMap.init(game_internal::kShadowMapSize);
    skybox.init(cubemapShader.get());
    crosshairRenderer.Init();
    explosionParticles.init(particleShader.get());
    setupBeamTarget();
    setupDeflectorPrism();
}

Game::~Game() {
    crosshairRenderer.Destroy();

    beamRenderer.Destroy();

    if (prismVAO != 0) {
        glDeleteVertexArrays(1, &prismVAO);
    }

    if (prismVBO != 0) {
        glDeleteBuffers(1, &prismVBO);
    }
}
