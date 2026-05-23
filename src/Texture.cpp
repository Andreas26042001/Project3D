#include "Texture.h"

#include "GameInternal.h"

#include <iostream>

#include "stb_image.h"

namespace {

GLenum imageFormatFromChannels(int channels) {
    if (channels == 1) {
        return GL_RED;
    }
    if (channels == 4) {
        return GL_RGBA;
    }
    return GL_RGB;
}

void applySampler2D() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void applySamplerCubeMap() {
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

bool loadCubemapFace(const std::string& path, GLenum targetFace) {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (data == nullptr) {
        return false;
    }

    const GLenum format = imageFormatFromChannels(channels);
    glTexImage2D(targetFace, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return true;
}

} // namespace

Texture::~Texture() {
    release();
}

Texture::Texture(Texture&& other) noexcept
    : id_(other.id_),
      target_(other.target_),
      path_(std::move(other.path_)) {
    other.id_ = 0;
    other.target_ = TextureTarget::Texture2D;
    other.path_.clear();
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        release();
        id_ = other.id_;
        target_ = other.target_;
        path_ = std::move(other.path_);
        other.id_ = 0;
        other.target_ = TextureTarget::Texture2D;
        other.path_.clear();
    }
    return *this;
}

void Texture::release() {
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
        id_ = 0;
    }
    path_.clear();
    target_ = TextureTarget::Texture2D;
}

Texture Texture::loadFromFile2D(const std::string& path) {
    Texture texture;
    texture.path_ = path;

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (data == nullptr) {
        std::cout << "WARNING: failed to load texture " << path << std::endl;
        return texture;
    }

    const GLenum format = imageFormatFromChannels(channels);
    glGenTextures(1, &texture.id_);
    glBindTexture(GL_TEXTURE_2D, texture.id_);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    applySampler2D();
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    texture.target_ = TextureTarget::Texture2D;
    return texture;
}

Texture Texture::loadVoidSpaceCubemap() {
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

    const std::string directory = game_internal::joinPath(
        PATH_TO_CUBESMAPS,
        "Void space cubemap"
    );

    Texture texture;
    texture.target_ = TextureTarget::CubeMap;
    texture.path_ = directory;

    glGenTextures(1, &texture.id_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture.id_);
    applySamplerCubeMap();

    for (int i = 0; i < 6; ++i) {
        const std::string facePath = game_internal::joinPath(directory, kFaceFiles[i]);
        if (!loadCubemapFace(facePath, kFaceTargets[i])) {
            std::cout << "WARNING: failed to load cubemap face: " << facePath << std::endl;
            texture.release();
            return Texture{};
        }
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    std::cout << "Cubemap loaded from " << directory << std::endl;
    return texture;
}

void Texture::bind(unsigned int unit) const {
    if (id_ == 0) {
        return;
    }

    glActiveTexture(GL_TEXTURE0 + unit);
    if (target_ == TextureTarget::CubeMap) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
    } else {
        glBindTexture(GL_TEXTURE_2D, id_);
    }
}

void Texture::unbind() const {
    if (target_ == TextureTarget::CubeMap) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}
