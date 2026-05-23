#ifndef GAME_INTERNAL_H
#define GAME_INTERNAL_H

#include <string>

#ifndef PATH_TO_OBJECTS
#define PATH_TO_OBJECTS "../assets/objects"
#endif
#ifndef PATH_TO_TEXTURE
#define PATH_TO_TEXTURE "../assets/textures"
#endif
#ifndef PATH_TO_SHADER
#define PATH_TO_SHADER "../shader"
#endif
#ifndef PATH_TO_CUBESMAPS
#define PATH_TO_CUBESMAPS "../assets/cubesmaps"
#endif

namespace game_internal {

constexpr int kShadowMapSize = 2048;

std::string joinPath(const std::string& base, const char* relativePath);
std::string objectPath(const char* filename);
std::string texturePath(const char* relativePath);
std::string shaderPath(const char* filename);

} // namespace game_internal

#endif
