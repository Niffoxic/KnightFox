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
#include "engine/render_manager/scene/cube_scene.h"

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

#include <d3d12.h>
#include <vector>
#include <cstring>

#pragma region Impl_Definition

struct CubeVertex
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT3 Tangent;
    DirectX::XMFLOAT3 Bitangent;
    DirectX::XMFLOAT2 TexCoord;
    DirectX::XMFLOAT3 Color;

public:

    static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout()
    {
        std::vector<D3D12_INPUT_ELEMENT_DESC> layout;

        layout.push_back({ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
                           D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        layout.push_back({ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
                           D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        layout.push_back({ "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24,
                           D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        layout.push_back({ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36,
                           D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        layout.push_back({ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 48,
                           D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        layout.push_back({ "COLOR",     0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 56,
                           D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        return layout;
    }

    static constexpr UINT GetStride() noexcept
    {
        return sizeof(CubeVertex);
    }
};

class kfe::KEFCubeSceneObject::Impl
{
public:
    Impl(KEFCubeSceneObject* obj, const std::uint32_t multiple = 1)
        : m_pObject(obj), m_nVertexMultiples(multiple)
    {
    }
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
    bool BindTextureFromPath(KFEGraphicsCommandList* cmdList);

    void UpdateConstantBuffer(const KFE_UPDATE_OBJECT_DESC& desc);

    std::vector<CubeVertex>    GetVertices() const noexcept;
    std::vector<std::uint16_t> GetIndices() const noexcept;

    void SetTexture(const std::string& texture)
    {
        m_texturePath = texture;
        m_bTextureDirty = true;
    }

public:
    ECullMode m_cullMode      { ECullMode::None };
    EDrawMode m_drawMode      { EDrawMode::Triangle };
    bool      m_bPipelineDirty{ false };
    bool      m_bTextureDirty { true };

private:
    KEFCubeSceneObject* m_pObject{ nullptr };
    float               m_nTimeLived{ 0.0f };

    // Shaders
    std::string m_vertexShaderPath  { "shaders/cube/vertex.hlsl" };
    std::string m_pixelShaderPath   { "shaders/cube/pixel.hlsl" };
    std::string m_geometryShaderPath{};
    std::string m_hullShaderPath    {};
    std::string m_domainShaderPath  {};
    std::string m_computeShaderPath {};

    std::unique_ptr<KFERootSignature>  m_pRootSignature{ nullptr };

    // Geometry
    std::uint32_t m_nVertexMultiples{};

    std::unique_ptr<KFEStagingBuffer> m_pVBStaging{ nullptr };
    std::unique_ptr<KFEStagingBuffer> m_pIBStaging{ nullptr };
    std::unique_ptr<KFEVertexBuffer>  m_pVertexView{ nullptr };
    std::unique_ptr<KFEIndexBuffer>   m_pIndexView{ nullptr };

    // Constant buffer
    std::unique_ptr<KFEBuffer>         m_pCBBuffer{ nullptr };
    std::unique_ptr<KFEConstantBuffer> m_pCBV{ nullptr };

    // Pipeline
    std::unique_ptr<KFEPipelineState>  m_pPipeline{ nullptr };
    KFEDevice* m_pDevice{ nullptr };

    // Textures / sampling
    KFEResourceHeap* m_pResourceHeap{ nullptr };
    KFESamplerHeap* m_pSamplerHeap{ nullptr };

    std::string      m_texturePath{ "assets/textures/default.png" };
    KFETextureSRV* m_pTextureSrv{ nullptr };

    std::unique_ptr<KFESampler> m_pSampler{ nullptr };

    std::uint32_t m_srvIndex{ KFE_INVALID_INDEX };
    std::uint32_t m_samplerIndex{ KFE_INVALID_INDEX };
};

#pragma endregion

#pragma region CubeScene_Body

kfe::KEFCubeSceneObject::KEFCubeSceneObject()
    : m_impl(std::make_unique<kfe::KEFCubeSceneObject::Impl>(this))
{
    SetTypeName("KEFCubeSceneObject");
}

kfe::KEFCubeSceneObject::KEFCubeSceneObject(const std::uint32_t multiple)
    : m_impl(std::make_unique<kfe::KEFCubeSceneObject::Impl>(this, multiple))
{
}

kfe::KEFCubeSceneObject::~KEFCubeSceneObject() = default;

kfe::KEFCubeSceneObject::KEFCubeSceneObject(KEFCubeSceneObject&&) = default;
kfe::KEFCubeSceneObject& kfe::KEFCubeSceneObject::operator=(KEFCubeSceneObject&&) = default;

std::string kfe::KEFCubeSceneObject::GetName() const noexcept
{
    return "KEFCubeSceneObject";
}

std::string kfe::KEFCubeSceneObject::GetDescription() const noexcept
{
    return "A Cube Object that can be used for rendering debug cube for colliders";
}

void kfe::KEFCubeSceneObject::Update(const KFE_UPDATE_OBJECT_DESC& desc)
{
    m_impl->Update(desc);
}

_Use_decl_annotations_
bool kfe::KEFCubeSceneObject::Build(const KFE_BUILD_OBJECT_DESC& desc)
{
    if (m_impl->Build(desc))
    {
        m_bInitialized = true;
        return true;
    }
    return false;
}

bool kfe::KEFCubeSceneObject::Destroy()
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
void kfe::KEFCubeSceneObject::Render(const KFE_RENDER_OBJECT_DESC& desc)
{
    m_impl->Render(desc);
}

void kfe::KEFCubeSceneObject::ImguiView(float deltaTime)
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

void kfe::KEFCubeSceneObject::SetCullMode(const ECullMode mode)
{
    if (!m_impl) return;
    m_impl->m_cullMode = mode;
    m_impl->m_bPipelineDirty = true;
}

void kfe::KEFCubeSceneObject::SetCullMode(const std::string& mode)
{
    if (!m_impl) return;
    m_impl->m_cullMode = FromStringToCull(mode);
    m_impl->m_bPipelineDirty = true;
}

void kfe::KEFCubeSceneObject::SetDrawMode(const EDrawMode mode)
{
    if (!m_impl) return;
    m_impl->m_drawMode = mode;
    m_impl->m_bPipelineDirty = true;
}

void kfe::KEFCubeSceneObject::SetDrawMode(const std::string& mode)
{
    if (!m_impl) return;
    m_impl->m_drawMode = FromStringToDraw(mode);
    m_impl->m_bPipelineDirty = true;
}

kfe::ECullMode kfe::KEFCubeSceneObject::GetCullMode() const
{
    return m_impl ? m_impl->m_cullMode : ECullMode::Back;
}

std::string kfe::KEFCubeSceneObject::GetCullModeString() const
{
    return m_impl ? ToString(m_impl->m_cullMode) : "Back";
}

kfe::EDrawMode kfe::KEFCubeSceneObject::GetDrawMode() const
{
    return m_impl ? m_impl->m_drawMode : EDrawMode::Triangle;
}

std::string kfe::KEFCubeSceneObject::GetDrawModeString() const
{
    return m_impl ? ToString(m_impl->m_drawMode) : "Triangle";
}

void kfe::IKFESceneObject::ImguiTransformView(float)
{
    if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    DirectX::XMFLOAT3 pos = GetPosition();
    if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
    {
        SetPosition(pos);
    }

    DirectX::XMFLOAT4 ori = GetOrientation();
    float q[4] = { ori.x, ori.y, ori.z, ori.w };

    if (ImGui::DragFloat4("Rotation (quat)", q, 0.01f))
    {
        DirectX::XMVECTOR qv = DirectX::XMVectorSet(q[0], q[1], q[2], q[3]);
        qv = DirectX::XMQuaternionNormalize(qv);
        DirectX::XMStoreFloat4(&ori, qv);
        SetOrientation(ori);
    }

    DirectX::XMFLOAT3 scl = GetScale();
    if (ImGui::DragFloat3("Scale", &scl.x, 0.01f))
    {
        SetScale(scl);
    }

    ImGui::TextDisabled("Tip: Rotation is edited as a normalized quaternion.");
}

void kfe::KEFCubeSceneObject::SetVertexShader(const std::string& path)
{
    if (!m_impl) return;
    m_impl->SetVertexShaderPath(path);
}

std::string kfe::KEFCubeSceneObject::VertexShader() const
{
    return m_impl ? m_impl->GetVertexShaderPath() : std::string{};
}

void kfe::KEFCubeSceneObject::SetPixelShader(const std::string& path)
{
    if (!m_impl) return;
    m_impl->SetPixelShaderPath(path);
}

std::string kfe::KEFCubeSceneObject::PixelShader() const
{
    return m_impl ? m_impl->GetPixelShaderPath() : std::string{};
}

void kfe::KEFCubeSceneObject::SetGeometryShader(const std::string& path)
{
    if (!m_impl) return;
    m_impl->SetGeometryShaderPath(path);
}

std::string kfe::KEFCubeSceneObject::GeometryShader() const
{
    return m_impl ? m_impl->GetGeometryShaderPath() : std::string{};
}

void kfe::KEFCubeSceneObject::SetHullShader(const std::string& path)
{
    if (!m_impl) return;
    m_impl->SetHullShaderPath(path);
}

std::string kfe::KEFCubeSceneObject::HullShader() const
{
    return m_impl ? m_impl->GetHullShaderPath() : std::string{};
}

void kfe::KEFCubeSceneObject::SetDomainShader(const std::string& path)
{
    if (!m_impl) return;
    m_impl->SetDomainShaderPath(path);
}

std::string kfe::KEFCubeSceneObject::DomainShader() const
{
    return m_impl ? m_impl->GetDomainShaderPath() : std::string{};
}

void kfe::KEFCubeSceneObject::SetComputeShader(const std::string& path)
{
    if (!m_impl) return;
    m_impl->SetComputeShaderPath(path);
}

std::string kfe::KEFCubeSceneObject::ComputeShader() const
{
    return m_impl ? m_impl->GetComputeShaderPath() : std::string{};
}

JsonLoader kfe::KEFCubeSceneObject::GetJsonData() const
{
    JsonLoader root{};
    root["Transformation"] = GetTransformJsonData();
    root["Properties"] = m_impl->GetJsonData();
    return root;
}

void kfe::KEFCubeSceneObject::LoadFromJson(const JsonLoader& loader)
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

void kfe::KEFCubeSceneObject::Impl::Update(const KFE_UPDATE_OBJECT_DESC& desc)
{
    m_nTimeLived += desc.deltaTime;
    UpdateConstantBuffer(desc);
}

bool kfe::KEFCubeSceneObject::Impl::Build(_In_ const KFE_BUILD_OBJECT_DESC& desc)
{
    m_pDevice       = desc.Device;
    m_pResourceHeap = desc.ResourceHeap;
    m_pSamplerHeap  = desc.SamplerHeap;

    BindTextureFromPath(desc.CommandList);
    if (!BuildSampler(desc)) return false;
    if (!BuildGeometry(desc)) return false;
    if (!BuildRootSignature(desc)) return false;
    if (!BuildPipeline(desc.Device)) return false;
    if (!BuildConstantBuffer(desc)) return false;

    LOG_SUCCESS("Cube Built!");
    return true;
}

bool kfe::KEFCubeSceneObject::Impl::Destroy()
{
    if (m_pVertexView)  m_pVertexView->Destroy();
    if (m_pIndexView)   m_pIndexView->Destroy();
    if (m_pVBStaging)   m_pVBStaging->Destroy();
    if (m_pIBStaging)   m_pIBStaging->Destroy();

    if (m_pCBV)      m_pCBV->Destroy();
    if (m_pCBBuffer) m_pCBBuffer->Destroy();

    if (m_pPipeline)      m_pPipeline->Destroy();
    if (m_pRootSignature) m_pRootSignature->Destroy();

    if (m_pSampler)
    {
        m_pSampler->Destroy();
    }

    m_pVertexView.reset();
    m_pIndexView.reset();
    m_pVBStaging.reset();
    m_pIBStaging.reset();
    m_pCBV.reset();
    m_pCBBuffer.reset();
    m_pPipeline.reset();
    m_pRootSignature.reset();
    m_pSampler.reset();

    m_pTextureSrv = nullptr;
    m_pResourceHeap = nullptr;
    m_pSamplerHeap = nullptr;
    m_pDevice = nullptr;

    m_srvIndex = KFE_INVALID_INDEX;
    m_samplerIndex = KFE_INVALID_INDEX;

    return true;
}

void kfe::KEFCubeSceneObject::Impl::Render(_In_ const KFE_RENDER_OBJECT_DESC& desc)
{
    if (m_bTextureDirty)
    {
        BindTextureFromPath(desc.CommandList);
    }
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
    if (!cmdListObj || !cmdListObj->GetNative()) return;
    auto* cmdList = cmdListObj->GetNative();

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

        if (m_pTextureSrv) 
        {
            m_srvIndex = m_pTextureSrv->GetDescriptorIndex();
        }
        if (m_srvIndex != KFE_INVALID_INDEX && m_pResourceHeap->IsValidIndex(m_srvIndex))
        {
            D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_pResourceHeap->GetGPUHandle(m_srvIndex);
            cmdList->SetGraphicsRootDescriptorTable(1u, srvHandle);
        }
        else
        {
            LOG_WARNING("KEFCubeSceneObject::Impl::Render: SRV index invalid, skipping bind.");
        }
    }

    auto addr = static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(m_pCBV->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(0u, addr);

    auto vertexView = m_pVertexView->GetView();
    auto indexView = m_pIndexView->GetView();

    cmdList->IASetVertexBuffers(0u, 1u, &vertexView);
    cmdList->IASetIndexBuffer(&indexView);

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

    cmdList->DrawIndexedInstanced(
        m_pIndexView->GetIndexCount(),
        1u, 0u, 0u, 0u);
}

_Use_decl_annotations_
bool kfe::KEFCubeSceneObject::Impl::BuildGeometry(const KFE_BUILD_OBJECT_DESC& desc)
{
    std::vector<CubeVertex>    vertices = GetVertices();
    std::vector<std::uint16_t> indices = GetIndices();

    const std::uint32_t vbSize = static_cast<std::uint32_t>(vertices.size() * sizeof(CubeVertex));
    const std::uint32_t ibSize = static_cast<std::uint32_t>(indices.size() * sizeof(std::uint16_t));

    m_pVBStaging = std::make_unique<KFEStagingBuffer>();

    KFE_STAGING_BUFFER_CREATE_DESC vertexBuffer{};
    vertexBuffer.Device = desc.Device;
    vertexBuffer.SizeInBytes = vbSize;

    if (!m_pVBStaging->Initialize(vertexBuffer))
    {
        LOG_ERROR("Failed to initialize Vertex Stagging Buffer!");
        return false;
    }

    if (!m_pVBStaging->WriteBytes(vertices.data(), vbSize, 0u))
    {
        LOG_ERROR("Failed to Write on Vertex Stagging Buffer!");
        return false;
    }

    if (!m_pVBStaging->RecordUploadToDefault(
        desc.CommandList->GetNative(),
        vbSize, 0u, 0u))
    {
        LOG_ERROR("Failed to Record: Vertex Stagging Buffer!");
        return false;
    }

    m_pIBStaging = std::make_unique<KFEStagingBuffer>();

    KFE_STAGING_BUFFER_CREATE_DESC indexBuffer{};
    indexBuffer.Device = desc.Device;
    indexBuffer.SizeInBytes = ibSize;

    if (!m_pIBStaging->Initialize(indexBuffer))
    {
        LOG_ERROR("Failed to initialize: index Stagging Buffer!");
        return false;
    }

    if (!m_pIBStaging->WriteBytes(indices.data(), ibSize, 0u))
    {
        LOG_ERROR("Failed to write: index Stagging Buffer!");
        return false;
    }

    if (!m_pIBStaging->RecordUploadToDefault(
        desc.CommandList->GetNative(),
        ibSize, 0u, 0u))
    {
        LOG_ERROR("Failed to Record: Index Stagging Buffer!");
        return false;
    }

    KFEBuffer* vbDefault = m_pVBStaging->GetDefaultBuffer();
    KFEBuffer* ibDefault = m_pIBStaging->GetDefaultBuffer();

    if (!vbDefault || !vbDefault->GetNative())
    {
        LOG_ERROR("InitializeTestTriangle: VB default buffer is null.");
        return false;
    }

    if (!ibDefault || !ibDefault->GetNative())
    {
        LOG_ERROR("InitializeTestTriangle: IB default buffer is null.");
        return false;
    }

    D3D12_RESOURCE_BARRIER barriers[2]{};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barriers[0].Transition.pResource = vbDefault->GetNative();
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barriers[1].Transition.pResource = ibDefault->GetNative();
    barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;

    auto* cmdList = desc.CommandList->GetNative();
    cmdList->ResourceBarrier(2u, barriers);

    m_pVertexView = std::make_unique<KFEVertexBuffer>();

    KFE_VERTEX_BUFFER_CREATE_DESC vertexView{};
    vertexView.DebugName = "Cube";
    vertexView.Device = desc.Device;
    vertexView.OffsetInBytes = 0u;
    vertexView.ResourceBuffer = vbDefault;
    vertexView.StrideInBytes = CubeVertex::GetStride();

    if (!m_pVertexView->Initialize(vertexView))
    {
        LOG_ERROR("Failed to Build Vertex View!");
        return false;
    }

    m_pIndexView = std::make_unique<KFEIndexBuffer>();

    KFE_INDEX_BUFFER_CREATE_DESC indexView{};
    indexView.Device = desc.Device;
    indexView.Format = DXGI_FORMAT_R16_UINT;
    indexView.OffsetInBytes = 0u;
    indexView.ResourceBuffer = ibDefault;

    if (!m_pIndexView->Initialize(indexView))
    {
        LOG_ERROR("Failed to Build Index View!");
        return false;
    }

    return true;
}

_Use_decl_annotations_
bool kfe::KEFCubeSceneObject::Impl::BuildConstantBuffer(const KFE_BUILD_OBJECT_DESC& desc)
{
    m_pCBBuffer = std::make_unique<KFEBuffer>();

    std::uint32_t bytes = static_cast<std::uint32_t>(sizeof(KFE_COMMON_VERTEX_AND_PIXEL_CB_DESC));
    bytes = kfe_helpers::AlignTo256(bytes);

    KFE_CREATE_BUFFER_DESC buffer{};
    buffer.Device = desc.Device;
    buffer.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    buffer.InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    buffer.ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
    buffer.SizeInBytes = bytes;

    if (!m_pCBBuffer->Initialize(buffer))
    {
        LOG_ERROR("Failed to build constant buffer!");
        return false;
    }

    m_pCBV = std::make_unique<KFEConstantBuffer>();

    KFE_CONSTANT_BUFFER_CREATE_DESC view{};
    view.Device = desc.Device;
    view.OffsetInBytes = 0u;
    view.ResourceBuffer = m_pCBBuffer.get();
    view.ResourceHeap = desc.ResourceHeap;
    view.SizeInBytes = bytes;

    if (!m_pCBV->Initialize(view))
    {
        LOG_ERROR("Failed to build constant buffer View!");
        return false;
    }

    LOG_SUCCESS("Cube Constant Buffer Created!");

    return true;
}

_Use_decl_annotations_
bool kfe::KEFCubeSceneObject::Impl::BuildRootSignature(const KFE_BUILD_OBJECT_DESC& desc)
{
    m_pRootSignature = std::make_unique<KFERootSignature>();

    D3D12_DESCRIPTOR_RANGE srvRange{};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1u;
    srvRange.BaseShaderRegister = 0u;
    srvRange.RegisterSpace = 0u;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER params[2]{};

    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    params[0].Descriptor.ShaderRegister = 0u;
    params[0].Descriptor.RegisterSpace = 0u;

    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    params[1].DescriptorTable.NumDescriptorRanges = 1u;
    params[1].DescriptorTable.pDescriptorRanges = &srvRange;

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
    staticSampler.ShaderRegister = 0;   // s0
    staticSampler.RegisterSpace = 0;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    KFE_RG_CREATE_DESC root{};
    root.Device = desc.Device;
    root.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    root.NumRootParameters = 2u;
    root.RootParameters = params;
    root.NumStaticSamplers = 1u;
    root.StaticSamplers = &staticSampler;

    if (!m_pRootSignature->Initialize(root))
    {
        LOG_ERROR("Failed to Create Root Signature!");
        return false;
    }

    m_pRootSignature->SetDebugName(L"Cube Scene Signature");
    LOG_SUCCESS("Cube Root Signature Created!");
    return true;
}

bool kfe::KEFCubeSceneObject::Impl::BuildPipeline(KFEDevice* device)
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

    auto layout = CubeVertex::GetInputLayout();
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

    raster.FrontCounterClockwise = FALSE;
    raster.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    raster.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    raster.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    raster.DepthClipEnable = TRUE;
    raster.MultisampleEnable = FALSE;
    raster.AntialiasedLineEnable = FALSE;
    raster.ForcedSampleCount = 0u;
    raster.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

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

bool kfe::KEFCubeSceneObject::Impl::BuildSampler(const KFE_BUILD_OBJECT_DESC& desc)
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

    sdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sdesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sdesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sdesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sdesc.MipLODBias = 0.0f;
    sdesc.MaxAnisotropy = 1u;
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

bool kfe::KEFCubeSceneObject::Impl::BindTextureFromPath(KFEGraphicsCommandList* cmdList)
{
    if (m_texturePath.empty())
    {
        m_pTextureSrv = nullptr;
        m_srvIndex = KFE_INVALID_INDEX;
        LOG_ERROR("TEXTURE PATH IS EMPTY!");
        return false;
    }

    if (!kfe_helpers::IsFile(m_texturePath))
    {
        LOG_ERROR("Texture Path: {}, Does not exits!");
        return false;
    }

    auto& pool = KFEImagePool::Instance();

    KFETextureSRV* srv = pool.GetImageSrv(m_texturePath, cmdList);
    if (!srv)
    {
        LOG_ERROR("KFailed to get SRV for '{}'", m_texturePath);
        m_pTextureSrv = nullptr;
        m_srvIndex = KFE_INVALID_INDEX;
        return false;
    }

    m_pTextureSrv = srv;
    m_srvIndex = srv->GetDescriptorIndex();
    m_bTextureDirty = false;
    LOG_SUCCESS("Cube bound texture '{}' at SRV index {}", m_texturePath, m_srvIndex);
    return true;
}

void kfe::KEFCubeSceneObject::Impl::UpdateConstantBuffer(const KFE_UPDATE_OBJECT_DESC& desc)
{
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

std::vector<CubeVertex> kfe::KEFCubeSceneObject::Impl::GetVertices() const noexcept
{
    using namespace DirectX;

    std::vector<CubeVertex> v;

    const XMFLOAT3 normals[6] =
    {
        {  0,  0,  1 }, // Front
        {  0,  0, -1 }, // Back
        {  0,  1,  0 }, // Top
        {  0, -1,  0 }, // Bottom
        {  1,  0,  0 }, // Right
        { -1,  0,  0 }  // Left
    };

    const XMFLOAT3 tangents[6] =
    {
        { 1, 0, 0 },  // Front
        { -1, 0, 0 }, // Back
        { 1, 0, 0 },  // Top
        { 1, 0, 0 },  // Bottom
        { 0, 0, -1 }, // Right
        { 0, 0,  1 }  // Left
    };

    const XMFLOAT3 bitangents[6] =
    {
        { 0, 1, 0 }, // Front
        { 0, 1, 0 }, // Back
        { 0, 0, 1 }, // Top
        { 0, 0,-1 }, // Bottom
        { 0, 1, 0 }, // Right
        { 0, 1, 0 }  // Left
    };

    const XMFLOAT3 p[] =
    {
        {-0.5f, -0.5f, -0.5f}, // 0
        {-0.5f,  0.5f, -0.5f}, // 1
        { 0.5f,  0.5f, -0.5f}, // 2
        { 0.5f, -0.5f, -0.5f}, // 3
        {-0.5f, -0.5f,  0.5f}, // 4
        {-0.5f,  0.5f,  0.5f}, // 5
        { 0.5f,  0.5f,  0.5f}, // 6
        { 0.5f, -0.5f,  0.5f}  // 7
    };

    auto addFace = [&](int a, int b, int c, int d, int face)
        {
            XMFLOAT2 uvTL = { 0,0 };
            XMFLOAT2 uvBL = { 0,1 };
            XMFLOAT2 uvTR = { 1,0 };
            XMFLOAT2 uvBR = { 1,1 };

            XMFLOAT3 col = { 1,1,1 };

            v.push_back({ p[a], normals[face], tangents[face], bitangents[face], uvTL, col });
            v.push_back({ p[b], normals[face], tangents[face], bitangents[face], uvBL, col });
            v.push_back({ p[c], normals[face], tangents[face], bitangents[face], uvTR, col });

            v.push_back({ p[c], normals[face], tangents[face], bitangents[face], uvTR, col });
            v.push_back({ p[b], normals[face], tangents[face], bitangents[face], uvBL, col });
            v.push_back({ p[d], normals[face], tangents[face], bitangents[face], uvBR, col });
        };

    addFace(5, 4, 6, 7, 0); // Front
    addFace(0, 1, 3, 2, 1); // Back
    addFace(1, 5, 2, 6, 2); // Top
    addFace(0, 4, 3, 7, 3); // Bottom
    addFace(6, 5, 2, 1, 4); // Right
    addFace(4, 5, 0, 1, 5); // Left

    return v;
}

std::vector<std::uint16_t> kfe::KEFCubeSceneObject::Impl::GetIndices() const noexcept
{
    std::vector<uint16_t> i(36);
    for (uint16_t n = 0; n < 36; ++n)
        i[n] = n;
    return i;
}

void kfe::KEFCubeSceneObject::Impl::SetVertexShaderPath(const std::string& path) noexcept
{
    if (m_vertexShaderPath == path)
        return;

    m_vertexShaderPath = path;
    m_bPipelineDirty = true;
}

void kfe::KEFCubeSceneObject::Impl::SetPixelShaderPath(const std::string& path) noexcept
{
    if (m_pixelShaderPath == path)
        return;

    m_pixelShaderPath = path;
    m_bPipelineDirty = true;
}

void kfe::KEFCubeSceneObject::Impl::SetGeometryShaderPath(const std::string& path) noexcept
{
    if (m_geometryShaderPath == path)
        return;

    m_geometryShaderPath = path;
    m_bPipelineDirty = true;
}

void kfe::KEFCubeSceneObject::Impl::SetHullShaderPath(const std::string& path) noexcept
{
    if (m_hullShaderPath == path)
        return;

    m_hullShaderPath = path;
    m_bPipelineDirty = true;
}

void kfe::KEFCubeSceneObject::Impl::SetDomainShaderPath(const std::string& path) noexcept
{
    if (m_domainShaderPath == path)
        return;

    m_domainShaderPath = path;
    m_bPipelineDirty = true;
}

void kfe::KEFCubeSceneObject::Impl::SetComputeShaderPath(const std::string& path) noexcept
{
    if (m_computeShaderPath == path)
        return;

    m_computeShaderPath = path;
    m_bPipelineDirty = true;
}

JsonLoader kfe::KEFCubeSceneObject::Impl::GetJsonData() const noexcept
{
    JsonLoader root{};

    root["CullMode"] = ToString(m_cullMode);
    root["DrawMode"] = ToString(m_drawMode);

    root["VertexShader"] = m_vertexShaderPath;
    root["PixelShader"] = m_pixelShaderPath;
    root["GeometryShader"] = m_geometryShaderPath;
    root["HullShader"] = m_hullShaderPath;
    root["DomainShader"] = m_domainShaderPath;
    root["ComputeShader"] = m_computeShaderPath;

    root["TexturePath"] = m_texturePath;

    return root;
}

void kfe::KEFCubeSceneObject::Impl::LoadFromJson(const JsonLoader& loader) noexcept
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

    if (loader.Contains("TexturePath"))
    {
        SetTexture(loader["TexturePath"].GetValue());
    }
}

void kfe::KEFCubeSceneObject::Impl::ImguiView(float)
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
            int currentCullIndex = 2; // None default
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

            EditPath("Vertex Shader", m_vertexShaderPath,
                [this](const std::string& p) { SetVertexShaderPath(p); m_bPipelineDirty = true; });

            EditPath("Pixel Shader", m_pixelShaderPath,
                [this](const std::string& p) { SetPixelShaderPath(p); m_bPipelineDirty = true; });

            EditPath("Geometry Shader", m_geometryShaderPath,
                [this](const std::string& p) { SetGeometryShaderPath(p); m_bPipelineDirty = true; });

            EditPath("Hull Shader", m_hullShaderPath,
                [this](const std::string& p) { SetHullShaderPath(p); m_bPipelineDirty = true; });

            EditPath("Domain Shader", m_domainShaderPath,
                [this](const std::string& p) { SetDomainShaderPath(p); m_bPipelineDirty = true; });

            EditPath("Compute Shader", m_computeShaderPath,
                [this](const std::string& p) { SetComputeShaderPath(p); m_bPipelineDirty = true; });

            ImGui::TreePop();
        }

        ImGui::Separator();

        if (ImGui::TreeNode("Texture Binding"))
        {
            auto ApplyTexturePath = [this]()
                {
                    m_bTextureDirty = true;
                };

            char texBuf[260];
            std::memset(texBuf, 0, sizeof(texBuf));
            {
                const size_t maxCopy = sizeof(texBuf) - 1;
                const size_t count = std::min(m_texturePath.size(), maxCopy);
                m_texturePath.copy(texBuf, count);
                texBuf[count] = '\0';
            }

            if (ImGui::InputText("Texture Path", texBuf, sizeof(texBuf)))
            {
                m_texturePath = std::string{ texBuf };
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
                        m_texturePath = pathUtf8;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (ImGui::Button("Apply Texture"))
            {
                ApplyTexturePath();
            }
            ImGui::TreePop();
        }

        ImGui::Separator();
        ImGui::TextDisabled("Pipeline dirty: %s", m_bPipelineDirty ? "Yes" : "No");
    }
}

#pragma endregion
