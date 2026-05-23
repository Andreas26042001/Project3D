#include "shader.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>

Shader::Shader(const char* vertexPath, const char* fragmentPath)
    : ID(0) {
    std::ifstream vShaderFile(vertexPath);
    std::ifstream fShaderFile(fragmentPath);
    if (!vShaderFile.is_open() || !fShaderFile.is_open()) {
        std::cout << "ERROR::SHADER::FILE_NOT_OPEN: " << vertexPath << " or " << fragmentPath << std::endl;
        return;
    }

    std::stringstream vShaderStream;
    std::stringstream fShaderStream;
    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();

    const std::string vertexCode = vShaderStream.str();
    const std::string fragmentCode = fShaderStream.str();
    if (vertexCode.empty() || fragmentCode.empty()) {
        std::cout << "ERROR::SHADER::EMPTY_SHADER_CODE" << std::endl;
        return;
    }

    const GLuint vertex = compileShader(vertexCode, GL_VERTEX_SHADER);
    const GLuint fragment = compileShader(fragmentCode, GL_FRAGMENT_SHADER);
    ID = compileProgram(vertex, fragment);
}

Shader::~Shader() {
    if (ID != 0) {
        glDeleteProgram(ID);
        ID = 0;
    }
}

void Shader::use() const {
    glUseProgram(ID);
}

void Shader::setInt(const std::string& name, GLint value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, GLfloat value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2f(glGetUniformLocation(ID, name.c_str()), value.x, value.y);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3f(glGetUniformLocation(ID, name.c_str()), value.x, value.y, value.z);
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4f(glGetUniformLocation(ID, name.c_str()), value.x, value.y, value.z, value.w);
}

void Shader::setMat4(const std::string& name, const glm::mat4& matrix) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
}

GLuint Shader::compileShader(const std::string& shaderCode, GLenum shaderType) const {
    const GLuint shader = glCreateShader(shaderType);
    const char* code = shaderCode.c_str();
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::string shaderLabel = "shader";
        if (shaderType == GL_VERTEX_SHADER) {
            shaderLabel = "vertex shader";
        } else if (shaderType == GL_FRAGMENT_SHADER) {
            shaderLabel = "fragment shader";
        }
        std::cout << "ERROR::SHADER_COMPILATION_ERROR of the " << shaderLabel << ": " << infoLog << std::endl;
    }
    return shader;
}

GLuint Shader::compileProgram(GLuint vertexShader, GLuint fragmentShader) const {
    const GLuint programID = glCreateProgram();
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint success = GL_FALSE;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetProgramInfoLog(programID, 1024, nullptr, infoLog);
        std::cout << "ERROR::PROGRAM_LINKING_ERROR: " << infoLog << std::endl;
        glDeleteProgram(programID);
        return 0;
    }
    return programID;
}
