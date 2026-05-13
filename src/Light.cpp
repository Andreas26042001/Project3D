#include "Light.h"
#include "shader.h"

Light::Light(glm::vec3 pos, glm::vec3 col, float ambient, float diffuse, float specular)
    : position(pos), color(col),
      ambientStrength(ambient), diffuseStrength(diffuse), specularStrength(specular) {}

void Light::setUniforms(Shader& shader, const std::string& prefix) {
    shader.setVec3(prefix + ".position", position);
    shader.setVec3(prefix + ".color", color);
    shader.setFloat(prefix + ".ambient", ambientStrength);
    shader.setFloat(prefix + ".diffuse", diffuseStrength);
    shader.setFloat(prefix + ".specular", specularStrength);
}