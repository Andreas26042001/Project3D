#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include <functional>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader;

class ShadowMap {
public:
    ShadowMap();
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    void init(int mapSize);

    struct RenderParams {
        bool dynamicActive;
        glm::vec3 lightPosition;
        glm::vec3 lightDirection;
    };

    using CasterDrawFn = std::function<void(Shader*, bool ceilingCastersOnly)>;

    void render(const RenderParams& params, Shader* depthShader, const CasterDrawFn& drawCasters);

    glm::mat4 getDynamicLightSpaceMatrix() const { return dynamicLightSpaceMatrix; }
    GLuint getDynamicTexture() const { return dynamicTexture; }

private:
    int mapSize;
    GLuint fbo;
    GLuint dynamicTexture;
    glm::mat4 dynamicLightSpaceMatrix;
};

#endif
