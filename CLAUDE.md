# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

**Build** (Visual Studio solution, from `C:\Juggler`):
```
msbuild Juggler.sln /p:Configuration=Debug /p:Platform=x64
msbuild Juggler.sln /p:Configuration=Release /p:Platform=x64
```

Output is `build\Debug\Juggler.scr` or `build\Release\Juggler.scr`. The `.scr` extension is a renamed Windows executable.

**Run the screensaver directly** (bypasses Windows screensaver registration):
```
build\Debug\Juggler.scr
```

**Build & run tests**:
```
msbuild tests\JugglerTests.vcxproj /p:Configuration=Debug /p:Platform=x64
build\tests\Debug\JugglerTests.exe
```

**Run a single test** (GoogleTest filter):
```
build\tests\Debug\JugglerTests.exe --gtest_filter=SphereIntersection.*
```

**Shader editing — no rebuild needed.** Shaders are HLSL compiled at runtime by DXC. The build copies `shaders\*.hlsl` and `shaders\*.hlsli` to `build\<Config>\shaders\` as a post-build step. To test a shader change immediately, edit the file in `build\Debug\shaders\` and re-run the screensaver. Edit `shaders\` (the canonical source) to persist the change across builds.

## Architecture

This is a **DXR 1.1 (DirectX Raytracing) screensaver** ported from a Java software ray tracer. It renders a juggling figure made entirely of spheres using GPU ray tracing with no rasterization.

### Per-frame pipeline

Each call to `Renderer::render()` does the following in sequence:
1. Advances `m_animTime` and calls `Animation::update()` to reposition all spheres in CPU-side `Scene::spheres`
2. Rebuilds the BLAS and TLAS from scratch each frame (procedural AABBs, one per sphere)
3. Uploads updated sphere center/radius data to a GPU structured buffer
4. Updates the per-frame constant buffer (`PerFrameConstants`) with camera, light, and frame state
5. Dispatches rays (4 samples per pixel via `SAMPLES_PER_PIXEL=4` in `common.hlsli`, each with per-sample jitter for anti-aliasing, AO, and shadow sampling; up to `MAX_DEPTH=10` bounces per sample for reflections)
6. Writes results to `g_Accum` (a float32 texture); temporal accumulation is reset every frame by the orbiting camera, so `accumulatedFrames` is always 1 and the `g_Accum` path in `raygen.hlsl` always overwrites rather than averages

### Geometry representation

There are no vertex meshes. Every object is a **procedural AABB primitive** in DXR. Two geometry slots live in a single BLAS:
- Geometry 0: one AABB for the ground plane (huge, y ≈ 0)
- Geometry 1: 84 AABBs, one per sphere (juggling balls, torso, head, limbs, eyes, hair)

The TLAS has a single instance with an identity transform, so object space equals world space throughout.

### Shader pipeline

Six HLSL shaders, all compiled from `lib_6_5` at startup by `DXRPipeline::init()`:

| Shader | Role |
|---|---|
| `raygen.hlsl` | Per-pixel ray generation, iterative reflection loop, accumulation write |
| `sphere_intersection.hlsl` | Quadratic sphere test, computes outward normal, assigns material index |
| `sphere_closesthit.hlsl` | Ambient/AO, diffuse, specular, reflection payload for spheres |
| `ground_intersection.hlsl` | Ray-plane test at y=0, checkerboard material selection |
| `ground_closesthit.hlsl` | Same lighting model as sphere; supports reflection (ground has `reflectionWeight=0.15`) |
| `miss.hlsl` | Sky gradient based on ray Y direction |

Shadow and AO tests use **inline `RayQuery`** inside the closest-hit shaders (not recursive `TraceRay`). Self-intersection is prevented by offsetting secondary ray origins by `SHADOW_BIAS = 0.05f` along the surface normal — removing this causes visible dark bands on spheres.

### Camera

`Camera::init()` sets up screen geometry (`DISTANCE_TO_VIRTUAL_SCREEN = 50`, virtual screen ratio, half-dimensions) with a default eye/look that is immediately overridden each frame. Each call to `render()` invokes `Camera::updateOrbit()`, which orbits the eye around the juggler center `{151, 100, -151}` at `ORBIT_RADIUS = 350` world units and `ORBIT_HEIGHT = 130`, rotating at `ORBIT_SPEED = 0.4` rad/s (~15 s per full orbit), then rebuilds the ONB (u, v, w) via `Vec3::onb()`. Camera state is uploaded to the GPU each frame via `PerFrameConstants`.

### Animation

`Animation::update(double T, ...)` takes a time value `T` in `[0, 30)` and directly writes world-space `center` coordinates into `Scene::spheres`. The juggler body is built from a hip origin `o` with body-tilt vectors `bodyV/bodyU/bodyW`; limbs are computed by `updateAppendage()` using a two-segment IK approximation. Ball trajectories are parabolic with constants derived from the Java reference.

### CPU/GPU data boundary

`shared_types.h` defines the structs that cross the CPU/GPU boundary with explicit 16-byte-aligned layout matching the HLSL `cbuffer` and structured buffer declarations in `common.hlsli`. When adding fields, both files must stay in sync and padding must be maintained.

### Tests

Tests are pure CPU-side and cover: math utilities, animation kinematics, scene/material initialization, camera setup, and CPU reimplementations of the sphere/ground intersection shaders (for verifying the quadratic logic). Tests do **not** cover shader behavior, shadow bias, or GPU rendering.
