# CanvasTexture — peindre sur une texture dynamique

`compages::renderer::CanvasTexture` est un tampon RGBA8 côté CPU, enregistré comme `TextureAsset`, que tu peux mettre à jour pendant la simulation. Le cas d’usage principal est une **toile** (mesh plan) sur laquelle un outil ou la souris dépose de la peinture, comme dans une démo de bras peintre.

**En-tête :** `<Compages/Renderer/Assets/CanvasTexture.hpp>`

**Exemple :** `39a_CanvasPaint` dans la galerie.

## Cycle de vie

```cpp
compages::renderer::CanvasTexture canvas;
COMPAGES_TRY(canvas.create(512u, 512u));  // blanc par défaut

// Correspond au mesh : plane 1×1 en local, avant scale()
canvas.setMapping(compages::core::AABB::fromCorners(
    { -0.5f, -0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f }));

COMPAGES_TRY(canvas.bind(scene.assets(), "my_canvas"));
canvas.applyLook(scene, easel_entity);   // PbrMinimal + base_color_texture

// Boucle :
canvas.paintWorld(hit.point, easel.worldMatrix(), color, radius_metres);
COMPAGES_TRY(canvas.sync());             // upload GPU (région sale ou tout)
scene.draw(frame);
```

| Étape | Rôle |
|--------|------|
| `create` | Alloue les pixels CPU |
| `setMapping` | Local mesh → UV (bbox + flip U/V optionnels) |
| `bind` | Crée ou met à jour le `TextureAsset` dans le catalogue |
| `paintLocal` / `paintWorld` / `paintLineWorld` | Tampon circulaire |
| `sync` | `Texture::write` (partiel si possible) |
| `applyLook` | Raccourci vers `Scene::look(entity, texture_id)` |

## Coordonnées

- Les pixels sont stockés **ligne par ligne depuis le bas**, comme OpenGL et `Texture::write`.
- `paintWorld` transforme le point monde en local via l’**inverse** de la matrice monde du canvas.
- Le rayon du pinceau est en **unités monde** ; il est converti en pixels à partir de l’étendue locale en X et de la largeur de la texture.

Si l’image est miroir ou à l’envers, ajuste `setMapping(..., flip_u, flip_v)`.

## API Scene associée

```cpp
scene.look(entity, texture_asset_id);
```

Crée une nouvelle `MaterialInstance` `PbrMinimal` avec `base_color_texture` pointant vers ton asset dynamique, sans passer par un fichier disque.

`AssetManager::texture(id)` non-const permet à `CanvasTexture` de mettre à jour l’image GPU déjà enregistrée.

## Performance

- `sync()` n’envoie qu’un **rectangle sale** après des tampons localisés ; un `clear()` marque toute la texture.
- Filtre **Nearest** à l’allocation : pas de mipmaps à régénérer à chaque frame.
- Pour de très grandes toiles (4K+), limite la fréquence de `sync()` ou regroupe les traits.

## Voir aussi

- [Renderer.md](Renderer.md) — apparences et catalogue
- [GPU.md](GPU.md) — `Texture::write`
- [Examples.md](Examples.md) — `39a_CanvasPaint`
