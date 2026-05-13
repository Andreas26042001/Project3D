# Assets du Projet

Ce dossier contient toutes les ressources du projet (modèles 3D, textures, etc.).

## Structure

- `models/` : Fichiers de modèles 3D (.obj, .fbx, etc.)
- `textures/` : Images de textures (.jpg, .png, etc.)

## Comment ajouter des assets

1. **Modèles 3D** :
   - Placez vos fichiers .obj dans `models/`
   - Assurez-vous que les fichiers de matériaux (.mtl) sont présents si nécessaire
   - Utilisez des chemins relatifs dans le code : `"assets/models/mon_modele.obj"`

2. **Textures** :
   - Placez vos images dans `textures/`
   - Formats supportés : JPG, PNG, BMP, etc.
   - Utilisez des chemins relatifs : `"assets/textures/ma_texture.png"`

## Exemple d'utilisation

```cpp
// Charger un modèle
Object* monObjet = new Object("assets/models/cube.obj");
monObjet->makeObject(*monShader);

// Charger une texture (dans un shader)
monShader->setInt("texture1", 0);
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, textureID);
```

## Notes importantes

- Les chemins sont relatifs au répertoire racine du projet
- Assurez-vous que les modèles ont des normales et des coordonnées de texture si nécessaire
- Pour les cubemaps, placez 6 images séparées ou utilisez un format spécial