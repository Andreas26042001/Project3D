#ifndef GAME_H
#define GAME_H

#include <vector>
#include <memory>
#include <glad/glad.h>
#include "object.h"
#include "Collider.h"
#include "Skybox.h"
#include "ParticleSystem.h"
#include "ShadowMap.h"
#include "Texture.h"
#include "LightProjectileSystem.h"
#include "PuzzleSystem.h"
#include "CrosshairRenderer.h"
#include "BeamRenderer.h"
#include "PrismRenderer.h"

class Player;

class Game {
public:
    static constexpr float kLightSourceOffsetY = 0.18f;

    Game();
    ~Game();

    void Update(float deltaTime);
    void Render(const Player& player);
    void SetViewportSize(int width, int height);
    void FireLightProjectile(const glm::vec3& origin, const glm::vec3& direction);
    float GetGroundHeight() const;
    float GetSupportHeightAtPosition(const glm::vec3& cameraPosition, float cameraRadius) const;
    void ResolveCameraCollisions(glm::vec3& cameraPosition, float cameraRadius) const;
    void UpdateMovablePillar(const glm::vec3& cameraPosition, float cameraRadius);
    void ToggleCrosshair();

private:
    ColliderWorld colliderWorld;
    Skybox skybox;
    ParticleSystem explosionParticles;
    ShadowMap shadowMap;

    std::vector<std::unique_ptr<Object>> objects;
    std::vector<glm::mat4> extraCubeModels;

    // Scene layout
    glm::vec3 centerPillarBaseCenter;
    float centerPillarHeight = 1.0f;
    float centerPillarHalfWidth = 0.2f;
    glm::vec3 beamSourcePos;
    glm::vec3 beamDirection{1.0f, 0.0f, 0.0f};
    int centerPillarColliderIndex = -1;
    float centerPillarOffsetZ = 0.0f;
    float centerPillarRailMin = 0.0f;
    float centerPillarRailMax = 0.0f;
    glm::mat4 railModel;
    float worldCollisionHalfExtent = 14.0f;
    float groundTopY = -1.0f;
    float scenePillarHeight = 5.8f;
    float sceneCeilingThickness = 0.6f;

    // Beam targets
    BeamRenderer beamRenderer;
    PuzzleSystem puzzleSystem;
    
    glm::vec3 targetPosition;
    float targetHitTolerance = 0.18f;
    
    
    
    glm::vec3 target2Position;
    glm::mat4 target2ModelMatrix;
    
    
    glm::mat4 target1ModelMatrix;

    // Deflector prism + pillar
    PrismRenderer prismRenderer;
    std::unique_ptr<Shader> prismShader;
    glm::vec3 prismCenter;
    float prismRadius = 0.33f;
    glm::vec3 prismDeflectDirection{0.0f, 0.0f, 1.0f};
    int deflectorPillarColliderIndex = -1;
    float deflectorPillarOffsetX = 0.0f;
    float deflectorRailMin = 0.0f;
    float deflectorRailMax = 0.0f;
    glm::mat4 deflectorRailModel;
    glm::vec3 deflectorPillarBase;
    glm::mat4 capturePillarModelMatrix;
    glm::mat4 deflectorPillarModelMatrix;
    std::unique_ptr<Object> capturePillarMesh;
    Texture capturePillarMetalTexture;

    // GPU resources
    std::unique_ptr<Shader> phongShader;
    std::unique_ptr<Shader> lampShader;
    std::unique_ptr<Shader> particleShader;
    std::unique_ptr<Shader> cubemapShader;
    std::unique_ptr<Shader> crosshairShader;
    std::unique_ptr<Shader> shadowDepthShader;

    std::unique_ptr<Object> lightMarker;
    std::unique_ptr<Object> groundObject;
    Texture groundDiffuseTexture;
    Texture pillarDiffuseTexture;

    // Lighting
    glm::vec3 ceilingLightPosition;
    float ceilingLightStrength = 2.4f;
    float ceilingLightRange = 8.0f;
    glm::vec3 ceilingLightColor{1.0f, 0.88f, 0.38f};
    LightProjectileSystem lightProjectile;

    // Viewport / HUD
    int viewportWidth = 800;
    int viewportHeight = 600;
    CrosshairRenderer crosshairRenderer;


    BeamTrace traceAnchoredBeam(float maxDistance) const;

    void initShaders();
    void loadMeshes();
    void buildSceneLayout();
    void initTextures();
    void initRenderingResources();

    
    void renderCrosshair();
    void renderSceneOpaque(
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPosition,
        const glm::vec3& playerWorldPosition
    );
    void renderShadowMap();
    void rebuildSceneColliders();
    bool isSegmentCollidingWithScene(const glm::vec3& start, const glm::vec3& end, float radius) const;
    bool raycastScene(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance,
        int excludeColliderIndex,
        glm::vec3& outHit,
        float& outDistance
    ) const;
    void syncCarriedLight(const glm::vec3& cameraPosition, const glm::vec3& cameraForward);
    void rebuildCenterPillarTransform();
    void setupBeamTarget();
    void updateBeamTarget(float deltaTime);
    void renderBeamTarget(const glm::mat4& view, const glm::mat4& projection);
    void setupDeflectorPrism();
    void renderDeflectorPrism(
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPosition
    );
    void rebuildDeflectorPillarTransform();
    void drawMovablePillarPhong(const glm::mat4& pillarModelMatrix);
    void drawMovablePillarShadows();
    void renderPlanarShadows(const glm::mat4& view, const glm::mat4& projection);
};

#endif
