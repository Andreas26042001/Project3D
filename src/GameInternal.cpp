#include "GameInternal.h"

#include <fstream>
#include <iostream>

#include "stb_image.h"

namespace game_internal {

std::string resourcePath(const char* relativePath) {
    const std::string root(GAME_RESOURCE_DIR);
    if (root.empty()) {
        return relativePath;
    }
    const char last = root.back();
    if (last == '/' || last == '\\') {
        return root + relativePath;
    }
    return root + "/" + relativePath;
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

} // namespace game_internal
