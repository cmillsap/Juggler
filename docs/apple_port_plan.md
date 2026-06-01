# Juggler Screensaver — macOS / iOS Port Plan

## Executive Summary

This is a substantial but well-scoped port. The core logic (animation, scene geometry, math,
camera) is clean platform-agnostic C++ that moves unchanged. Everything else — the GPU API,
shaders, windowing, and app lifecycle — must be fully replaced. The DXR 1.1 ray tracing pipeline
maps directly to Apple's Metal ray tracing API, which uses the same fundamental concepts
(acceleration structures, intersection functions, inline ray queries) on M-series and A-series
silicon.

---

## Hardware and Software to Acquire

### Hardware

A Mac with Apple Silicon is required. Xcode runs only on macOS, and hardware ray tracing on Metal
requires Apple Silicon — Intel Macs are explicitly excluded from Metal's ray tracing API.

| Option | Price | Notes |
|--------|-------|-------|
| **Mac Mini M4** (recommended) | ~$599 | 10-core GPU, 16 GB RAM. Fully sufficient for this project. Requires existing monitor, keyboard, and mouse. |
| Mac Mini M4 Pro | ~$1,399 | 20-core GPU, 24 GB RAM. More headroom but unnecessary for this scope. |
| MacBook Air M4 | ~$1,099 | Portable, good battery, built-in screen. Worth it if portability matters. |

**Recommendation: Mac Mini M4 at $599.** This screensaver is CPU-light and GPU-moderate. The
Juggler scene (84 spheres, ground plane) is simple enough that even the base M4's 10-core GPU
handles it at 60fps with room to spare. See the performance notes section for full context.

### Software

| Item | Cost | Notes |
|------|------|-------|
| Xcode | Free | Mac App Store or developer.apple.com. Includes the Metal compiler, simulator, and device deployment tools. |
| Apple Developer Program | $99/year | Required to deploy to physical iPhone and iPad. Not required for the macOS screensaver itself — `.saver` bundles install locally without a Developer account. |
| CMake | Free | Recommended for cross-platform build management. Optional; a pure Xcode project works too. |
| metal-cpp | Free | Apple's official pure-C++ wrapper for Metal. Lets the GPU backend stay in `.cpp` files with no Objective-C. Ships as a header-only library. |

**Total: ~$599 hardware + $99/year Apple Developer Program.**

---

## Performance: Mac vs. PC for Ray Tracing

This section addresses a common concern before committing to the hardware investment.

### The general claim is accurate for heavy workloads

Apple introduced dedicated hardware RT cores in **M3 (late 2023)** and continued in **M4 (2024)**.
M1 and M2 handled ray tracing entirely in shader cores (software RT). For scene-heavy workloads
like Blender Cycles path tracing or full game-scale ray tracing:

| GPU | Approximate RT class |
|-----|---------------------|
| NVIDIA RTX 4090 | ~5× faster than M4 Max in Blender Cycles |
| NVIDIA RTX 4080 | ~2-3× faster than M4 Max |
| NVIDIA RTX 3080 | Roughly equal to M4 Max (40 GPU cores) |
| NVIDIA RTX 3070 | Roughly equal to M4 Pro (20 GPU cores) |
| NVIDIA RTX 3060 | Roughly equal to M4 base (10 GPU cores) |
| Mac Mini M4 (base) | Behind RTX 3070 for heavy RT scenes |

So yes — an RTX 3080 from 2020 beats a base Mac Mini M4 for demanding ray tracing work. The
fundamental reason is that Apple Silicon is optimized for performance-per-watt (the Mac Mini M4
draws ~30W total system power), while a desktop RTX 3080 draws 320W on the GPU alone. You get
what you pay in watts.

### For the Juggler screensaver specifically, this does not matter

The Juggler scene is 84 spheres and a ground plane — roughly the complexity of a single object in
a modern ray-traced game. The performance bottleneck is **pixel count × bounce depth**, not
geometry complexity. At 1080p with `MAX_DEPTH=10`:

- Rays per frame: ~20 million (1920 × 1080 × ~10 average bounces including shadows/AO)
- M4 base (10-core GPU): Easily sustains 60fps at 1080p with this scene
- iPad Air M4: 60fps at its native 2360×1640 is achievable
- iPhone 15 Pro (A17 Pro, 120Hz ProMotion): 120fps at 1080p is realistic

The performance gap only becomes a practical concern if the project later expands to:
- Hundreds or thousands of objects
- Full path tracing with many samples per pixel
- 4K resolution with high sample counts and maximum bounce depth

### Bottom line

For the Juggler screensaver as it exists today, any Apple Silicon device — including the base Mac
Mini M4 — is more than adequate. The RTX vs Apple Silicon gap is real for general RT workloads but
is not the constraint here. If the project later grows into a more ambitious renderer, the Mac Mini
M4 Pro (20-core GPU) would be worth the extra investment.

---

## What Ports for Free (No Changes)

These files contain zero platform dependencies and compile on the Mac as-is:

| File | Contents |
|------|----------|
| `src/animation.h/cpp` | Juggler kinematics — pure math, `std::vector`, `double` |
| `src/scene.h/cpp` | Sphere/material data structures |
| `src/camera.h/cpp` | Camera setup and orthonormal basis |
| `src/math_utils.h` | Vec3, matrix helpers |
| `src/shared_types.h` | CPU/GPU boundary structs (minor cleanup needed; see Phase 2) |
| `tests/` | All GoogleTest unit tests — pure CPU C++ |

---

## What Must Be Completely Replaced

### 1. GPU Backend

Every line of D3D12/DXR code must be replaced. The conceptual mapping is one-to-one but every
API call is different.

| D3D12 / DXR | Metal equivalent |
|-------------|-----------------|
| `ID3D12Device5` | `MTLDevice` |
| `ID3D12CommandQueue` | `MTLCommandQueue` |
| `IDXGISwapChain3` | `CAMetalLayer` + `MTLDrawable` |
| `ID3D12Resource` (buffer) | `MTLBuffer` |
| `ID3D12Resource` (texture) | `MTLTexture` |
| `ID3D12DescriptorHeap` | Argument buffers |
| `ID3D12GraphicsCommandList4` | `MTLCommandBuffer` + `MTLComputeCommandEncoder` |
| `ID3D12RootSignature` | Argument encoders / function constants |
| `ID3D12StateObject` (RT pipeline) | `MTLComputePipelineState` with `MTLLinkedFunctions` |
| BLAS | `MTLPrimitiveAccelerationStructure` |
| TLAS | `MTLInstanceAccelerationStructure` |
| Shader table | `MTLIntersectionFunctionTable` |
| DXC (runtime HLSL compiler) | `metal` compiler (offline, ships with Xcode) |
| `ComPtr<T>` (WRL) | `NS::SharedPtr<T>` via metal-cpp |
| `ID3D12Fence` + `HANDLE` | `MTLSharedEvent` or `DispatchSemaphore` |

**Note on metal-cpp:** Apple ships metal-cpp, a pure C++ wrapper for Metal, which lets the entire
GPU backend live in `.cpp` files with no Objective-C. This is the right choice here — it preserves
the existing code style. The windowing layer still needs `.mm` files (Objective-C++) but those
files can be kept minimal.

### 2. Shaders

All six HLSL shaders must be rewritten in Metal Shading Language (MSL). The critical architectural
difference is that **Metal ray tracing has no separate closest-hit or miss shader stage** — Metal
uses a compute kernel that calls an `intersector<>` object. This maps cleanly onto the existing
code because `raygen.hlsl` already uses an iterative bounce loop rather than DXR recursion.

| HLSL shader | Metal equivalent |
|-------------|-----------------|
| `raygen.hlsl` | Compute kernel; iterative bounce loop calls `intersector::intersect()` |
| `sphere_intersection.hlsl` | `[[intersection(bounding_box)]]` function; same quadratic math |
| `ground_intersection.hlsl` | `[[intersection(bounding_box)]]` function; same ray-plane math |
| `sphere_closesthit.hlsl` | Inlined into the compute kernel after the sphere hit branch |
| `ground_closesthit.hlsl` | Inlined into the compute kernel after the ground hit branch |
| `miss.hlsl` | Inlined into the compute kernel as the `intersection_type::none` branch |
| `common.hlsli` | Metal shader header; `cbuffer` → `constant PerFrameConstants&`; register bindings → `[[buffer(N)]]` |

The shadow/AO `RayQuery` calls in the closest-hit shaders port directly — Metal 2.4+ has
`intersection_query<>`, the exact same concept. All target devices (iPad Air M4, iPhone 15 Pro)
fully support Metal 2.4.

Metal shaders are compiled **offline by Xcode** into a `.metallib` bundle asset. There is no
runtime compilation, unlike DXC — this eliminates the startup compilation delay.

### 3. Windowing and App Lifecycle

The entire Win32 screensaver harness is replaced twice — once for macOS, once for iOS.

**macOS screensaver:**
- Subclass `ScreenSaverView` from `ScreenSaver.framework`
- Override `animateOneFrame` to call `renderer.render()` each frame
- Back the view with a `CAMetalLayer`; set `wantsLayer = YES` and override `makeBackingLayer`
- Bundle as a `.saver` package (directory bundle with `Info.plist` and principal class declared)
- Installs by double-clicking, same user experience as any macOS screensaver

**iOS app:**
- iOS has no screensaver API. The port ships as a fullscreen Metal app.
- `UIViewController` subclass with a `CAMetalLayer` (or `MTKView` as a simpler wrapper)
- `CADisplayLink` drives the render loop at display refresh rate
- Set `UIApplication.shared.isIdleTimerDisabled = true` to prevent sleep
- Distributable on the App Store or sideloaded via Xcode directly to personal devices

---

## Proposed Project Structure

```
Juggler/
├── shared/                    # Platform-agnostic C++ (moved from src/)
│   ├── animation.h/cpp
│   ├── scene.h/cpp
│   ├── camera.h/cpp
│   ├── math_utils.h
│   └── shared_types.h
│
├── metal_backend/             # New: Metal GPU layer (metal-cpp, no Obj-C)
│   ├── MetalRenderer.h/cpp
│   ├── MetalResources.h/cpp
│   └── MetalAccelStructs.h/cpp
│
├── shaders_metal/             # New: MSL shaders
│   ├── raygen.metal
│   ├── intersect_sphere.metal
│   ├── intersect_ground.metal
│   └── common.h
│
├── macos/                     # New: macOS screensaver harness (.mm files)
│   ├── JugglerScreenSaverView.h/mm
│   └── Info.plist
│
├── ios/                       # New: iOS app harness (.mm or Swift)
│   ├── AppDelegate.h/mm
│   ├── ViewController.h/mm
│   └── Info.plist
│
├── windows/                   # Existing Windows code (unchanged)
│   ├── src/
│   └── shaders/
│
└── tests/                     # Unchanged; runs on both platforms via CMake
```

---

## Implementation Phases

### Phase 1 — Development Environment (1 week)
- Acquire Mac with Apple Silicon; install Xcode
- Set up Apple Developer account ($99/year, needed for device deployment)
- Install CMake on the Mac
- Clone the repo, verify the CPU tests build and pass on macOS via CMake
- No GPU code yet — confirm the shared layer compiles cleanly with Clang on macOS

### Phase 2 — Portable C++ Isolation (3–5 days)
- Move `animation`, `scene`, `camera`, `math_utils`, `shared_types` to `shared/`
- Audit for accidental Windows headers (a few `<Windows.h>` transitive includes to remove)
- Add a thin abstract `IRenderer` interface that both the D3D12 and Metal backends implement
- Verify the existing tests still pass on both platforms

### Phase 3 — Metal GPU Backend (3–4 weeks; critical path)

Sub-tasks in order:

1. **Device and command infrastructure** — `MTLDevice`, `MTLCommandQueue`, command buffer
   creation. Analogous to `createDevice()` and `createCommandQueue()` in `renderer.cpp`.

2. **Buffers and textures** (`MetalResources`) — `MTLBuffer` for sphere data, material data,
   and constants; `MTLTexture` for output and accumulation. The `GPUResources` namespace maps
   directly.

3. **Acceleration structures** (`MetalAccelStructs`) — Create
   `MTLAccelerationStructureBoundingBoxGeometryDescriptor` for both ground and sphere geometry
   (matching the current two-geometry-slot BLAS exactly). Build a
   `MTLPrimitiveAccelerationStructure` (BLAS) and a `MTLInstanceAccelerationStructure` with one
   identity-transform instance (TLAS). Both are rebuilt per-frame, same as the current code.

4. **Compute pipeline and intersection function table** (`MetalPipeline`) — Create a
   `MTLComputePipelineState` with `MTLLinkedFunctions` referencing the sphere and ground
   intersection functions (analogous to the DXR `ID3D12StateObject`). Create an
   `MTLIntersectionFunctionTable` populated with two function handles (analogous to the shader
   table).

5. **Render loop** (`MetalRenderer`) — Encode a `MTLComputeCommandEncoder`, bind buffers and
   textures, dispatch the compute kernel at `[width, height, 1]` threads, blit the output
   texture to the `CAMetalLayer` drawable, commit.

### Phase 4 — MSL Shader Translation (2 weeks)

In dependency order:

1. **`common.h`** — Translate `common.hlsli`. The `cbuffer` fields become
   `constant PerFrameConstants&`. `RayPayload` and `ProceduralAttributes` structs are preserved
   with syntax changes. PCG hash RNG, `randomUnitVector`, `isOccluded`, and `isAOOccluded`
   helpers translate directly. `intersection_query` replaces `RayQuery` for shadow/AO.

2. **`intersect_sphere.metal`** — Translate `sphere_intersection.hlsl`. Quadratic math is
   identical; function signature changes to `[[intersection(bounding_box, world_space_data)]]`.
   Material index lookup is unchanged.

3. **`intersect_ground.metal`** — Same pattern as sphere intersection.

4. **`raygen.metal`** — Translate `raygen.hlsl` into a compute kernel. The iterative bounce
   loop structure is already correct for Metal. Replace `TraceRay()` with
   `intersector.intersect()`. After each intersection, branch on result type to run inlined
   closest-hit logic (sphere or ground) or the miss sky gradient. Accumulation write maps to
   Metal's `texture2d<float, access::read_write>`.

5. **Lighting helpers** — The ambient, diffuse, specular, and reflection logic from
   `sphere_closesthit.hlsl` and `ground_closesthit.hlsl` is inlined into the compute kernel.
   The math is verbatim; only syntax changes.

The CPU test suite validates the sphere/ground intersection math and will catch translation errors
in the intersection functions before any GPU code runs.

### Phase 5 — macOS Screensaver (1 week)
- Create a `.saver` bundle with `NSPrincipalClass` pointing to `JugglerScreenSaverView`
- `JugglerScreenSaverView.mm`: Obj-C++ file. Override `initWithFrame:isPreview:` to create the
  `MetalRenderer`. Override `animateOneFrame` to call `renderer.render()`. Override
  `makeBackingLayer` to return a configured `CAMetalLayer`.
- The `.saver` bundle installs to `~/Library/Screen Savers/` by double-clicking

### Phase 6 — iOS App (1 week)
- New Xcode iOS app target
- `ViewController.mm`: Creates `MetalRenderer` in `viewDidLoad`, attaches a `CADisplayLink`
  calling `renderer.render()` at display refresh rate
- `UIApplication.shared.isIdleTimerDisabled = true`
- `Info.plist`: `UIRequiresFullScreen = YES`, status bar suppressed

### Phase 7 — Testing and Polish (1 week)
- Visual comparison: run Windows and macOS builds side-by-side, verify rendering matches
- Test on iPad Air M4 and iPhone 15 Pro via Xcode
- Profile with Metal System Trace in Instruments to identify any bottlenecks
- Verify the 30-frame animation cycle and temporal accumulation reset behavior match between
  platforms

---

## Risk Register

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| `MTLLinkedFunctions` / `MTLIntersectionFunctionTable` API is verbose and sparsely documented | Medium | Apple's Metal Sample Code repo has a Raytracing sample demonstrating this exact setup. Use it as a reference. |
| Subtle float precision differences between HLSL and MSL break lighting or animation | Low | CPU intersection tests catch math bugs before any GPU code runs. Run them on Mac first. |
| `ScreenSaverView` + `CAMetalLayer` quirks on macOS Sonoma (14+) | Low–Medium | macOS 14 changed screensaver hosting. The `CAMetalLayer` approach requires setting `layerContentsPlacement = NSViewLayerContentsPlacementScaleAxesIndependently`. Apple's screensaver sample code documents this. |
| iOS has no screensaver API | None (known) | Treated as a design decision up front — ships as a fullscreen app. |
| Windows build broken by restructuring | Low | Windows and Apple code share only the `shared/` layer. The `windows/` directory is untouched. |

---

## Cost Summary

| Item | Cost |
|------|------|
| Mac Mini M4 (hardware) | ~$599 |
| Apple Developer Program | $99/year |
| Xcode, CMake, metal-cpp | Free |
| **Total** | **~$698 first year, $99/year thereafter** |

---

## Recommended First Step

Once the Mac arrives, the highest-value first move is getting the CPU tests building and passing
under CMake on macOS (`cmake -G Xcode ..` or `cmake -G "Unix Makefiles" ..`). This confirms the
shared layer is clean and gives a working development loop before writing a single line of Metal
code. The GPU backend can then be built incrementally with the Metal frame debugger in Xcode
providing immediate visual feedback.
