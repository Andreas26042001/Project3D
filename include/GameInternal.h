#ifndef GAME_INTERNAL_H
#define GAME_INTERNAL_H

#include <string>
#include <glad/glad.h>

#ifndef GAME_RESOURCE_DIR
#define GAME_RESOURCE_DIR "../resources"
#endif

namespace game_internal {

constexpr int kShadowMapSize = 2048;

std::string resourcePath(const char* relativePath);
bool fileExists(const std::string& path);
GLuint loadTexture2D(const std::string& path);

} // namespace game_internal

#endif
