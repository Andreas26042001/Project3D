#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader;

class ParticleSystem {
public:
    static constexpr int kMaxParticles = 512;

    ParticleSystem();
    ~ParticleSystem();

    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    void init(Shader* shader);
    void spawnExplosion(const glm::vec3& position);
    void update(float deltaTime);
    void render(const glm::mat4& view, const glm::mat4& projection);

private:
    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec3 color;
        float life;
        float maxLife;
        float size;
        float cameraDist;
    };

    Shader* shader;
    GLuint vao;
    GLuint quadVBO;
    GLuint centerVBO;
    GLuint colorVBO;
    std::vector<Particle> particles;
    std::vector<GLfloat> centerData;
    std::vector<GLfloat> colorData;
};

#endif
