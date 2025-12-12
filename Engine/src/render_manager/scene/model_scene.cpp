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
#include "engine/render_manager/assets_library/model/model.h"
#include <d3d12.h>
#include <vector>
#include <cstring>
#include <unordered_map>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <array>


static void UpdateMetaCB(_Inout_ kfe::KFEModelSubmesh& sm) noexcept
{
    if (!sm.MetaCBView)
        return;

    auto* dst = static_cast<kfe::ModelTextureMetaInformation*>(sm.MetaCBView->GetMappedData());
    if (!dst)
        return;

    *dst = sm.m_textureMetaInformation;
}

#pragma region Impl_Definition

class kfe::KFEMeshSceneObject::Impl
{
public:
    explicit Impl(KFEMeshSceneObject* obj)
        : m_pObject(obj)
    {}

    ~Impl() = default;

    void Update (const KFE_UPDATE_OBJECT_DESC& desc);
    bool Build  (_In_ const KFE_BUILD_OBJECT_DESC& desc);
    bool Destroy();
    void Render (_In_ const KFE_RENDER_OBJECT_DESC& desc);

    //~ Model path
    void SetModelPath(const std::string& path) noexcept;

    //~ Shader paths
    void SetVertexShaderPath    (const std::string& path) noexcept;
    void SetPixelShaderPath     (const std::string& path) noexcept;
    void SetGeometryShaderPath  (const std::string& path) noexcept;
    void SetHullShaderPath      (const std::string& path) noexcept;
    void SetDomainShaderPath    (const std::string& path) noexcept;
    void SetComputeShaderPath   (const std::string& path) noexcept;

    const std::string& GetVertexShaderPath  () const noexcept { return m_vertexShaderPath; }
    const std::string& GetPixelShaderPath   () const noexcept { return m_pixelShaderPath; }
    const std::string& GetGeometryShaderPath() const noexcept { return m_geometryShaderPath; }
    const std::string& GetHullShaderPath    () const noexcept { return m_hullShaderPath; }
    const std::string& GetDomainShaderPath  () const noexcept { return m_domainShaderPath; }
    const std::string& GetComputeShaderPath () const noexcept { return m_computeShaderPath; }
    const std::string& GetModelPath         () const noexcept { return m_modelPath; }

    JsonLoader GetJsonData() const noexcept;
    void       LoadFromJson(const JsonLoader& loader) noexcept;

    void ImguiView(float deltaTime);

private:
    bool BuildGeometry      (_In_ const KFE_BUILD_OBJECT_DESC& desc);
    bool BuildConstantBuffer(_In_ const KFE_BUILD_OBJECT_DESC& desc);
    bool BuildRootSignature (_In_ const KFE_BUILD_OBJECT_DESC& desc);
    bool BuildPipeline      (KFEDevice* device);
    bool BuildSampler       (_In_ const KFE_BUILD_OBJECT_DESC& desc);

    //~ Build Constant buffer for submeshes
    bool BuildSubmeshConstantBuffers(const KFE_BUILD_OBJECT_DESC& desc);

    void BuildSubmeshCBDataCacheFromModel() noexcept;
    void CacheNodeMeshesRecursive(const KFEModelNode& node) noexcept;

    void UpdateConstantBuffer(const KFE_UPDATE_OBJECT_DESC& desc);
    void UpdateSubmeshConstantBuffers(const KFE_UPDATE_OBJECT_DESC& desc);
    void UpdateCBDataForNodeMeshes(const KFEModelNode& node) noexcept;

    void RenderNodeRecursive(
        ID3D12GraphicsCommandList* cmdList,
        const KFE_RENDER_OBJECT_DESC& desc,
        const KFEModelNode& node,
        const DirectX::XMMATRIX& parentWorld);

    //~ Imgui stuff
    void DrawMeshImgui();

    void DrawNodeRecursiveImgui  (const KFEModelNode& node) noexcept;
    bool ShouldSkipNodeForDisplay(const KFEModelNode& node) const noexcept;

    void DrawNodeHeaderImgui(const KFEModelNode& node) const noexcept;
    bool DrawNodeTransformEditorImgui(KFEModelNode& node) noexcept;

public:
    ECullMode m_cullMode        { ECullMode::None };
    EDrawMode m_drawMode        { EDrawMode::Triangle };
    bool      m_bPipelineDirty  { false };
    bool      m_bTextureDirty   { true };
    bool      m_bModelDirty     { true };
    bool      m_bMetaDirty      { true };

private:
    KFEMeshSceneObject* m_pObject{ nullptr };
    float               m_nTimeLived{ 0.0f };
    bool                m_bBuild{ false };

    //~ Shaders
    std::string m_vertexShaderPath  { "shaders/mesh/vertex.hlsl" };
    std::string m_pixelShaderPath   { "shaders/mesh/pixel.hlsl" };
    std::string m_geometryShaderPath{};
    std::string m_hullShaderPath    {};
    std::string m_domainShaderPath  {};
    std::string m_computeShaderPath {};

    std::unique_ptr<KFERootSignature> m_pRootSignature{ nullptr };

    //~ Constant buffers
    std::unique_ptr<KFEBuffer>         m_pCBBuffer{ nullptr };
    std::unique_ptr<KFEConstantBuffer> m_pCBV     { nullptr };

    std::unique_ptr<KFEBuffer>         m_pMetaBuffer{ nullptr };
    std::unique_ptr<KFEConstantBuffer> m_pMetaCBV{ nullptr };

    //~ Pipeline
    std::unique_ptr<KFEPipelineState> m_pPipeline{ nullptr };
    KFEDevice*                        m_pDevice  { nullptr };

    //~ Sampling
    KFEResourceHeap*            m_pResourceHeap { nullptr };
    KFESamplerHeap*             m_pSamplerHeap  { nullptr };
    std::unique_ptr<KFESampler> m_pSampler      { nullptr };
    std::uint32_t               m_samplerIndex  { KFE_INVALID_INDEX };

    //~ Model
    KFEModel    m_mesh     {};
    std::string m_modelPath{ "assets/3d/blacksmith/source/BS_Final_Apose_Sketchfab.fbx" };
    std::unordered_map<std::uint32_t, KFE_COMMON_VERTEX_AND_PIXEL_CB_DESC> m_cbData     {};
    std::unordered_map<std::uint32_t, DirectX::XMFLOAT4X4>                 m_cbTransLazy{};

    //~ imgui
    bool m_bShowOnlyMeshNodes{ false };

    std::string m_modelPathPending;
    std::unordered_map<std::uint32_t, std::string> m_pendingTexturePath;
    std::unordered_set<std::uint32_t>              m_pendingTextureDirty;
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
    UpdateSubmeshConstantBuffers(desc);
}

_Use_decl_annotations_
bool kfe::KFEMeshSceneObject::Impl::Build(const KFE_BUILD_OBJECT_DESC& desc)
{
    m_pDevice = desc.Device;
    m_pResourceHeap = desc.ResourceHeap;
    m_pSamplerHeap = desc.SamplerHeap;

    if (!m_pDevice || !m_pResourceHeap || !m_pSamplerHeap)
    {
        LOG_ERROR("KFEMeshSceneObject::Impl::Build - One or more required pointers are null.");
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
    //~ Guard
    if (!m_bBuild)
        return true;

    //~ Destroy constant buffers
    if (m_pCBV)      m_pCBV->Destroy();
    if (m_pCBBuffer) m_pCBBuffer->Destroy();

    if (m_pMetaCBV)      m_pMetaCBV->Destroy();
    if (m_pMetaBuffer)   m_pMetaBuffer->Destroy();

    //~ Destroy pipeline & root signature
    if (m_pPipeline)      m_pPipeline->Destroy();
    if (m_pRootSignature) m_pRootSignature->Destroy();

    //~ Destroy sampler
    if (m_pSampler)       m_pSampler->Destroy();

    //~ Reset smart pointers
    m_pCBV.reset();
    m_pCBBuffer.reset();
    m_pMetaCBV.reset();
    m_pMetaBuffer.reset();

    m_pPipeline.reset();
    m_pRootSignature.reset();
    m_pSampler.reset();

    //~ Free sampler heap slot
    if (m_pSamplerHeap && m_samplerIndex != KFE_INVALID_INDEX)
    {
        m_pSamplerHeap->Free(m_samplerIndex);
        m_samplerIndex = KFE_INVALID_INDEX;
    }

    //~ Reset model + submesh state
    m_mesh.Reset();

    m_bTextureDirty = true;
    m_bModelDirty = false;
    m_bBuild = false;

    //~ Null references
    m_pResourceHeap = nullptr;
    m_pSamplerHeap = nullptr;
    m_pDevice = nullptr;

    return true;
}

void kfe::KFEMeshSceneObject::Impl::Render(_In_ const KFE_RENDER_OBJECT_DESC& desc)
{
    if (!m_bBuild) return;
    if (!m_mesh.IsValid()) return;

    if (m_bPipelineDirty)
    {
        if (!m_pDevice)
        {
            LOG_ERROR("Cannot rebuild pipeline, device is null.");
            return;
        }

        if (!BuildPipeline(m_pDevice))
        {
            LOG_ERROR("Failed to rebuild pipeline.");
            return;
        }
    }

    if (m_bModelDirty)
    {
        if (!m_pDevice)
        {
            LOG_ERROR("Cannot rebuild pipeline, device is null.");
            return;
        }

        KFE_BUILD_OBJECT_DESC builder{};
        builder.Device = m_pDevice;
        builder.CommandList = desc.CommandList;
        builder.ResourceHeap = m_pResourceHeap;

        if (!BuildGeometry(builder))
        {
            LOG_ERROR("Failed to rebuild model.");
            return;
        }
    }

    auto* cmdListObj = desc.CommandList;
    if (!cmdListObj || !cmdListObj->GetNative())
        return;

    ID3D12GraphicsCommandList* cmdList = cmdListObj->GetNative();

    cmdList->SetPipelineState(m_pPipeline->GetNative());
    cmdList->SetGraphicsRootSignature(m_pPipeline->GetRootSignature());

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

        auto& subs = m_mesh.GetSubmeshesMutable();
        for (auto& sm : subs)
        {
            sm.BindTextureFromPath(desc.CommandList, m_pDevice, m_pResourceHeap);
        }
    }
    else return;

    const KFEModelNode* root = m_mesh.GetRootNode();
    if (!root)
        return;

    const DirectX::XMMATRIX parentWorld = m_pObject->GetWorldMatrix();
    RenderNodeRecursive(cmdList, desc, *root, parentWorld);
}

void kfe::KFEMeshSceneObject::Impl::SetModelPath(const std::string& path) noexcept
{
    if (m_modelPath == path)
        return;

    m_modelPath = path;
    m_bModelDirty = true;
}

_Use_decl_annotations_
bool kfe::KFEMeshSceneObject::Impl::BuildGeometry(const KFE_BUILD_OBJECT_DESC& desc)
{
    if (m_modelPath.empty())
    {
        LOG_ERROR("Model path is empty.");
        return false;
    }

    if (!kfe_helpers::IsFile(m_modelPath))
    {
        LOG_ERROR("Model file does not exist: '{}'", m_modelPath);
        return false;
    }

    m_mesh.Reset();

    if (!m_mesh.Initialize(m_modelPath, desc.Device, desc.CommandList, desc.ResourceHeap))
    {
        LOG_ERROR("Failed to initialize model from '{}'", m_modelPath);
        return false;
    }

    LOG_SUCCESS("Model loaded: '{}'", m_modelPath);

    m_bModelDirty = false;

    if (!BuildSubmeshConstantBuffers(desc))
        return false;

    BuildSubmeshCBDataCacheFromModel();

    if (!m_cbTransLazy.empty())
    {
        const std::uint32_t subCount = m_mesh.GetSubmeshCount();

        for (const auto& kv : m_cbTransLazy)
        {
            const std::uint32_t idx = kv.first;
            if (idx >= subCount)
                continue;

            auto& cb       = m_cbData[idx];
            cb.WorldMatrix = DirectX::XMLoadFloat4x4(&kv.second);
        }

        m_cbTransLazy.clear();
    }

    auto& subs = m_mesh.GetSubmeshesMutable();

    for (auto& sm : subs)
    {
        if (!sm.AllocateReserveSolt(desc.ResourceHeap))
        {
            return false;
        }
    }

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

    //~ SRV descriptor table
    D3D12_DESCRIPTOR_RANGE srvRange{};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = static_cast<UINT>(EModelTextureSlot::Count);
    srvRange.BaseShaderRegister = 0u;    //~ t0
    srvRange.RegisterSpace = 0u;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER params[3]{};

    //~ b0: per object and common constant buffer
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    params[0].Descriptor.ShaderRegister = 0u;  //~ b0
    params[0].Descriptor.RegisterSpace = 0u;

    //~ SRV descriptor table
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    params[1].DescriptorTable.NumDescriptorRanges = 1u;
    params[1].DescriptorTable.pDescriptorRanges = &srvRange;

    //~ b1: per submesh meta constant buffer
    params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    params[2].Descriptor.ShaderRegister = 1u;  //~ b1
    params[2].Descriptor.RegisterSpace = 0u;

    //~ Static sampler s0
    D3D12_STATIC_SAMPLER_DESC staticSampler{};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.MipLODBias = 0.0f;
    staticSampler.MaxAnisotropy = 1;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSampler.MinLOD = 0.0f;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0u;   //~ s0
    staticSampler.RegisterSpace = 0u;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    KFE_RG_CREATE_DESC root{};
    root.Device = desc.Device;
    root.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    root.NumRootParameters = 3u;
    root.RootParameters = params;
    root.NumStaticSamplers = 1u;
    root.StaticSamplers = &staticSampler;

    if (!m_pRootSignature->Initialize(root))
    {
        LOG_ERROR("Failed to Create Root Signature!");
        return false;
    }

    m_pRootSignature->SetDebugName(L"Mesh Scene Signature");
    LOG_SUCCESS("Mesh Root Signature Created!");
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

bool kfe::KFEMeshSceneObject::Impl::BuildSubmeshConstantBuffers(const KFE_BUILD_OBJECT_DESC& desc)
{
    if (!desc.Device || !desc.ResourceHeap)
    {
        LOG_ERROR("BuildSubmeshConstantBuffers: Device or ResourceHeap is null.");
        return false;
    }

    if (!m_mesh.IsValid())
    {
        LOG_ERROR("BuildSubmeshConstantBuffers: Model is not valid.");
        return false;
    }

    auto& submeshes = m_mesh.GetSubmeshesMutable();
    if (submeshes.empty())
    {
        LOG_ERROR("BuildSubmeshConstantBuffers: No submeshes.");
        return false;
    }

    std::uint32_t cbBytes = static_cast<std::uint32_t>(sizeof(KFE_COMMON_VERTEX_AND_PIXEL_CB_DESC));
    cbBytes               = kfe_helpers::AlignTo256(cbBytes);

    std::uint32_t metaBytes = static_cast<std::uint32_t>(sizeof(ModelTextureMetaInformation));
    metaBytes               = kfe_helpers::AlignTo256(metaBytes);

    KFE_CREATE_BUFFER_DESC buffer{};
    buffer.Device        = desc.Device;
    buffer.HeapType      = D3D12_HEAP_TYPE_UPLOAD;
    buffer.InitialState  = D3D12_RESOURCE_STATE_GENERIC_READ;
    buffer.ResourceFlags = D3D12_RESOURCE_FLAG_NONE;

    for (std::size_t i = 0; i < submeshes.size(); ++i)
    {
        auto& sm = submeshes[i];

        sm.ConstantBuffer = std::make_unique<KFEBuffer>();

        buffer.SizeInBytes = cbBytes;

        if (!sm.ConstantBuffer->Initialize(buffer))
        {
            LOG_ERROR("Failed to build submesh CB buffer (i={})", i);
            return false;
        }

        sm.CBView = std::make_unique<KFEConstantBuffer>();

        KFE_CONSTANT_BUFFER_CREATE_DESC view{};
        view.Device         = desc.Device;
        view.OffsetInBytes  = 0u;
        view.ResourceBuffer = sm.ConstantBuffer.get();
        view.ResourceHeap   = desc.ResourceHeap;
        view.SizeInBytes    = cbBytes;

        if (!sm.CBView->Initialize(view))
        {
            LOG_ERROR("Failed to build submesh CBV (i={})", i);
            return false;
        }

        sm.MetaCB = std::make_unique<KFEBuffer>();

        buffer.SizeInBytes = metaBytes;

        if (!sm.MetaCB->Initialize(buffer))
        {
            LOG_ERROR("Failed to build submesh Meta CB buffer (i={})", i);
            return false;
        }

        sm.MetaCBView = std::make_unique<KFEConstantBuffer>();

        view.ResourceBuffer = sm.MetaCB.get();
        view.SizeInBytes = metaBytes;

        if (!sm.MetaCBView->Initialize(view))
        {
            LOG_ERROR("Failed to build submesh Meta CBV (i={})", i);
            return false;
        }
    }

    LOG_SUCCESS("Per-submesh constant buffers created! Count={}", static_cast<std::uint32_t>(submeshes.size()));
    return true;
}

void kfe::KFEMeshSceneObject::Impl::BuildSubmeshCBDataCacheFromModel() noexcept
{
    m_cbData.clear();

    if (!m_mesh.IsValid())
        return;

    const std::uint32_t submeshCount = m_mesh.GetSubmeshCount();
    m_cbData.reserve(static_cast<std::size_t>(submeshCount));

    const KFEModelNode* root = m_mesh.GetRootNode();
    if (!root)
        return;

    CacheNodeMeshesRecursive(*root);

    KFE_COMMON_VERTEX_AND_PIXEL_CB_DESC init{};
    init.WorldMatrix        = DirectX::XMMatrixIdentity();
    init.ViewMatrix         = DirectX::XMMatrixIdentity();
    init.ProjectionMatrix   = DirectX::XMMatrixIdentity();
    init.OrthogonalMatrix   = DirectX::XMMatrixIdentity();

    for (std::uint32_t i = 0u; i < submeshCount; ++i)
    {
        if (m_cbData.find(i) == m_cbData.end())
            m_cbData.emplace(i, init);
    }
}

void kfe::KFEMeshSceneObject::Impl::CacheNodeMeshesRecursive(const KFEModelNode& node) noexcept
{
    const DirectX::XMMATRIX nodeLocal = node.GetMatrix();

    for (std::uint32_t meshIndex : node.MeshIndices)
    {
        auto it = m_cbData.find(meshIndex);
        if (it == m_cbData.end())
        {
            KFE_COMMON_VERTEX_AND_PIXEL_CB_DESC cb{};
            cb.WorldMatrix = nodeLocal;
            cb.ViewMatrix = DirectX::XMMatrixIdentity();
            cb.ProjectionMatrix = DirectX::XMMatrixIdentity();
            cb.OrthogonalMatrix = DirectX::XMMatrixIdentity();
            m_cbData.emplace(meshIndex, cb);
        }
        else
        {
            it->second.WorldMatrix = nodeLocal;
        }
    }

    for (const auto& child : node.Children)
    {
        if (child)
            CacheNodeMeshesRecursive(*child);
    }
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

void kfe::KFEMeshSceneObject::Impl::UpdateSubmeshConstantBuffers(const KFE_UPDATE_OBJECT_DESC& desc)
{
    if (!m_bBuild)
        return;

    if (!m_mesh.IsValid())
        return;

    const auto& submeshes = m_mesh.GetSubmeshes();

    for (std::size_t i = 0; i < submeshes.size(); ++i)
    {
        const auto& sm = submeshes[i];

        if (!sm.CBView)
            continue;

        auto* cv = static_cast<KFE_COMMON_VERTEX_AND_PIXEL_CB_DESC*>(sm.CBView->GetMappedData());
        if (!cv)
            continue;

        cv->ViewMatrix = desc.ViewMatrix;
        cv->ProjectionMatrix = desc.PerpectiveMatrix;
        cv->OrthogonalMatrix = desc.OrthographicMatrix;

        cv->Resolution = desc.Resolution;
        cv->MousePosition = desc.MousePosition;

        cv->ObjectPosition = m_pObject ? m_pObject->GetPosition() : DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f };
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
}

void kfe::KFEMeshSceneObject::Impl::UpdateCBDataForNodeMeshes(const KFEModelNode& node) noexcept
{
    const DirectX::XMMATRIX local = node.GetMatrix();

    for (std::uint32_t meshIndex : node.MeshIndices)
    {
        auto it = m_cbData.find(meshIndex);
        if (it == m_cbData.end())
        {
            KFE_COMMON_VERTEX_AND_PIXEL_CB_DESC init{};
            init.WorldMatrix = local;
            init.ViewMatrix = DirectX::XMMatrixIdentity();
            init.ProjectionMatrix = DirectX::XMMatrixIdentity();
            init.OrthogonalMatrix = DirectX::XMMatrixIdentity();
            m_cbData.emplace(meshIndex, init);
        }
        else
        {
            it->second.WorldMatrix = local;
        }
    }
}

void kfe::KFEMeshSceneObject::Impl::RenderNodeRecursive(
    ID3D12GraphicsCommandList* cmdList,
    const KFE_RENDER_OBJECT_DESC& desc,
    const KFEModelNode& node,
    const DirectX::XMMATRIX& parentWorld)
{
    if (!cmdList || !m_pObject || !m_pDevice || !m_pResourceHeap)
        return;

    using namespace DirectX;

    if (!node.IsEnabled())
        return;

    const KFE_MESH_CACHE_SHARE* share = m_mesh.GetCacheShare();
    if (!share || !share->Entry)
        return;

    const auto& submeshes = m_mesh.GetSubmeshes();
    const auto& meshesGPU = share->Entry->MeshesGPU;

    const XMMATRIX local = node.GetMatrix();
    const XMMATRIX nodeWorld = local * parentWorld;

    for (std::uint32_t meshIndex : node.MeshIndices)
    {
        if (meshIndex >= submeshes.size())
            continue;

        const auto& sub = submeshes[meshIndex];

        if (sub.CacheMeshIndex >= meshesGPU.size())
            continue;

        const auto& gpuMeshPtr = meshesGPU[sub.CacheMeshIndex];
        if (!gpuMeshPtr || !gpuMeshPtr->IsValid())
            continue;

        const KFEGpuMesh& gpuMesh = *gpuMeshPtr;

        const KFEVertexBuffer* vbView = gpuMesh.GetVertexBufferView();
        const KFEIndexBuffer* ibView = gpuMesh.GetIndexBufferView();
        if (!vbView || !ibView)
            continue;

        auto& sm = const_cast<KFEModelSubmesh&>(sub);

        //~ per-submesh b0 (WorldMatrix only) + bind CBV
        if (sm.CBView)
        {
            XMMATRIX cachedLocal = XMMatrixIdentity();

            auto it = m_cbData.find(meshIndex);
            if (it != m_cbData.end())
                cachedLocal = it->second.WorldMatrix;

            const XMMATRIX finalWorld = cachedLocal * nodeWorld;

            if (auto* cv = static_cast<KFE_COMMON_VERTEX_AND_PIXEL_CB_DESC*>(sm.CBView->GetMappedData()))
            {
                cv->WorldMatrix = finalWorld;
            }

            cmdList->SetGraphicsRootConstantBufferView(
                0u,
                static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(sm.CBView->GetGPUVirtualAddress()));
        }

        //~ per-submesh SRV table update (CPU copy) + bind table
        sm.BindTextureFromPath(desc.CommandList, m_pDevice, m_pResourceHeap);

        if (sm.GetBaseSrvIndex() != KFE_INVALID_INDEX)
        {
            cmdList->SetGraphicsRootDescriptorTable(
                1u,
                m_pResourceHeap->GetGPUHandle(sm.GetBaseSrvIndex()));
        }

        //~ per-submesh b1 (meta) update + bind CBV
        if (sm.MetaCBView)
        {
            UpdateMetaCB(sm);

            cmdList->SetGraphicsRootConstantBufferView(
                2u,
                static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(sm.MetaCBView->GetGPUVirtualAddress()));
        }

        const D3D12_VERTEX_BUFFER_VIEW vb = vbView->GetView();
        const D3D12_INDEX_BUFFER_VIEW  ib = ibView->GetView();

        cmdList->IASetVertexBuffers(0u, 1u, &vb);
        cmdList->IASetIndexBuffer(&ib);

        cmdList->IASetPrimitiveTopology(
            m_drawMode == EDrawMode::Point
            ? D3D_PRIMITIVE_TOPOLOGY_POINTLIST
            : D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        cmdList->DrawIndexedInstanced(
            gpuMesh.GetIndexCount(),
            1u,
            0u,
            0u,
            0u);
    }

    for (const auto& child : node.Children)
    {
        if (child)
            RenderNodeRecursive(cmdList, desc, *child, nodeWorld);
    }
}

void kfe::KFEMeshSceneObject::Impl::DrawMeshImgui()
{

}

bool kfe::KFEMeshSceneObject::Impl::ShouldSkipNodeForDisplay(const KFEModelNode& node) const noexcept
{
    return m_bShowOnlyMeshNodes && !node.HasMeshes();
}

void kfe::KFEMeshSceneObject::Impl::DrawNodeHeaderImgui(const KFEModelNode& node) const noexcept
{
    ImGui::Text("Meshes: %u", static_cast<std::uint32_t>(node.MeshIndices.size()));
    ImGui::SameLine();
    ImGui::Text("Children: %u", static_cast<std::uint32_t>(node.Children.size()));
}

bool kfe::KFEMeshSceneObject::Impl::DrawNodeTransformEditorImgui(KFEModelNode& node) noexcept
{
    bool changed = false;

    {
        bool enabled = node.IsEnabled();
        if (ImGui::Checkbox("Enabled", &enabled))
        {
            node.SetEnabled(enabled);
            changed = true;
        }
    }

    {
        auto p = node.GetPosition();
        if (ImGui::DragFloat3("Position", &p.x, 0.01f))
        {
            node.SetPosition(p);
            changed = true;
        }

        auto r = node.GetRotation();
        if (ImGui::DragFloat3("Rotation (deg)", &r.x, 0.25f))
        {
            node.SetRotation(r);
            changed = true;
        }

        auto s = node.GetScale();
        if (ImGui::DragFloat3("Scale", &s.x, 0.01f, 0.0001f, 10000.0f))
        {
            node.SetScale(s);
            changed = true;
        }

        auto pv = node.GetPivot();
        if (ImGui::DragFloat3("Pivot", &pv.x, 0.01f))
        {
            node.SetPivot(pv);
            changed = true;
        }
    }

    if (ImGui::Button("Reset Node Transform"))
    {
        node.ResetTransform();
        changed = true;
    }

    if (changed)
    {
        UpdateCBDataForNodeMeshes(node);
    }

    return changed;
}

void kfe::KFEMeshSceneObject::Impl::DrawNodeRecursiveImgui(const KFEModelNode& node) noexcept
{
    if (ShouldSkipNodeForDisplay(node))
    {
        if (node.HasChildren())
        {
            for (const auto& c : node.Children)
                if (c) DrawNodeRecursiveImgui(*c);
        }
        return;
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!node.HasChildren())
        flags |= ImGuiTreeNodeFlags_Leaf;

    ImGui::PushID(&node);
    bool open = ImGui::TreeNodeEx(node.Name.c_str(), flags);
    ImGui::PopID();

    if (open)
    {
        DrawNodeHeaderImgui(node);

        auto& editable = const_cast<KFEModelNode&>(node);
        DrawNodeTransformEditorImgui(editable);

        ImGui::Separator();

        for (const auto& child : node.Children)
        {
            if (child)
                DrawNodeRecursiveImgui(*child);
        }

        ImGui::TreePop();
    }
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
    root["CullMode"]        = ToString(m_cullMode);
    root["DrawMode"]        = ToString(m_drawMode);

    root["VertexShader"]    = m_vertexShaderPath;
    root["PixelShader"]     = m_pixelShaderPath;
    root["GeometryShader"]  = m_geometryShaderPath;
    root["HullShader"]      = m_hullShaderPath;
    root["DomainShader"]    = m_domainShaderPath;
    root["ComputeShader"]   = m_computeShaderPath;
    root["ModelPath"]       = m_modelPath;

    JsonLoader subRoot{};

    if (m_mesh.IsValid())
    {
        const auto& submeshes = m_mesh.GetSubmeshes();

        for (std::uint32_t i = 0u; i < static_cast<std::uint32_t>(submeshes.size()); ++i)
        {
            const auto& sm = submeshes[i];

            const bool attached = (sm.CBView != nullptr);

            if (!attached)
                continue;

            JsonLoader smNode{};
            smNode["CacheMeshIndex"] = std::to_string(sm.CacheMeshIndex);

            auto it = m_cbData.find(i);
            if (it != m_cbData.end())
            {
                const auto& cb = it->second;

                auto PutMat = [](JsonLoader& j, const char* key, const DirectX::XMMATRIX& M)
                    {
                        DirectX::XMFLOAT4X4 f{};
                        DirectX::XMStoreFloat4x4(&f, M);

                        j[key]["m00"] = std::to_string(f._11); j[key]["m01"] = std::to_string(f._12); j[key]["m02"] = std::to_string(f._13); j[key]["m03"] = std::to_string(f._14);
                        j[key]["m10"] = std::to_string(f._21); j[key]["m11"] = std::to_string(f._22); j[key]["m12"] = std::to_string(f._23); j[key]["m13"] = std::to_string(f._24);
                        j[key]["m20"] = std::to_string(f._31); j[key]["m21"] = std::to_string(f._32); j[key]["m22"] = std::to_string(f._33); j[key]["m23"] = std::to_string(f._34);
                        j[key]["m30"] = std::to_string(f._41); j[key]["m31"] = std::to_string(f._42); j[key]["m32"] = std::to_string(f._43); j[key]["m33"] = std::to_string(f._44);
                    };

                PutMat(smNode, "LocalWorldMatrix", cb.WorldMatrix);
            }

            subRoot[std::to_string(i)] = std::move(smNode);
        }
    }

    root["Submeshes"] = std::move(subRoot);

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

    if (loader.Contains("ModelPath"))
    {
        SetModelPath(loader["ModelPath"].GetValue());
        m_bModelDirty = true;
    }

    m_cbTransLazy.clear();

    if (loader.Contains("Submeshes"))
    {
        const auto& sub = loader["Submeshes"];

        for (const auto& kv : sub)
        {
            const std::string& key = kv.first;
            const auto& smNode = sub[key];

            std::uint32_t idx = 0u;

            try
            {
                idx = static_cast<std::uint32_t>(std::stoul(key));
            }
            catch (...)
            {
                continue;
            }

            if (!smNode.Contains("LocalWorldMatrix"))
                continue;

            DirectX::XMFLOAT4X4 f{};
            f._11 = std::stof(smNode["LocalWorldMatrix"]["m00"].GetValue()); f._12 = std::stof(smNode["LocalWorldMatrix"]["m01"].GetValue()); f._13 = std::stof(smNode["LocalWorldMatrix"]["m02"].GetValue()); f._14 = std::stof(smNode["LocalWorldMatrix"]["m03"].GetValue());
            f._21 = std::stof(smNode["LocalWorldMatrix"]["m10"].GetValue()); f._22 = std::stof(smNode["LocalWorldMatrix"]["m11"].GetValue()); f._23 = std::stof(smNode["LocalWorldMatrix"]["m12"].GetValue()); f._24 = std::stof(smNode["LocalWorldMatrix"]["m13"].GetValue());
            f._31 = std::stof(smNode["LocalWorldMatrix"]["m20"].GetValue()); f._32 = std::stof(smNode["LocalWorldMatrix"]["m21"].GetValue()); f._33 = std::stof(smNode["LocalWorldMatrix"]["m22"].GetValue()); f._34 = std::stof(smNode["LocalWorldMatrix"]["m23"].GetValue());
            f._41 = std::stof(smNode["LocalWorldMatrix"]["m30"].GetValue()); f._42 = std::stof(smNode["LocalWorldMatrix"]["m31"].GetValue()); f._43 = std::stof(smNode["LocalWorldMatrix"]["m32"].GetValue()); f._44 = std::stof(smNode["LocalWorldMatrix"]["m33"].GetValue());

            m_cbTransLazy[idx] = f;
        }
    }
}

void kfe::KFEMeshSceneObject::Impl::ImguiView(float)
{
    if (ImGui::CollapsingHeader("Model Settings", ImGuiTreeNodeFlags_DefaultOpen))
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
    DrawMeshImgui();
}

#pragma endregion
