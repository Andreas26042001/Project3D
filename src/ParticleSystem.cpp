#include "ParticleSystem.h"
#include "shader.h"

#include <algorithm>
#include <cmath>

ParticleSystem::ParticleSystem()
    : shader(nullptr),
      vao(0),
      quadVBO(0),
      centerVBO(0),
      colorVBO(0) {}

ParticleSystem::~ParticleSystem() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }
    if (quadVBO != 0) {
        glDeleteBuffers(1, &quadVBO);
    }
    if (centerVBO != 0) {
        glDeleteBuffers(1, &centerVBO);
    }
    if (colorVBO != 0) {
        glDeleteBuffers(1, &colorVBO);
    }
}

void ParticleSystem::init(Shader* particleShader) {
    shader = particleShader;

    const float quadVertices[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
         1.0f, -1.0f, 0.0f
    };

    centerData.assign(kMaxParticles * 4, 0.0f);
    colorData.assign(kMaxParticles * 4, 0.0f);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &centerVBO);
    glGenBuffers(1, &colorVBO);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glVertexAttribDivisor(0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, centerVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(centerData.size() * sizeof(GLfloat)),
        nullptr,
        GL_STREAM_DRAW
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glVertexAttribDivisor(1, 1);

    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(colorData.size() * sizeof(GLfloat)),
        nullptr,
        GL_STREAM_DRAW
    );
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glVertexAttribDivisor(2, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void ParticleSystem::spawnExplosion(const glm::vec3& position) {
    const int particleCount = 26;
    const float baseLife = 2.0f;
    const float goldenAngle = 2.39996323f;

    for (int i = 0; i < particleCount; ++i) {
        const float t = (i + 0.5f) / static_cast<float>(particleCount);
        const float y = 1.0f - 2.0f * t;
        const float radius = std::sqrt(std::max(0.0f, 1.0f - y * y));
        const float theta = goldenAngle * static_cast<float>(i);

        glm::vec3 dir(
            radius * std::cos(theta),
            y,
            radius * std::sin(theta)
        );
        dir = glm::normalize(dir);

        const float speed = 1.8f + 2.8f * t;
        Particle particle;
        particle.position = position;
        particle.velocity = dir * speed;
        particle.color = glm::vec3(0.30f, 0.55f + 0.30f * (1.0f - t), 1.0f);
        particle.life = baseLife * (0.75f + 0.35f * t);
        particle.maxLife = particle.life;
        particle.size = 0.04f + 0.08f * (1.0f - t);
        particle.cameraDist = 0.0f;
        particles.push_back(particle);
    }
}

void ParticleSystem::update(float deltaTime) {
    for (auto& particle : particles) {
        if (particle.life <= 0.0f) {
            continue;
        }
        particle.life -= deltaTime;
        particle.position += particle.velocity * deltaTime;
        particle.velocity *= 0.96f;
    }

    particles.erase(
        std::remove_if(
            particles.begin(),
            particles.end(),
            [](const Particle& p) { return p.life <= 0.0f; }
        ),
        particles.end()
    );
}

void ParticleSystem::render(const glm::mat4& view, const glm::mat4& projection) {
    if (particles.empty() || shader == nullptr || vao == 0) {
        return;
    }

    const glm::mat4 invView = glm::inverse(view);
    const glm::vec3 cameraPosition = glm::vec3(invView[3]);
    const glm::vec3 cameraRight = glm::vec3(invView[0]);
    const glm::vec3 cameraUp = glm::vec3(invView[1]);

    std::vector<Particle> sortedParticles = particles;
    for (auto& particle : sortedParticles) {
        const glm::vec3 diff = particle.position - cameraPosition;
        particle.cameraDist = glm::dot(diff, diff);
    }

    std::sort(
        sortedParticles.begin(),
        sortedParticles.end(),
        [](const Particle& a, const Particle& b) {
            return a.cameraDist > b.cameraDist;
        }
    );

    const int particleCount = static_cast<int>(std::min(
        sortedParticles.size(),
        static_cast<size_t>(kMaxParticles)
    ));

    for (int i = 0; i < particleCount; ++i) {
        const Particle& particle = sortedParticles[static_cast<size_t>(i)];
        const float lifeRatio = std::max(0.0f, particle.life / particle.maxLife);
        const float size = particle.size * (0.55f + 0.45f * lifeRatio);

        centerData[i * 4 + 0] = particle.position.x;
        centerData[i * 4 + 1] = particle.position.y;
        centerData[i * 4 + 2] = particle.position.z;
        centerData[i * 4 + 3] = size;

        colorData[i * 4 + 0] = particle.color.r;
        colorData[i * 4 + 1] = particle.color.g;
        colorData[i * 4 + 2] = particle.color.b;
        colorData[i * 4 + 3] = lifeRatio * 0.85f;
    }

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, centerVBO);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        static_cast<GLsizeiptr>(particleCount * 4 * sizeof(GLfloat)),
        centerData.data()
    );

    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        static_cast<GLsizeiptr>(particleCount * 4 * sizeof(GLfloat)),
        colorData.data()
    );

    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLint previousBlendSrc = GL_SRC_ALPHA;
    GLint previousBlendDst = GL_ONE_MINUS_SRC_ALPHA;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &previousBlendSrc);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &previousBlendDst);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader->use();
    shader->setMat4("view", view);
    shader->setMat4("projection", projection);
    shader->setVec3("cameraRight", cameraRight);
    shader->setVec3("cameraUp", cameraUp);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, particleCount);

    glDepthMask(GL_TRUE);
    if (blendWasEnabled) {
        glBlendFunc(previousBlendSrc, previousBlendDst);
    } else {
        glDisable(GL_BLEND);
    }

    glBindVertexArray(0);
}
