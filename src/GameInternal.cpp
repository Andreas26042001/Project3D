#include "GameInternal.h"

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

} // namespace game_internal
