# Assets

Données chargées à l'exécution, calquées sur la structure de [DylanMichel0304/GLSL](https://github.com/DylanMichel0304/GLSL).

```
assets/
├── objects/     # Modèles .obj / .mtl
├── textures/    # Images diffuses
├── cubesmaps/   # Faces de skybox
└── blender/     # Sources Blender (non chargées directement)
```

## Workflow Blender

1. Modéliser dans `assets/blender/`.
2. Exporter le maillage (`.obj`) dans `assets/objects/`.
3. Charger dans le code avec `game_internal::objectPath("mon_modele.obj")`.

## Chemins CMake

Le `CMakeLists.txt` injecte les macros suivantes :

| Macro | Dossier |
|-------|---------|
| `PATH_TO_OBJECTS` | `assets/objects/` |
| `PATH_TO_TEXTURE` | `assets/textures/` |
| `PATH_TO_CUBESMAPS` | `assets/cubesmaps/` |
| `PATH_TO_SHADER` | `shader/` |

Les shaders GLSL sont dans le dossier **`shader/`** à la racine du projet (pas sous `assets/`).
