#include "object.h"

#include "shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

void parseFaceVertex(
    const std::string& faceToken,
    const std::vector<glm::vec3>& positions,
    const std::vector<glm::vec2>& textures,
    const std::vector<glm::vec3>& normals,
    Vertex& outVertex
) {
    std::string token = faceToken;
    const std::string positionIndex = token.substr(0, token.find("/"));
    token.erase(0, token.find("/") + 1);

    const std::string textureIndex = token.substr(0, token.find("/"));
    token.erase(0, token.find("/") + 1);

    const std::string normalIndex = token.substr(0, token.find("/"));

    outVertex.Position = positions.at(static_cast<size_t>(std::stof(positionIndex) - 1.0f));
    outVertex.Texture = textures.at(static_cast<size_t>(std::stof(textureIndex) - 1.0f));
    outVertex.Normal = normals.at(static_cast<size_t>(std::stof(normalIndex) - 1.0f));
}

} // namespace

Object::Object(const char* path)
    : numVertices(0),
      VBO(0),
      VAO(0),
      model(1.0f) {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> textureCoords;
    std::vector<glm::vec3> normals;

    std::ifstream infile(path);
    if (!infile.is_open()) {
        std::cout << "ERROR: Cannot open file " << path << std::endl;
        return;
    }

    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "v") {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            iss >> x >> y >> z;
            positions.push_back(glm::vec3(x, y, z));
        } else if (token == "vn") {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            iss >> x >> y >> z;
            normals.push_back(glm::vec3(x, y, z));
        } else if (token == "vt") {
            float u = 0.0f;
            float v = 0.0f;
            iss >> u >> v;
            textureCoords.push_back(glm::vec2(u, v));
        } else if (token == "f") {
            std::string f1;
            std::string f2;
            std::string f3;
            iss >> f1 >> f2 >> f3;

            std::string f4;
            const bool isQuad = static_cast<bool>(iss >> f4);

            Vertex v1;
            Vertex v2;
            Vertex v3;
            parseFaceVertex(f1, positions, textureCoords, normals, v1);
            parseFaceVertex(f2, positions, textureCoords, normals, v2);
            parseFaceVertex(f3, positions, textureCoords, normals, v3);
            vertices.push_back(v1);
            vertices.push_back(v2);
            vertices.push_back(v3);

            if (isQuad) {
                Vertex v4;
                parseFaceVertex(f4, positions, textureCoords, normals, v4);
                vertices.push_back(v1);
                vertices.push_back(v3);
                vertices.push_back(v4);
            }
        }
    }

    std::cout << "Load model with " << vertices.size() << " vertices" << std::endl;
    numVertices = static_cast<int>(vertices.size());
}

Object::~Object() {
    releaseGpuResources();
}

void Object::releaseGpuResources() {
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
}

void Object::makeObject(const Shader& shader, bool texture) {
    if (numVertices <= 0) {
        return;
    }

    releaseGpuResources();

    float* data = new float[8 * numVertices];
    for (int i = 0; i < numVertices; ++i) {
        const Vertex& vertex = vertices.at(static_cast<size_t>(i));
        data[i * 8 + 0] = vertex.Position.x;
        data[i * 8 + 1] = vertex.Position.y;
        data[i * 8 + 2] = vertex.Position.z;
        data[i * 8 + 3] = vertex.Texture.x;
        data[i * 8 + 4] = vertex.Texture.y;
        data[i * 8 + 5] = vertex.Normal.x;
        data[i * 8 + 6] = vertex.Normal.y;
        data[i * 8 + 7] = vertex.Normal.z;
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8 * numVertices, data, GL_STATIC_DRAW);

    const GLint attPos = glGetAttribLocation(shader.ID, "aPos");
    glEnableVertexAttribArray(attPos);
    glVertexAttribPointer(attPos, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(0));

    if (texture) {
        const GLint attTex = glGetAttribLocation(shader.ID, "aTexCoords");
        glEnableVertexAttribArray(attTex);
        glVertexAttribPointer(
            attTex,
            2,
            GL_FLOAT,
            GL_FALSE,
            8 * sizeof(float),
            reinterpret_cast<void*>(3 * sizeof(float))
        );
    }

    const GLint attNormal = glGetAttribLocation(shader.ID, "aNormal");
    glEnableVertexAttribArray(attNormal);
    glVertexAttribPointer(
        attNormal,
        3,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        reinterpret_cast<void*>(5 * sizeof(float))
    );

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    delete[] data;
}

void Object::draw() const {
    if (VAO == 0 || numVertices <= 0) {
        return;
    }

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, numVertices);
}
