# Architecture

## System Overview

```
main.cpp (WinMain, .scr arg parsing)
    |
    v
screensaver.cpp (window, input handling)
    |
    v
renderer.cpp (D3D12 init, swap chain, DXR dispatch, timing)
    |
    +-- dxr_pipeline.cpp (root signature, PSO, shader tables)
    +-- acceleration_structures.cpp (BLAS/TLAS build per frame)
    +-- scene.cpp (84 spheres + materials)
    +-- animation.cpp (juggler physics/IK)
    +-- camera.cpp (static camera setup)
    +-- gpu_resources.cpp (D3D12 buffer helpers)
```

## Data Flow Per Frame

1. **Animation**: `Animation::update(T)` updates all 84 sphere positions
2. **AABB Generation**: Compute bounding boxes for each sphere
3. **BLAS Build**: 2 geometries (ground + spheres), `PREFER_FAST_BUILD`
4. **TLAS Build**: Single instance with identity transform
5. **Constants Upload**: Camera, light, sky, accumulation state
6. **DispatchRays**: 1 ray per pixel, jittered for AA
7. **Accumulation**: Running average in float4 texture
8. **Output**: Gamma-corrected copy to swap chain back buffer

## Scene Hierarchy (84 spheres)

| Index Range | Count | Part | Material |
|-------------|-------|------|----------|
| 0-2 | 3 | Juggling balls | Mirror |
| 3-10 | 8 | Torso | Torso plastic |
| 11 | 1 | Head | Skin plastic |
| 12 | 1 | Neck | Skin plastic |
| 13-29 | 17 | Left leg | Skin plastic |
| 30-46 | 17 | Right leg | Skin plastic |
| 47-63 | 17 | Left arm | Skin plastic |
| 64-80 | 17 | Right arm | Skin plastic |
| 81-82 | 2 | Eyes | Eye plastic |
| 83 | 1 | Hair | Hair plastic |

## DXR Pipeline

- **Global Root Signature**: Single descriptor table with 6 ranges (TLAS, output UAV, CBV, sphere SRV, material SRV, accumulation UAV)
- **State Object**: 6 DXIL libraries, 2 hit groups (ground + sphere), shader config, pipeline config (max recursion = 1)
- **Shader Table**: 4 records (raygen, miss, ground hit group, sphere hit group)

## Acceleration Structure

- **BLAS**: 2 geometries
  - Geometry 0: Ground (1 AABB, 2M x 0.02 x 2M units)
  - Geometry 1: Spheres (84 AABBs, each tightly bounding its sphere)
- **TLAS**: 1 instance, identity transform

Hit group selection: `GeometryIndex * 1` maps ground (0) to GroundHitGroup and spheres (1) to SphereHitGroup.

## Animation

30-frame cycle ported from Java reference:
- **Ball physics**: 3 balls with gravity, 2 high throws (60-frame arc) and 1 low throw (30-frame arc)
- **Body oscillation**: Hips height oscillates between 81-85 units, torso tilts via sine rotation
- **IK limbs**: Quadratic joint solver for arms and legs, each rendered as 17 sphere segments
- **Arm swing**: Angle proportional to body oscillation
