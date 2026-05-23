#include "GameInternal.h"

#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#include "stb_image.h"

namespace game_internal {

std::string joinPath(const std::string& base, const char* relativePath) {
    if (base.empty()) {
        return relativePath;
    }
    const char last = base.back();
    if (last == '/' || last == '\\') {
        return base + relativePath;
    }
    return base + "/" + relativePath;
}

std::string objectPath(const char* filename) {
    return joinPath(PATH_TO_OBJECTS, filename);
}

std::string texturePath(const char* relativePath) {
    return joinPath(PATH_TO_TEXTURE, relativePath);
}

std::string shaderPath(const char* filename) {
    return joinPath(PATH_TO_SHADER, filename);
}

bool fileExists(const std::string& path) {
    std::ifstream file(path.c_str());
    return file.good();
}

GLuint loadTexture2D(const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (data == nullptr) {
        std::cout << "WARNING: Impossible de charger la texture " << path << std::endl;
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 1) {
        format = GL_RED;
    } else if (channels == 4) {
        format = GL_RGBA;
    }

    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    return textureId;
}

namespace {

bool loadCubemapFace(const std::string& path, GLenum targetFace) {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (data == nullptr) {
        return false;
    }

    GLenum format = GL_RGB;
    if (channels == 1) {
        format = GL_RED;
    } else if (channels == 4) {
        format = GL_RGBA;
    }

    glTexImage2D(targetFace, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return true;
}

} // namespace

GLuint loadVoidSpaceCubemap() {
    static const GLenum kFaceTargets[6] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    static const char* kFaceFiles[6] = {
        "jettelly_space_common_black_RIGHT.png",
        "jettelly_space_common_black_LEFT.png",
        "jettelly_space_common_black_UP.png",
        "jettelly_space_common_black_DOWN.png",
        "jettelly_space_common_black_FRONT.png",
        "jettelly_space_common_black_BACK.png",
    };

    const std::string directory = joinPath(PATH_TO_CUBESMAPS, "Void space cubemap");

    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    for (int i = 0; i < 6; ++i) {
        const std::string facePath = joinPath(directory, kFaceFiles[i]);
        if (!loadCubemapFace(facePath, kFaceTargets[i])) {
            std::cout << "WARNING: echec chargement cubemap: " << facePath << std::endl;
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            glDeleteTextures(1, &textureId);
            return 0;
        }
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    std::cout << "Cubemap chargee depuis " << directory << std::endl;
    return textureId;
}

} // namespace game_internal
