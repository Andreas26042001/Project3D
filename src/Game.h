#ifndef GAME_H
#define GAME_H

#include <vector>
#include <glad/glad.h>
#include "object.h"
#include "Collider.h"
#include "Skybox.h"
#include "ParticleSystem.h"
#include "ShadowMap.h"
#include "Texture.h"

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

    std::vector<Object*> objects;
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
    GLuint targetVAO = 0;
    GLuint targetVBO = 0;
    int targetVertexCount = 0;
    glm::vec3 targetPosition;
    float targetHitTolerance = 0.18f;
    float targetActivationTimer = 0.0f;
    float targetActivationDuration = 3.0f;
    bool targetActivated = false;
    glm::vec3 target2Position;
    glm::mat4 target2ModelMatrix;
    float target2ActivationTimer = 0.0f;
    bool target2Activated = false;
    glm::mat4 target1ModelMatrix;

    // Deflector prism + pillar
    GLuint prismVAO = 0;
    GLuint prismVBO = 0;
    int prismVertexCount = 0;
    Shader* prismShader = nullptr;
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
    Object* capturePillarMesh = nullptr;
    Texture capturePillarMetalTexture;

    // GPU resources
    Shader* phongShader = nullptr;
    Shader* lampShader = nullptr;
    Shader* particleShader = nullptr;
    Shader* cubemapShader = nullptr;
    Shader* crosshairShader = nullptr;
    Shader* shadowDepthShader = nullptr;
    Object* lightMarker = nullptr;
    Object* groundObject = nullptr;
    Texture groundDiffuseTexture;
    Texture pillarDiffuseTexture;
    GLuint crosshairVAO = 0;
    GLuint crosshairVBO = 0;

    // Lighting
    glm::vec3 ceilingLightPosition;
    float ceilingLightStrength = 2.4f;
    float ceilingLightRange = 8.0f;
    glm::vec3 ceilingLightColor{1.0f, 0.88f, 0.38f};
    glm::vec3 lightPosition{0.0f};
    bool lightProjectileActive = false;
    glm::vec3 lightProjectileDirection{0.0f, 0.0f, -1.0f};
    float lightProjectileLifetime = 0.0f;
    float lightProjectileSpeed = 9.0f;
    float lightProjectileMaxLifetime = 1.8f;
    float lightProjectileMaxDistance = 14.0f;
    bool lightAnchoredOnPillar = false;
    bool lightAnchorAnimating = false;
    float lightAnchorAnimT = 0.0f;
    float lightAnchorAnimDuration = 0.45f;
    glm::vec3 lightAnchorSourcePos;
    glm::vec3 lightAnchorTargetPos;

    // Viewport / HUD
    int viewportWidth = 800;
    int viewportHeight = 600;
    bool crosshairVisible = true;

    struct BeamTrace {
        int segmentCount = 0;
        glm::vec3 starts[2];
        glm::vec3 ends[2];
        float lengths[2] = {0.0f, 0.0f};
    };

    BeamTrace traceAnchoredBeam(float maxDistance) const;

    void setupCrosshair();
    void renderCrosshair();
    void renderSceneOpaque(
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPosition,
        const glm::vec3& playerWorldPosition
    );
    void renderShadowMap();
    void disableLightProjectile();
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
