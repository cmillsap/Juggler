#include "acceleration_structures.h"
#include "shared_types.h"
#include <algorithm>
#include <cstring>

void AccelerationStructures::init(ID3D12Device5* device) {
    // Pre-allocate persistent buffers will happen on first build
}

void AccelerationStructures::build(
    ID3D12Device5* device,
    ID3D12GraphicsCommandList4* cmdList,
    const std::vector<SphereData>& spheres) {

    buildBLAS(device, cmdList, spheres);
    buildTLAS(device, cmdList);

    // Upload sphere data to GPU structured buffer
    uint64_t sphereDataSize = spheres.size() * sizeof(GPUSphereData);
    if (!m_sphereDataBuffer) {
        m_sphereDataBuffer = GPUResources::CreateBuffer(device, sphereDataSize,
            D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON);
    }

    std::vector<GPUSphereData> gpuSpheres(spheres.size());
    for (size_t i = 0; i < spheres.size(); i++) {
        gpuSpheres[i].center = { (float)spheres[i].center.x,
                                  (float)spheres[i].center.y,
                                  (float)spheres[i].center.z };
        gpuSpheres[i].radius = (float)spheres[i].radius;
    }

    m_sphereDataUploadBuffer = GPUResources::CreateUploadBuffer(device, sphereDataSize);
    void* mapped = nullptr;
    ThrowIfFailed(m_sphereDataUploadBuffer->Map(0, nullptr, &mapped), "Map sphere data");
    memcpy(mapped, gpuSpheres.data(), (size_t)sphereDataSize);
    m_sphereDataUploadBuffer->Unmap(0, nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_sphereDataBuffer.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    cmdList->CopyBufferRegion(m_sphereDataBuffer.Get(), 0,
        m_sphereDataUploadBuffer.Get(), 0, sphereDataSize);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(1, &barrier);
}

void AccelerationStructures::buildBLAS(
    ID3D12Device5* device,
    ID3D12GraphicsCommandList4* cmdList,
    const std::vector<SphereData>& spheres) {

    // Build AABB buffer: 1 ground AABB + 84 sphere AABBs
    int numSpheres = (int)spheres.size();
    int totalAABBs = 1 + numSpheres;

    std::vector<D3D12_RAYTRACING_AABB> aabbs(totalAABBs);

    // Ground AABB: huge plane at y=0
    aabbs[0].MinX = -1e6f;
    aabbs[0].MinY = -0.01f;
    aabbs[0].MinZ = -1e6f;
    aabbs[0].MaxX = 1e6f;
    aabbs[0].MaxY = 0.01f;
    aabbs[0].MaxZ = 1e6f;

    // Sphere AABBs
    for (int i = 0; i < numSpheres; i++) {
        float cx = (float)spheres[i].center.x;
        float cy = (float)spheres[i].center.y;
        float cz = (float)spheres[i].center.z;
        float r = (float)spheres[i].radius;
        aabbs[1 + i].MinX = cx - r;
        aabbs[1 + i].MinY = cy - r;
        aabbs[1 + i].MinZ = cz - r;
        aabbs[1 + i].MaxX = cx + r;
        aabbs[1 + i].MaxY = cy + r;
        aabbs[1 + i].MaxZ = cz + r;
    }

    uint64_t aabbSize = totalAABBs * sizeof(D3D12_RAYTRACING_AABB);

    m_aabbBuffer = GPUResources::CreateBuffer(device, aabbSize,
        D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON);
    m_aabbUploadBuffer = GPUResources::CreateUploadBuffer(device, aabbSize);

    void* mapped = nullptr;
    ThrowIfFailed(m_aabbUploadBuffer->Map(0, nullptr, &mapped), "Map AABB");
    memcpy(mapped, aabbs.data(), (size_t)aabbSize);
    m_aabbUploadBuffer->Unmap(0, nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_aabbBuffer.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    cmdList->CopyBufferRegion(m_aabbBuffer.Get(), 0,
        m_aabbUploadBuffer.Get(), 0, aabbSize);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(1, &barrier);

    // Two geometries: ground (1 AABB) and spheres (numSpheres AABBs)
    D3D12_RAYTRACING_GEOMETRY_DESC geomDescs[2] = {};

    // Geometry 0: Ground
    geomDescs[0].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
    geomDescs[0].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    geomDescs[0].AABBs.AABBCount = 1;
    geomDescs[0].AABBs.AABBs.StartAddress = m_aabbBuffer->GetGPUVirtualAddress();
    geomDescs[0].AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);

    // Geometry 1: Spheres
    geomDescs[1].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
    geomDescs[1].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    geomDescs[1].AABBs.AABBCount = numSpheres;
    geomDescs[1].AABBs.AABBs.StartAddress =
        m_aabbBuffer->GetGPUVirtualAddress() + sizeof(D3D12_RAYTRACING_AABB);
    geomDescs[1].AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blasInputs = {};
    blasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    blasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
    blasInputs.NumDescs = 2;
    blasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    blasInputs.pGeometryDescs = geomDescs;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO blasPrebuild = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&blasInputs, &blasPrebuild);

    uint64_t scratchSize = GPUResources::Align(
        blasPrebuild.ScratchDataSizeInBytes, (uint64_t)D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
    uint64_t blasSize = GPUResources::Align(
        blasPrebuild.ResultDataMaxSizeInBytes, (uint64_t)D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

    if (!m_scratchBuffer || m_scratchBuffer->GetDesc().Width < scratchSize) {
        m_scratchBuffer = GPUResources::CreateUAVBuffer(device, scratchSize);
    }

    m_blas = GPUResources::CreateBuffer(device, blasSize,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blasBuildDesc = {};
    blasBuildDesc.Inputs = blasInputs;
    blasBuildDesc.DestAccelerationStructureData = m_blas->GetGPUVirtualAddress();
    blasBuildDesc.ScratchAccelerationStructureData = m_scratchBuffer->GetGPUVirtualAddress();

    cmdList->BuildRaytracingAccelerationStructure(&blasBuildDesc, 0, nullptr);

    // UAV barrier for BLAS
    D3D12_RESOURCE_BARRIER uavBarrier = {};
    uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource = m_blas.Get();
    cmdList->ResourceBarrier(1, &uavBarrier);
}

void AccelerationStructures::buildTLAS(
    ID3D12Device5* device,
    ID3D12GraphicsCommandList4* cmdList) {

    // Single instance desc
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.Transform[0][0] = 1.0f;
    instanceDesc.Transform[1][1] = 1.0f;
    instanceDesc.Transform[2][2] = 1.0f;
    instanceDesc.InstanceMask = 0xFF;
    instanceDesc.InstanceContributionToHitGroupIndex = 0;
    instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    instanceDesc.AccelerationStructure = m_blas->GetGPUVirtualAddress();

    uint64_t instanceSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

    m_instanceBuffer = GPUResources::CreateBuffer(device, instanceSize,
        D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON);
    m_instanceUploadBuffer = GPUResources::CreateUploadBuffer(device, instanceSize);

    void* mapped = nullptr;
    ThrowIfFailed(m_instanceUploadBuffer->Map(0, nullptr, &mapped), "Map instance");
    memcpy(mapped, &instanceDesc, instanceSize);
    m_instanceUploadBuffer->Unmap(0, nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_instanceBuffer.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    cmdList->CopyBufferRegion(m_instanceBuffer.Get(), 0,
        m_instanceUploadBuffer.Get(), 0, instanceSize);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(1, &barrier);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlasInputs = {};
    tlasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    tlasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
    tlasInputs.NumDescs = 1;
    tlasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    tlasInputs.InstanceDescs = m_instanceBuffer->GetGPUVirtualAddress();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlasPrebuild = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&tlasInputs, &tlasPrebuild);

    uint64_t scratchSize = GPUResources::Align(
        tlasPrebuild.ScratchDataSizeInBytes, (uint64_t)D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
    uint64_t tlasSize = GPUResources::Align(
        tlasPrebuild.ResultDataMaxSizeInBytes, (uint64_t)D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

    // Reuse or grow scratch buffer
    if (!m_scratchBuffer || m_scratchBuffer->GetDesc().Width < scratchSize) {
        m_scratchBuffer = GPUResources::CreateUAVBuffer(device, scratchSize);
    }

    m_tlas = GPUResources::CreateBuffer(device, tlasSize,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasBuildDesc = {};
    tlasBuildDesc.Inputs = tlasInputs;
    tlasBuildDesc.DestAccelerationStructureData = m_tlas->GetGPUVirtualAddress();
    tlasBuildDesc.ScratchAccelerationStructureData = m_scratchBuffer->GetGPUVirtualAddress();

    cmdList->BuildRaytracingAccelerationStructure(&tlasBuildDesc, 0, nullptr);

    D3D12_RESOURCE_BARRIER uavBarrier = {};
    uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource = m_tlas.Get();
    cmdList->ResourceBarrier(1, &uavBarrier);
}
