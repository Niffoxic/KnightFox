// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */
#include "pch.h"
#include "engine/render_manager/scene/model_scene.h"

#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/buffer/vertex_buffer.h"
#include "engine/render_manager/api/buffer/index_buffer.h"
#include "engine/render_manager/api/buffer/constant_buffer.h"
#include "engine/render_manager/api/buffer/staging_buffer.h"
#include "engine/render_manager/api/commands/graphics_list.h"
#include "engine/render_manager/api/root_signature.h"
#include "engine/render_manager/api/pso.h"
#include "engine/render_manager/api/queue/graphics_queue.h"
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"
#include "engine/render_manager/api/heap/heap_sampler.h"
#include "engine/render_manager/api/sampler.h"
#include "engine/render_manager/api/texture/texture_srv.h"

#include "engine/render_manager/assets_library/shader_library.h"
#include "engine/render_manager/assets_library/texture_library.h"

#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"

#include "imgui/imgui.h"
#include "engine/editor/widgets/assets_panel.h"
#include "engine/system/exception/base_exception.h"

#include "engine/render_manager/assets_library/model/geometry.h"
#include "engine/render_manager/assets_library/model/gpu_mesh.h"
#include "engine/render_manager/assets_library/model/mesh_cache.h"

#include <d3d12.h>
#include <vector>
#include <cstring>
#include <unordered_map>
#include <map>
#include <array>

#pragma region Impl_Definition


class kfe::KFEMeshSceneObject::Impl
{
public:
    Impl(KFEMeshSceneObject* obj)
        : m_pObject(obj)
    {}
    ~Impl() = default;

    void Update(const KFE_UPDATE_OBJECT_DESC& desc);
    bool Build(_In_ const KFE_BUILD_OBJECT_DESC& desc);

    bool Destroy();

    void Render(_In_ const KFE_RENDER_OBJECT_DESC& desc);

    // Shader path setters
    void SetVertexShaderPath(const std::string& path) noexcept;
    void SetPixelShaderPath(const std::string& path) noexcept;
    void SetGeometryShaderPath(const std::string& path) noexcept;
    void SetHullShaderPath(const std::string& path) noexcept;
    void SetDomainShaderPath(const std::string& path) noexcept;
    void SetComputeShaderPath(const std::string& path) noexcept;

    // Shader path getters
    const std::string& GetVertexShaderPath() const noexcept { return m_vertexShaderPath; }
    const std::string& GetPixelShaderPath() const noexcept { return m_pixelShaderPath; }
    const std::string& GetGeometryShaderPath() const noexcept { return m_geometryShaderPath; }
    const std::string& GetHullShaderPath() const noexcept { return m_hullShaderPath; }
    const std::string& GetDomainShaderPath() const noexcept { return m_domainShaderPath; }
    const std::string& GetComputeShaderPath() const noexcept { return m_computeShaderPath; }

    JsonLoader GetJsonData() const                      noexcept;
    void       LoadFromJson(const JsonLoader& loader)   noexcept;

    void ImguiView(float deltaTime);

private:
    bool BuildGeometry(_In_ const KFE_BUILD_OBJECT_DESC& desc);
    bool BuildConstantBuffer(_In_ const KFE_BUILD_OBJECT_DESC& desc);
    bool BuildRootSignature(_In_ const KFE_BUILD_OBJECT_DESC& desc);
    bool BuildPipeline(KFEDevice* device);
    bool BuildSampler(_In_ const KFE_BUILD_OBJECT_DESC& desc);

    void UpdateConstantBuffer(const KFE_UPDATE_OBJECT_DESC& desc);

public:
    ECullMode m_cullMode{ ECullMode::None };
    EDrawMode m_drawMode{ EDrawMode::Triangle };
    bool      m_bPipelineDirty{ false };

private:
    KFEMeshSceneObject* m_pObject{ nullptr };
    float               m_nTimeLived{ 0.0f };
    bool m_bBuild{ false };
    // Shaders
    std::string m_vertexShaderPath  { "shaders/mesh/vertex.hlsl" };
    std::string m_pixelShaderPath   { "shaders/mesh/pixel.hlsl" };
    std::string m_geometryShaderPath{};
    std::string m_hullShaderPath    {};
    std::string m_domainShaderPath  {};
    std::string m_computeShaderPath {};

    std::unique_ptr<KFERootSignature>  m_pRootSignature{ nullptr };

    // Constant buffer
    std::unique_ptr<KFEBuffer>         m_pCBBuffer{ nullptr };
    std::unique_ptr<KFEConstantBuffer> m_pCBV     { nullptr };

    // Pipeline
    std::unique_ptr<KFEPipelineState> m_pPipeline{ nullptr };
    KFEDevice*                        m_pDevice  { nullptr };

    //~ sampling
    KFEResourceHeap*            m_pResourceHeap { nullptr };
    KFESamplerHeap*             m_pSamplerHeap  { nullptr };
    std::unique_ptr<KFESampler> m_pSampler      { nullptr };
    std::uint32_t               m_samplerIndex  { KFE_INVALID_INDEX };

    //~ Test
    KFEGpuMesh* m_pGpuMesh{ nullptr };
};

#pragma endregion

#pragma region MeshScene_Body

kfe::KFEMeshSceneObject::KFEMeshSceneObject()
    : m_impl(std::make_unique<kfe::KFEMeshSceneObject::Impl>(this))
{
    SetTypeName("KFEMeshSceneObject");
}

kfe::KFEMeshSceneObject::~KFEMeshSceneObject() = default;

kfe::KFEMeshSceneObject::KFEMeshSceneObject(KFEMeshSceneObject&&) = default;
kfe::KFEMeshSceneObject& kfe::KFEMeshSceneObject::operator=(KFEMeshSceneObject&&) = default;

std::string kfe::KFEMeshSceneObject::GetName() const noexcept
{
    return "KFEMeshSceneObject";
}

std::string kfe::KFEMeshSceneObject::GetDescription() const noexcept
{
    return "A Mesh Object that can be used for rendering debug cube for colliders";
}

void kfe::KFEMeshSceneObject::Update(const KFE_UPDATE_OBJECT_DESC& desc)
{
    m_impl->Update(desc);
}

_Use_decl_annotations_
bool kfe::KFEMeshSceneObject::Build(const KFE_BUILD_OBJECT_DESC& desc)
{
    if (m_impl->Build(desc))
    {
        m_bInitialized = true;
        return true;
    }
    return false;
}

bool kfe::KFEMeshSceneObject::Destroy()
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
void kfe::KFEMeshSceneObject::Render(const KFE_RENDER_OBJECT_DESC& desc)
{
    m_impl->Render(desc);
}

void kfe::KFEMeshSceneObject::ImguiView(float deltaTime)
{
    if (ImGui::CollapsingHeader("Object", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const std::string typeName = GetTypeName();
        ImGui::Text("Type: %s", typeName.c_str());

        std::string objName = GetObjectName();

        char nameBuf[128];
        std::snprintf(nameBuf, sizeof(nameBuf), "%s", objName.c_str());

        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
        {
            SetObjectName(std::string{ nameBuf });
        }
    }

    m_impl->ImguiView(deltaTime);
    ImguiTransformView(deltaTime);
}

void kfe::KFEMeshSceneObject::SetCullMode(const ECullMode mode)
{
    if (!m_impl) return;
    m_impl->m_cullMode = mode;
    m_impl->m_bPipelineDirty = true;
}

void kfe::KFEMeshSceneObject::SetCullMode(const std::string& mode)
{
    if (!m_impl) return;
    m_impl->m_cullMode = FromStringToCull(mode);
    m_impl->m_bPipelineDirty = true;
}

void kfe::KFEMeshSceneObject::SetDrawMode(const EDrawMode mode)
{
    if (!m_impl) return;
    m_impl->m_drawMode = mode;
    m_impl->m_bPipelineDirty = true;
}

void kfe::KFEMeshSceneObject::SetDrawMode(const std::string& mode)
{
    if (!m_impl) return;
    m_impl->m_drawMode = FromStringToDraw(mode);
    m_impl->m_bPipelineDirty = true;
}

kfe::ECullMode kfe::KFEMeshSceneObject::GetCullMode() const
{
    return m_impl ? m_impl->m_cullMode : ECullMode::Back;
}

std::string kfe::KFEMeshSceneObject::GetCullModeString() const
{
    return m_impl ? ToString(m_impl->m_cullMode) : "Back";
}

kfe::EDrawMode kfe::KFEMeshSceneObject::GetDrawMode() const
{
    return m_impl ? m_impl->m_drawMode : EDrawMode::Triangle;
}

std::string kfe::KFEMeshSceneObject::GetDrawModeString() const
{
    return m_impl ? ToString(m_impl->m_drawMode) : "Triangle";
}

void kfe::KFEMeshSceneObject::SetVertexShader(const std::string& path)
{
    if (!m_impl) return;
    m_impl->SetVertexShaderPath(path);
}

std::string kfe::KFEMeshSceneObject::VertexShader() const
{
    return m_impl ? m_impl->GetVertexShaderPath() : std::string{};
}

void kfe::KFEMeshSceneObject::SetPixelShader(const std::string& path)
{
    if (!m_impl) return;
    m_impl->SetPixelShaderPath(path);
}

std::string kfe::KFEMeshSceneObject::PixelShader() const
{
    return m_impl ? m_impl->GetPixelShaderPath() : std::string{};
}

void kfe::KFEMeshSceneObject::SetGeometryShader(const std::string& path)
{
    if (!m_impl) return;
    m_impl->SetGeometryShaderPath(path);
}

std::string kfe::KFEMeshSceneObject::GeometryShader() const
{
    return m_impl ? m_impl->GetGeometryShaderPath() : std::string{};
}

void kfe::KFEMeshSceneObject::SetHullShader(const std::string& path)
{
    if (!m_impl) return;
    m_impl->SetHullShaderPath(path);
}

std::string kfe::KFEMeshSceneObject::HullShader() const
{
    return m_impl ? m_impl->GetHullShaderPath() : std::string{};
}

void kfe::KFEMeshSceneObject::SetDomainShader(const std::string& path)
{
    if (!m_impl) return;
    m_impl->SetDomainShaderPath(path);
}

std::string kfe::KFEMeshSceneObject::DomainShader() const
{
    return m_impl ? m_impl->GetDomainShaderPath() : std::string{};
}

void kfe::KFEMeshSceneObject::SetComputeShader(const std::string& path)
{
    if (!m_impl) return;
    m_impl->SetComputeShaderPath(path);
}

std::string kfe::KFEMeshSceneObject::ComputeShader() const
{
    return m_impl ? m_impl->GetComputeShaderPath() : std::string{};
}

JsonLoader kfe::KFEMeshSceneObject::GetJsonData() const
{
    JsonLoader root{};
    root["Transformation"] = GetTransformJsonData();
    root["Properties"] = m_impl->GetJsonData();
    return root;
}

void kfe::KFEMeshSceneObject::LoadFromJson(const JsonLoader& loader)
{
    if (loader.Contains("Transformation"))
    {
        LoadTransformFromJson(loader["Transformation"]);
    }
    if (loader.Contains("Properties"))
    {
        m_impl->LoadFromJson(loader["Properties"]);
    }
}

#pragma endregion

#pragma region Impl_body

void kfe::KFEMeshSceneObject::Impl::Update(const KFE_UPDATE_OBJECT_DESC& desc)
{
    m_nTimeLived += desc.deltaTime;
    UpdateConstantBuffer(desc);
}

_Use_decl_annotations_
bool kfe::KFEMeshSceneObject::Impl::Build(const KFE_BUILD_OBJECT_DESC& desc)
{
    m_pDevice = desc.Device;
    m_pResourceHeap = desc.ResourceHeap;
    m_pSamplerHeap = desc.SamplerHeap;

    if (!m_pDevice || !m_pResourceHeap || !m_pSamplerHeap)
    {
        LOG_ERROR("One or more required pointers are null.");
        return false;
    }

    if (!BuildSampler(desc))
        return false;

    if (!BuildGeometry(desc))
        return false;

    if (!BuildRootSignature(desc))
        return false;

    if (!BuildPipeline(desc.Device))
        return false;

    if (!BuildConstantBuffer(desc))
        return false;

    LOG_SUCCESS("Mesh Built!");
    m_bBuild = true;
    return true;
}

bool kfe::KFEMeshSceneObject::Impl::Destroy()
{
    //~ Destroy GPU resources
    if (m_pCBV)           m_pCBV->Destroy();
    if (m_pCBBuffer)      m_pCBBuffer->Destroy();

    if (m_pPipeline)      m_pPipeline->Destroy();
    if (m_pRootSignature) m_pRootSignature->Destroy();

    if (m_pSampler)       m_pSampler->Destroy();

    //~ Reset smart pointers
    m_pCBV.reset();
    m_pCBBuffer.reset();
    m_pPipeline.reset();
    m_pRootSignature.reset();
    m_pSampler.reset();

    //~ Free sampler slot
    if (m_pSamplerHeap && m_samplerIndex != KFE_INVALID_INDEX)
    {
        m_pSamplerHeap->Free(m_samplerIndex);
        m_samplerIndex = KFE_INVALID_INDEX;
    }

    //~ Null references
    m_pResourceHeap = nullptr;
    m_pSamplerHeap = nullptr;
    m_pDevice = nullptr;

    return true;
}

void kfe::KFEMeshSceneObject::Impl::Render(_In_ const KFE_RENDER_OBJECT_DESC& desc)
{
    if (!m_bBuild) return;
    if (m_bPipelineDirty)
    {
        if (!m_pDevice)
        {
            LOG_ERROR("KEFCubeSceneObject::Impl::Render: Cannot rebuild pipeline, device is null.");
            return;
        }

        if (!BuildPipeline(m_pDevice))
        {
            LOG_ERROR("KEFCubeSceneObject::Impl::Render: Failed to rebuild pipeline.");
            return;
        }
    }

    auto* cmdListObj = desc.CommandList;
    if (!cmdListObj || !cmdListObj->GetNative())
        return;

    ID3D12GraphicsCommandList* cmdList = cmdListObj->GetNative();

    cmdList->SetPipelineState(m_pPipeline->GetNative());
    cmdList->SetGraphicsRootSignature(m_pPipeline->GetRootSignature());

    //~ Bind descriptor heaps
    if (m_pResourceHeap)
    {
        if (m_pSamplerHeap)
        {
            ID3D12DescriptorHeap* heaps[2] =
            {
                m_pResourceHeap->GetNative(),
                m_pSamplerHeap->GetNative()
            };
            cmdList->SetDescriptorHeaps(2u, heaps);
        }
        else
        {
            ID3D12DescriptorHeap* heaps[1] =
            {
                m_pResourceHeap->GetNative()
            };
            cmdList->SetDescriptorHeaps(1u, heaps);
        }

    }

    //~ Bind per object constant buffer at b0
    {
        const D3D12_GPU_VIRTUAL_ADDRESS addr =
            static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(m_pCBV->GetGPUVirtualAddress());

        cmdList->SetGraphicsRootConstantBufferView(0u, addr);
    }

    const KFEVertexBuffer* vbView = m_pGpuMesh->GetVertexBufferView();
    const KFEIndexBuffer* ibView = m_pGpuMesh->GetIndexBufferView();
    D3D12_VERTEX_BUFFER_VIEW vb_view = vbView->GetView();
    D3D12_INDEX_BUFFER_VIEW ib_view = ibView->GetView();

    cmdList->IASetVertexBuffers(0u, 1u, &vb_view);
    cmdList->IASetIndexBuffer(&ib_view);

    //~ Primitive topology
    switch (m_drawMode)
    {
    case EDrawMode::Triangle:
    case EDrawMode::WireFrame:
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        break;
    case EDrawMode::Point:
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
        break;
    }

    //~ Draw call
    cmdList->DrawIndexedInstanced(
        ibView->GetIndexCount(),
        1u,
        0u,
        0u,
        0u
    );
}

_Use_decl_annotations_
bool kfe::KFEMeshSceneObject::Impl::BuildGeometry(const KFE_BUILD_OBJECT_DESC& desc)
{
    const std::string meshPath = "assets/3d/dark_knight_obj/Male/SKM_DKM_Full.obj";

    if (!KFEMeshCache::Instance().GetOrCreate(
        meshPath,
        desc.Device,
        desc.CommandList, m_pGpuMesh))
    {
        LOG_ERROR("Failed to get GPU mesh for '{}'", meshPath);
        return false;
    }
    LOG_SUCCESS("Mesh Loaded!");
    return true;
}

_Use_decl_annotations_
bool kfe::KFEMeshSceneObject::Impl::BuildConstantBuffer(const KFE_BUILD_OBJECT_DESC& desc)
{
    m_pCBBuffer = std::make_unique<KFEBuffer>();

    std::uint32_t bytes = static_cast<std::uint32_t>(sizeof(KFE_COMMON_VERTEX_AND_PIXEL_CB_DESC));
    bytes = kfe_helpers::AlignTo256(bytes);

    KFE_CREATE_BUFFER_DESC buffer{};
    buffer.Device           = desc.Device;
    buffer.HeapType         = D3D12_HEAP_TYPE_UPLOAD;
    buffer.InitialState     = D3D12_RESOURCE_STATE_GENERIC_READ;
    buffer.ResourceFlags    = D3D12_RESOURCE_FLAG_NONE;
    buffer.SizeInBytes      = bytes;

    if (!m_pCBBuffer->Initialize(buffer))
    {
        LOG_ERROR("Failed to build constant buffer!");
        return false;
    }

    m_pCBV = std::make_unique<KFEConstantBuffer>();

    KFE_CONSTANT_BUFFER_CREATE_DESC view{};
    view.Device         = desc.Device;
    view.OffsetInBytes  = 0u;
    view.ResourceBuffer = m_pCBBuffer.get();
    view.ResourceHeap   = desc.ResourceHeap;
    view.SizeInBytes    = bytes;

    if (!m_pCBV->Initialize(view))
    {
        LOG_ERROR("Failed to build constant buffer View!");
        return false;
    }

    LOG_SUCCESS("Cube Constant Buffer Created!");
    return true;
}

_Use_decl_annotations_
bool kfe::KFEMeshSceneObject::Impl::BuildRootSignature(const KFE_BUILD_OBJECT_DESC& desc)
{
    m_pRootSignature = std::make_unique<KFERootSignature>();

    D3D12_ROOT_PARAMETER param{};
    param.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
    param.Descriptor.ShaderRegister = 0u;
    param.Descriptor.RegisterSpace  = 0u;

    KFE_RG_CREATE_DESC root{};
    root.Device             = desc.Device;
    root.Flags              = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    root.NumRootParameters  = 1u;
    root.RootParameters     = &param;
    root.NumStaticSamplers  = 0u;
    root.StaticSamplers     = nullptr;

    if (!m_pRootSignature->Initialize(root))
    {
        LOG_ERROR("Failed to Create Root Signature!");
        return false;
    }

    m_pRootSignature->SetDebugName(L"Mesh CBV-Only Root Signature");
    LOG_SUCCESS("CBV-only Root Signature Created!");
    return true;
}

bool kfe::KFEMeshSceneObject::Impl::BuildPipeline(KFEDevice* device)
{
    if (!device)
    {
        LOG_ERROR("KEFCubeSceneObject::Impl::BuildPipeline: Device is null.");
        return false;
    }

    if (!kfe_helpers::IsFile(m_vertexShaderPath))
    {
        LOG_ERROR("Vertex Shader Path: '{}', Does not exist!", m_vertexShaderPath);
        return false;
    }

    if (!kfe_helpers::IsFile(m_pixelShaderPath))
    {
        LOG_ERROR("Pixel Shader Path: '{}', Does Not Exist!", m_pixelShaderPath);
        return false;
    }

    if (!m_geometryShaderPath.empty() && !kfe_helpers::IsFile(m_geometryShaderPath))
    {
        LOG_ERROR("Geometry Shader Path: '{}', Does Not Exist!", m_geometryShaderPath);
        m_geometryShaderPath.clear();
    }

    if (!m_hullShaderPath.empty() && !kfe_helpers::IsFile(m_hullShaderPath))
    {
        LOG_ERROR("Hull Shader Path: '{}', Does Not Exist!", m_hullShaderPath);
        m_hullShaderPath.clear();
    }

    if (!m_domainShaderPath.empty() && !kfe_helpers::IsFile(m_domainShaderPath))
    {
        LOG_ERROR("Domain Shader Path: '{}', Does Not Exist!", m_domainShaderPath);
        m_domainShaderPath.clear();
    }

    ID3DBlob* vertexBlob = shaders::GetOrCompile(
        m_vertexShaderPath, "main", "vs_5_0");
    ID3DBlob* pixelBlob = shaders::GetOrCompile(
        m_pixelShaderPath, "main", "ps_5_0");

    ID3DBlob* geometryBlob = nullptr;
    ID3DBlob* hullBlob = nullptr;
    ID3DBlob* domainBlob = nullptr;

    if (!vertexBlob)
    {
        LOG_ERROR("Failed to load Vertex Shader: {}", m_vertexShaderPath);
        return false;
    }

    if (!pixelBlob)
    {
        LOG_ERROR("Failed to load Pixel Shader: {}", m_pixelShaderPath);
        return false;
    }

    if (!m_geometryShaderPath.empty())
    {
        geometryBlob = shaders::GetOrCompile(
            m_geometryShaderPath, "main", "gs_5_0");
        if (!geometryBlob)
        {
            LOG_ERROR("Failed to load Geometry Shader: {}", m_geometryShaderPath);
            m_geometryShaderPath.clear();
        }
    }

    if (!m_hullShaderPath.empty())
    {
        hullBlob = shaders::GetOrCompile(
            m_hullShaderPath, "main", "hs_5_0");
        if (!hullBlob)
        {
            LOG_ERROR("Failed to load Hull Shader: {}", m_hullShaderPath);
            m_hullShaderPath.clear();
        }
    }

    if (!m_domainShaderPath.empty())
    {
        domainBlob = shaders::GetOrCompile(
            m_domainShaderPath, "main", "ds_5_0");
        if (!domainBlob)
        {
            LOG_ERROR("Failed to load Domain Shader: {}", m_domainShaderPath);
            m_domainShaderPath.clear();
        }
    }

    if (m_pPipeline)
    {
        m_pPipeline->Destroy();
    }
    else
    {
        m_pPipeline = std::make_unique<KFEPipelineState>();
    }

    auto layout = KFEMeshGeometry::GetInputLayout();
    m_pPipeline->SetInputLayout(layout.data(), layout.size());

    D3D12_SHADER_BYTECODE vertexCode{};
    vertexCode.BytecodeLength = vertexBlob->GetBufferSize();
    vertexCode.pShaderBytecode = vertexBlob->GetBufferPointer();
    m_pPipeline->SetVS(vertexCode);

    D3D12_SHADER_BYTECODE pixelCode{};
    pixelCode.BytecodeLength = pixelBlob->GetBufferSize();
    pixelCode.pShaderBytecode = pixelBlob->GetBufferPointer();
    m_pPipeline->SetPS(pixelCode);

    if (geometryBlob)
    {
        D3D12_SHADER_BYTECODE code{};
        code.BytecodeLength = geometryBlob->GetBufferSize();
        code.pShaderBytecode = geometryBlob->GetBufferPointer();
        m_pPipeline->SetGS(code);
    }

    if (hullBlob)
    {
        D3D12_SHADER_BYTECODE code{};
        code.BytecodeLength = hullBlob->GetBufferSize();
        code.pShaderBytecode = hullBlob->GetBufferPointer();
        m_pPipeline->SetHS(code);
    }

    if (domainBlob)
    {
        D3D12_SHADER_BYTECODE code{};
        code.BytecodeLength = domainBlob->GetBufferSize();
        code.pShaderBytecode = domainBlob->GetBufferPointer();
        m_pPipeline->SetDS(code);
    }

    auto* rs = static_cast<ID3D12RootSignature*>(m_pRootSignature->GetNative());
    m_pPipeline->SetRootSignature(rs);

    D3D12_RASTERIZER_DESC raster{};
    raster.FillMode =
        (m_drawMode == EDrawMode::WireFrame) ? D3D12_FILL_MODE_WIREFRAME
        : D3D12_FILL_MODE_SOLID;

    switch (m_cullMode)
    {
    case ECullMode::Front: raster.CullMode = D3D12_CULL_MODE_FRONT; break;
    case ECullMode::Back:  raster.CullMode = D3D12_CULL_MODE_BACK;  break;
    case ECullMode::None:  raster.CullMode = D3D12_CULL_MODE_NONE;  break;
    }

    raster.FrontCounterClockwise    = FALSE;
    raster.DepthBias                = D3D12_DEFAULT_DEPTH_BIAS;
    raster.DepthBiasClamp           = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    raster.SlopeScaledDepthBias     = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    raster.DepthClipEnable          = TRUE;
    raster.MultisampleEnable        = FALSE;
    raster.AntialiasedLineEnable    = FALSE;
    raster.ForcedSampleCount        = 0u;
    raster.ConservativeRaster       = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    m_pPipeline->SetRasterizer(raster);

    switch (m_drawMode)
    {
    case EDrawMode::Triangle:
    case EDrawMode::WireFrame:
        m_pPipeline->SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        break;

    case EDrawMode::Point:
        m_pPipeline->SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
        break;
    }

    if (!m_pPipeline->Build(device))
    {
        LOG_ERROR("Failed to build Cube Scene Pipeline!");
        return false;
    }

    m_bPipelineDirty = false;

    LOG_SUCCESS("Cube Pipeline Created!");
    return true;
}

_Use_decl_annotations_
bool kfe::KFEMeshSceneObject::Impl::BuildSampler(const KFE_BUILD_OBJECT_DESC& desc)
{
    if (!desc.Device)
    {
        LOG_ERROR("KEFCubeSceneObject::Impl::BuildSampler: Device is null. Skipping sampler creation.");
        return true;
    }

    if (!desc.SamplerHeap)
    {
        LOG_ERROR("KEFCubeSceneObject::Impl::BuildSampler: Sampler is null. Skipping sampler creation.");
        return true;
    }

    m_pSampler = std::make_unique<KFESampler>();

    KFE_SAMPLER_CREATE_DESC sdesc{};
    sdesc.Device = desc.Device;
    sdesc.Heap = m_pSamplerHeap;

    sdesc.Filter         = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sdesc.AddressU       = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sdesc.AddressV       = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sdesc.AddressW       = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sdesc.MipLODBias     = 0.0f;
    sdesc.MaxAnisotropy  = 1u;
    sdesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    sdesc.BorderColor[0] = 0.0f;
    sdesc.BorderColor[1] = 0.0f;
    sdesc.BorderColor[2] = 0.0f;
    sdesc.BorderColor[3] = 0.0f;
    sdesc.MinLOD = 0.0f;
    sdesc.MaxLOD = D3D12_FLOAT32_MAX;

    sdesc.DescriptorIndex = KFE_INVALID_INDEX;

    if (!m_pSampler->Initialize(sdesc))
    {
        LOG_ERROR("KEFCubeSceneObject::Impl::BuildSampler: Failed to initialize sampler.");
        m_pSampler.reset();
        m_samplerIndex = KFE_INVALID_INDEX;
        return false;
    }

    m_samplerIndex = m_pSampler->GetDescriptorIndex();

    if (m_samplerIndex == KFE_INVALID_INDEX) THROW_MSG("SAMPLERRR");

    LOG_SUCCESS("Cube Sampler Created. Index = {}", m_samplerIndex);

    return true;
}

void kfe::KFEMeshSceneObject::Impl::UpdateConstantBuffer(const KFE_UPDATE_OBJECT_DESC& desc)
{
    if (!m_bBuild) return;
    auto* cv = static_cast<KFE_COMMON_VERTEX_AND_PIXEL_CB_DESC*>(m_pCBV->GetMappedData());
    if (!cv) return;

    cv->WorldMatrix = m_pObject->GetWorldMatrix();
    cv->ViewMatrix = desc.ViewMatrix;
    cv->ProjectionMatrix = desc.PerpectiveMatrix;
    cv->OrthogonalMatrix = desc.OrthographicMatrix;
    cv->Resolution = desc.Resolution;
    cv->MousePosition = desc.MousePosition;
    cv->ObjectPosition = m_pObject->GetPosition();
    cv->_PaddingObjectPos = 0.f;
    cv->CameraPosition = desc.CameraPosition;
    cv->_PaddingCameraPos = 0.f;
    cv->PlayerPosition = desc.PlayerPosition;
    cv->_PaddingPlayerPos = 0.f;
    cv->Time = m_nTimeLived;
    cv->FrameIndex = 0u;
    cv->DeltaTime = desc.deltaTime;
    cv->ZNear = desc.ZNear;
    cv->ZFar = desc.ZFar;

    cv->_PaddingFinal[0] = 0.f;
    cv->_PaddingFinal[1] = 0.f;
    cv->_PaddingFinal[2] = 0.f;
}

void kfe::KFEMeshSceneObject::Impl::SetVertexShaderPath(const std::string& path) noexcept
{
    if (m_vertexShaderPath == path)
        return;

    m_vertexShaderPath = path;
    m_bPipelineDirty = true;
}

void kfe::KFEMeshSceneObject::Impl::SetPixelShaderPath(const std::string& path) noexcept
{
    if (m_pixelShaderPath == path)
        return;

    m_pixelShaderPath = path;
    m_bPipelineDirty = true;
}

void kfe::KFEMeshSceneObject::Impl::SetGeometryShaderPath(const std::string& path) noexcept
{
    if (m_geometryShaderPath == path)
        return;

    m_geometryShaderPath = path;
    m_bPipelineDirty = true;
}

void kfe::KFEMeshSceneObject::Impl::SetHullShaderPath(const std::string& path) noexcept
{
    if (m_hullShaderPath == path)
        return;

    m_hullShaderPath = path;
    m_bPipelineDirty = true;
}

void kfe::KFEMeshSceneObject::Impl::SetDomainShaderPath(const std::string& path) noexcept
{
    if (m_domainShaderPath == path)
        return;

    m_domainShaderPath = path;
    m_bPipelineDirty = true;
}

void kfe::KFEMeshSceneObject::Impl::SetComputeShaderPath(const std::string& path) noexcept
{
    if (m_computeShaderPath == path)
        return;

    m_computeShaderPath = path;
    m_bPipelineDirty = true;
}

JsonLoader kfe::KFEMeshSceneObject::Impl::GetJsonData() const noexcept
{
    JsonLoader root{};

    // Basic properties
    root["CullMode"] = ToString(m_cullMode);
    root["DrawMode"] = ToString(m_drawMode);

    root["VertexShader"] = m_vertexShaderPath;
    root["PixelShader"] = m_pixelShaderPath;
    root["GeometryShader"] = m_geometryShaderPath;
    root["HullShader"] = m_hullShaderPath;
    root["DomainShader"] = m_domainShaderPath;
    root["ComputeShader"] = m_computeShaderPath;

    return root;
}

void kfe::KFEMeshSceneObject::Impl::LoadFromJson(const JsonLoader& loader) noexcept
{
    if (loader.Contains("CullMode"))
    {
        m_cullMode = FromStringToCull(loader["CullMode"].GetValue());
        m_bPipelineDirty = true;
    }

    if (loader.Contains("DrawMode"))
    {
        m_drawMode = FromStringToDraw(loader["DrawMode"].GetValue());
        m_bPipelineDirty = true;
    }

    if (loader.Contains("VertexShader"))
    {
        m_vertexShaderPath = loader["VertexShader"].GetValue();
        m_bPipelineDirty = true;
    }

    if (loader.Contains("PixelShader"))
    {
        m_pixelShaderPath = loader["PixelShader"].GetValue();
        m_bPipelineDirty = true;
    }

    if (loader.Contains("GeometryShader"))
    {
        m_geometryShaderPath = loader["GeometryShader"].GetValue();
        m_bPipelineDirty = true;
    }

    if (loader.Contains("HullShader"))
    {
        m_hullShaderPath = loader["HullShader"].GetValue();
        m_bPipelineDirty = true;
    }

    if (loader.Contains("DomainShader"))
    {
        m_domainShaderPath = loader["DomainShader"].GetValue();
        m_bPipelineDirty = true;
    }

    if (loader.Contains("ComputeShader"))
    {
        m_computeShaderPath = loader["ComputeShader"].GetValue();
        m_bPipelineDirty = true;
    }
}

void kfe::KFEMeshSceneObject::Impl::ImguiView(float)
{
    if (ImGui::CollapsingHeader("Cube Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        {
            const char* drawModeItems[] = { "Triangle", "Point", "WireFrame" };
            int currentDrawIndex = 0;
            switch (m_drawMode)
            {
            case EDrawMode::Triangle:  currentDrawIndex = 0; break;
            case EDrawMode::Point:     currentDrawIndex = 1; break;
            case EDrawMode::WireFrame: currentDrawIndex = 2; break;
            }

            if (ImGui::Combo("Draw Mode", &currentDrawIndex, drawModeItems, IM_ARRAYSIZE(drawModeItems)))
            {
                switch (currentDrawIndex)
                {
                case 0: m_drawMode = EDrawMode::Triangle;  break;
                case 1: m_drawMode = EDrawMode::Point;     break;
                case 2: m_drawMode = EDrawMode::WireFrame; break;
                default: m_drawMode = EDrawMode::Triangle; break;
                }
                m_bPipelineDirty = true;
            }
        }

        {
            const char* cullModeItems[] = { "Front", "Back", "None" };
            int currentCullIndex = 2;
            switch (m_cullMode)
            {
            case ECullMode::Front: currentCullIndex = 0; break;
            case ECullMode::Back:  currentCullIndex = 1; break;
            case ECullMode::None:  currentCullIndex = 2; break;
            }

            if (ImGui::Combo("Cull Mode", &currentCullIndex, cullModeItems, IM_ARRAYSIZE(cullModeItems)))
            {
                switch (currentCullIndex)
                {
                case 0: m_cullMode = ECullMode::Front; break;
                case 1: m_cullMode = ECullMode::Back;  break;
                case 2: m_cullMode = ECullMode::None;  break;
                default: m_cullMode = ECullMode::None; break;
                }
                m_bPipelineDirty = true;
            }
        }

        ImGui::Separator();

        if (ImGui::TreeNode("Shaders"))
        {
            auto EditPath = [](const char* label, std::string& path, auto setter)
                {
                    char buf[260];
                    std::memset(buf, 0, sizeof(buf));

                    const size_t maxCopy = sizeof(buf) - 1;
                    const size_t count = std::min(path.size(), maxCopy);

                    path.copy(buf, count);
                    buf[count] = '\0';

                    if (ImGui::InputText(label, buf, sizeof(buf)))
                    {
                        setter(std::string{ buf });
                    }

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload =
                            ImGui::AcceptDragDropPayload(kfe::KFEAssetPanel::kPayloadType))
                        {
                            kfe::KFEAssetPanel::PayloadHeader hdr{};
                            std::string pathUtf8;

                            if (kfe::KFEAssetPanel::ParsePayload(payload, hdr, pathUtf8))
                            {
                                setter(pathUtf8);
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                };

            EditPath("Vertex Shader", m_vertexShaderPath, [this](const std::string& p) { SetVertexShaderPath(p);   m_bPipelineDirty = true; });
            EditPath("Pixel Shader", m_pixelShaderPath, [this](const std::string& p) { SetPixelShaderPath(p);    m_bPipelineDirty = true; });
            EditPath("Geometry Shader", m_geometryShaderPath, [this](const std::string& p) { SetGeometryShaderPath(p); m_bPipelineDirty = true; });
            EditPath("Hull Shader", m_hullShaderPath, [this](const std::string& p) { SetHullShaderPath(p);     m_bPipelineDirty = true; });
            EditPath("Domain Shader", m_domainShaderPath, [this](const std::string& p) { SetDomainShaderPath(p);   m_bPipelineDirty = true; });
            EditPath("Compute Shader", m_computeShaderPath, [this](const std::string& p) { SetComputeShaderPath(p);  m_bPipelineDirty = true; });

            ImGui::TreePop();
        }
    }
}

#pragma endregion
