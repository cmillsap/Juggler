#pragma once

#include "gpu_resources.h"
#include <d3d12.h>
#include <vector>
#include <string>

class DXRPipeline {
public:
    void init(ID3D12Device5* device, const std::wstring& shaderDir);

    ID3D12RootSignature* getRootSignature() const { return m_globalRootSig.Get(); }
    ID3D12StateObject* getStateObject() const { return m_stateObject.Get(); }

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE getRayGenRecord() const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE getMissTable() const;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE getHitGroupTable() const;

private:
    void createGlobalRootSignature(ID3D12Device5* device);
    void createStateObject(ID3D12Device5* device, const std::wstring& shaderDir);
    void buildShaderTable(ID3D12Device5* device);

    ComPtr<ID3D12RootSignature> m_globalRootSig;
    ComPtr<ID3D12StateObject> m_stateObject;
    ComPtr<ID3D12Resource> m_shaderTable;

    uint32_t m_shaderTableRecordSize = 0;
    static constexpr uint32_t NUM_SHADER_TABLE_RECORDS = 4; // raygen, miss, hit_ground, hit_sphere
};
