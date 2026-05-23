#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>

#include <string>

enum class TextureTarget {
    Texture2D,
    CubeMap
};

class Texture {
public:
    Texture() = default;
    ~Texture();

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    static Texture loadFromFile2D(const std::string& path);
    static Texture loadVoidSpaceCubemap();

    bool isValid() const { return id_ != 0; }
    const std::string& path() const { return path_; }

    void bind(unsigned int unit = 0) const;
    void unbind() const;

private:
    GLuint id_ = 0;
    TextureTarget target_ = TextureTarget::Texture2D;
    std::string path_;

    void release();
};

#endif
