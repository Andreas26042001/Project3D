# Third-party dependencies (`import/`)

This project uses the `import/` folder (like [DylanMichel0304/GLSL](https://github.com/DylanMichel0304/GLSL)) for GLFW, GLAD, GLM, and STB.

## Expected layout

```
import/
├── glad/
│   ├── include/
│   │   └── glad/
│   │       └── glad.h
│   └── src/
│       └── glad.c
├── glfw/
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── GLFW/
│   └── src/
├── glm/
│   └── glm/
│       ├── glm.hpp
│       └── ... (all GLM headers)
└── stb/
    ├── stb_image.h
    └── stb_image_write.h
```

## How to obtain dependencies

### Option 1: copy from the INFO-H-502 course materials

```bash
cp -R /path/to/info-h502_202526/3rdParty import
```

### Option 2: manual download

1. **GLAD** — https://glad.dav1d.de/ (OpenGL 4.0 Core) → `import/glad/`
2. **GLFW** — https://www.glfw.org/download.html (sources) → `import/glfw/`
3. **GLM** — https://github.com/g-truc/glm/releases → `import/glm/`
4. **STB** — https://github.com/nothings/stb → `import/stb/`

## Notes

- GLFW may require system libraries (X11 on Linux, etc.)
- On macOS, GLFW uses Cocoa
- On Windows, GLFW uses Win32
