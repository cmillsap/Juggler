# Shader Pipeline

## Overview

All shaders use HLSL Shader Model 6.3 (lib_6_3) with DXR 1.1 features.

## Resource Bindings

| Register | Type | Resource |
|----------|------|----------|
| t0 | SRV | TLAS (RaytracingAccelerationStructure) |
| t1 | SRV | Sphere data (StructuredBuffer\<GPUSphereData\>) |
| t2 | SRV | Material data (StructuredBuffer\<GPUMaterialData\>) |
| u0 | UAV | Output texture (RWTexture2D\<float4\>) |
| u1 | UAV | Accumulation buffer (RWTexture2D\<float4\>) |
| b0 | CBV | Per-frame constants |

## Shader Files

### common.hlsli
Shared definitions included by all shaders:
- Resource binding declarations
- `RayPayload` struct (color, reflection data)
- `ProceduralAttributes` struct (normal + material index)
- PCG random number generator
- Inline ray query helpers (`isOccluded`, `isAOOccluded`)

### raygen.hlsl
- Computes camera ray from pixel coordinates using virtual screen model
- Jitters pixel position for anti-aliasing (PCG RNG seeded per pixel + frame)
- Iterative reflection loop (up to 10 bounces)
- Accumulates color in float4 buffer, gamma-corrects for output

### miss.hlsl
- Sky gradient: lerp between minColor (light blue) and maxColor (deep blue)
- Interpolation factor = saturate(ray direction Y component)
- Matches Java SkyMaterial behavior

### sphere_intersection.hlsl
- Analytic ray-sphere intersection (quadratic formula)
- Reports hit with normal and material index
- Material index determined by sphere index ranges

### ground_intersection.hlsl
- Ray-plane intersection at Y=0
- Checkerboard material selection: `(floor(x/107) & 1) XOR (floor(z/107) & 1)`

### sphere_closesthit.hlsl
Full Blinn-Phong lighting model:
- **Ambient**: Material ambient weight * diffuse color * ambient occlusion
- **AO**: Single inline RayQuery with random hemisphere direction
- **Diffuse**: Lambert shading with soft shadow test
- **Specular**: Phong reflection model with material shininess
- **Shadows**: Inline RayQuery toward jittered light position (radius 10)
- **Reflection**: Sets payload fields for iterative reflection in raygen

### ground_closesthit.hlsl
Same lighting model as sphere, minus specular (matte materials).

## Inline Ray Tracing (DXR 1.1)

Shadow and AO rays use `RayQuery` with `RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH` for performance.
The inline proceed loop handles both procedural geometries (ground plane and spheres) without invoking hit shaders.

## Payload Structure

```hlsl
struct RayPayload {
    float3 color;                 // Direct illumination from this hit
    float3 reflectionAttenuation; // Weight for next bounce
    float3 hitPoint;              // Origin for reflection ray
    float3 reflectionDir;         // Reflection direction
    bool   hasReflection;         // Continue tracing?
};
```

Max payload size: 64 bytes. Max attribute size: 16 bytes.
