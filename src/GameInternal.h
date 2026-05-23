#ifndef GAME_INTERNAL_H
#define GAME_INTERNAL_H

#include <string>
#include <glad/glad.h>

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

constexpr bool kEnableChapter22Collision = true;
constexpr bool kEnableChapter14Translucency = true;

std::string joinPath(const std::string& base, const char* relativePath);
std::string objectPath(const char* filename);
std::string texturePath(const char* relativePath);
std::string shaderPath(const char* filename);
bool fileExists(const std::string& path);
GLuint loadTexture2D(const std::string& path);
GLuint loadVoidSpaceCubemap();

} // namespace game_internal

#endif
