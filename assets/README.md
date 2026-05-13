# Dossier `assets/`

Ce répertoire regroupe les **fichiers sources** qui ne sont pas nécessaires au lancement du jeu tel quel (scènes Blender, maquettes, etc.).

## Contenu prévu

- **`blender/`** : fichiers `.blend` et sauvegardes d’édition. Exportez les maillages (par ex. en `.obj`) vers `resources/models/` pour les charger dans le code.

## Ressources runtime

Les fichiers réellement chargés par l’application se trouvent sous **`resources/`** à la racine du projet :

- `resources/glsl/` — shaders
- `resources/models/` — `.obj` exportés
- `resources/textures/` — images

Le chemin de base est injecté à la compilation (`GAME_RESOURCE_DIR` dans `CMakeLists.txt`). Dans le code C++, utilisez la fonction `resourcePath("…")` définie dans `Game.cpp` pour composer un chemin sous `resources/`.

## Exemple

1. Modéliser dans `assets/blender/ma_scene.blend`.
2. Exporter `ma_scene.obj` dans `resources/models/`.
3. Charger avec `new Object(resourcePath("models/ma_scene.obj").c_str())`.
