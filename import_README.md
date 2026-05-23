# Dépendances tierces (`import/`)

Ce projet utilise le dossier `import/` (comme [DylanMichel0304/GLSL](https://github.com/DylanMichel0304/GLSL)) pour GLFW, GLAD, GLM et STB.

## Structure attendue

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
│       └── ... (tous les headers GLM)
└── stb/
    ├── stb_image.h
    └── stb_image_write.h
```

## Comment obtenir les dépendances

### Option 1 : copier depuis le cours INFO-H-502

```bash
cp -R /chemin/vers/info-h502_202526/3rdParty import
```

### Option 2 : téléchargement manuel

1. **GLAD** — https://glad.dav1d.de/ (OpenGL 4.0 Core) → `import/glad/`
2. **GLFW** — https://www.glfw.org/download.html (sources) → `import/glfw/`
3. **GLM** — https://github.com/g-truc/glm/releases → `import/glm/`
4. **STB** — https://github.com/nothings/stb → `import/stb/`

## Notes

- GLFW peut nécessiter des dépendances système (X11 sur Linux, etc.)
- Sur macOS, GLFW utilise Cocoa
- Sur Windows, GLFW utilise Win32
