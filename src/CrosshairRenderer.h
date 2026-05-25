#ifndef CROSSHAIR_RENDERER_H
#define CROSSHAIR_RENDERER_H

#include <glad/glad.h>
#include "shader.h"

class CrosshairRenderer {
public:
    void Init();
    void Render(Shader& shader, int viewportWidth, int viewportHeight);
    void Destroy();

    void Toggle();
    bool IsVisible() const;

private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    bool visible = true;
};

#endif