# Amiga Juggler DXR 1.1 Screen Saver

Recreates the iconic 1985 Amiga Juggler demo as a Windows screen saver (.scr) using C++ with DirectX 12 DXR 1.1 hardware ray tracing.

## Requirements

- Windows 10 version 1909+ or Windows 11
- Visual Studio 2022 with C++ Desktop workload
- Windows 10 SDK (10.0.26100.0 or later)
- GPU with DXR 1.1 support (NVIDIA RTX 2000+, AMD RX 6000+, Intel Arc)
- DirectX Shader Compiler (dxcompiler.dll, included with Windows SDK)

## Building

1. Open `Juggler.sln` in Visual Studio 2022
2. Select **Release | x64** configuration
3. Build Solution (Ctrl+Shift+B)
4. Output: `build\Release\Juggler.scr`

Or from command line:
```
MSBuild Juggler.vcxproj /p:Configuration=Release /p:Platform=x64
```

## Installation as Screen Saver

1. Copy `build\Release\Juggler.scr` and the `build\Release\shaders\` folder to `C:\Windows\System32\`
2. Right-click desktop > Personalize > Lock screen > Screen saver settings
3. Select "Juggler" from the dropdown

## Usage

| Command | Mode |
|---------|------|
| `Juggler.scr /s` | Run full-screen screen saver |
| `Juggler.scr /c` | Show configuration/about dialog |
| `Juggler.scr /p HWND` | Preview in Settings window |

The screen saver exits on any key press or mouse movement (>5px threshold).

## Architecture

- **84 procedural spheres** + 1 ground plane, all rendered with analytic ray-sphere/ray-plane intersection shaders
- **Single BLAS** with 2 geometries (ground + spheres), rebuilt each frame
- **DXR 1.1 inline ray tracing** (`RayQuery`) for shadow and ambient occlusion rays
- **Iterative reflections** in ray generation shader (up to 10 bounces)
- **Progressive accumulation** with jittered sampling for anti-aliasing
- 30-frame animation cycle with cosine body oscillation, IK limbs, projectile ball physics

See `docs/architecture.md` and `docs/shaders.md` for detailed documentation.

## Reference

Based on the Java reference implementation in `Juggler_Java/`, which provides all geometry, animation, materials, and rendering logic.
