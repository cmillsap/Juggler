#include "renderer.h"
#include <cmath>
#include <cstring>
#include <filesystem>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

bool Renderer::init(HWND hwnd, uint32_t width, uint32_t height) {
    m_hwnd = hwnd;
    m_width = width;
    m_height = height;

    try {
        createDevice();
        createCommandQueue();
        createSwapChain(hwnd);
        createRTVHeap();
        createCommandAllocatorsAndList();
        createFence();
        createOutputAndAccumulation();
        createDescriptorHeap();

        // Init scene
        m_scene.init();
        m_camera.init(m_width, m_height);

        // Upload material data
        uploadMaterialData();

        // Init acceleration structures
        m_accelStructs.init(m_device.Get());

        // Find shader directory
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring shaderDir = std::filesystem::path(exePath).parent_path().wstring() + L"\\shaders";

        // Init DXR pipeline
        m_pipeline.init(m_device.Get(), shaderDir);

        // Init timing
        QueryPerformanceFrequency(&m_frequency);
        QueryPerformanceCounter(&m_lastTime);

        m_initialized = true;
    } catch (const std::exception& e) {
        OutputDebugStringA(e.what());
        MessageBoxA(hwnd, e.what(), "Juggler - Initialization Error", MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}

void Renderer::createDevice() {
#ifdef _DEBUG
    {
        ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
            debug->EnableDebugLayer();
        }
    }
#endif

    ThrowIfFailed(CreateDXGIFactory2(
#ifdef _DEBUG
        DXGI_CREATE_FACTORY_DEBUG,
#else
        0,
#endif
        IID_PPV_ARGS(&m_factory)), "CreateDXGIFactory2");

    ComPtr<IDXGIAdapter1> adapter;
    for (uint32_t i = 0; m_factory->EnumAdapterByGpuPreference(
            i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; i++) {

        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

        ComPtr<ID3D12Device5> device;
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1,
                IID_PPV_ARGS(&device)))) {
            // Check DXR 1.1 support
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(
                    D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)))) {
                if (options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1) {
                    m_device = device;
                    return;
                }
            }
        }
        adapter.Reset();
    }

    throw std::runtime_error("No DXR 1.1 capable GPU found");
}

void Renderer::createCommandQueue() {
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue)),
        "CreateCommandQueue");
}

void Renderer::createSwapChain(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = m_width;
    desc.Height = m_height;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = FRAME_COUNT;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    // Retry on E_ACCESSDENIED: DXGI's cross-process swap chain state for this
    // HWND may not have been released yet if a previous instance was recently
    // terminated.  Retry for up to ~2 seconds before giving up.
    ComPtr<IDXGISwapChain1> swapChain;
    HRESULT hr = E_FAIL;
    for (int attempt = 0; attempt < 20; ++attempt) {
        hr = m_factory->CreateSwapChainForHwnd(
            m_commandQueue.Get(), hwnd, &desc, nullptr, nullptr, &swapChain);
        if (SUCCEEDED(hr)) break;
        if (hr != HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) break;
        swapChain.Reset();
        Sleep(100);
    }
    ThrowIfFailed(hr, "CreateSwapChainForHwnd");

    ThrowIfFailed(m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER),
        "MakeWindowAssociation");

    ThrowIfFailed(swapChain.As(&m_swapChain), "SwapChain As");
}

void Renderer::createRTVHeap() {
    m_rtvHeap = GPUResources::CreateDescriptorHeap(
        m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FRAME_COUNT);
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (uint32_t i = 0; i < FRAME_COUNT; i++) {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])),
            "GetBuffer");
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;
    }
}

void Renderer::createCommandAllocatorsAndList() {
    ThrowIfFailed(m_device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)),
        "CreateCommandAllocator");

    ThrowIfFailed(m_device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(),
        nullptr, IID_PPV_ARGS(&m_commandList)),
        "CreateCommandList");

    ThrowIfFailed(m_commandList->Close(), "Close command list");
}

void Renderer::createFence() {
    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)),
        "CreateFence");
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent) throw std::runtime_error("CreateEvent failed");
}

void Renderer::createOutputAndAccumulation() {
    m_outputTexture = GPUResources::CreateTexture2D(
        m_device.Get(), m_width, m_height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    m_accumTexture = GPUResources::CreateTexture2D(
        m_device.Get(), m_width, m_height,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    // Constant buffer (256-byte aligned)
    uint64_t cbSize = GPUResources::Align((uint64_t)sizeof(PerFrameConstants), (uint64_t)256);
    m_constantBuffer = GPUResources::CreateUploadBuffer(m_device.Get(), cbSize);
}

void Renderer::createDescriptorHeap() {
    // 6 descriptors: TLAS(SRV), Output(UAV), Constants(CBV), SphereData(SRV), Materials(SRV), Accum(UAV)
    m_srvUavHeap = GPUResources::CreateDescriptorHeap(
        m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 6, true);

    uint32_t descSize = m_device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();

    // Slot 0: TLAS SRV - created during render after AS build

    // Slot 1: Output UAV
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    D3D12_CPU_DESCRIPTOR_HANDLE outputHandle = cpuHandle;
    outputHandle.ptr += descSize;
    m_device->CreateUnorderedAccessView(m_outputTexture.Get(), nullptr, &uavDesc, outputHandle);

    // Slot 2: CBV
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = GPUResources::Align((uint32_t)sizeof(PerFrameConstants), 256u);
    D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = cpuHandle;
    cbvHandle.ptr += 2 * descSize;
    m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);

    // Slot 3: Sphere data SRV - created during render after data upload
    // Slot 4: Material data SRV - created after material upload
    // Slot 5: Accumulation UAV
    D3D12_UNORDERED_ACCESS_VIEW_DESC accumUavDesc = {};
    accumUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    accumUavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    D3D12_CPU_DESCRIPTOR_HANDLE accumHandle = cpuHandle;
    accumHandle.ptr += 5 * descSize;
    m_device->CreateUnorderedAccessView(m_accumTexture.Get(), nullptr, &accumUavDesc, accumHandle);
}

void Renderer::uploadMaterialData() {
    uint64_t matSize = m_scene.materials.size() * sizeof(GPUMaterialData);
    m_materialBuffer = GPUResources::CreateBuffer(m_device.Get(), matSize,
        D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON);

    std::vector<GPUMaterialData> gpuMats(m_scene.materials.size());
    for (size_t i = 0; i < m_scene.materials.size(); i++) {
        const auto& m = m_scene.materials[i];
        gpuMats[i].diffuseColor = { (float)m.diffuseColor[0], (float)m.diffuseColor[1], (float)m.diffuseColor[2] };
        gpuMats[i].ambientWeight = (float)m.ambientWeight;
        gpuMats[i].highlightColor = { (float)m.highlightColor[0], (float)m.highlightColor[1], (float)m.highlightColor[2] };
        gpuMats[i].ambientOcclusionPercent = (float)m.ambientOcclusionPercent;
        gpuMats[i].reflectionColor = { (float)m.reflectionColor[0], (float)m.reflectionColor[1], (float)m.reflectionColor[2] };
        gpuMats[i].diffuseWeight = (float)m.diffuseWeight;
        gpuMats[i].specularWeight = (float)m.specularWeight;
        gpuMats[i].reflectionWeight = (float)m.reflectionWeight;
        gpuMats[i].shininess = (float)m.shininess;
        gpuMats[i].padding = 0;
    }

    // Upload
    ThrowIfFailed(m_commandAllocator->Reset(), "Reset allocator");
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr), "Reset cmdlist");

    m_materialUploadBuffer = GPUResources::CreateUploadBuffer(m_device.Get(), matSize);
    void* mapped = nullptr;
    ThrowIfFailed(m_materialUploadBuffer->Map(0, nullptr, &mapped), "Map materials");
    memcpy(mapped, gpuMats.data(), (size_t)matSize);
    m_materialUploadBuffer->Unmap(0, nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_materialBuffer.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(1, &barrier);

    m_commandList->CopyBufferRegion(m_materialBuffer.Get(), 0,
        m_materialUploadBuffer.Get(), 0, matSize);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    m_commandList->ResourceBarrier(1, &barrier);

    executeCommandList();
    waitForGPU();

    // Create material SRV (slot 4)
    uint32_t descSize = m_device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE matHandle = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    matHandle.ptr += 4 * descSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC matSrvDesc = {};
    matSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    matSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    matSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    matSrvDesc.Buffer.NumElements = (uint32_t)m_scene.materials.size();
    matSrvDesc.Buffer.StructureByteStride = sizeof(GPUMaterialData);
    m_device->CreateShaderResourceView(m_materialBuffer.Get(), &matSrvDesc, matHandle);
}

void Renderer::updateConstants() {
    PerFrameConstants cb = {};
    cb.cameraPos = { (float)m_camera.eye.x, (float)m_camera.eye.y, (float)m_camera.eye.z };
    cb.frameCount = m_totalFrameCount;
    cb.cameraU = { (float)m_camera.u.x, (float)m_camera.u.y, (float)m_camera.u.z };
    cb.virtualScreenRatio = (float)m_camera.virtualScreenRatio;
    cb.cameraV = { (float)m_camera.v.x, (float)m_camera.v.y, (float)m_camera.v.z };
    cb.distanceToScreen = (float)m_camera.distanceToScreen;
    cb.virtualScreenCenter = { (float)m_camera.virtualScreenCenter.x,
                                (float)m_camera.virtualScreenCenter.y,
                                (float)m_camera.virtualScreenCenter.z };
    cb.numSpheres = Scene::NUM_SPHERES;

    cb.lightPos = { -564.0f, 686.0f, 147.0f };
    cb.lightRadius = 10.0f;
    cb.lightColor = { 1.0f, 1.0f, 1.0f };
    cb.groundSquareSize = 107.0f;
    cb.ambientColor = { 1.0f, 1.0f, 1.0f };
    cb.invGroundSquareSize = 1.0f / 107.0f;

    // Sky colors (gamma-encoded from Java reference)
    // minColor = 0xBDBDFF scale=1.0, maxColor = 0x2223F6 scale=1.0
    cb.skyMinColor = { (float)std::pow(0xBD / 255.0, 2.2),
                       (float)std::pow(0xBD / 255.0, 2.2),
                       (float)std::pow(0xFF / 255.0, 2.2) };
    cb.skyMaxColor = { (float)std::pow(0x22 / 255.0, 2.2),
                       (float)std::pow(0x23 / 255.0, 2.2),
                       (float)std::pow(0xF6 / 255.0, 2.2) };

    cb.maxOcclusionDist = 100.0f;
    cb.accumulatedFrames = m_accumulatedFrames;
    cb.screenSizeX = m_width;
    cb.screenSizeY = m_height;
    cb.halfWidth = (float)(m_width / 2.0);
    cb.halfHeight = (float)(m_height / 2.0);

    void* mapped = nullptr;
    ThrowIfFailed(m_constantBuffer->Map(0, nullptr, &mapped), "Map constants");
    memcpy(mapped, &cb, sizeof(cb));
    m_constantBuffer->Unmap(0, nullptr);
}

void Renderer::render() {
    if (!m_initialized) return;

    // Update animation time
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double dt = (double)(now.QuadPart - m_lastTime.QuadPart) / m_frequency.QuadPart;
    m_lastTime = now;

    m_animTime += dt * ANIMATION_SPEED;
    while (m_animTime >= Animation::FRAME_COUNT) {
        m_animTime -= Animation::FRAME_COUNT;
    }

    int currentFrame = (int)m_animTime;
    if (currentFrame != m_lastAnimFrame) {
        m_accumulatedFrames = 0;
        m_lastAnimFrame = currentFrame;
    }

    // Update scene
    m_animation.update(m_animTime, m_scene.spheres);

    // Orbit camera — reset accumulation each frame since the view changes continuously
    m_accumulatedFrames = 0;
    m_orbitAngle += dt * ORBIT_SPEED;
    static const Vec3 orbitTarget = { 151.0, 100.0, -151.0 };
    m_camera.updateOrbit(orbitTarget, m_orbitAngle, ORBIT_RADIUS, ORBIT_HEIGHT);

    // Begin command recording
    ThrowIfFailed(m_commandAllocator->Reset(), "Reset allocator");
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr), "Reset cmdlist");

    // Build acceleration structures
    m_accelStructs.build(m_device.Get(), m_commandList.Get(), m_scene.spheres);

    // Create TLAS SRV (slot 0) - recreated each frame since TLAS may be new
    uint32_t descSize = m_device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE tlasHandle = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_SHADER_RESOURCE_VIEW_DESC tlasSrvDesc = {};
    tlasSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    tlasSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    tlasSrvDesc.RaytracingAccelerationStructure.Location =
        m_accelStructs.getTLAS()->GetGPUVirtualAddress();
    m_device->CreateShaderResourceView(nullptr, &tlasSrvDesc, tlasHandle);

    // Create sphere data SRV (slot 3)
    D3D12_CPU_DESCRIPTOR_HANDLE sphereHandle = tlasHandle;
    sphereHandle.ptr += 3 * descSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC sphereSrvDesc = {};
    sphereSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    sphereSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sphereSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    sphereSrvDesc.Buffer.NumElements = Scene::NUM_SPHERES;
    sphereSrvDesc.Buffer.StructureByteStride = sizeof(GPUSphereData);
    m_device->CreateShaderResourceView(
        m_accelStructs.getSphereDataBuffer(), &sphereSrvDesc, sphereHandle);

    // Update constants
    m_accumulatedFrames++;
    m_totalFrameCount++;
    updateConstants();

    // Set descriptor heap and root signature
    ID3D12DescriptorHeap* heaps[] = { m_srvUavHeap.Get() };
    m_commandList->SetDescriptorHeaps(1, heaps);
    m_commandList->SetComputeRootSignature(m_pipeline.getRootSignature());
    m_commandList->SetComputeRootDescriptorTable(0,
        m_srvUavHeap->GetGPUDescriptorHandleForHeapStart());

    // Dispatch rays
    D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
    dispatchDesc.RayGenerationShaderRecord = m_pipeline.getRayGenRecord();
    dispatchDesc.MissShaderTable = m_pipeline.getMissTable();
    dispatchDesc.HitGroupTable = m_pipeline.getHitGroupTable();
    dispatchDesc.Width = m_width;
    dispatchDesc.Height = m_height;
    dispatchDesc.Depth = 1;

    m_commandList->SetPipelineState1(m_pipeline.getStateObject());
    m_commandList->DispatchRays(&dispatchDesc);

    // Copy output to back buffer
    uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    D3D12_RESOURCE_BARRIER barriers[2] = {};

    // Output: UAV -> COPY_SOURCE
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = m_outputTexture.Get();
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    // Back buffer: PRESENT -> COPY_DEST
    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = m_renderTargets[backBufferIndex].Get();
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    m_commandList->ResourceBarrier(2, barriers);

    m_commandList->CopyResource(m_renderTargets[backBufferIndex].Get(), m_outputTexture.Get());

    // Transition back
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

    m_commandList->ResourceBarrier(2, barriers);

    executeCommandList();
    ThrowIfFailed(m_swapChain->Present(1, 0), "Present");
    waitForGPU();
}

void Renderer::executeCommandList() {
    ThrowIfFailed(m_commandList->Close(), "Close command list");
    ID3D12CommandList* lists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(1, lists);
}

void Renderer::waitForGPU() {
    m_fenceValue++;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue), "Signal");

    if (m_fence->GetCompletedValue() < m_fenceValue) {
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent),
            "SetEventOnCompletion");
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void Renderer::shutdown() {
    if (!m_initialized) return;
    waitForGPU();
    if (m_fenceEvent) {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
    m_initialized = false;
}
