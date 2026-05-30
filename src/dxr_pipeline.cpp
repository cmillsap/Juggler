#include "dxr_pipeline.h"
#include <dxcapi.h>
#include <fstream>
#include <sstream>
#include <filesystem>

#pragma comment(lib, "dxcompiler.lib")

static std::vector<uint8_t> ReadFile(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::string narrow(path.length(), ' ');
        for (size_t i = 0; i < path.length(); i++) narrow[i] = (char)path[i];
        throw std::runtime_error("Cannot open file: " + narrow);
    }
    size_t size = (size_t)file.tellg();
    file.seekg(0);
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

// Custom include handler for HLSL #include directives
class ShaderIncludeHandler : public IDxcIncludeHandler {
public:
    ShaderIncludeHandler(IDxcUtils* utils, const std::wstring& baseDir)
        : m_utils(utils), m_baseDir(baseDir), m_refCount(1) {}

    HRESULT STDMETHODCALLTYPE LoadSource(
        LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override {
        std::wstring fullPath = m_baseDir + L"\\" + pFilename;
        ComPtr<IDxcBlobEncoding> source;
        HRESULT hr = m_utils->LoadFile(fullPath.c_str(), nullptr, &source);
        if (SUCCEEDED(hr)) {
            *ppIncludeSource = source.Detach();
        }
        return hr;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if (riid == __uuidof(IDxcIncludeHandler) || riid == __uuidof(IUnknown)) {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refCount; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG ref = --m_refCount;
        if (ref == 0) delete this;
        return ref;
    }

private:
    IDxcUtils* m_utils;
    std::wstring m_baseDir;
    ULONG m_refCount;
};

static ComPtr<IDxcBlob> CompileShader(
    const std::wstring& filename, const wchar_t* entryPoint,
    const wchar_t* target, const std::wstring& shaderDir) {

    ComPtr<IDxcUtils> utils;
    ComPtr<IDxcCompiler3> compiler;
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)), "DxcCreateInstance utils");
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)), "DxcCreateInstance compiler");

    std::wstring fullPath = shaderDir + L"\\" + filename;
    ComPtr<IDxcBlobEncoding> source;
    ThrowIfFailed(utils->LoadFile(fullPath.c_str(), nullptr, &source), "LoadFile");

    ShaderIncludeHandler* includeHandler = new ShaderIncludeHandler(utils.Get(), shaderDir);

    std::wstring includeArg = L"-I";
    std::wstring includeDir = shaderDir;

    const wchar_t* args[] = {
        filename.c_str(),
        L"-E", entryPoint,
        L"-T", target,
        includeArg.c_str(), includeDir.c_str(),
        L"-HV", L"2021",
    };

    DxcBuffer sourceBuffer = {};
    sourceBuffer.Ptr = source->GetBufferPointer();
    sourceBuffer.Size = source->GetBufferSize();
    sourceBuffer.Encoding = DXC_CP_ACP;

    ComPtr<IDxcResult> result;
    ThrowIfFailed(compiler->Compile(&sourceBuffer, args, _countof(args),
        includeHandler, IID_PPV_ARGS(&result)), "Compile");

    includeHandler->Release();

    ComPtr<IDxcBlobUtf8> errors;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    if (errors && errors->GetStringLength() > 0) {
        OutputDebugStringA(errors->GetStringPointer());
    }

    HRESULT hrStatus;
    result->GetStatus(&hrStatus);
    if (FAILED(hrStatus)) {
        std::string errMsg = "Shader compilation failed: ";
        if (errors) errMsg += errors->GetStringPointer();
        throw std::runtime_error(errMsg);
    }

    ComPtr<IDxcBlob> shader;
    ThrowIfFailed(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr), "GetOutput");
    return shader;
}

void DXRPipeline::createGlobalRootSignature(ID3D12Device5* device) {
    // Global root signature:
    // t0 - TLAS (SRV)
    // u0 - Output texture (UAV)
    // b0 - PerFrameConstants (CBV)
    // t1 - Sphere data (SRV)
    // t2 - Material data (SRV)
    // u1 - Accumulation buffer (UAV)

    D3D12_DESCRIPTOR_RANGE1 ranges[6] = {};

    // t0: TLAS
    ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[0].NumDescriptors = 1;
    ranges[0].BaseShaderRegister = 0;
    ranges[0].RegisterSpace = 0;
    ranges[0].OffsetInDescriptorsFromTableStart = 0;

    // u0: Output
    ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    ranges[1].NumDescriptors = 1;
    ranges[1].BaseShaderRegister = 0;
    ranges[1].RegisterSpace = 0;
    ranges[1].OffsetInDescriptorsFromTableStart = 1;

    // b0: Constants
    ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    ranges[2].NumDescriptors = 1;
    ranges[2].BaseShaderRegister = 0;
    ranges[2].RegisterSpace = 0;
    ranges[2].OffsetInDescriptorsFromTableStart = 2;

    // t1: Sphere data
    ranges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[3].NumDescriptors = 1;
    ranges[3].BaseShaderRegister = 1;
    ranges[3].RegisterSpace = 0;
    ranges[3].OffsetInDescriptorsFromTableStart = 3;

    // t2: Material data
    ranges[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[4].NumDescriptors = 1;
    ranges[4].BaseShaderRegister = 2;
    ranges[4].RegisterSpace = 0;
    ranges[4].OffsetInDescriptorsFromTableStart = 4;

    // u1: Accumulation
    ranges[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    ranges[5].NumDescriptors = 1;
    ranges[5].BaseShaderRegister = 1;
    ranges[5].RegisterSpace = 0;
    ranges[5].OffsetInDescriptorsFromTableStart = 5;

    D3D12_ROOT_PARAMETER1 rootParam = {};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParam.DescriptorTable.NumDescriptorRanges = 6;
    rootParam.DescriptorTable.pDescriptorRanges = ranges;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rootSigDesc.Desc_1_1.NumParameters = 1;
    rootSigDesc.Desc_1_1.pParameters = &rootParam;
    rootSigDesc.Desc_1_1.NumStaticSamplers = 0;
    rootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> blob, error;
    ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSigDesc, &blob, &error),
        "SerializeRootSignature");

    ThrowIfFailed(device->CreateRootSignature(0, blob->GetBufferPointer(),
        blob->GetBufferSize(), IID_PPV_ARGS(&m_globalRootSig)),
        "CreateRootSignature");
}

void DXRPipeline::createStateObject(ID3D12Device5* device, const std::wstring& shaderDir) {
    // Compile all shaders
    auto raygenBlob = CompileShader(L"raygen.hlsl", L"RayGen", L"lib_6_5", shaderDir);
    auto missBlob = CompileShader(L"miss.hlsl", L"Miss", L"lib_6_5", shaderDir);
    auto sphereIsectBlob = CompileShader(L"sphere_intersection.hlsl", L"SphereIntersection", L"lib_6_5", shaderDir);
    auto groundIsectBlob = CompileShader(L"ground_intersection.hlsl", L"GroundIntersection", L"lib_6_5", shaderDir);
    auto sphereHitBlob = CompileShader(L"sphere_closesthit.hlsl", L"SphereClosestHit", L"lib_6_5", shaderDir);
    auto groundHitBlob = CompileShader(L"ground_closesthit.hlsl", L"GroundClosestHit", L"lib_6_5", shaderDir);

    // State object with subobjects
    std::vector<D3D12_STATE_SUBOBJECT> subobjects;
    subobjects.reserve(16);

    auto addDXILLib = [&](IDxcBlob* blob, const wchar_t* exportName) {
        auto* libDesc = new D3D12_DXIL_LIBRARY_DESC();
        libDesc->DXILLibrary.pShaderBytecode = blob->GetBufferPointer();
        libDesc->DXILLibrary.BytecodeLength = blob->GetBufferSize();

        auto* exports = new D3D12_EXPORT_DESC[1];
        exports[0].Name = exportName;
        exports[0].ExportToRename = nullptr;
        exports[0].Flags = D3D12_EXPORT_FLAG_NONE;
        libDesc->NumExports = 1;
        libDesc->pExports = exports;

        D3D12_STATE_SUBOBJECT sub = {};
        sub.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        sub.pDesc = libDesc;
        subobjects.push_back(sub);
    };

    addDXILLib(raygenBlob.Get(), L"RayGen");
    addDXILLib(missBlob.Get(), L"Miss");
    addDXILLib(sphereIsectBlob.Get(), L"SphereIntersection");
    addDXILLib(groundIsectBlob.Get(), L"GroundIntersection");
    addDXILLib(sphereHitBlob.Get(), L"SphereClosestHit");
    addDXILLib(groundHitBlob.Get(), L"GroundClosestHit");

    // Hit group: Ground
    auto* groundHitGroup = new D3D12_HIT_GROUP_DESC();
    groundHitGroup->HitGroupExport = L"GroundHitGroup";
    groundHitGroup->Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
    groundHitGroup->IntersectionShaderImport = L"GroundIntersection";
    groundHitGroup->ClosestHitShaderImport = L"GroundClosestHit";
    groundHitGroup->AnyHitShaderImport = nullptr;

    D3D12_STATE_SUBOBJECT groundHitGroupSub = {};
    groundHitGroupSub.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    groundHitGroupSub.pDesc = groundHitGroup;
    subobjects.push_back(groundHitGroupSub);

    // Hit group: Sphere
    auto* sphereHitGroup = new D3D12_HIT_GROUP_DESC();
    sphereHitGroup->HitGroupExport = L"SphereHitGroup";
    sphereHitGroup->Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
    sphereHitGroup->IntersectionShaderImport = L"SphereIntersection";
    sphereHitGroup->ClosestHitShaderImport = L"SphereClosestHit";
    sphereHitGroup->AnyHitShaderImport = nullptr;

    D3D12_STATE_SUBOBJECT sphereHitGroupSub = {};
    sphereHitGroupSub.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    sphereHitGroupSub.pDesc = sphereHitGroup;
    subobjects.push_back(sphereHitGroupSub);

    // Shader config
    auto* shaderConfig = new D3D12_RAYTRACING_SHADER_CONFIG();
    shaderConfig->MaxPayloadSizeInBytes = 64; // RayPayload
    shaderConfig->MaxAttributeSizeInBytes = 16; // float3 normal + uint matIndex

    D3D12_STATE_SUBOBJECT shaderConfigSub = {};
    shaderConfigSub.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    shaderConfigSub.pDesc = shaderConfig;
    subobjects.push_back(shaderConfigSub);

    // Pipeline config
    auto* pipelineConfig = new D3D12_RAYTRACING_PIPELINE_CONFIG();
    pipelineConfig->MaxTraceRecursionDepth = 1;

    D3D12_STATE_SUBOBJECT pipelineConfigSub = {};
    pipelineConfigSub.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    pipelineConfigSub.pDesc = pipelineConfig;
    subobjects.push_back(pipelineConfigSub);

    // Global root signature
    auto* globalRootSigDesc = new D3D12_GLOBAL_ROOT_SIGNATURE();
    globalRootSigDesc->pGlobalRootSignature = m_globalRootSig.Get();

    D3D12_STATE_SUBOBJECT globalRootSigSub = {};
    globalRootSigSub.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    globalRootSigSub.pDesc = globalRootSigDesc;
    subobjects.push_back(globalRootSigSub);

    // Create state object
    D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
    stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    stateObjectDesc.NumSubobjects = (uint32_t)subobjects.size();
    stateObjectDesc.pSubobjects = subobjects.data();

    ThrowIfFailed(device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&m_stateObject)),
        "CreateStateObject");

    // Cleanup
    for (auto& sub : subobjects) {
        if (sub.Type == D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY) {
            auto* lib = (D3D12_DXIL_LIBRARY_DESC*)sub.pDesc;
            delete[] lib->pExports;
            delete lib;
        } else if (sub.Type == D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP) {
            delete (D3D12_HIT_GROUP_DESC*)sub.pDesc;
        } else if (sub.Type == D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG) {
            delete (D3D12_RAYTRACING_SHADER_CONFIG*)sub.pDesc;
        } else if (sub.Type == D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG) {
            delete (D3D12_RAYTRACING_PIPELINE_CONFIG*)sub.pDesc;
        } else if (sub.Type == D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE) {
            delete (D3D12_GLOBAL_ROOT_SIGNATURE*)sub.pDesc;
        }
    }
}

void DXRPipeline::buildShaderTable(ID3D12Device5* device) {
    ComPtr<ID3D12StateObjectProperties> stateObjectProps;
    ThrowIfFailed(m_stateObject->QueryInterface(IID_PPV_ARGS(&stateObjectProps)),
        "QueryInterface StateObjectProperties");

    uint32_t shaderIDSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    m_shaderTableRecordSize = GPUResources::Align(shaderIDSize,
        (uint32_t)D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

    uint64_t tableSize = m_shaderTableRecordSize * NUM_SHADER_TABLE_RECORDS;
    tableSize = GPUResources::Align(tableSize, (uint64_t)D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

    m_shaderTable = GPUResources::CreateUploadBuffer(device, tableSize);

    uint8_t* mapped = nullptr;
    ThrowIfFailed(m_shaderTable->Map(0, nullptr, (void**)&mapped), "Map shader table");
    memset(mapped, 0, (size_t)tableSize);

    // Record 0: RayGen
    memcpy(mapped, stateObjectProps->GetShaderIdentifier(L"RayGen"), shaderIDSize);
    mapped += m_shaderTableRecordSize;

    // Record 1: Miss
    memcpy(mapped, stateObjectProps->GetShaderIdentifier(L"Miss"), shaderIDSize);
    mapped += m_shaderTableRecordSize;

    // Record 2: GroundHitGroup
    memcpy(mapped, stateObjectProps->GetShaderIdentifier(L"GroundHitGroup"), shaderIDSize);
    mapped += m_shaderTableRecordSize;

    // Record 3: SphereHitGroup
    memcpy(mapped, stateObjectProps->GetShaderIdentifier(L"SphereHitGroup"), shaderIDSize);

    m_shaderTable->Unmap(0, nullptr);
}

void DXRPipeline::init(ID3D12Device5* device, const std::wstring& shaderDir) {
    createGlobalRootSignature(device);
    createStateObject(device, shaderDir);
    buildShaderTable(device);
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE DXRPipeline::getRayGenRecord() const {
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE range = {};
    range.StartAddress = m_shaderTable->GetGPUVirtualAddress();
    range.SizeInBytes = m_shaderTableRecordSize;
    return range;
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE DXRPipeline::getMissTable() const {
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE table = {};
    table.StartAddress = m_shaderTable->GetGPUVirtualAddress() + m_shaderTableRecordSize;
    table.SizeInBytes = m_shaderTableRecordSize;
    table.StrideInBytes = m_shaderTableRecordSize;
    return table;
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE DXRPipeline::getHitGroupTable() const {
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE table = {};
    table.StartAddress = m_shaderTable->GetGPUVirtualAddress() + 2 * m_shaderTableRecordSize;
    table.SizeInBytes = 2 * m_shaderTableRecordSize; // 2 hit groups
    table.StrideInBytes = m_shaderTableRecordSize;
    return table;
}
