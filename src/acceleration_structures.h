#pragma once

#include "gpu_resources.h"
#include "scene.h"
#include <d3d12.h>
#include <vector>

class AccelerationStructures {
public:
    void init(ID3D12Device5* device);
    void build(
        ID3D12Device5* device,
        ID3D12GraphicsCommandList4* cmdList,
        const std::vector<SphereData>& spheres);

    ID3D12Resource* getTLAS() const { return m_tlas.Get(); }
    ID3D12Resource* getSphereDataBuffer() const { return m_sphereDataBuffer.Get(); }

private:
    void buildBLAS(
        ID3D12Device5* device,
        ID3D12GraphicsCommandList4* cmdList,
        const std::vector<SphereData>& spheres);
    void buildTLAS(
        ID3D12Device5* device,
        ID3D12GraphicsCommandList4* cmdList);

    ComPtr<ID3D12Resource> m_blasBuffer;
    ComPtr<ID3D12Resource> m_tlasBuffer;
    ComPtr<ID3D12Resource> m_tlas;
    ComPtr<ID3D12Resource> m_blas;
    ComPtr<ID3D12Resource> m_scratchBuffer;
    ComPtr<ID3D12Resource> m_instanceBuffer;
    ComPtr<ID3D12Resource> m_aabbBuffer;
    ComPtr<ID3D12Resource> m_aabbUploadBuffer;
    ComPtr<ID3D12Resource> m_instanceUploadBuffer;
    ComPtr<ID3D12Resource> m_sphereDataBuffer;
    ComPtr<ID3D12Resource> m_sphereDataUploadBuffer;
};
