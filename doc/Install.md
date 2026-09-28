# Compilation et installation

Compages se compile en **C++20**, surtout sous Linux. Le backend `GL45` demande un contexte OpenGL **4.5 core** pour les exemples et les tests GPU. La bibliothèque elle-même ne dépend pas de GLFW : la fenêtre est celle de votre application.

Vue d’ensemble : [README](../README.md). Plan du code : [Architecture.md](Architecture.md).

## Outils

| Outil | Rôle |
|-------|------|
| g++ ou clang, C++20 | compilation |
| Make, CMake | le build (MyMakefile) et quelques tiers |
| Git | sources, sous-module `.makefile`, clones du manifeste |
| pkg-config | `Compages.pc` après `make install` |
| GLFW + un GL 4.5 | **exemples et tests GPU seulement** |

Adaptez le `-j` au nombre de cœurs. Les extraits utilisent `$(nproc)`.

### Ubuntu / Debian

```sh
sudo apt-get update
sudo apt-get install --no-install-recommends \
    build-essential cmake git pkg-config \
    libglfw3-dev libgl1-mesa-dev
```

Tests unitaires :

```sh
sudo apt-get install libgtest-dev libgmock-dev
```

Machine sans écran (CI, serveur) — GL logiciel :

```sh
sudo apt-get install xvfb mesa-utils libgl1-mesa-dri
```

```sh
xvfb-run -a env \
    LIBGL_ALWAYS_SOFTWARE=1 \
    MESA_GL_VERSION_OVERRIDE=4.5 \
    MESA_GLSL_VERSION_OVERRIDE=450 \
    ./build/Compages-examples --check
```

### Fedora

```sh
sudo dnf install gcc-c++ make cmake git pkgconf-pkg-config \
    glfw-devel mesa-libGL-devel
```

Tests :

```sh
sudo dnf install gtest-devel gmock-devel
```

Headless :

```sh
sudo dnf install xorg-x11-server-Xvfb mesa-dri-drivers
```

La commande `xvfb-run` est la même que sous Ubuntu.

### macOS (Homebrew)

Le système s’arrête à OpenGL 4.1. Le backend 4.5 n’y est pas complet : la galerie graphique et les tests GPU sont limités. Les tests qui ne touchent pas le device passent.

```sh
brew install cmake glfw googletest pkg-config
```

Le workflow CI exporte ensuite les chemins Homebrew (`USER_CXXFLAGS`, `USER_LDFLAGS`, `PKG_CONFIG_PATH`). Voir `.github/workflows/compile.yml`.

## Cloner

```sh
git clone --recurse-submodules https://github.com/Lecrapouille/Compages.git
cd Compages
```

`--recurse-submodules` récupère `.makefile` (le système de build). Sans lui, `make` n’a pas ses règles.

## Dépendances tierces

Elles ne sont pas des sous-modules git. `external/manifest` en donne la liste, une dépôt GitHub par ligne :

| Dépôt | Rôle |
|-------|------|
| `skypjack/entt` | ECS, en-têtes publics |
| `nholthaus/units` | littéraux d’angle (`90.0_deg`), en-tête public |
| `jkuhlmann/cgltf` | lecture glTF, compilation seulement |
| `nlohmann/json` | sérialisation de scènes |
| `nothings/stb` | images |
| `zeux/pugixml` | URDF |
| `ocornut/imgui` | panneaux de la galerie |
| `Lecrapouille/Compages-data` | textures, glTF, URDF des démos |
| `DanielChappuis/reactphysics3d` | `attic/Physics/` uniquement |

```sh
make download-external-libs
make compile-external-libs
```

Le clone des assets atterrit dans `external/Compages-data/`. Les exemples cherchent aussi `external/Compages-data/` et la variable `COMPAGES_DATA_PATH`.

## Compiler

```sh
make -j"$(nproc)"
```

Produits utiles :

| Chemin | Contenu |
|--------|---------|
| `build/libCompages.a` | bibliothèque statique |
| `build/libCompages.so` | bibliothèque partagée |
| `build/Compages-examples` | la galerie |

`make help` liste les variables (`GPU_BACKEND`, `PREFIX`, …). `GPU_BACKEND` vaut `GL45` dans `Makefile.common`. Changer de backend, le jour où un second existe, se fait là, pas dans le code qui appelle `gpu::`.

## Galerie

```sh
./build/Compages-examples
./build/Compages-examples 13_Galaxy
./build/Compages-examples --check
```

`--check` construit chaque démo, dessine quelques frames, ferme, et échoue si un handle GPU traîne. Régénérer les captures dans le dépôt d’assets :

```sh
./build/Compages-examples --check \
    --shots external/Compages-data/doc/examples
```

Touches, chapitres, budgets de lignes : [Examples.md](Examples.md).

Contrats pédagogiques (manifeste, budgets) :

```sh
make -C examples check-contracts
```

## Tests

```sh
make -C tests -j"$(nproc)"
./build/Compages-UnitTest
```

Sous Linux sans affichage, le même `xvfb-run` et les mêmes variables `MESA_*` que pour la galerie. Couverture locale : `make -C tests coverage`.

## Installer sur le système

Défauts (`.makefile/project/Makefile`) :

| Variable | Défaut |
|----------|--------|
| `PREFIX` | `/usr/local` |
| `INCLUDEDIR` | `$(PREFIX)/include` |
| `LIBDIR` | `$(PREFIX)/lib` |
| `PKGLIBDIR` | `$(LIBDIR)/pkgconfig` |

```sh
sudo make install
sudo ldconfig
```

Sous **Fedora / RHEL**, le chargeur dynamique ne parcourt en pratique que `/etc/ld.so.cache` puis `/lib64` et `/usr/lib64` : sans entrée pour `/usr/local/lib`, `libCompages.so` reste « introuvable » même après install. Si `sudo ldconfig` ne suffit pas :

```sh
echo '/usr/local/lib' | sudo tee /etc/ld.so.conf.d/usrlocal.conf
sudo ldconfig
```

Contournement ponctuel : `LD_LIBRARY_PATH=/usr/local/lib ./votre-programme`.

Cela installe la bibliothèque statique et la partagée, les en-têtes `include/Compages/`, EnTT et `units.h` (inclus par des en-têtes publics), et `Compages.pc`.

Staging, pour inspecter sans toucher au système :

```sh
make DESTDIR="$PWD/stage" PREFIX=/usr install
```

Consommer :

```sh
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
pkg-config --cflags --libs Compages
c++ -std=c++20 app.cpp -o app $(pkg-config --cflags --libs Compages)
```

Dans `app.cpp` : créez le contexte OpenGL 4.5, appelez `compages::gpu::init(votreGetProcAddress)`, incluez la couche voulue ([CheatSheet.md](CheatSheet.md)). Rien sous `src/` n’est installé.

## Ce qui n’est pas lié

GLEW, SOIL, Bullet, bzip2 ne font pas partie du build. ImGui et GLFW restent dans la galerie et les tests. glad est privé au backend GL45.

## Suite

- [Tutorial.md](Tutorial.md) — premier programme.
- [Debug.md](Debug.md) — traces et RenderDoc.
