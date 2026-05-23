#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include <string>

#include <glm/glm.hpp>

class Shader {
public:
    GLuint ID;

    Shader(const char* vertexPath, const char* fragmentPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;
    void setInt(const std::string& name, GLint value) const;
    void setFloat(const std::string& name, GLfloat value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat4(const std::string& name, const glm::mat4& matrix) const;

private:
    GLuint compileShader(const std::string& shaderCode, GLenum shaderType) const;
    GLuint compileProgram(GLuint vertexShader, GLuint fragmentShader) const;
};

#endif
