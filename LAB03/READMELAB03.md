# LAB03 3D Rendering and Lighting - Exercise Summary

This folder contains 11 lab exercises focused on 3D object loading, camera control, lighting models, cubemaps, and debugging. LAB03 builds on earlier OpenGL work by introducing 3D scene management, object file parsing, shading techniques, and surface effects.

## Exercises Overview

| Exercise | Title | Key Concepts | Description |
|----------|-------|--------------|-------------|
| **Ex01** | Camera | 3D camera control, view matrix | Understand the LAB03 camera class and how the view matrix is used in the scene. Test camera translation and rotation keys. |
| **Ex02** | Object Reader | `.obj` parsing, mesh loading | Load a Wavefront `.obj` file with the provided object reader and render the object using VAO/VBO. |
| **Ex03** | Gouraud Diffuse Lighting | Gouraud shading, diffuse lighting | Implement diffuse Gouraud shading on a sphere and send light position to the shader. |
| **Ex04** | Phong Diffuse Lighting | Phong shading, per-pixel diffuse | Implement Phong diffuse shading and compare results with Gouraud. |
| **Ex05** | Gouraud Specular Lighting | Specular highlights, Gouraud model | Add specular lighting to Gouraud shading. Determine which vectors are needed and which shader computes them. |
| **Ex06** | Phong Specular Lighting | Phong shading, specular highlights | Implement full Phong specular lighting with correct vectors and shader stage placement. |
| **Ex07** | Full Lighting + Attenuation | Light equation, attenuation | Implement complete lighting with time-based light movement and distance attenuation. |
| **Ex08** | Cubemap / Skybox | Cubemap textures, environment mapping | Load a cubemap/skybox and use it in your scene. This exercise also uses cube map sampling with `samplerCube`. |
| **Ex09** | Reflection | Reflection mapping | Implement reflection on an object using a cubemap and reflection vectors in the fragment shader. |
| **Ex10** | Refraction | Refraction effect | Implement refraction on an object using the cubemap and refracted direction vectors. |
| **Ex11** | RenderDoc Debugging | Graphics debugging, texture debugging | Use RenderDoc (or equivalent) to debug a broken texture rendering case and fix the texture issue. |

## Core LAB03 Concepts

### Geometry and Scene Data
- Loading 3D objects from `.obj` files
- Object parsing and vertex/index buffer setup
- Using `PATH_TO_OBJECTS` macro for `.obj` file paths
- Working with scene assets in `LAB03/objects`

### Camera and 3D Viewing
- 3D camera movement and rotation
- View matrix generation from camera parameters
- Projection matrix and camera frustum
- Integrating `camera.h` into render code

### Shading and Lighting
- Gouraud shading vs Phong shading
- Diffuse lighting and specular highlights
- Light position, view position, and normal vectors
- Attenuation and complete lighting equation
- Reflection and refraction using cubemaps

### Textures and Environment Maps
- Loading cubemap textures from `LAB03/textures`
- Using `samplerCube` in GLSL
- Skybox / environment map rendering
- Reflection/refraction sampling from a cubemap

### Debugging and Tools
- OpenGL debug context with `glDebugMessageCallback`
- Using RenderDoc for frame inspection
- Shader and pipeline debugging
- Finding missing texture and state issues

## Project Structure

- `LAB03/exercices/` – exercise source code
- `LAB03/solutions/` – solution source code (build only if enabled)
- `LAB03/objects/` – `.obj` models used by exercises
- `LAB03/textures/` – texture assets and cubemaps
- `camera.h`, `shader.h` – shared utility classes

## Compilation and Execution

### Build LAB03 exercises
```bash
cd /Users/andreasvuillet/info-h502_202526
cmake -S . -B build -DCOMPILE_LAB03=ON
cmake --build build --target LAB03_ex01
```

### Build all LAB03 exercises
```bash
cmake -S . -B build -DCOMPILE_LAB03=ON
cmake --build build --target LAB03_ex01
cmake --build build --target LAB03_ex02
# ... up to LAB03_ex11
```

### Compile solutions
```bash
cmake -S . -B build -DCOMPILE_LAB03=ON -DCOMPILE_SOLUTIONS=ON
cmake --build build --target LAB03_ex01_sol
```

### Run a specific executable
```bash
./build/LAB03/LAB03_ex01
./build/LAB03/LAB03_ex02
./build/LAB03/LAB03_ex03
# ... up to LAB03_ex11
```

### Run a solution executable
```bash
./build/LAB03/LAB03_ex01_sol
./build/LAB03/LAB03_ex02_sol
# ... up to LAB03_ex10_sol
```

## Notes

- `LAB03` uses `camera.h` and `shader.h` to manage camera transforms and shader programs.
- `LAB03/exercices/ex02/ex02.cpp` requires reading `.obj` geometry data and sending it to the GPU.
- `LAB03/exercices/ex08/ex08.cpp` uses cubemap textures and environment mapping.
- Exercise 11 is primarily about using RenderDoc or another graphics debugger to diagnose texture rendering issues.

## Learning Path

LAB03 is designed to progress from 3D camera control and object loading to advanced lighting and environment effects:

1. Learn camera integration and depth testing
2. Load and render real 3D models
3. Implement lighting models across shader stages
4. Add specular, attenuation, reflection, and refraction
5. Debug real graphics problems with a professional tool
