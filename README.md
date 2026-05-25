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

cmake -B build  
cmake --build build
.\bin\SpacePuzzle.exe
```

The executable is also available directly as `./bin/SpacePuzzle`.


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

## Author

Mabrouk Bouzouita Chahine
Vuillet Andreas
