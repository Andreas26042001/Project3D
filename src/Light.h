#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>

class Light {
public:
    glm::vec3 position;
    glm::vec3 color;
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;

    Light(glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f),
          glm::vec3 col = glm::vec3(1.0f, 1.0f, 1.0f),
          float ambient = 0.1f,
          float diffuse = 0.8f,
          float specular = 1.0f);

    void setUniforms(class Shader& shader, const std::string& prefix = "light");
};

#endif