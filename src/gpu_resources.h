#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include <stdexcept>
#include <string>

using Microsoft::WRL::ComPtr;

inline void ThrowIfFailed(HRESULT hr, const char* msg = "") {
    if (FAILED(hr)) {
        char buf[256];
        snprintf(buf, sizeof(buf), "D3D12 Error 0x%08X: %s", (unsigned)hr, msg);
        throw std::runtime_error(buf);
    }
}

namespace GPUResources {

    ComPtr<ID3D12Resource> CreateBuffer(
        ID3D12Device* device,
        uint64_t size,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON,
        D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT);

    ComPtr<ID3D12Resource> CreateUploadBuffer(
        ID3D12Device* device,
        uint64_t size);

    ComPtr<ID3D12Resource> CreateUAVBuffer(
        ID3D12Device* device,
        uint64_t size,
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    void UploadBufferData(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        ID3D12Resource* dest,
        const void* data,
        uint64_t size,
        ComPtr<ID3D12Resource>& uploadBuffer);

    ComPtr<ID3D12Resource> CreateTexture2D(
        ID3D12Device* device,
        uint32_t width, uint32_t height,
        DXGI_FORMAT format,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON);

    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
        ID3D12Device* device,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        uint32_t numDescriptors,
        bool shaderVisible = false);

    uint32_t Align(uint32_t value, uint32_t alignment);
    uint64_t Align(uint64_t value, uint64_t alignment);

} // namespace GPUResources
