# Mon Projet OpenGL — INFO-H-502

Jeu de puzzle en vue à la première personne dans une grotte éclairée au Phong. Le joueur manipule des piliers mobiles sur des rails, tire un projectile de lumière et oriente un faisceau vers des cibles murales via un prisme déflecteur.

**Contexte OpenGL :** profil Core **4.0** (GLFW + GLAD), shaders **GLSL 330 core**.

## Fonctionnalités du jeu

- Caméra FPS (WASD, souris, molette) avec collisions AABB et hauteur de support dynamique
- Scène fermée : sol texturé, grille de piliers, murs, plafond, skybox cubemap
- Éclairage Phong multi-sources : 4 torches d'angle (scintillement) + torche portée par le joueur
- Puzzle mécanique :
  - Pilier central mobile sur rail (poussé par le joueur) avec canon lumineux
  - Pilier déflecteur mobile sur rail perpendiculaire + prisme translucide
  - Deux cibles murales activées par maintien du faisceau (~3 s)
- Projectile de lumière (`F`) avec collisions et explosion de particules
- Chargement de maillages `.obj` (cube procédural pour la scène statique, `capture_pillar.obj` pour les piliers mobiles)
- Textures diffuses (sol, briques des piliers, métal du pilier récepteur)
- Reticle de visée et mode pause

## Contrôles

| Touche | Action |
|--------|--------|
| **W A S D** | Déplacement |
| **Souris** | Orientation |
| **Molette** | Zoom (FOV) |
| **F** | Tirer un projectile de lumière |
| **P** | Pause / reprise (libère ou recapture la souris) |
| **C** | Afficher / masquer le réticule |
| **Échap** | Quitter |

## Récupérer et compiler le projet

Le dépôt public est [github.com/Andreas26042001/Project3D](https://github.com/Andreas26042001/Project3D).

Le dépôt contient **tout le nécessaire** pour compiler (`import/`, `assets/`, `shader/`, code source). Après un clone, suivez uniquement les étapes ci-dessous.

### Prérequis

- **CMake** ≥ 3.20
- Compilateur **C++14** (`g++`, Clang, MSVC…)
- **OpenGL** (drivers / SDK système)
- **make** ou **cmake --build**

Sous Linux : `build-essential`, `cmake`, et au besoin `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, `libxi-dev`, `mesa-common-dev`.

### Build rapide

```bash
git clone https://github.com/Andreas26042001/Project3D.git
cd Project3D

mkdir build && cd build
cmake ..
cmake --build .
```

Puis, depuis la **racine du projet** :

```bash
./bin/MonProjetOpenGL
```

Sous macOS / Linux, `make` peut remplacer `cmake --build .` si vous préférez.

### Vérification (optionnelle)

Après le clone, pour confirmer que rien ne manque :

```bash
test -f import/glad/src/glad.c \
  && test -f shader/phong.vert \
  && test -f assets/objects/cube.obj \
  && echo "OK — lancez cmake"
```

Si cette commande échoue, le clone est incomplet (vérifiez que `git pull` a bien récupéré tout le dépôt, y compris `import/`).

### Contenu versionné sur Git

| Inclus dans le dépôt | Ignoré (`.gitignore`) |
|----------------------|------------------------|
| `CMakeLists.txt`, `main.cpp`, `src/` | `build/`, `bin/` |
| `shader/`, `assets/` | `CMakeFiles/`, `CMakeCache.txt` |
| `import/` (GLFW, GLAD, GLM, STB) | `.DS_Store`, `.vscode/` |
| `LAB03/` (référence, non compilé par le CMake racine) | — |

Les chemins vers les assets sont injectés à la compilation :

| Macro CMake | Dossier |
|-------------|---------|
| `PATH_TO_OBJECTS` | `assets/objects/` |
| `PATH_TO_TEXTURE` | `assets/textures/` |
| `PATH_TO_CUBESMAPS` | `assets/cubesmaps/` |
| `PATH_TO_SHADER` | `shader/` |

L’exécutable fonctionne quel que soit le répertoire courant.

### Dépannage

| Erreur | Cause probable | Solution |
|--------|----------------|----------|
| `import/glad/src/glad.c: No such file` | Dossier `import/` absent | Refaire un clone complet ; ne pas utiliser « Download ZIP » sans `import/` |
| `Failed to create GLFW window` | Drivers OpenGL | Mettre à jour les drivers GPU |
| Textures manquantes en jeu | `assets/` incomplet | Vérifier que `assets/textures/.../*_Color.jpg` est présent |
| `cmake: command not found` | CMake non installé | Installer CMake ≥ 3.20 |

### IDE (optionnel)

Ouvrir le dossier cloné dans VS Code / Cursor avec les extensions **C++** et **CMake Tools**, puis **CMake: Configure** → **Build**. L’exécutable sera dans `bin/MonProjetOpenGL`.

## Structure du projet

```
Project3D/
├── CMakeLists.txt              # Style GLSL / INFO-H-502, sortie dans bin/
├── README.md
├── import_README.md            # Instructions pour GLFW, GLAD, GLM, STB
├── main.cpp                    # Point d'entrée (à la racine)
├── assets/                     # Données chargées à l'exécution
│   ├── objects/                # Maillages .obj
│   ├── textures/               # Images .jpg / .png
│   ├── cubesmaps/              # Faces de skybox
│   └── blender/                # Sources Blender (hors pipeline)
├── shader/                     # Shaders GLSL (.vert, .frag)
├── import/                     # GLFW, GLAD, GLM, STB
├── src/                        # Code source + headers
│   ├── Game.cpp                # Initialisation scène, ressources GPU
│   ├── Game.h
│   ├── Game_Render.cpp         # Passes de rendu (ombres, Phong, skybox…)
│   ├── Game_Physics.cpp        # Collisions, puzzle, particules
│   ├── GameInternal.cpp        # Chemins ressources, chargement textures
│   ├── camera.h, shader.h, object.h, …
│   └── Light.cpp / Light.h
├── LAB03/                      # Exercices du laboratoire (référence)
├── build/                      # Dossier de configuration CMake (généré)
└── bin/                        # Exécutable compilé (généré)
```

## Fonctionnalités OpenGL / GLSL

Liste des techniques graphiques implémentées dans le projet principal (`src/` + `shader/`).

### Pipeline et état OpenGL

| Fonctionnalité | Détail |
|----------------|--------|
| Profil Core 4.0 | Contexte GLFW + chargement des fonctions via GLAD |
| VAO / VBO | Géométrie statique (modèles OBJ), skybox, réticule, cibles, prisme |
| Test de profondeur | `GL_DEPTH_TEST` ; `GL_LEQUAL` pour le skybox |
| Tampon stencil | 8 bits configurés à la création de la fenêtre |
| Framebuffers (FBO) | Passes shadow map depth-only (`GL_DRAW_BUFFER = GL_NONE`) |
| Redimensionnement | `glViewport` via callback framebuffer |
| Multi-textures | Unités `GL_TEXTURE0`–`GL_TEXTURE5` (diffuse + 5 shadow maps) |
| Face culling | `GL_FRONT` en passe d'ombre (second-depth mapping) |
| Polygon offset | `glPolygonOffset` slope-scale en génération des shadow maps |
| Alpha blending | Prisme déflecteur (`GL_SRC_ALPHA`, `GL_ONE_MINUS_SRC_ALPHA`) |
| Masque de profondeur | `glDepthMask(GL_FALSE)` pour la géométrie translucide |

### Shaders actifs

| Paire | Rôle |
|-------|------|
| `phong.vert` / `phong.frag` | Éclairage Phong par fragment, textures, ombres |
| `lamp.vert` / `lamp.frag` | Marqueurs émissifs unlit (torche, rails, cibles, particules) |
| `cubemap.vert` / `cubemap.frag` | Skybox (`samplerCube`) |
| `shadow_depth.vert` / `shadow_depth.frag` | Passe profondeur seule pour shadow mapping |
| *(inline)* crosshair | Overlay 2D en espace écran (`GL_LINES`) |
| *(inline)* prism | Octaèdre translucide (couleur RGBA uniforme) |

Shaders hérités du labo (`basic.*`, `gouraud.*`) présents dans `shader/` mais **non utilisés** par l'exécutable principal.

### Éclairage (GLSL)

- Modèle **Phong** : ambient, diffuse (`dot`), specular (`reflect` + `pow`)
- **Plusieurs lumières ponctuelles** avec atténuation linéaire / quadratique
- **4 torches d'angle** avec scintillement procédural (`sin` côté CPU)
- **Torche portée** avec shadow map dynamique perspective
- **Sources linéaires** (faisceau canon / rayon dévié) : point le plus proche sur segment (approche Heidrich/Seidel)
- Matrice normale : `transpose(inverse(model))`
- Structures GLSL `Material` et `Light` avec uniforms

### Textures

- `sampler2D` pour cartes diffuses (sol, briques, métal)
- Uniform `uvScale` pour répétition / tiling par type de surface
- Chargement JPEG via STB Image (`GameInternal::loadTexture2D`)
- Cubemap void space (`assets/cubesmaps/Void space cubemap/`, style LAB03)

### Shadow mapping

| Technique | Implémentation |
|-----------|----------------|
| Shadow maps statiques | 4 cartes 2048×2048, projection **orthographique**, calculées une fois au démarrage |
| Shadow map dynamique | 1 carte 2048×2048, projection **perspective** (spotlight portée), recalculée chaque frame |
| Échantillonnage | `texture(shadowMap, projCoords.xy).r` + comparaison de profondeur |
| Biais slope-scale | `glPolygonOffset(1.5, 4.0)` côté CPU |
| Biais normal-offset | Décalage Holbert dans `phong.frag` (`sinTheta`) |
| Second-depth mapping | Culling des faces avant en passe d'ombre |
| Bordure éclairée | `GL_CLAMP_TO_BORDER` + `GL_TEXTURE_BORDER_COLOR` blanc |
| Ombres stables | Snap position (grille 1 mm) + snap texel NDC pour la lumière dynamique |
| Ombres planaires analytiques | Disques `smoothstep` sous les piliers (passe sol uniquement) |

### Clipping et effets avancés

- **`gl_ClipDistance[0..4]`** : plan de clip réflexion + 4 plans de bord (infrastructure prête, désactivée par défaut)
- **Skybox infinie** : `gl_Position = pos.xyww` (depth = 1.0)
- **Réticule HUD** : coordonnées NDC, depth test désactivé
- **Géométrie procédurale GPU** : disques/anneaux (cibles), octaèdre (prisme), particules (cubes instanciés)

### Passes de rendu (ordre)

1. Shadow maps (statiques + dynamique) → FBO depth
2. Scène opaque Phong (piliers, murs, sol texturé)
3. Marqueurs lamp shader (torche, rails, cibles, joueur, torches d'angle)
4. Particules d'explosion
5. Prisme translucide (blending)
6. Skybox
7. Réticule 2D

### Références graphiques

Les commentaires du code renvoient aux chapitres pertinents de *Real-Time Rendering* (4e éd.) : shadow maps (§7.4), ombres planaires (§7.1), translucidité (ch. 14).

## Développement

Le code est découpé en modules (`Game_Render`, `Game_Physics`, `GameInternal`) pour faciliter l'extension. Flags de compilation dans `GameInternal.h` :

- `kEnableChapter22Collision` — collisions et puzzle des piliers
- `kEnableChapter14Translucency` — prisme déflecteur translucide

## Auteur

Projet INFO-H-502 — template étendu en démo de puzzle lumineux.
