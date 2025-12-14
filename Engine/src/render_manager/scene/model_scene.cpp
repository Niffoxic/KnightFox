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

    JsonLoader GetJsonData               () const noexcept;
    JsonLoader GetChildTransformation    () const noexcept;
    JsonLoader GetChildMetaInformation   () const noexcept;
    JsonLoader GetChildTextureInformation() const noexcept;
    void       LoadFromJson            (const JsonLoader& loader) noexcept;
    void       LoadChildTransformations(const JsonLoader& loader) noexcept;
    void       LoadChildMetaInformation(const JsonLoader& loader) noexcept;
    void       LoadChildTextureInformation(const JsonLoader& loader) noexcept;

    void ImguiView                (float deltaTime);
    void ImguiChildTransformation (float deltaTime);
    void ImguiTextureMetaConfig   (float deltaTime);

private:
    bool BuildGeometry      (_In_ const KFE_BUILD_OBJECT_DESC& desc);
    bool BuildConstantBuffer(_In_ const KFE_BUILD_OBJECT_DESC& desc);
    bool BuildRootSignature (_In_ const KFE_BUILD_OBJECT_DESC& desc);
    bool BuildPipeline      (KFEDevice* device);
    bool BuildSampler       (_In_ const KFE_BUILD_OBJECT_DESC& desc);

    //~ Build Constant buffer for submeshes
    void ApplyChildTransformationsFromCache() noexcept;
    void ApplyChildMetaInformationFromCache() noexcept;
    void ApplyChildTextureInformationFromCache() noexcept;
    bool BuildSubmeshConstantBuffers(const KFE_BUILD_OBJECT_DESC& desc);
    void BuildSubmeshCBDataCacheFromModel() noexcept;
    void CacheNodeMeshesRecursive(const KFEModelNode& node) noexcept;

    //~ Constant buffer updates
    void UpdateConstantBuffer(const KFE_UPDATE_OBJECT_DESC& desc);
    void UpdateSubmeshConstantBuffers(const KFE_UPDATE_OBJECT_DESC& desc);
    void UpdateCBDataForNodeMeshes(const KFEModelNode& node) noexcept;

    void RenderNodeRecursive(
        ID3D12GraphicsCommandList* cmdList,
        const KFE_RENDER_OBJECT_DESC& desc,
        const KFEModelNode& node,
        const DirectX::XMMATRIX& parentWorld);

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
    std::unordered_map<std::uint32_t, ModelTextureMetaInformation>         m_cbMetaLazy{};
    std::unordered_map<std::uint32_t, std::array<std::string, (size_t)EModelTextureSlot::Count>> m_cbTexLazy;

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
    if (!cmdListObj)
        return;

    ID3D12GraphicsCommandList* cmdList = cmdListObj;

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

    BuildSubmeshCBDataCacheFromModel  ();
    ApplyChildTransformationsFromCache();
    ApplyChildMetaInformationFromCache();
    ApplyChildTextureInformationFromCache();

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
    if (!m_pObject->BuildLightCB(desc)) return false;

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

    D3D12_ROOT_PARAMETER params[4]{};

    //~ b0: per object and common constant buffer
    params[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
    params[0].Descriptor.ShaderRegister = 0u;  //~ b0
    params[0].Descriptor.RegisterSpace  = 0u;

    //~ SRV descriptor table
    params[1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;
    params[1].DescriptorTable.NumDescriptorRanges = 1u;
    params[1].DescriptorTable.pDescriptorRanges   = &srvRange;

    //~ b1: per submesh meta constant buffer
    params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    params[2].Descriptor.ShaderRegister = 1u;  //~ b1
    params[2].Descriptor.RegisterSpace = 0u;

    //~ b2: directional light constant buffer
    params[3].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[3].ShaderVisibility          = D3D12_SHADER_VISIBILITY_PIXEL;
    params[3].Descriptor.ShaderRegister = 2u;  //~ b2
    params[3].Descriptor.RegisterSpace  = 0u;

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
    root.NumRootParameters = 4u;
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

void kfe::KFEMeshSceneObject::Impl::ApplyChildTransformationsFromCache() noexcept
{
    if (m_cbTransLazy.empty())
        return;

    const KFEModelNode* rootConst = m_mesh.GetRootNode();
    if (!rootConst)
        return;

    KFEModelNode* root = const_cast<KFEModelNode*>(rootConst);

    //~ Skip dummy-root chains
    while (root && root->Children.size() == 1 && !root->HasMeshes())
        root = root->Children[0].get();

    auto Walk = [&](auto&& self, KFEModelNode* n, const std::string& parentPath) noexcept -> void
        {
            if (!n)
                return;

            const std::string name = n->Name;
            const std::string path = parentPath.empty() ? name : (parentPath + "/" + name);

            if (!path.empty())
            {
                const std::uint32_t key = static_cast<std::uint32_t>(std::hash<std::string>{}(path));
                auto it = m_cbTransLazy.find(key);
                if (it != m_cbTransLazy.end())
                {
                    n->SetMatrixFromImport(it->second, { 0,0,0 }, { 0,0,0 }, { 1,1,1 });
                }
            }

            for (auto& c : n->Children)
                self(self, c.get(), path);
        };

    Walk(Walk, root, std::string{});
    m_cbTransLazy.clear();
}

void kfe::KFEMeshSceneObject::Impl::ApplyChildMetaInformationFromCache() noexcept
{
    if (m_cbMetaLazy.empty())
        return;

    auto& subs = m_mesh.GetSubmeshesMutable();
    if (subs.empty())
    {
        m_cbMetaLazy.clear();
        return;
    }

    const std::uint32_t subCount = static_cast<std::uint32_t>(subs.size());

    for (const auto& kv : m_cbMetaLazy)
    {
        const std::uint32_t idx = kv.first;
        if (idx >= subCount)
            continue;

        auto& sm = subs[idx];
        sm.m_textureMetaInformation = kv.second;
        sm.m_bMetaDirty = true;
    }

    m_cbMetaLazy.clear();
}

void kfe::KFEMeshSceneObject::Impl::ApplyChildTextureInformationFromCache() noexcept
{
    if (m_cbTexLazy.empty())
        return;

    auto& subs = m_mesh.GetSubmeshesMutable();
    if (subs.empty())
    {
        m_cbTexLazy.clear();
        return;
    }

    const std::uint32_t subCount = static_cast<std::uint32_t>(subs.size());

    for (const auto& kv : m_cbTexLazy)
    {
        const std::uint32_t idx = kv.first;
        if (idx >= subCount)
            continue;

        const auto& slots = kv.second;
        auto& sm = subs[idx];

        sm.SetBaseColor(slots[(size_t)EModelTextureSlot::BaseColor]);
        sm.SetNormal(slots[(size_t)EModelTextureSlot::Normal]);
        sm.SetORM(slots[(size_t)EModelTextureSlot::ORM]);
        sm.SetEmissive(slots[(size_t)EModelTextureSlot::Emissive]);
        sm.SetOpacity(slots[(size_t)EModelTextureSlot::Opacity]);
        sm.SetHeight(slots[(size_t)EModelTextureSlot::Height]);

        sm.SetOcclusion(slots[(size_t)EModelTextureSlot::Occlusion]);
        sm.SetRoughness(slots[(size_t)EModelTextureSlot::Roughness]);
        sm.SetMetallic(slots[(size_t)EModelTextureSlot::Metallic]);

        sm.SetDyeMask(slots[(size_t)EModelTextureSlot::DyeMask]);

        sm.m_bTextureDirty = true;
    }

    m_cbTexLazy.clear();
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

    cv->WorldMatrix = DirectX::XMMatrixTranspose(m_pObject->GetWorldMatrix());
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

    if (m_pObject && m_pObject->m_pLightCBV)
    {
        cmdList->SetGraphicsRootConstantBufferView(
            3u,
            m_pObject->m_pLightCBV->GetGPUVirtualAddress());
    }
    else LOG_ERROR("NOPE!");

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

        //~ per submesh b0 (WorldMatrix only) + bind CBV
        if (sm.CBView)
        {
            XMMATRIX cachedLocal = XMMatrixIdentity();

            auto it = m_cbData.find(meshIndex);
            if (it != m_cbData.end())
                cachedLocal = it->second.WorldMatrix;

            const XMMATRIX finalWorld = cachedLocal * nodeWorld;

            if (auto* cv = static_cast<KFE_COMMON_VERTEX_AND_PIXEL_CB_DESC*>(sm.CBView->GetMappedData()))
            {
                cv->WorldMatrix = DirectX::XMMatrixTranspose(finalWorld);
            }

            cmdList->SetGraphicsRootConstantBufferView(
                0u,
                static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(sm.CBView->GetGPUVirtualAddress()));
        }

        //~ per submesh SRV table update (CPU copy) + bind table
        sm.BindTextureFromPath(desc.CommandList, m_pDevice, m_pResourceHeap);

        if (sm.GetBaseSrvIndex() != KFE_INVALID_INDEX)
        {
            cmdList->SetGraphicsRootDescriptorTable(
                1u,
                m_pResourceHeap->GetGPUHandle(sm.GetBaseSrvIndex()));
        }

        //~ per submesh b1 (meta) update + bind CBV
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

    root["VertexShader"]        = m_vertexShaderPath;
    root["PixelShader"]         = m_pixelShaderPath;
    root["GeometryShader"]      = m_geometryShaderPath;
    root["HullShader"]          = m_hullShaderPath;
    root["DomainShader"]        = m_domainShaderPath;
    root["ComputeShader"]       = m_computeShaderPath;
    root["ModelPath"]           = m_modelPath;
    root["Submesh"]             = GetChildTransformation();
    root["MetaInformation"]     = GetChildMetaInformation();
    root["TextureInformation"]  = GetChildTextureInformation();
    return root;
}

JsonLoader kfe::KFEMeshSceneObject::Impl::GetChildTransformation() const noexcept
{
    JsonLoader out{};

    const KFEModelNode* rootConst = m_mesh.GetRootNode();
    if (!rootConst)
        return out;

    KFEModelNode* root = const_cast<KFEModelNode*>(rootConst);

    while (root &&
        root->Children.size() == 1 &&
        !root->HasMeshes())
    {
        root = root->Children[0].get();
    }

    auto WriteMatrix = [](JsonLoader& j, const DirectX::XMFLOAT4X4& m) noexcept
        {
            j["m00"] = m._11; j["m01"] = m._12; j["m02"] = m._13; j["m03"] = m._14;
            j["m10"] = m._21; j["m11"] = m._22; j["m12"] = m._23; j["m13"] = m._24;
            j["m20"] = m._31; j["m21"] = m._32; j["m22"] = m._33; j["m23"] = m._34;
            j["m30"] = m._41; j["m31"] = m._42; j["m32"] = m._43; j["m33"] = m._44;
        };

    auto WriteNodeRecursive =
        [&](auto&& self, const KFEModelNode* n, JsonLoader& dst) noexcept -> void
        {
            if (!n)
                return;

            dst["Name"] = n->Name;

            const DirectX::XMFLOAT4X4& mat = n->GetMatrixF4x4();
            WriteMatrix(dst["Matrix"], mat);

            for (std::size_t i = 0; i < n->Children.size(); ++i)
            {
                JsonLoader child{};
                self(self, n->Children[i].get(), child);
                dst["Children"][std::to_string(i)] = std::move(child);
            }
        };

    JsonLoader rootNode{};
    WriteNodeRecursive(WriteNodeRecursive, root, rootNode);
    out["Root"] = std::move(rootNode);

    return out;
}

JsonLoader kfe::KFEMeshSceneObject::Impl::GetChildMetaInformation() const noexcept
{
    JsonLoader out{};

    const auto& subs = m_mesh.GetSubmeshes();
    if (subs.empty())
        return out;

    auto WriteFloat3 = [](JsonLoader& j, const char* key, const float v[3]) noexcept
        {
            j[key]["x"] = v[0];
            j[key]["y"] = v[1];
            j[key]["z"] = v[2];
        };

    auto WriteMeta = [&](JsonLoader& dst, const ModelTextureMetaInformation& meta) noexcept
        {
            //~ BaseColor
            dst["BaseColor"]["IsTextureAttached"] = meta.BaseColor.IsTextureAttached;
            dst["BaseColor"]["UvTilingX"] = meta.BaseColor.UvTilingX;
            dst["BaseColor"]["UvTilingY"] = meta.BaseColor.UvTilingY;
            dst["BaseColor"]["Strength"] = meta.BaseColor.Strength;

            //~ Normal
            dst["Normal"]["IsTextureAttached"] = meta.Normal.IsTextureAttached;
            dst["Normal"]["NormalStrength"] = meta.Normal.NormalStrength;
            dst["Normal"]["UvTilingX"] = meta.Normal.UvTilingX;
            dst["Normal"]["UvTilingY"] = meta.Normal.UvTilingY;

            //~ ORM
            dst["ORM"]["IsTextureAttached"] = meta.ORM.IsTextureAttached;
            dst["ORM"]["IsMixed"] = meta.ORM.IsMixed;
            dst["ORM"]["UvTilingX"] = meta.ORM.UvTilingX;
            dst["ORM"]["UvTilingY"] = meta.ORM.UvTilingY;

            //~ Emissive
            dst["Emissive"]["IsTextureAttached"] = meta.Emissive.IsTextureAttached;
            dst["Emissive"]["EmissiveIntensity"] = meta.Emissive.EmissiveIntensity;
            dst["Emissive"]["UvTilingX"] = meta.Emissive.UvTilingX;
            dst["Emissive"]["UvTilingY"] = meta.Emissive.UvTilingY;

            //~ Opacity
            dst["Opacity"]["IsTextureAttached"] = meta.Opacity.IsTextureAttached;
            dst["Opacity"]["AlphaMultiplier"] = meta.Opacity.AlphaMultiplier;
            dst["Opacity"]["AlphaCutoff"] = meta.Opacity.AlphaCutoff;

            //~ Height
            dst["Height"]["IsTextureAttached"] = meta.Height.IsTextureAttached;
            dst["Height"]["HeightScale"] = meta.Height.HeightScale;
            dst["Height"]["ParallaxMinLayers"] = meta.Height.ParallaxMinLayers;
            dst["Height"]["ParallaxMaxLayers"] = meta.Height.ParallaxMaxLayers;

            //~ Singular (Occlusion/Roughness/Metallic)
            dst["Singular"]["IsOcclusionAttached"] = meta.Singular.IsOcclusionAttached;
            dst["Singular"]["IsRoughnessAttached"] = meta.Singular.IsRoughnessAttached;
            dst["Singular"]["IsMetallicAttached"] = meta.Singular.IsMetallicAttached;

            dst["Singular"]["OcclusionStrength"] = meta.Singular.OcclusionStrength;
            dst["Singular"]["RoughnessValue"] = meta.Singular.RoughnessValue;
            dst["Singular"]["MetallicValue"] = meta.Singular.MetallicValue;

            dst["Singular"]["OcclusionTilingX"] = meta.Singular.OcclusionTilingX;
            dst["Singular"]["OcclusionTilingY"] = meta.Singular.OcclusionTilingY;
            dst["Singular"]["RoughnessTilingX"] = meta.Singular.RoughnessTilingX;
            dst["Singular"]["RoughnessTilingY"] = meta.Singular.RoughnessTilingY;

            dst["Singular"]["MetallicTilingX"] = meta.Singular.MetallicTilingX;
            dst["Singular"]["MetallicTilingY"] = meta.Singular.MetallicTilingY;

            //~ Dye
            dst["Dye"]["IsEnabled"] = meta.Dye.IsEnabled;
            dst["Dye"]["Strength"] = meta.Dye.Strength;
            WriteFloat3(dst["Dye"], "Color", meta.Dye.Color);

            //~ Global
            dst["Global"]["ForcedMipLevel"] = meta.ForcedMipLevel;
            dst["Global"]["UseForcedMip"] = meta.UseForcedMip;
        };

    for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(subs.size()); ++i)
    {
        const auto& sm = subs[i];

        JsonLoader entry{};
        entry["Name"] = m_mesh.GetSubmeshName(i);

        WriteMeta(entry["Meta"], sm.m_textureMetaInformation);

        out[std::to_string(i)] = std::move(entry);
    }

    return out;
}

JsonLoader kfe::KFEMeshSceneObject::Impl::GetChildTextureInformation() const noexcept
{
    JsonLoader out{};

    const auto& subs = m_mesh.GetSubmeshes();
    if (subs.empty())
        return out;

    auto WritePath = [](JsonLoader& j, const char* key, const std::string& path) noexcept
    {
        j[key] = path;
    };

    auto WriteSubmeshTextures = [&](JsonLoader& dst, const KFEModelSubmesh& sm) noexcept
        {
            WritePath(dst, "BaseColor", sm.GetBaseColorPath());
            WritePath(dst, "Normal", sm.GetNormalPath());
            WritePath(dst, "ORM", sm.GetORMPath());
            WritePath(dst, "Emissive", sm.GetEmissivePath());
            WritePath(dst, "Opacity", sm.GetOpacityPath());
            WritePath(dst, "Height", sm.GetHeightPath());

            WritePath(dst, "Occlusion", sm.GetOcclusionPath());
            WritePath(dst, "Roughness", sm.GetRoughnessPath());
            WritePath(dst, "Metallic", sm.GetMetallicPath());

            WritePath(dst, "DyeMask", sm.GetDyeMaskPath());
        };

    for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(subs.size()); ++i)
    {
        const auto& sm = subs[i];

        JsonLoader entry{};
        entry["Name"] = m_mesh.GetSubmeshName(i);

        WriteSubmeshTextures(entry["Textures"], sm);

        out[std::to_string(i)] = std::move(entry);
    }

    return out;
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

    LoadChildTransformations(loader);
    LoadChildMetaInformation(loader);
    LoadChildTextureInformation(loader);
}

void kfe::KFEMeshSceneObject::Impl::LoadChildTransformations(const JsonLoader& loader) noexcept
{
    m_cbTransLazy.clear();

    const JsonLoader* root = nullptr;

    if (loader.Contains("Submesh") && loader["Submesh"].Contains("Root"))
        root = &loader["Submesh"]["Root"];
    else if (loader.Contains("Root"))
        root = &loader["Root"];
    else
        return;

    auto ReadFloat = [](const JsonLoader& j, const char* key, float fallback) noexcept -> float
        {
            if (!j.Contains(key))
                return fallback;

            return j[key].AsFloat();
        };

    auto ReadMatrix = [&](const JsonLoader& jMat) noexcept -> DirectX::XMFLOAT4X4
        {
            DirectX::XMFLOAT4X4 m{};

            m._11 = ReadFloat(jMat, "m00", 1.0f); m._12 = ReadFloat(jMat, "m01", 0.0f); m._13 = ReadFloat(jMat, "m02", 0.0f); m._14 = ReadFloat(jMat, "m03", 0.0f);
            m._21 = ReadFloat(jMat, "m10", 0.0f); m._22 = ReadFloat(jMat, "m11", 1.0f); m._23 = ReadFloat(jMat, "m12", 0.0f); m._24 = ReadFloat(jMat, "m13", 0.0f);
            m._31 = ReadFloat(jMat, "m20", 0.0f); m._32 = ReadFloat(jMat, "m21", 0.0f); m._33 = ReadFloat(jMat, "m22", 1.0f); m._34 = ReadFloat(jMat, "m23", 0.0f);
            m._41 = ReadFloat(jMat, "m30", 0.0f); m._42 = ReadFloat(jMat, "m31", 0.0f); m._43 = ReadFloat(jMat, "m32", 0.0f); m._44 = ReadFloat(jMat, "m33", 1.0f);

            return m;
        };

    auto Walk =
        [&](auto&& self, const JsonLoader& n, const std::string& parentPath) noexcept -> void
        {
            const std::string name = n.Contains("Name") ? n["Name"].GetValue() : std::string{};
            const std::string path = parentPath.empty() ? name : (parentPath + "/" + name);

            if (!path.empty() && n.Contains("Matrix"))
            {
                const std::uint32_t key = static_cast<std::uint32_t>(std::hash<std::string>{}(path));
                m_cbTransLazy[key] = ReadMatrix(n["Matrix"]);
            }

            if (!n.Contains("Children"))
                return;

            const JsonLoader& ch = n["Children"];

            for (std::uint32_t i = 0;; ++i)
            {
                const std::string idx = std::to_string(i);
                if (!ch.Contains(idx))
                    break;

                self(self, ch[idx], path);
            }
        };

    Walk(Walk, *root, std::string{});
}

void kfe::KFEMeshSceneObject::Impl::LoadChildMetaInformation(const JsonLoader& loader) noexcept
{
    m_cbMetaLazy.clear();

    const JsonLoader* metaRoot = nullptr;

    if (loader.Has("MetaInformation"))
        metaRoot = &loader["MetaInformation"];
    else
        return;

    auto ReadFloat = [](const JsonLoader& j, const char* key, float fallback) noexcept -> float
        {
            if (!j.Has(key))
                return fallback;
            return j[key].AsFloat();
        };

    auto ReadFloat3 = [&](const JsonLoader& j, const char* key, float out3[3]) noexcept
        {
            if (!j.Has(key))
                return;

            const JsonLoader& v = j[key];
            out3[0] = ReadFloat(v, "x", out3[0]);
            out3[1] = ReadFloat(v, "y", out3[1]);
            out3[2] = ReadFloat(v, "z", out3[2]);
        };

    auto ReadMeta = [&](const JsonLoader& src, ModelTextureMetaInformation& meta) noexcept
        {
            //~ BaseColor
            if (src.Has("BaseColor"))
            {
                const auto& b = src["BaseColor"];
                meta.BaseColor.IsTextureAttached = ReadFloat(b, "IsTextureAttached", meta.BaseColor.IsTextureAttached);
                meta.BaseColor.UvTilingX = ReadFloat(b, "UvTilingX", meta.BaseColor.UvTilingX);
                meta.BaseColor.UvTilingY = ReadFloat(b, "UvTilingY", meta.BaseColor.UvTilingY);
                meta.BaseColor.Strength = ReadFloat(b, "Strength", meta.BaseColor.Strength);
            }

            //~ Normal
            if (src.Has("Normal"))
            {
                const auto& n = src["Normal"];
                meta.Normal.IsTextureAttached = ReadFloat(n, "IsTextureAttached", meta.Normal.IsTextureAttached);
                meta.Normal.NormalStrength = ReadFloat(n, "NormalStrength", meta.Normal.NormalStrength);
                meta.Normal.UvTilingX = ReadFloat(n, "UvTilingX", meta.Normal.UvTilingX);
                meta.Normal.UvTilingY = ReadFloat(n, "UvTilingY", meta.Normal.UvTilingY);
            }

            //~ ORM
            if (src.Has("ORM"))
            {
                const auto& o = src["ORM"];
                meta.ORM.IsTextureAttached = ReadFloat(o, "IsTextureAttached", meta.ORM.IsTextureAttached);
                meta.ORM.IsMixed = ReadFloat(o, "IsMixed", meta.ORM.IsMixed);
                meta.ORM.UvTilingX = ReadFloat(o, "UvTilingX", meta.ORM.UvTilingX);
                meta.ORM.UvTilingY = ReadFloat(o, "UvTilingY", meta.ORM.UvTilingY);
            }

            //~ Emissive
            if (src.Has("Emissive"))
            {
                const auto& e = src["Emissive"];
                meta.Emissive.IsTextureAttached = ReadFloat(e, "IsTextureAttached", meta.Emissive.IsTextureAttached);
                meta.Emissive.EmissiveIntensity = ReadFloat(e, "EmissiveIntensity", meta.Emissive.EmissiveIntensity);
                meta.Emissive.UvTilingX = ReadFloat(e, "UvTilingX", meta.Emissive.UvTilingX);
                meta.Emissive.UvTilingY = ReadFloat(e, "UvTilingY", meta.Emissive.UvTilingY);
            }

            //~ Opacity
            if (src.Has("Opacity"))
            {
                const auto& o = src["Opacity"];
                meta.Opacity.IsTextureAttached = ReadFloat(o, "IsTextureAttached", meta.Opacity.IsTextureAttached);
                meta.Opacity.AlphaMultiplier = ReadFloat(o, "AlphaMultiplier", meta.Opacity.AlphaMultiplier);
                meta.Opacity.AlphaCutoff = ReadFloat(o, "AlphaCutoff", meta.Opacity.AlphaCutoff);
            }

            //~ Height
            if (src.Has("Height"))
            {
                const auto& h = src["Height"];
                meta.Height.IsTextureAttached = ReadFloat(h, "IsTextureAttached", meta.Height.IsTextureAttached);
                meta.Height.HeightScale = ReadFloat(h, "HeightScale", meta.Height.HeightScale);
                meta.Height.ParallaxMinLayers = ReadFloat(h, "ParallaxMinLayers", meta.Height.ParallaxMinLayers);
                meta.Height.ParallaxMaxLayers = ReadFloat(h, "ParallaxMaxLayers", meta.Height.ParallaxMaxLayers);
            }

            //~ Singular
            if (src.Has("Singular"))
            {
                const auto& s = src["Singular"];
                meta.Singular.IsOcclusionAttached = ReadFloat(s, "IsOcclusionAttached", meta.Singular.IsOcclusionAttached);
                meta.Singular.IsRoughnessAttached = ReadFloat(s, "IsRoughnessAttached", meta.Singular.IsRoughnessAttached);
                meta.Singular.IsMetallicAttached = ReadFloat(s, "IsMetallicAttached", meta.Singular.IsMetallicAttached);

                meta.Singular.OcclusionStrength = ReadFloat(s, "OcclusionStrength", meta.Singular.OcclusionStrength);
                meta.Singular.RoughnessValue = ReadFloat(s, "RoughnessValue", meta.Singular.RoughnessValue);
                meta.Singular.MetallicValue = ReadFloat(s, "MetallicValue", meta.Singular.MetallicValue);

                meta.Singular.OcclusionTilingX = ReadFloat(s, "OcclusionTilingX", meta.Singular.OcclusionTilingX);
                meta.Singular.OcclusionTilingY = ReadFloat(s, "OcclusionTilingY", meta.Singular.OcclusionTilingY);
                meta.Singular.RoughnessTilingX = ReadFloat(s, "RoughnessTilingX", meta.Singular.RoughnessTilingX);
                meta.Singular.RoughnessTilingY = ReadFloat(s, "RoughnessTilingY", meta.Singular.RoughnessTilingY);
                meta.Singular.MetallicTilingX = ReadFloat(s, "MetallicTilingX", meta.Singular.MetallicTilingX);
                meta.Singular.MetallicTilingY = ReadFloat(s, "MetallicTilingY", meta.Singular.MetallicTilingY);
            }

            //~ Dye
            if (src.Has("Dye"))
            {
                const auto& d = src["Dye"];
                meta.Dye.IsEnabled = ReadFloat(d, "IsEnabled", meta.Dye.IsEnabled);
                meta.Dye.Strength = ReadFloat(d, "Strength", meta.Dye.Strength);
                ReadFloat3(d, "Color", meta.Dye.Color);
            }

            //~ Global
            if (src.Has("Global"))
            {
                const auto& g = src["Global"];
                meta.ForcedMipLevel = ReadFloat(g, "ForcedMipLevel", meta.ForcedMipLevel);
                meta.UseForcedMip = ReadFloat(g, "UseForcedMip", meta.UseForcedMip);
            }
        };

    //~ Saved as MetaInformation["0"],
    for (std::uint32_t i = 0;; ++i)
    {
        const std::string idx = std::to_string(i);
        if (!metaRoot->Has(idx))
            break;

        const JsonLoader& entry = (*metaRoot)[idx];

        if (!entry.Contains("Meta"))
            continue;

        ModelTextureMetaInformation meta{};
        ReadMeta(entry["Meta"], meta);

        m_cbMetaLazy[i] = meta;
    }
}

void kfe::KFEMeshSceneObject::Impl::LoadChildTextureInformation(const JsonLoader& loader) noexcept
{
    const JsonLoader* root = nullptr;

    if (loader.Has("TextureInformation"))
        root = &loader["TextureInformation"];
    else
        return;

    auto ReadPath = [](const JsonLoader& j, const char* key, std::string& outPath) noexcept
        {
            if (!j.Has(key))
                return;

            outPath = j[key].GetValue();
        };

    auto FillSlots = [&](const JsonLoader& texNode,
        std::array<std::string, (size_t)EModelTextureSlot::Count>& slots) noexcept
        {
            slots.fill(std::string{});

            ReadPath(texNode, "BaseColor", slots[(size_t)EModelTextureSlot::BaseColor]);
            ReadPath(texNode, "Normal", slots[(size_t)EModelTextureSlot::Normal]);
            ReadPath(texNode, "ORM", slots[(size_t)EModelTextureSlot::ORM]);
            ReadPath(texNode, "Emissive", slots[(size_t)EModelTextureSlot::Emissive]);
            ReadPath(texNode, "Opacity", slots[(size_t)EModelTextureSlot::Opacity]);
            ReadPath(texNode, "Height", slots[(size_t)EModelTextureSlot::Height]);

            ReadPath(texNode, "Occlusion", slots[(size_t)EModelTextureSlot::Occlusion]);
            ReadPath(texNode, "Roughness", slots[(size_t)EModelTextureSlot::Roughness]);
            ReadPath(texNode, "Metallic", slots[(size_t)EModelTextureSlot::Metallic]);

            ReadPath(texNode, "DyeMask", slots[(size_t)EModelTextureSlot::DyeMask]);
        };

    //~ If mesh isn't built yet: cache lazily
    if (!m_mesh.IsValid())
    {
        for (std::uint32_t i = 0;; ++i)
        {
            const std::string idx = std::to_string(i);
            if (!root->Has(idx))
                break;

            const JsonLoader& entry = (*root)[idx];
            if (!entry.Has("Textures"))
                continue;

            std::array<std::string, (size_t)EModelTextureSlot::Count> slots{};
            FillSlots(entry["Textures"], slots);

            m_cbTexLazy[i] = std::move(slots);
        }
        return;
    }

    auto& subs = m_mesh.GetSubmeshesMutable();
    const std::uint32_t subCount = static_cast<std::uint32_t>(subs.size());

    for (std::uint32_t i = 0;; ++i)
    {
        const std::string idx = std::to_string(i);
        if (!root->Has(idx))
            break;

        if (i >= subCount)
            continue;

        const JsonLoader& entry = (*root)[idx];
        if (!entry.Has("Textures"))
            continue;

        std::array<std::string, (size_t)EModelTextureSlot::Count> slots{};
        FillSlots(entry["Textures"], slots);

        auto& sm = subs[i];

        sm.SetBaseColor(slots[(size_t)EModelTextureSlot::BaseColor]);
        sm.SetNormal(slots[(size_t)EModelTextureSlot::Normal]);
        sm.SetORM(slots[(size_t)EModelTextureSlot::ORM]);
        sm.SetEmissive(slots[(size_t)EModelTextureSlot::Emissive]);
        sm.SetOpacity(slots[(size_t)EModelTextureSlot::Opacity]);
        sm.SetHeight(slots[(size_t)EModelTextureSlot::Height]);

        sm.SetOcclusion(slots[(size_t)EModelTextureSlot::Occlusion]);
        sm.SetRoughness(slots[(size_t)EModelTextureSlot::Roughness]);
        sm.SetMetallic(slots[(size_t)EModelTextureSlot::Metallic]);

        sm.SetDyeMask(slots[(size_t)EModelTextureSlot::DyeMask]);

        sm.m_bTextureDirty = true;
    }
}

void kfe::KFEMeshSceneObject::Impl::ImguiView(float dt)
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

    ImguiTextureMetaConfig(dt);
    ImguiChildTransformation (dt);
}

void kfe::KFEMeshSceneObject::Impl::ImguiChildTransformation(float deltaTime)
{
    ImGui::SeparatorText("Hierarchy Transform");

    const KFEModelNode* rootConst = m_mesh.GetRootNode();
    if (!rootConst)
    {
        ImGui::TextDisabled("No model root node.");
        return;
    }

    KFEModelNode* root = const_cast<KFEModelNode*>(rootConst);

    const float dt = (deltaTime > 0.0f) ? deltaTime : (1.0f / 60.0f);
    const float posSpeed = 1.0f * dt * 60.0f;
    const float rotSpeed = 15.0f * dt * 60.0f;
    const float scaleSpeed = 0.25f * dt * 60.0f;

    static KFEModelNode* s_selected = nullptr;
    if (!s_selected)
        s_selected = root;

    auto DrawNodeTreeRecursive = [&](auto&& self, KFEModelNode* node) -> void
        {
            if (!node)
                return;

            ImGui::PushID(node);

            const bool isSelected = (s_selected == node);

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanFullWidth;

            if (isSelected)
                flags |= ImGuiTreeNodeFlags_Selected;

            if (!node->HasChildren())
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

            const char* label = node->Name.empty() ? "(unnamed)" : node->Name.c_str();

            const bool opened = ImGui::TreeNodeEx(label, flags);

            if (ImGui::IsItemClicked())
                s_selected = node;

            if (node->HasChildren() && opened)
            {
                for (auto& c : node->Children)
                    self(self, c.get());

                ImGui::TreePop();
            }

            ImGui::PopID();
        };

    auto EditNodeTRS = [&](KFEModelNode* node) -> void
        {
            if (!node)
                return;

            ImGui::PushID(node);

            const char* label = node->Name.empty() ? "(unnamed)" : node->Name.c_str();
            ImGui::Text("Selected: %s", label);

            {
                bool enabled = node->IsEnabled();
                if (ImGui::Checkbox("Enabled", &enabled))
                    node->SetEnabled(enabled);
            }

            ImGui::Spacing();

            auto dragFloat3 = [](const char* label, DirectX::XMFLOAT3& v, float speed, float minV, float maxV) -> bool
                {
                    float tmp[3]{ v.x, v.y, v.z };
                    const bool changed = ImGui::DragFloat3(label, tmp, speed, minV, maxV, "%.4f");
                    if (changed)
                        v = { tmp[0], tmp[1], tmp[2] };
                    return changed;
                };

            DirectX::XMFLOAT3 p = node->GetPosition();
            DirectX::XMFLOAT3 r = node->GetRotation();
            DirectX::XMFLOAT3 s = node->GetScale();
            DirectX::XMFLOAT3 pv = node->GetPivot();

            bool dirty = false;

            dirty |= dragFloat3("Position", p, posSpeed, -100000.0f, 100000.0f);
            dirty |= dragFloat3("Rotation (deg)", r, rotSpeed, -36000.0f, 36000.0f);
            dirty |= dragFloat3("Scale", s, scaleSpeed, 0.0001f, 100000.0f);

            if (dirty)
                node->SetTRS(p, r, s);

            ImGui::Spacing();

            {
                float tmp[3]{ pv.x, pv.y, pv.z };
                if (ImGui::DragFloat3("Pivot", tmp, posSpeed, -100000.0f, 100000.0f, "%.4f"))
                    node->SetPivot({ tmp[0], tmp[1], tmp[2] });
            }

            ImGui::Spacing();

            if (ImGui::Button("Reset TRS"))
                node->ResetTransform();

            ImGui::SameLine();

            if (ImGui::Button("Pivot -> 0"))
                node->SetPivotZero();

            ImGui::PopID();
        };

    ImGui::Columns(2, nullptr, true);

    ImGui::TextDisabled("Nodes");
    ImGui::Separator();

    KFEModelNode* displayRoot = root;
    while (displayRoot &&
        displayRoot->Children.size() == 1 &&
        !displayRoot->HasMeshes())
    {
        displayRoot = displayRoot->Children[0].get();
    }

    DrawNodeTreeRecursive(DrawNodeTreeRecursive, root);

    if (!s_selected)
        s_selected = displayRoot;

    ImGui::NextColumn();

    ImGui::TextDisabled("Transform Editor");
    ImGui::Separator();
    EditNodeTRS(s_selected);

    ImGui::Columns(1);
}

void kfe::KFEMeshSceneObject::Impl::ImguiTextureMetaConfig(float dt)
{
    ImGui::SeparatorText("Submesh Texture + Meta");

    auto& subs = m_mesh.GetSubmeshesMutable();
    if (subs.empty())
    {
        ImGui::TextDisabled("No submeshes.");
        return;
    }

    auto CheckboxFloat01 = [](const char* label, float* v01) -> bool
        {
            bool b = (*v01 > 0.5f);
            if (ImGui::Checkbox(label, &b))
            {
                *v01 = b ? 1.0f : 0.0f;
                return true;
            }
            return false;
        };

    auto EditPath = [](const char* label, std::string& path, auto setter)
        {
            char buf[260]{};

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

            ImGui::SameLine();
            if (ImGui::Button("X"))
            {
                setter(std::string{});
            }
        };

    auto DrawBaseColorPair = [&](kfe::KFEModelSubmesh& sm) -> bool
        {
            bool metaDirty = false;

            if (ImGui::TreeNode("Base Color"))
            {
                if (ImGui::TreeNode("Base Color Texture"))
                {
                    EditPath("Path",
                        sm.m_srvs[static_cast<std::size_t>(EModelTextureSlot::BaseColor)].TexturePath,
                        [&sm](const std::string& p) { sm.SetBaseColor(p); });

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Meta Base Color"))
                {
                    auto& m = sm.m_textureMetaInformation.BaseColor;

                    metaDirty |= CheckboxFloat01("Attached", &m.IsTextureAttached);
                    metaDirty |= ImGui::DragFloat("Strength", &m.Strength, 0.01f, 0.0f, 10.0f);
                    metaDirty |= ImGui::DragFloat2("UV Tiling", &m.UvTilingX, 0.01f, 0.01f, 100.0f);

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            return metaDirty;
        };

    auto DrawNormalPair = [&](kfe::KFEModelSubmesh& sm) -> bool
        {
            bool metaDirty = false;

            if (ImGui::TreeNode("Normal"))
            {
                if (ImGui::TreeNode("Normal Texture"))
                {
                    EditPath("Path",
                        sm.m_srvs[static_cast<std::size_t>(EModelTextureSlot::Normal)].TexturePath,
                        [&sm](const std::string& p) { sm.SetNormal(p); });

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Meta Normal"))
                {
                    auto& n = sm.m_textureMetaInformation.Normal;

                    metaDirty |= CheckboxFloat01("Attached", &n.IsTextureAttached);
                    metaDirty |= ImGui::DragFloat("Normal Strength", &n.NormalStrength, 0.01f, 0.0f, 5.0f);
                    metaDirty |= ImGui::DragFloat2("UV Tiling", &n.UvTilingX, 0.01f, 0.01f, 100.0f);

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            return metaDirty;
        };

    auto DrawORMPair = [&](kfe::KFEModelSubmesh& sm) -> bool
        {
            bool metaDirty = false;

            if (ImGui::TreeNode("ORM"))
            {
                if (ImGui::TreeNode("ORM Texture"))
                {
                    EditPath("Path",
                        sm.m_srvs[static_cast<std::size_t>(EModelTextureSlot::ORM)].TexturePath,
                        [&sm](const std::string& p) { sm.SetORM(p); });

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Meta ORM"))
                {
                    auto& o = sm.m_textureMetaInformation.ORM;

                    metaDirty |= CheckboxFloat01("Attached", &o.IsTextureAttached);
                    metaDirty |= CheckboxFloat01("Mixed", &o.IsMixed);
                    metaDirty |= ImGui::DragFloat2("UV Tiling", &o.UvTilingX, 0.01f, 0.01f, 100.0f);

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            return metaDirty;
        };

    auto DrawEmissivePair = [&](kfe::KFEModelSubmesh& sm) -> bool
        {
            bool metaDirty = false;

            if (ImGui::TreeNode("Emissive"))
            {
                if (ImGui::TreeNode("Emissive Texture"))
                {
                    EditPath("Path",
                        sm.m_srvs[static_cast<std::size_t>(EModelTextureSlot::Emissive)].TexturePath,
                        [&sm](const std::string& p) { sm.SetEmissive(p); });

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Meta Emissive"))
                {
                    auto& e = sm.m_textureMetaInformation.Emissive;

                    metaDirty |= CheckboxFloat01("Attached", &e.IsTextureAttached);
                    metaDirty |= ImGui::DragFloat("Intensity", &e.EmissiveIntensity, 0.01f, 0.0f, 50.0f);
                    metaDirty |= ImGui::DragFloat2("UV Tiling", &e.UvTilingX, 0.01f, 0.01f, 100.0f);

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            return metaDirty;
        };

    auto DrawOpacityPair = [&](kfe::KFEModelSubmesh& sm) -> bool
        {
            bool metaDirty = false;

            if (ImGui::TreeNode("Opacity"))
            {
                if (ImGui::TreeNode("Opacity Texture"))
                {
                    EditPath("Path",
                        sm.m_srvs[static_cast<std::size_t>(EModelTextureSlot::Opacity)].TexturePath,
                        [&sm](const std::string& p) { sm.SetOpacity(p); });

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Meta Opacity"))
                {
                    auto& o = sm.m_textureMetaInformation.Opacity;

                    metaDirty |= CheckboxFloat01("Attached", &o.IsTextureAttached);
                    metaDirty |= ImGui::DragFloat("Alpha Multiplier", &o.AlphaMultiplier, 0.01f, 0.0f, 1.0f);
                    metaDirty |= ImGui::DragFloat("Alpha Cutoff", &o.AlphaCutoff, 0.01f, 0.0f, 1.0f);

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            return metaDirty;
        };

    auto DrawHeightPair = [&](kfe::KFEModelSubmesh& sm) -> bool
        {
            bool metaDirty = false;

            if (ImGui::TreeNode("Height"))
            {
                if (ImGui::TreeNode("Height Texture"))
                {
                    EditPath("Path",
                        sm.m_srvs[static_cast<std::size_t>(EModelTextureSlot::Height)].TexturePath,
                        [&sm](const std::string& p) { sm.SetHeight(p); });

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Meta Height"))
                {
                    auto& h = sm.m_textureMetaInformation.Height;

                    metaDirty |= CheckboxFloat01("Attached", &h.IsTextureAttached);
                    metaDirty |= ImGui::DragFloat("Height Scale", &h.HeightScale, 0.001f, 0.0f, 0.2f);
                    metaDirty |= ImGui::DragFloat("Min Layers", &h.ParallaxMinLayers, 1.0f, 1.0f, 64.0f);
                    metaDirty |= ImGui::DragFloat("Max Layers", &h.ParallaxMaxLayers, 1.0f, 1.0f, 128.0f);

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            return metaDirty;
        };

    auto DrawSplitORMPair = [&](kfe::KFEModelSubmesh& sm) -> bool
        {
            bool metaDirty = false;

            if (ImGui::TreeNode("Occlusion / Roughness / Metallic"))
            {
                if (ImGui::TreeNode("Occlusion Texture"))
                {
                    EditPath("Path",
                        sm.m_srvs[static_cast<std::size_t>(EModelTextureSlot::Occlusion)].TexturePath,
                        [&sm](const std::string& p) { sm.SetOcclusion(p); });

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Roughness Texture"))
                {
                    EditPath("Path",
                        sm.m_srvs[static_cast<std::size_t>(EModelTextureSlot::Roughness)].TexturePath,
                        [&sm](const std::string& p) { sm.SetRoughness(p); });

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Metallic Texture"))
                {
                    EditPath("Path",
                        sm.m_srvs[static_cast<std::size_t>(EModelTextureSlot::Metallic)].TexturePath,
                        [&sm](const std::string& p) { sm.SetMetallic(p); });

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Meta Occlusion / Roughness / Metallic"))
                {
                    auto& s = sm.m_textureMetaInformation.Singular;

                    metaDirty |= CheckboxFloat01("Occlusion Attached", &s.IsOcclusionAttached);
                    metaDirty |= CheckboxFloat01("Roughness Attached", &s.IsRoughnessAttached);
                    metaDirty |= CheckboxFloat01("Metallic Attached", &s.IsMetallicAttached);

                    metaDirty |= ImGui::DragFloat("Occlusion Strength", &s.OcclusionStrength, 0.01f, 0.0f, 5.0f);
                    metaDirty |= ImGui::DragFloat("Roughness", &s.RoughnessValue, 0.01f, 0.0f, 1.0f);
                    metaDirty |= ImGui::DragFloat("Metallic", &s.MetallicValue, 0.01f, 0.0f, 1.0f);

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            return metaDirty;
        };

    auto DrawDyePair = [&](kfe::KFEModelSubmesh& sm) -> bool
        {
            bool metaDirty = false;

            if (ImGui::TreeNode("Dye"))
            {
                if (ImGui::TreeNode("Dye Mask Texture"))
                {
                    EditPath("Path",
                        sm.m_srvs[static_cast<std::size_t>(EModelTextureSlot::DyeMask)].TexturePath,
                        [&sm](const std::string& p) { sm.SetDyeMask(p); });

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Meta Dye"))
                {
                    auto& d = sm.m_textureMetaInformation.Dye;

                    metaDirty |= CheckboxFloat01("Enabled", &d.IsEnabled);
                    metaDirty |= ImGui::DragFloat("Strength", &d.Strength, 0.01f, 0.0f, 5.0f);
                    metaDirty |= ImGui::ColorEdit3("Color", d.Color);

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            return metaDirty;
        };

    auto DrawGlobalMeta = [&](kfe::KFEModelSubmesh& sm) -> bool
        {
            bool metaDirty = false;

            if (ImGui::TreeNode("Global Meta"))
            {
                auto& meta = sm.m_textureMetaInformation;

                metaDirty |= CheckboxFloat01("Use Forced Mip", &meta.UseForcedMip);
                metaDirty |= ImGui::DragFloat("Forced Mip Level", &meta.ForcedMipLevel, 1.0f, 0.0f, 12.0f);

                ImGui::TreePop();
            }

            return metaDirty;
        };

    for (std::size_t i = 0; i < subs.size(); ++i)
    {
        auto& sm = subs[i];

        const std::string name = m_mesh.GetSubmeshName(static_cast<std::uint32_t>(i));
        const std::string header =
            name.empty()
            ? ("Submesh " + std::to_string(i))
            : (name + "##submesh_" + std::to_string(i));

        ImGui::PushID(static_cast<int>(i));

        if (!ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PopID();
            continue;
        }

        bool metaDirty = false;

        metaDirty |= DrawBaseColorPair(sm);
        metaDirty |= DrawNormalPair(sm);
        metaDirty |= DrawORMPair(sm);
        metaDirty |= DrawEmissivePair(sm);
        metaDirty |= DrawOpacityPair(sm);
        metaDirty |= DrawHeightPair(sm);
        metaDirty |= DrawSplitORMPair(sm);
        metaDirty |= DrawDyePair(sm);
        metaDirty |= DrawGlobalMeta(sm);

        if (metaDirty)
            sm.m_bMetaDirty = true;

        ImGui::PopID();
    }
}


#pragma endregion

