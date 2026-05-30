#include "gpu_resources.h"

namespace GPUResources {

ComPtr<ID3D12Resource> CreateBuffer(
    ID3D12Device* device,
    uint64_t size,
    D3D12_RESOURCE_FLAGS flags,
    D3D12_RESOURCE_STATES initialState,
    D3D12_HEAP_TYPE heapType) {

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = heapType;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = flags;

    ComPtr<ID3D12Resource> resource;
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        initialState, nullptr, IID_PPV_ARGS(&resource)),
        "CreateBuffer");
    return resource;
}

ComPtr<ID3D12Resource> CreateUploadBuffer(
    ID3D12Device* device,
    uint64_t size) {
    return CreateBuffer(device, size, D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
}

ComPtr<ID3D12Resource> CreateUAVBuffer(
    ID3D12Device* device,
    uint64_t size,
    D3D12_RESOURCE_STATES initialState) {
    return CreateBuffer(device, size,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, initialState);
}

void UploadBufferData(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12Resource* dest,
    const void* data,
    uint64_t size,
    ComPtr<ID3D12Resource>& uploadBuffer) {

    uploadBuffer = CreateUploadBuffer(device, size);

    void* mapped = nullptr;
    ThrowIfFailed(uploadBuffer->Map(0, nullptr, &mapped), "Map upload buffer");
    memcpy(mapped, data, (size_t)size);
    uploadBuffer->Unmap(0, nullptr);

    cmdList->CopyBufferRegion(dest, 0, uploadBuffer.Get(), 0, size);
}

ComPtr<ID3D12Resource> CreateTexture2D(
    ID3D12Device* device,
    uint32_t width, uint32_t height,
    DXGI_FORMAT format,
    D3D12_RESOURCE_FLAGS flags,
    D3D12_RESOURCE_STATES initialState) {

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.Flags = flags;

    ComPtr<ID3D12Resource> resource;
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        initialState, nullptr, IID_PPV_ARGS(&resource)),
        "CreateTexture2D");
    return resource;
}

ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
    ID3D12Device* device,
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    uint32_t numDescriptors,
    bool shaderVisible) {

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = type;
    desc.NumDescriptors = numDescriptors;
    desc.Flags = shaderVisible ?
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE :
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ComPtr<ID3D12DescriptorHeap> heap;
    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)),
        "CreateDescriptorHeap");
    return heap;
}

uint32_t Align(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

uint64_t Align(uint64_t value, uint64_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace GPUResources
