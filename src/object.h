#ifndef OBJECT_H
#define OBJECT_H

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader;

struct Vertex {
    glm::vec3 Position;
    glm::vec2 Texture;
    glm::vec3 Normal;
};

class Object {
public:
    std::vector<Vertex> vertices;
    int numVertices;
    GLuint VBO;
    GLuint VAO;
    glm::mat4 model;

    explicit Object(const char* path);
    ~Object();

    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    void makeObject(const Shader& shader, bool texture = true);
    void draw() const;

private:
    void releaseGpuResources();
};

#endif
