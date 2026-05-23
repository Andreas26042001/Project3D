# SpacePuzzle — INFO-H-502

First-person light puzzle in a Phong-lit cave. The player pushes movable pillars on rails, fires a light projectile, and steers a beam toward wall targets via a deflector prism.

**OpenGL context:** Core profile **4.0** (GLFW + GLAD), **GLSL 330 core** shaders.

## Puzzle goal

1. **Fire** (`F`) the blue light projectile from your torch toward the center pillar to anchor the beam cannon on it.
2. **Push** the center pillar along its rail (Z axis) to aim the main beam at the **east wall** target.
3. **Push** the deflector pillar along its rail (X axis) so the beam hits the translucent prism.
4. **Hold** the beam on each wall target for about **3 seconds** to activate both receivers and complete the puzzle.

The deflected beam (+Z) must reach the **south wall** target after passing through the prism.

## Quick start

Public repository: [github.com/Andreas26042001/Project3D](https://github.com/Andreas26042001/Project3D).

```bash
git clone https://github.com/Andreas26042001/Project3D.git
cd Project3D

./build.sh    # configure + compile
./run.sh      # build (if needed) + launch the game
```

The executable is also available directly as `./bin/SpacePuzzle`.

### Push changes to GitHub

After committing your work:

```bash
git add -A
git commit -m "Your message"
./push.sh     # verifies build, blocks LAB folders, then git push
```

`./push.sh` runs a release check before pushing:

1. Verifies that required assets and dependencies are present
2. Builds the project
3. Ensures no `LAB01/`–`LAB04/` folder is staged
4. Pushes to `origin` on the current branch

## Game features

- FPS camera (WASD, mouse, scroll wheel) with AABB collisions and dynamic support height
- Enclosed scene: textured ground, pillar grid, walls, ceiling, cubemap skybox
- Multi-source Phong lighting: ceiling fill light + player-carried torch / beam cannon
- Mechanical puzzle:
  - Center pillar movable on a rail (pushed by the player) with a light cannon
  - Deflector pillar movable on a perpendicular rail + translucent prism
  - Two wall targets activated by holding the beam on them (~3 s)
- Light projectile (`F`) with collisions and particle explosion
- `.obj` mesh loading (procedural cube for static scene, `capture_pillar.obj` for movable pillars)
- Diffuse textures (ground, pillar bricks, receiver pillar metal)
- Crosshair and pause mode

## Controls

| Key | Action |
|-----|--------|
| **W A S D** | Move |
| **Mouse** | Look |
| **Scroll wheel** | Zoom (FOV) |
| **F** | Fire a light projectile |
| **P** | Pause / resume (release or recapture mouse) |
| **C** | Show / hide crosshair |
| **Escape** | Quit |

## Prerequisites

- **CMake** ≥ 3.20
- **C++14** compiler (`g++`, Clang, MSVC…)
- **OpenGL** (system drivers / SDK)
- **make** or **cmake --build**

On Linux (Debian/Ubuntu):

```bash
sudo apt install build-essential cmake libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev mesa-common-dev
```

On macOS: install Xcode Command Line Tools (`xcode-select --install`) and CMake (`brew install cmake`).

## Manual build (without scripts)

```bash
cmake -S . -B build
cmake --build build --parallel
./bin/SpacePuzzle
```

On macOS / Linux, `make -C build` can replace `cmake --build build` if you prefer.

### Verification (optional)

After cloning, to confirm nothing is missing:

```bash
test -f import/glad/src/glad.c \
  && test -f shader/phong.vert \
  && test -f assets/objects/cube.obj \
  && echo "OK — run ./build.sh"
```

If this command fails, the clone is incomplete (check that `git pull` fetched the full repo, including `import/`).

## Versioned content on Git

| Included in the repo | Ignored (`.gitignore`) |
|----------------------|------------------------|
| `CMakeLists.txt`, `main.cpp`, `src/` | `build/`, `bin/` |
| `shader/`, `assets/` | `CMakeFiles/`, `CMakeCache.txt` |
| `import/` (GLFW, GLAD, GLM, STB) | `.DS_Store`, `.vscode/` |
| `build.sh`, `run.sh`, `push.sh` | `LAB01/`–`LAB04/` (local lab exercises) |

The `LAB01/`–`LAB04/` folders may exist locally for course exercises but are **not** pushed to GitHub.

Asset paths are injected at compile time:

| CMake macro | Folder |
|-------------|--------|
| `PATH_TO_OBJECTS` | `assets/objects/` |
| `PATH_TO_TEXTURE` | `assets/textures/` |
| `PATH_TO_CUBESMAPS` | `assets/cubesmaps/` |
| `PATH_TO_SHADER` | `shader/` |

The executable works regardless of the current working directory.

### Troubleshooting

| Error | Likely cause | Fix |
|-------|--------------|-----|
| `import/glad/src/glad.c: No such file` | Missing `import/` folder | Re-clone fully; do not use “Download ZIP” without `import/` |
| `Failed to create GLFW window` | OpenGL drivers | Update GPU drivers |
| Missing textures in game | Incomplete `assets/` | Check that `assets/textures/.../*_Color.jpg` is present |
| `cmake: command not found` | CMake not installed | Install CMake ≥ 3.20 |
| `permission denied: ./build.sh` | Scripts not executable | Run `chmod +x build.sh run.sh push.sh` |

### IDE (optional)

Open the cloned folder in VS Code / Cursor with **C++** and **CMake Tools** extensions, then **CMake: Configure** → **Build**. The executable will be in `bin/SpacePuzzle`.

## Project structure

```
SpacePuzzle/                    # clone folder name may differ (e.g. Project3D on GitHub)
├── build.sh                    # Configure + compile
├── run.sh                      # Build + launch
├── push.sh                     # Pre-push checks + git push
├── SpacePuzzle.code-workspace  # Optional VS Code / Cursor workspace
├── CMakeLists.txt              # GLSL / INFO-H-502 style, output in bin/
├── README.md
├── import_README.md            # Instructions for GLFW, GLAD, GLM, STB
├── main.cpp                    # Entry point (project root)
├── assets/                     # Runtime-loaded data
│   ├── objects/                # .obj meshes
│   ├── textures/               # .jpg / .png images
│   ├── cubesmaps/              # Skybox faces
│   └── blender/                # Blender sources (not in pipeline)
├── shader/                     # GLSL shaders (.vert, .frag)
├── import/                     # GLFW, GLAD, GLM, STB
├── src/                        # Source code + headers
│   ├── Game.cpp                # Scene init, GPU resources
│   ├── Game.h
│   ├── Game_Render.cpp         # Render passes (shadows, Phong, skybox…)
│   ├── Game_Physics.cpp        # Collisions, puzzle, particles
│   ├── GameInternal.cpp / GameInternal.h   # Resource paths, file helpers
│   ├── Texture.cpp / Texture.h             # STB Image loading (2D + cubemap)
│   ├── shader.cpp / shader.h               # GLSL program wrapper
│   ├── camera.cpp / camera.h               # FPS camera
│   ├── object.cpp / object.h               # OBJ mesh + VAO/VBO
│   ├── Player.cpp / Player.h               # Player movement and input
│   ├── Collider.cpp / Collider.h           # AABB collision world
│   ├── ParticleSystem.cpp / ParticleSystem.h
│   ├── ShadowMap.cpp / ShadowMap.h         # Dynamic shadow map FBO
│   ├── Skybox.cpp / Skybox.h
├── build/                      # CMake build directory (generated)
└── bin/                        # Compiled executable (generated)
```

## OpenGL / GLSL features

List of graphics techniques implemented in the main project (`src/` + `shader/`).

### Pipeline and OpenGL state

| Feature | Detail |
|---------|--------|
| Core 4.0 profile | GLFW context + GLAD function loading |
| VAO / VBO | Static geometry (OBJ models), skybox, crosshair, targets, prism |
| Depth test | `GL_DEPTH_TEST`; `GL_LEQUAL` for skybox |
| Stencil buffer | 8 bits configured at window creation |
| Framebuffers (FBO) | Depth-only shadow map passes (`GL_DRAW_BUFFER = GL_NONE`) |
| Resize | `glViewport` via framebuffer callback |
| Multi-textures | Units `GL_TEXTURE0` (diffuse) and `GL_TEXTURE5` (dynamic shadow map) |
| Face culling | `GL_FRONT` in shadow pass (second-depth mapping) |
| Polygon offset | `glPolygonOffset` slope-scale in shadow map generation |
| Alpha blending | Deflector prism (`GL_SRC_ALPHA`, `GL_ONE_MINUS_SRC_ALPHA`) |
| Depth mask | `glDepthMask(GL_FALSE)` for translucent geometry |

### Active shaders

| Pair | Role |
|------|------|
| `phong.vert` / `phong.frag` | Per-fragment Phong lighting, textures, shadows |
| `lamp.vert` / `lamp.frag` | Unlit emissive markers (torch, rails, targets, particles) |
| `cubemap.vert` / `cubemap.frag` | Skybox (`samplerCube`) |
| `shadow_depth.vert` / `shadow_depth.frag` | Depth-only pass for shadow mapping |
| `particle.vert` / `particle.frag` | Explosion particles |
| `crosshair.vert` / `crosshair.frag` | 2D screen-space overlay (`GL_LINES`) |
| `prism.vert` / `prism.frag` | Translucent deflector octahedron (cubemap refraction) |

Lab shaders (`basic.*`, `gouraud.*`) are not part of this project.

### Lighting (GLSL)

- **Diffuse-style lighting**: ambient base + ceiling fill + dynamic point light + linear beam sources (closest point on segment)
- **Single dynamic point light** with perspective shadow map when carried or anchored
- **Linear sources** (cannon beam / deflected ray): closest point on segment (Heidrich/Seidel approach)
- Normal matrix: `transpose(inverse(model))`
- GLSL `Material` struct with uniforms

### Textures

- `sampler2D` for diffuse maps (ground, bricks, metal)
- `uvScale` uniform for per-surface tiling
- JPEG loading via STB Image (`Texture::loadFromFile2D`, cubemap via `Texture::loadVoidSpaceCubemap`)
- Void space cubemap (`assets/cubesmaps/Void space cubemap/`)

### Shadow mapping

| Technique | Implementation |
|-----------|------------------|
| Dynamic shadow map | 1× 2048×2048 map, **perspective** projection (carried spotlight), recomputed each frame |
| Planar shadows | RTR4 §7.1.1 geometric projection on the ground (stencil-limited) |
| Sampling | `texture(shadowMap, projCoords.xy).r` + depth comparison |
| Slope-scale bias | `glPolygonOffset(1.5, 4.0)` on CPU |
| Normal-offset bias | Holbert offset in `phong.frag` (`sinTheta`) |
| Second-depth mapping | Front-face culling in shadow pass |
| Lit border | `GL_CLAMP_TO_BORDER` + white `GL_TEXTURE_BORDER_COLOR` |
| Stable shadows | Position snap (1 mm grid) + NDC texel snap for dynamic light |

### Clipping and advanced effects

- **`gl_ClipDistance[1..4]`**: four edge planes for planar shadow projection on the ground (stencil-limited pass)
- **Infinite skybox**: `gl_Position = pos.xyww` (depth = 1.0)
- **HUD crosshair**: NDC coordinates, depth test disabled
- **Procedural GPU geometry**: disks/rings (targets), octahedron (prism), particles (instanced cubes)

### Render passes (order)

Main loop in `Game::Render()`:

1. Dynamic shadow map → depth FBO (carried or anchored light only)
2. Opaque scene pass (`renderSceneOpaque`):
   - Phong-lit geometry (walls, ceiling, ground, static and movable pillars)
   - Stencil-marked ground receiver + planar shadows (RTR4 §7.1.1)
   - Lamp overlays (torch, rails, wall targets, beam segments, ceiling marker, player proxy)
3. Skybox (depth `GL_LEQUAL`, depth write off)
4. Explosion particles (alpha blended billboards)
5. Translucent deflector prism (cubemap refraction + alpha blending)
6. 2D crosshair (screen-space overlay, depth test off)

### Graphics references

Code comments refer to relevant chapters of *Real-Time Rendering* (4th ed.): shadow maps (§7.4), planar shadows (§7.1), translucency (ch. 14).

## Development

Code is split into modules for easier extension:

| Module | Role |
|--------|------|
| `Game.cpp` | Scene setup, GPU resource init, puzzle state |
| `Game_Render.cpp` | Shadow map, Phong pass, planar shadows, HUD |
| `Game_Physics.cpp` | Collisions, movable pillars, beam targets, projectile |
| `GameInternal` | Compile-time asset paths (`PATH_TO_*` macros) |
| `Player` | Input and camera physics against `Game` colliders |

## Author

Mabrouk Bouzouita Chahine
Vuillet Andreas
