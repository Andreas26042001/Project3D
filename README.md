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
├── CMakeLists.txt          # Configuration CMake
├── README.md               # Documentation
├── assets/                 # Ressources
│   ├── models/            # Fichiers .obj
│   └── textures/           # Images .jpg/.png
├── include/               # Headers
│   ├── camera.h           # Classe caméra
│   ├── shader.h           # Classe shader
│   └── object.h           # Classe pour charger modèles
├── src/                   # Code source
│   ├── main.cpp           # Point d'entrée
│   ├── Game.cpp           # Logique de jeu
│   ├── Game.h
│   ├── Light.cpp          # Système d'éclairage
│   └── Light.h
├── shaders/               # Shaders GLSL
│   ├── basic.vert
│   ├── basic.frag
│   ├── phong.vert
│   ├── phong.frag
│   ├── cubemap.vert
│   └── cubemap.frag
└── build/                 # Dossier de build (généré)
```

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