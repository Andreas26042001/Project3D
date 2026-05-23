#ifndef SKYBOX_H
#define SKYBOX_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Texture.h"

class Shader;

class Skybox {
public:
    Skybox();
    ~Skybox();

    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    bool init(Shader* shader);
    void render(const glm::mat4& view, const glm::mat4& projection) const;
    bool isReady() const;
    const Texture& getCubemapTexture() const { return cubemapTexture; }

private:
    Shader* shader;
    GLuint vao;
    GLuint vbo;
    Texture cubemapTexture;
};

#endif
