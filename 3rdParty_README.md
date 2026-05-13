# Dépendances Tierces (3rdParty)

Ce projet nécessite plusieurs bibliothèques externes pour fonctionner.

## Structure attendue

```
3rdParty/
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

### Option 1: Copier depuis le projet existant
Si vous avez accès au projet INFO-H-502, copiez le dossier `3rdParty/` complet.

### Option 2: Télécharger manuellement

1. **GLAD** :
   - Allez sur https://glad.dav1d.de/
   - Sélectionnez OpenGL 3.3 Core
   - Téléchargez et placez dans `3rdParty/glad/`

2. **GLFW** :
   - Téléchargez depuis https://www.glfw.org/download.html
   - Version source, placez dans `3rdParty/glfw/`

3. **GLM** :
   - Téléchargez depuis https://github.com/g-truc/glm/releases
   - Placez dans `3rdParty/glm/`

4. **STB** :
   - Téléchargez stb_image.h depuis https://github.com/nothings/stb
   - Placez dans `3rdParty/stb/`

## Configuration CMake

Le CMakeLists.txt est configuré pour trouver ces bibliothèques automatiquement.
Assurez-vous que les chemins dans `include_directories()` correspondent à votre structure.

## Notes

- GLFW peut nécessiter des dépendances système (X11 sur Linux, etc.)
- Sur macOS, GLFW utilise Cocoa
- Sur Windows, GLFW utilise Win32