#pragma once

#include "gpu_resources.h"
#include "dxr_pipeline.h"
#include "acceleration_structures.h"
#include "scene.h"
#include "animation.h"
#include "camera.h"
#include "shared_types.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <string>

class Renderer {
public:
    bool init(HWND hwnd, uint32_t width, uint32_t height);
    void render();
    void shutdown();

    bool isInitialized() const { return m_initialized; }

private:
    static constexpr uint32_t FRAME_COUNT = 2;
    static constexpr float ANIMATION_SPEED = 15.0f; // animation frames per second
    static constexpr float ORBIT_RADIUS = 350.0f;   // world units from juggler center
    static constexpr float ORBIT_HEIGHT = 130.0f;   // camera Y (above juggler's head ~155)
    static constexpr float ORBIT_SPEED  = 0.4f;     // radians/sec (~full orbit every 15 sec)

    void createDevice();
    void createCommandQueue();
    void createSwapChain(HWND hwnd);
    void createRTVHeap();
    void createCommandAllocatorsAndList();
    void createFence();
    void createOutputAndAccumulation();
    void createDescriptorHeap();
    void uploadMaterialData();

    void waitForGPU();
    void executeCommandList();
    void updateConstants();

    bool m_initialized = false;
    HWND m_hwnd = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // D3D12 core
    ComPtr<IDXGIFactory6> m_factory;
    ComPtr<ID3D12Device5> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Resource> m_renderTargets[FRAME_COUNT];
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    uint32_t m_rtvDescriptorSize = 0;

    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList4> m_commandList;

    // Fence
    ComPtr<ID3D12Fence> m_fence;
    HANDLE m_fenceEvent = nullptr;
    uint64_t m_fenceValue = 0;

    // DXR
    DXRPipeline m_pipeline;
    AccelerationStructures m_accelStructs;

    // Output resources
    ComPtr<ID3D12Resource> m_outputTexture;    // UAV for ray tracing output
    ComPtr<ID3D12Resource> m_accumTexture;     // Accumulation buffer
    ComPtr<ID3D12DescriptorHeap> m_srvUavHeap; // Shader-visible descriptor heap
    ComPtr<ID3D12Resource> m_constantBuffer;
    ComPtr<ID3D12Resource> m_materialBuffer;
    ComPtr<ID3D12Resource> m_materialUploadBuffer;

    // Scene
    Scene m_scene;
    Animation m_animation;
    Camera m_camera;

    // Timing
    double m_animTime = 0.0;
    double m_orbitAngle = 0.0;
    int m_lastAnimFrame = -1;
    uint32_t m_accumulatedFrames = 0;
    uint32_t m_totalFrameCount = 0;
    LARGE_INTEGER m_lastTime = {};
    LARGE_INTEGER m_frequency = {};
};
