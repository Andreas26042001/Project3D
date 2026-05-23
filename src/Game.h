#ifndef GAME_H
#define GAME_H

#include <vector>
#include <array>
#include <string>
#include <glad/glad.h>
#include "camera.h"
#include "object.h"
#include "Light.h"

class Game {
public:
    Game();
    ~Game();

    void Update(float deltaTime);
    void Render(Camera& camera);
    void SetViewportSize(int width, int height);
    void FireLightProjectile(const glm::vec3& origin, const glm::vec3& direction);
    float GetGroundHeight() const;
    float GetSupportHeightAtPosition(const glm::vec3& cameraPosition, float cameraRadius) const;
    void ResolveCameraCollisions(glm::vec3& cameraPosition, float cameraRadius) const;
    void UpdateMovablePillar(const glm::vec3& cameraPosition, float cameraRadius);
    void ToggleCrosshair();

private:
    struct ExplosionParticle {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec3 color;
        float life;
        float maxLife;
        float size;
    };
    struct SceneCollider {
        enum class Type { AABB, Sphere };
        Type type;
        bool collisionEnabled;
        glm::vec3 center;
        glm::vec3 halfExtents;
        float radius;
        bool canSupport = true;
    };
    std::vector<Object*> objects;
    std::vector<Light*> lights;
    std::vector<ExplosionParticle> explosionParticles;
    std::vector<glm::mat4> extraCubeModels;
    std::vector<glm::vec3> pillarBaseCenters;
    std::vector<SceneCollider> sceneColliders;
    glm::vec3 centerPillarBaseCenter;
    float centerPillarHeight;
    float centerPillarHalfWidth;
    glm::vec3 cannonMuzzlePos;
    glm::vec3 cannonDirection;
    float cannonLength;
    float cannonHalfWidth;
    int cannonColliderIndex;
    int centerPillarColliderIndex;
    int centerPillarShadowIndex;
    float centerPillarOffsetZ;
    float centerPillarRailMin;
    float centerPillarRailMax;
    glm::mat4 railModel;
    GLuint targetVAO;
    GLuint targetVBO;
    int targetVertexCount;
    glm::vec3 targetPosition;
    float targetHitTolerance;
    float targetActivationTimer;
    float targetActivationDuration;
    bool targetActivated;
    GLuint prismVAO;
    GLuint prismVBO;
    int prismVertexCount;
    Shader* prismShader;
    glm::vec3 prismCenter;
    float prismRadius;
    glm::vec3 prismDeflectDirection;
    int deflectorPillarColliderIndex;
    int deflectorPillarShadowIndex;
    float deflectorPillarOffsetX;
    float deflectorRailMin;
    float deflectorRailMax;
    float deflectorPillarHeight;
    glm::mat4 deflectorRailModel;
    glm::vec3 deflectorPillarBase;
    glm::vec3 target2Position;
    glm::mat4 target2ModelMatrix;
    float target2ActivationTimer;
    bool target2Activated;
    glm::mat4 target1ModelMatrix;

    // Shaders
    Shader* phongShader;
    Shader* lampShader;
    Shader* cubemapShader;
    Shader* crosshairShader;
    Shader* shadowDepthShader;
    Object* lightMarker;
    Object* groundObject;
    Object* capturePillarMesh;
    glm::mat4 capturePillarModelMatrix;
    GLuint capturePillarMetalTexture;
    bool capturePillarMetalTextureLoaded;
    GLuint skyboxVAO;
    GLuint skyboxVBO;
    GLuint cubemapTexture;
    GLuint crosshairVAO;
    GLuint crosshairVBO;
    GLuint groundDiffuseTexture;
    bool groundDiffuseTextureLoaded;
    GLuint pillarDiffuseTexture;
    bool pillarDiffuseTextureLoaded;
    GLuint shadowMapFBO;
    GLuint shadowMapTexture;
    glm::mat4 lightSpaceMatrix;
    // La lumiere du plafond et ses occluders (piliers, murs, plafond) sont statiques :
    // la shadow map n'est calculee qu'une fois au demarrage (RTR4 §7.4 p. 235).
    bool staticShadowMapsBuilt;
    GLuint dynamicShadowMapTexture;
    glm::mat4 dynamicLightSpaceMatrix;
    glm::vec3 ceilingLightPosition;
    int viewportWidth;
    int viewportHeight;

    // Game state
    float time;
    bool lightProjectileActive;
    glm::vec3 lightProjectileStart;
    glm::vec3 lightProjectileDirection;
    float lightProjectileLifetime;
    float lightProjectileSpeed;
    float lightProjectileMaxLifetime;
    float lightProjectileMaxDistance;
    bool lightAnchoredOnPillar;
    bool lightAnchorAnimating;
    float lightAnchorAnimT;
    float lightAnchorAnimDuration;
    glm::vec3 lightAnchorSourcePos;
    glm::vec3 lightAnchorTargetPos;
    float worldCollisionHalfExtent;
    float groundTopY;
    float ceilingLightStrength;
    float ceilingLightRange;
    glm::vec3 ceilingLightColor;
    bool reflectionClipEnabled;
    glm::vec4 reflectionClipPlane;
    bool edgeClipEnabled;
    std::array<glm::vec4, 4> edgeClipPlanes;
    bool crosshairVisible;

    void setupSkybox();
    void setupCrosshair();
    void renderSkybox(const glm::mat4& view, const glm::mat4& projection);
    void renderCrosshair();
    void renderSceneOpaque(
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPosition,
        const glm::vec3& playerWorldPosition
    );
    void renderShadowMap();
    void disableLightProjectile();
    void spawnLightExplosion(const glm::vec3& position);
    void updateExplosionParticles(float deltaTime);
    void renderExplosionParticles(const glm::mat4& view, const glm::mat4& projection);
    void rebuildSceneColliders();
    bool isCollidingWithScene(const glm::vec3& point) const;
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
    bool raySphereIntersect(
        const glm::vec3& origin,
        const glm::vec3& direction,
        const glm::vec3& sphereCenter,
        float sphereRadius,
        float maxDistance,
        float& outDistance
    ) const;
};

#endif