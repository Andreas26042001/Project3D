# Mon Projet OpenGL - INFO-H-502

Template de projet OpenGL pour le cours INFO-H-502.

## Fonctionnalités

- ✅ Caméra contrôlable (WASD + souris)
- ✅ Chargement de modèles .obj
- ✅ Système d'éclairage de base (Phong)
- ✅ Architecture modulaire extensible
- ✅ Support pour textures et cubemaps

## Structure du Projet

```
mon_projet_opengl/
├── CMakeLists.txt          # Configuration CMake (définit GAME_RESOURCE_DIR → resources/)
├── README.md               # Documentation
├── resources/              # Données chargées à l'exécution
│   ├── glsl/               # Shaders GLSL (.vert, .frag)
│   ├── models/             # Maillages exportés (.obj, etc.)
│   └── textures/           # Images (.jpg, .png, …)
├── assets/                 # Fichiers sources / hors pipeline runtime
│   └── blender/            # Scènes .blend (exportez vers resources/models/)
├── include/                # Headers
│   ├── camera.h
│   ├── shader.h
│   └── object.h
├── src/                    # Code source
│   ├── main.cpp
│   ├── Game.cpp
│   ├── Game.h
│   ├── Light.cpp
│   └── Light.h
└── build/                  # Dossier de build (généré)
```

Les chemins vers `resources/` sont résolus via la macro `GAME_RESOURCE_DIR` (chemin absolu défini par CMake) : l'exécutable peut être lancé depuis n'importe quel répertoire courant.

## Installation

1. **Ajouter les dépendances** dans un dossier `3rdParty/` :
   - GLFW (window management)
   - GLAD (OpenGL function loading)
   - GLM (mathématiques)
   - STB Image (chargement d'images)

2. **Compiler le projet** :
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

3. **Exécuter** :
   ```bash
   ./MonProjetOpenGL
   ```

## Contrôles

- **WASD** : Déplacement caméra
- **Souris** : Rotation caméra
- **Molette** : Zoom
- **Échap** : Quitter

## Développement

Ce template fournit une base solide pour développer votre projet final. Vous pouvez étendre les fonctionnalités en ajoutant :

- Nouveaux shaders (particules, effets spéciaux)
- Systèmes de jeu (physique, IA)
- Nouvelles classes (Player, Enemy, etc.)
- Effets avancés (FBO, instancing)

## Auteur

Template généré pour le cours INFO-H-502