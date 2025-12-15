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

#include <d3d12.h>
#include <vector>
#include <cstring>
#include <unordered_map>
#include <map>
#include <array>

#include "engine/render_manager/api/frame_cb.h"
#include "engine/render_manager/shadow/shadow_map.h"
#include "engine/render_manager/assets_library/model/geometry.h"
#include "engine/render_manager/assets_library/model/model.h"

#pragma region Impl_Definition


class kfe::KEFCubeSceneObject::Impl
{
public:
    Impl(KEFCubeSceneObject* obj)
        : m_pObject(obj)
    {
        for (auto& entry : m_srvs)
        {
            entry.TexturePath = {};
            entry.TextureSrv = nullptr;
            entry.ResourceHandle = KFE_INVALID_INDEX;
            entry.ReservedSlot = 0u;
        }
    }
    ~Impl() = default;

    void Update(const KFE_UPDATE_OBJECT_DESC& desc);
    bool Build(_In_ const KFE_BUILD_OBJECT_DESC& desc);
    bool InitShadowPipeline(KFEDevice* device);

    bool Destroy();

    void Render    (_In_ const KFE_RENDER_OBJECT_DESC& desc);
    void ShadowPass(_In_ const KFE_RENDER_OBJECT_DESC& desc);

    JsonLoader GetJsonData() const                      noexcept;
    void       LoadFromJson(const JsonLoader& loader)   noexcept;

    void ImguiViewHeader(float deltaTime);
    void ImguiView(float deltaTime);

private:
    bool BuildGeometry      (_In_ const KFE_BUILD_OBJECT_DESC& desc);
    bool BuildConstantBuffer(_In_ const KFE_BUILD_OBJECT_DESC& desc);
    bool BindTextureFromPath(ID3D12GraphicsCommandList* cmdList);

    void UpdateConstantBuffer(const KFE_UPDATE_OBJECT_DESC& desc);

    std::vector<KFEVertexOnly> GetVertices() const noexcept;
    std::vector<std::uint16_t> GetIndices () const noexcept;

    void SetTexture(EModelTextureSlot tex, const std::string& path)
    {
        auto index = static_cast<std::size_t>(tex);

        auto& data          = m_srvs[index];
        data.TexturePath    = path;
        data.TextureSrv     = nullptr;
        data.ResourceHandle = KFE_INVALID_INDEX;
        data.Dirty          = true;
        m_bTextureDirty     = true;
    }

    //~ Core PBR
    void SetBaseColorTexture(const std::string& p) { SetTexture(EModelTextureSlot::BaseColor, p);  }
    void SetNormalMap   (const std::string& p)     { SetTexture(EModelTextureSlot::Normal, p);     }
    void SetORMTexture(const std::string& p)       { SetTexture(EModelTextureSlot::ORM, p);        }
    void SetEmissiveTexture(const std::string& p)  { SetTexture(EModelTextureSlot::Emissive, p);   }

    //~ Separate PBR
    void SetRoughnessTexture(const std::string& p)  { SetTexture(EModelTextureSlot::Roughness, p);  }
    void SetMetallicTexture(const std::string& p)   { SetTexture(EModelTextureSlot::Metallic, p);   }
    void SetOcclusionTexture(const std::string& p)  { SetTexture(EModelTextureSlot::Occlusion, p);  }

    //~ Transparency
    void SetOpacityTexture(const std::string& p) { SetTexture(EModelTextureSlot::Opacity, p);    }

    //~ Height
    void SetHeightMap   (const std::string& p)        { SetTexture(EModelTextureSlot::Height, p);       }
    void SetDisplacementTexture(const std::string& p) { SetTexture(EModelTextureSlot::Displacement, p); }

    //~ Specular
    void SetSpecularTexture(const std::string& p)   { SetTexture(EModelTextureSlot::Specular, p);     }
    void SetGlossinessTexture(const std::string& p) { SetTexture(EModelTextureSlot::Glossiness, p);   }
    void SetDetailNormalMap(const std::string& p)   { SetTexture(EModelTextureSlot::DetailNormal, p); }

    //~ bind shadow
    void BindShadowMapSRV(KFEResourceHeap* heap,
                          KFEShadowMap* shadowMap) noexcept;
public:
    bool      m_bTextureDirty { true };

private:
    KEFCubeSceneObject* m_pObject{ nullptr };
    float               m_nTimeLived{ 0.0f };

    std::unique_ptr<KFEStagingBuffer> m_pVBStaging{ nullptr };
    std::unique_ptr<KFEStagingBuffer> m_pIBStaging{ nullptr };
    std::unique_ptr<KFEVertexBuffer>  m_pVertexView{ nullptr };
    std::unique_ptr<KFEIndexBuffer>   m_pIndexView{ nullptr };

    //~ Meta information
    KFEFrameConstantBuffer      m_metaFrameCB{};
    ModelTextureMetaInformation m_metaInformation{};
    KFEResourceHeap*            m_pResourceHeap{ nullptr };
    KFEDevice*                  m_pDevice      { nullptr };
    std::uint32_t               m_subdivisionLevel{ 10u };
    bool                        m_bDirtyGeometry{ false };

    //~ Textures
    struct SrvData
    {
        std::string    TexturePath;
        KFETextureSRV* TextureSrv;
        std::uint32_t  ResourceHandle;
        std::uint32_t  ReservedSlot;
        bool           Dirty{ false };
    };
    std::array<SrvData, static_cast<std::size_t>(EModelTextureSlot::Count)> m_srvs;
    std::uint32_t m_baseSrvIndex{ KFE_INVALID_INDEX };
};

#pragma endregion

#pragma region CubeScene_Body

kfe::KEFCubeSceneObject::KEFCubeSceneObject()
    : m_impl(std::make_unique<kfe::KEFCubeSceneObject::Impl>(this))
{
    SetTypeName("KEFCubeSceneObject");
}

kfe::KEFCubeSceneObject::~KEFCubeSceneObject() = default;

std::string kfe::KEFCubeSceneObject::GetName() const noexcept
{
    return "KEFCubeSceneObject";
}

std::string kfe::KEFCubeSceneObject::GetDescription() const noexcept
{
    return "A Cube Object that can be used for rendering debug cube for colliders";
}

void kfe::KEFCubeSceneObject::ChildUpdate(const KFE_UPDATE_OBJECT_DESC& desc)
{
    m_impl->Update(desc);
}

_Use_decl_annotations_
bool kfe::KEFCubeSceneObject::ChildBuild(const KFE_BUILD_OBJECT_DESC& desc)
{
    if (m_impl->Build(desc))
    {
        return true;
    }
    return false;
}

_Use_decl_annotations_
void kfe::KEFCubeSceneObject::ChildMainPass(const KFE_RENDER_OBJECT_DESC& desc)
{
    m_impl->Render(desc);
}

void kfe::KEFCubeSceneObject::ChildShadowPass(const KFE_RENDER_OBJECT_DESC& desc)
{
    m_impl->ShadowPass(desc);
}

void kfe::KEFCubeSceneObject::ChildImguiViewHeader(float deltaTime)
{
    m_impl->ImguiViewHeader(deltaTime);
}

void kfe::KEFCubeSceneObject::ChildImguiViewBody(float deltaTime)
{
    m_impl->ImguiView(deltaTime);
}

bool kfe::KEFCubeSceneObject::ChildDestroy()
{
    return m_impl->Destroy();
}

JsonLoader kfe::KEFCubeSceneObject::ChildGetJsonData() const
{
    return m_impl->GetJsonData();
}

void kfe::KEFCubeSceneObject::ChildLoadFromJson(const JsonLoader& loader)
{
    m_impl->LoadFromJson(loader);
}

#pragma endregion

#pragma region Impl_body

void kfe::KEFCubeSceneObject::Impl::Update(const KFE_UPDATE_OBJECT_DESC& desc)
{
    m_nTimeLived += desc.DeltaTime;
    UpdateConstantBuffer(desc);
}

_Use_decl_annotations_
bool kfe::KEFCubeSceneObject::Impl::Build(const KFE_BUILD_OBJECT_DESC& desc)
{
    m_pResourceHeap = desc.ResourceHeap;
    m_pDevice       = desc.Device;

    if (!m_pResourceHeap)
    {
        LOG_ERROR("ResourceHeap is null.");
        return false;
    }
    
    //~ Allocate Block
    const std::uint32_t srvCount = static_cast<std::uint32_t>(EModelTextureSlot::Count);
    m_baseSrvIndex = m_pResourceHeap->Allocate(srvCount);

    for (std::uint32_t i = 0; i < srvCount; ++i)
    {
        m_srvs[i].ReservedSlot = m_baseSrvIndex + i;
    }

    if (!BindTextureFromPath(desc.CommandList))
    {
        LOG_ERROR("BindTextureFromPath failed.");
        return false;
    }

    if (!BuildConstantBuffer(desc))
        return false;

    if (!BuildGeometry(desc))
        return false;

    LOG_SUCCESS("Cube Built!");
    return true;
}

bool kfe::KEFCubeSceneObject::Impl::Destroy()
{
    //~ Destroy GPU resources
    if (m_pVertexView)    m_pVertexView->Destroy();
    if (m_pIndexView)     m_pIndexView->Destroy();
    if (m_pVBStaging)     m_pVBStaging->Destroy();
    if (m_pIBStaging)     m_pIBStaging->Destroy();

    m_metaFrameCB.Destroy();

    //~ Reset smart pointers
    m_pVertexView.reset();
    m_pIndexView .reset();
    m_pVBStaging .reset();
    m_pIBStaging .reset();
 
    //~ Free SRV descriptor slots
    if (m_pResourceHeap)
    {
        for (auto& data : m_srvs)
        {
            if (data.ReservedSlot != KFE_INVALID_INDEX)
            {
                m_pResourceHeap->Free(data.ReservedSlot);
                data.ReservedSlot = KFE_INVALID_INDEX;
            }

            data.TextureSrv = nullptr;
            data.ResourceHandle = KFE_INVALID_INDEX;
        }

    }

    //~ Clear tracking
    m_bTextureDirty = false;
    m_baseSrvIndex  = KFE_INVALID_INDEX;

    //~ Null references
    m_pResourceHeap = nullptr;

    return true;
}

void kfe::KEFCubeSceneObject::Impl::Render(_In_ const KFE_RENDER_OBJECT_DESC& desc)
{
    //BindShadowMapSRV(m_pResourceHeap, desc.ShadowMap);

    if (m_bTextureDirty)
    {
        BindTextureFromPath(desc.CommandList);
    }

    if (m_bDirtyGeometry)
    {
        KFE_BUILD_OBJECT_DESC builder{};
        builder.Device      = m_pDevice;
        builder.CommandList = desc.CommandList;
        BuildGeometry(builder);
    } 


    auto* cmdList = desc.CommandList;
    if (!cmdList)
        return;

    //~ Bind descriptor heaps
    if (m_pResourceHeap)
    {
        //~ Bind SRV descriptor table to t0..tN-1 starting at m_baseSrvIndex
        if (m_baseSrvIndex != KFE_INVALID_INDEX &&
            m_pResourceHeap->IsValidIndex(m_baseSrvIndex))
        {
            D3D12_GPU_DESCRIPTOR_HANDLE srvTableHandle =
                m_pResourceHeap->GetGPUHandle(m_baseSrvIndex);

            cmdList->SetGraphicsRootDescriptorTable(
                1u, // root parameter index for SRV table
                srvTableHandle
            );
        }
        else
        {
            LOG_WARNING("Base SRV index invalid, skipping texture bind.");
        }
    }

    //~ Bind texture meta constant buffer at b1
    if (m_metaFrameCB.IsInitialized())
    {
        auto* view = m_metaFrameCB.GetView();
        const D3D12_GPU_VIRTUAL_ADDRESS metaAddr =
            static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(view->GetGPUVirtualAddress());

        cmdList->SetGraphicsRootConstantBufferView(2u, metaAddr);
    }

    //~ Set vertex and index buffers
    auto vertexView = m_pVertexView->GetView();
    auto indexView  = m_pIndexView->GetView ();

    cmdList->IASetVertexBuffers(0u, 1u, &vertexView);
    cmdList->IASetIndexBuffer(&indexView);

    //~ Draw call
    cmdList->DrawIndexedInstanced(
        m_pIndexView->GetIndexCount(),
        1u,
        0u,
        0u,
        0u
    );
}

_Use_decl_annotations_
void kfe::KEFCubeSceneObject::Impl::ShadowPass(const KFE_RENDER_OBJECT_DESC& desc)
{
    // TODO: TO be implemented
}

_Use_decl_annotations_
bool kfe::KEFCubeSceneObject::Impl::BuildGeometry(const KFE_BUILD_OBJECT_DESC& desc)
{
    if (!desc.Device || !desc.CommandList)
    {
        LOG_ERROR("Device/CommandList is null.");
        return false;
    }

    //~ If nothing changed and we already have valid views
    if (!m_bDirtyGeometry && m_pVertexView && m_pIndexView)
        return true;

    // Generate CPU geometry
    std::vector<KFEVertexOnly>   vertices = GetVertices();
    std::vector<std::uint16_t>   indices = GetIndices();

    if (vertices.empty() || indices.empty())
    {
        LOG_ERROR("KEFCubeSceneObject::Impl::BuildGeometry: Generated empty geometry.");
        return false;
    }

    // sizes
    const std::uint32_t vbSize = static_cast<std::uint32_t>(vertices.size() * sizeof(KFEVertexOnly));
    const std::uint32_t ibSize = static_cast<std::uint32_t>(indices.size() * sizeof(std::uint16_t));

    if (vertices.size() > 0xFFFFu)
    {
        LOG_ERROR("KEFCubeSceneObject::Impl::BuildGeometry: Too many vertices for uint16 indices.");
        return false;
    }

    // Recreate resources
    m_pVertexView.reset();
    m_pIndexView.reset();
    m_pVBStaging.reset();
    m_pIBStaging.reset();

    // Vertex staging + upload
    m_pVBStaging = std::make_unique<KFEStagingBuffer>();

    KFE_STAGING_BUFFER_CREATE_DESC vbDesc{};
    vbDesc.Device = desc.Device;
    vbDesc.SizeInBytes = vbSize;

    if (!m_pVBStaging->Initialize(vbDesc))
    {
        LOG_ERROR("Failed to initialize Vertex Staging Buffer!");
        return false;
    }

    if (!m_pVBStaging->WriteBytes(vertices.data(), vbSize, 0u))
    {
        LOG_ERROR("Failed to write to Vertex Staging Buffer!");
        return false;
    }

    if (!m_pVBStaging->RecordUploadToDefault(desc.CommandList, vbSize, 0u, 0u))
    {
        LOG_ERROR("Failed to record upload for Vertex Staging Buffer!");
        return false;
    }

    // Index staging + upload
    m_pIBStaging = std::make_unique<KFEStagingBuffer>();

    KFE_STAGING_BUFFER_CREATE_DESC ibDesc{};
    ibDesc.Device = desc.Device;
    ibDesc.SizeInBytes = ibSize;

    if (!m_pIBStaging->Initialize(ibDesc))
    {
        LOG_ERROR("Failed to initialize Index Staging Buffer!");
        return false;
    }

    if (!m_pIBStaging->WriteBytes(indices.data(), ibSize, 0u))
    {
        LOG_ERROR("Failed to write to Index Staging Buffer!");
        return false;
    }

    if (!m_pIBStaging->RecordUploadToDefault(desc.CommandList, ibSize, 0u, 0u))
    {
        LOG_ERROR("Failed to record upload for Index Staging Buffer!");
        return false;
    }

    // Default buffers
    KFEBuffer* vbDefault = m_pVBStaging->GetDefaultBuffer();
    KFEBuffer* ibDefault = m_pIBStaging->GetDefaultBuffer();

    if (!vbDefault || !vbDefault->GetNative())
    {
        LOG_ERROR("BuildGeometry: VB default buffer is null.");
        return false;
    }

    if (!ibDefault || !ibDefault->GetNative())
    {
        LOG_ERROR("BuildGeometry: IB default buffer is null.");
        return false;
    }

    // Transition to usable states
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

    desc.CommandList->ResourceBarrier(2u, barriers);

    // Build VB/IB views
    m_pVertexView = std::make_unique<KFEVertexBuffer>();

    KFE_VERTEX_BUFFER_CREATE_DESC vbView{};
    vbView.DebugName      = "Cube";
    vbView.Device         = desc.Device;
    vbView.OffsetInBytes  = 0u;
    vbView.ResourceBuffer = vbDefault;
    vbView.StrideInBytes  = static_cast<std::uint32_t>(sizeof(KFEVertexOnly));

    if (!m_pVertexView->Initialize(vbView))
    {
        LOG_ERROR("Failed to build Vertex View!");
        return false;
    }

    m_pIndexView = std::make_unique<KFEIndexBuffer>();

    KFE_INDEX_BUFFER_CREATE_DESC ibView{};
    ibView.Device = desc.Device;
    ibView.Format = DXGI_FORMAT_R16_UINT;
    ibView.OffsetInBytes = 0u;
    ibView.ResourceBuffer = ibDefault;

    if (!m_pIndexView->Initialize(ibView))
    {
        LOG_ERROR("Failed to build Index View!");
        return false;
    }

    // Mark clean
    m_bDirtyGeometry = false;
    return true;
}

_Use_decl_annotations_
bool kfe::KEFCubeSceneObject::Impl::BuildConstantBuffer(const KFE_BUILD_OBJECT_DESC& desc)
{

    KFE_FRAME_CONSTANT_BUFFER_DESC cb{};
    cb.Device       = desc.Device;
    cb.FrameCount   = 6u;
    cb.ResourceHeap = desc.ResourceHeap;
    cb.SizeInBytes = sizeof(ModelTextureMetaInformation);

    if (!m_metaFrameCB.Initialize(cb)) 
    {
        LOG_ERROR("Failed to build meta texture config constant buffer!");
        return false;
    }

    LOG_SUCCESS("meta texture config Created!");
    return true;
}

bool kfe::KEFCubeSceneObject::Impl::BindTextureFromPath(ID3D12GraphicsCommandList* cmdList)
{
    //~ No textures need updating
    if (!m_bTextureDirty)
        return true;

    auto& pool = KFEImagePool::Instance();

    //~ Loop all defined texture slots
    const std::size_t count = static_cast<std::size_t>(EModelTextureSlot::Count);

    // Track first valid texture so we can alias others to it
    std::size_t      firstValidIndex = static_cast<std::size_t>(-1);
    std::uint32_t    firstValidResource = KFE_INVALID_INDEX;

    for (std::size_t i = 0; i < count; ++i)
    {
        auto& data = m_srvs[i];

        if (i == static_cast<std::size_t>(EModelTextureSlot::ShadowMap)) continue;

        //~ Skip: nothing dirty here
        if (!data.Dirty)
            continue;

        //~ no reserved descriptor slot, means Build() didn't allocate
        if (data.ReservedSlot == KFE_INVALID_INDEX)
        {
            LOG_ERROR("Cube SRV slot {} has no ReservedSlot allocated!", i);
            data.Dirty = false;
            continue;
        }

        //~ Skip empty textures
        if (data.TexturePath.empty())
        {
            data.TextureSrv = nullptr;
            data.ResourceHandle = KFE_INVALID_INDEX;
            data.Dirty = false;
            continue;
        }

        //~ Check file exists
        if (!kfe_helpers::IsFile(data.TexturePath))
        {
            LOG_ERROR("Texture '{}' does not exist!", data.TexturePath);
            data.TextureSrv = nullptr;
            data.ResourceHandle = KFE_INVALID_INDEX;
            data.Dirty = false;
            continue;
        }

        //~ Fetch texture SRV from the image pool
        KFETextureSRV* srv = pool.GetImageSrv(data.TexturePath, cmdList);
        if (!srv)
        {
            LOG_ERROR("Failed to load SRV for '{}'", data.TexturePath);
            data.TextureSrv = nullptr;
            data.ResourceHandle = KFE_INVALID_INDEX;
            data.Dirty = false;
            continue;
        }

        data.TextureSrv = srv;
        data.ResourceHandle = srv->GetDescriptorIndex();

        //~ Copy SRV into our contiguous block
        const D3D12_CPU_DESCRIPTOR_HANDLE src =
            m_pResourceHeap->GetHandle(data.ResourceHandle);

        const D3D12_CPU_DESCRIPTOR_HANDLE dst =
            m_pResourceHeap->GetHandle(data.ReservedSlot);

        auto* device = m_pDevice->GetNative();
        device->CopyDescriptorsSimple(1, dst, src,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        LOG_SUCCESS("Bound texture '{}' into ReservedSlot {}",
            data.TexturePath, data.ReservedSlot);

        //~ Track first valid texture so we can alias empties to it later
        if (firstValidResource == KFE_INVALID_INDEX)
        {
            firstValidResource = data.ResourceHandle;
            firstValidIndex = i;
        }

        //~ Mark clean
        data.Dirty = false;
    }

    if (firstValidResource != KFE_INVALID_INDEX)
    {
        const D3D12_CPU_DESCRIPTOR_HANDLE firstSrc =
            m_pResourceHeap->GetHandle(firstValidResource);

        for (std::size_t i = 0; i < count; ++i)
        {
            auto& data = m_srvs[i];

            // No reserved slot
            if (data.ReservedSlot == KFE_INVALID_INDEX)
                continue;

            // Already has a valid handle
            if (data.ResourceHandle != KFE_INVALID_INDEX)
                continue;

            // This slot had no texture
            const D3D12_CPU_DESCRIPTOR_HANDLE dst =
                m_pResourceHeap->GetHandle(data.ReservedSlot);

            m_pDevice->GetNative()->CopyDescriptorsSimple(
                1,
                dst,
                firstSrc,
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            );

            data.ResourceHandle = firstValidResource;
            data.TextureSrv = m_srvs[firstValidIndex].TextureSrv;

            LOG_WARNING(
                "Cube SRV slot {} had no valid texture; aliased to slot {} (descriptor {}).",
                i, firstValidIndex, firstValidResource
            );
        }
    }
    m_bTextureDirty = false;
    return true;
}

void kfe::KEFCubeSceneObject::Impl::UpdateConstantBuffer(const KFE_UPDATE_OBJECT_DESC& desc)
{
    //~ Update texture meta CB (at b1)
    auto enforceAttachment = [&](EModelTextureSlot tex, float& isAttachedField)
        {
            const auto& srv = m_srvs[static_cast<std::size_t>(tex)];

            bool hasTexture = !srv.TexturePath.empty();
            bool userEnabled = (isAttachedField > 0.5f);

            isAttachedField = (hasTexture && userEnabled) ? 1.0f : 0.0f;
        };

    // Main texture
    enforceAttachment(EModelTextureSlot::BaseColor,
        m_metaInformation.BaseColor.IsTextureAttached);

    // Secondary
    enforceAttachment(EModelTextureSlot::Displacement,
        m_metaInformation.Displacement.IsTextureAttached);

    // Normal
    enforceAttachment(EModelTextureSlot::Normal,
        m_metaInformation.Normal.IsTextureAttached);

    // Specular
    enforceAttachment(EModelTextureSlot::Specular,
        m_metaInformation.Specular.IsTextureAttached);

    // Height
    enforceAttachment(EModelTextureSlot::Height,
        m_metaInformation.Height.IsTextureAttached);

    //~ Copy meta buffer to GPU
    if (m_metaFrameCB.IsInitialized())
    {
        void* dst = m_metaFrameCB.GetView()->GetMappedData();
        if (dst)
        {
            std::memcpy(dst, &m_metaInformation, sizeof(m_metaInformation));
        }
    }
}

std::vector<kfe::KFEVertexOnly> kfe::KEFCubeSceneObject::Impl::GetVertices() const noexcept
{
    using namespace DirectX;

    std::vector<KFEVertexOnly> vertices;

    // Cube dimensions
    constexpr float half = 0.5f;
    constexpr float size = 1.0f;

    vertices.reserve(static_cast<size_t>(6) *
        static_cast<size_t>(m_subdivisionLevel + 1) * static_cast<size_t>(m_subdivisionLevel + 1));

    auto AddFace = [&](XMFLOAT3 corner, XMFLOAT3 uDir, XMFLOAT3 vDir, XMFLOAT3 nDir)
        {
            // Normalize tangent basis
            XMVECTOR U  = XMVector3Normalize(XMLoadFloat3(&uDir));
            XMVECTOR V  = XMVector3Normalize(XMLoadFloat3(&vDir));
            XMVECTOR Nn = XMVector3Normalize(XMLoadFloat3(&nDir));

            XMFLOAT3 tangent{}, bitangent{}, normal{};
            XMStoreFloat3(&tangent, U);
            XMStoreFloat3(&bitangent, V);
            XMStoreFloat3(&normal, Nn);

            for (int j = 0; j <= m_subdivisionLevel; ++j)
            {
                const float v = static_cast<float>(j) / static_cast<float>(m_subdivisionLevel);
                for (int i = 0; i <= m_subdivisionLevel; ++i)
                {
                    const float u = static_cast<float>(i) / static_cast<float>(m_subdivisionLevel);

                    // pos = corner + uDir*u + vDir*v
                    XMFLOAT3 pos{
                        corner.x + uDir.x * u + vDir.x * v,
                        corner.y + uDir.y * u + vDir.y * v,
                        corner.z + uDir.z * u + vDir.z * v
                    };

                    KFEVertexOnly vert{};
                    vert.Position = pos;
                    vert.Normal = normal;
                    vert.Tangent = tangent;
                    vert.Bitangent = bitangent;
                    vert.UV0 = XMFLOAT2{ u, v };
                    vert.UV1 = XMFLOAT2{ u, v };

                    vertices.push_back(vert);
                }
            }
        };


    // +Z face
    AddFace(
        XMFLOAT3{ -half, -half, +half },
        XMFLOAT3{ +size,  0.0f,  0.0f },
        XMFLOAT3{ 0.0f, +size,  0.0f },
        XMFLOAT3{ 0.0f,  0.0f, +1.0f }
    );

    // -Z face
    AddFace(
        XMFLOAT3{ +half, -half, -half },
        XMFLOAT3{ -size,  0.0f,  0.0f },
        XMFLOAT3{ 0.0f, +size,  0.0f },
        XMFLOAT3{ 0.0f,  0.0f, -1.0f }
    );

    // +X face
    AddFace(
        XMFLOAT3{ +half, -half, +half },
        XMFLOAT3{ 0.0f,  0.0f, -size },
        XMFLOAT3{ 0.0f, +size,  0.0f },
        XMFLOAT3{ +1.0f,  0.0f,  0.0f }
    );

    // -X face
    AddFace(
        XMFLOAT3{ -half, -half, -half },
        XMFLOAT3{ 0.0f,  0.0f, +size },
        XMFLOAT3{ 0.0f, +size,  0.0f },
        XMFLOAT3{ -1.0f,  0.0f,  0.0f }
    );

    // +Y face
    AddFace(
        XMFLOAT3{ -half, +half, +half },
        XMFLOAT3{ +size,  0.0f,  0.0f },
        XMFLOAT3{ 0.0f,  0.0f, -size },
        XMFLOAT3{ 0.0f, +1.0f,  0.0f }
    );

    // -Y face
    AddFace(
        XMFLOAT3{ -half, -half, -half },
        XMFLOAT3{ +size,  0.0f,  0.0f },
        XMFLOAT3{ 0.0f,  0.0f, +size },
        XMFLOAT3{ 0.0f, -1.0f,  0.0f }
    );

    return vertices;
}

std::vector<std::uint16_t> kfe::KEFCubeSceneObject::Impl::GetIndices() const noexcept
{
    using namespace DirectX;

    std::vector<std::uint16_t> indices;

    int vertsPerFace = (m_subdivisionLevel + 1) * (m_subdivisionLevel + 1);
    int quadsPerFace = m_subdivisionLevel * m_subdivisionLevel;
    int trisPerFace = quadsPerFace * 2;
    int idxPerFace = trisPerFace * 3;

    indices.reserve(static_cast<size_t>(6) * static_cast<size_t>(idxPerFace));

    auto AppendFaceIndices = [&](int faceBaseVertex,
        DirectX::XMFLOAT3 uDir,
        DirectX::XMFLOAT3 vDir,
        DirectX::XMFLOAT3 nDir)
        {
            XMVECTOR U = XMLoadFloat3(&uDir);
            XMVECTOR V = XMLoadFloat3(&vDir);
            XMVECTOR Nn = XMLoadFloat3(&nDir);

            const float d = XMVectorGetX(XMVector3Dot(XMVector3Cross(U, V), Nn));
            const bool ccw = (d > 0.0f); //~ outward normal

            for (int j = 0; j < m_subdivisionLevel; ++j)
            {
                for (int i = 0; i < m_subdivisionLevel; ++i)
                {
                    const int row0 = j * (m_subdivisionLevel + 1);
                    const int row1 = (j + 1) * (m_subdivisionLevel + 1);

                    const std::uint16_t a = static_cast<std::uint16_t>(faceBaseVertex + row0 + i);
                    const std::uint16_t b = static_cast<std::uint16_t>(faceBaseVertex + row0 + (i + 1));
                    const std::uint16_t c = static_cast<std::uint16_t>(faceBaseVertex + row1 + i);
                    const std::uint16_t d0 = static_cast<std::uint16_t>(faceBaseVertex + row1 + (i + 1));

                    if (ccw)
                    {
                        indices.push_back(a);  indices.push_back(b);  indices.push_back(c);
                        indices.push_back(b);  indices.push_back(d0); indices.push_back(c);
                    }
                    else
                    {
                        indices.push_back(a);  indices.push_back(c);  indices.push_back(b);
                        indices.push_back(b);  indices.push_back(c);  indices.push_back(d0);
                    }
                }
            }
        };

    // Face +Z
    AppendFaceIndices(0 * vertsPerFace,
        XMFLOAT3{ +1.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, +1.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, +1.0f });

    // Face -Z
    AppendFaceIndices(1 * vertsPerFace,
        XMFLOAT3{ -1.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, +1.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, -1.0f });

    // Face +X
    AppendFaceIndices(2 * vertsPerFace,
        XMFLOAT3{ 0.0f, 0.0f, -1.0f }, XMFLOAT3{ 0.0f, +1.0f, 0.0f }, XMFLOAT3{ +1.0f, 0.0f, 0.0f });

    // Face -X
    AppendFaceIndices(3 * vertsPerFace,
        XMFLOAT3{ 0.0f, 0.0f, +1.0f }, XMFLOAT3{ 0.0f, +1.0f, 0.0f }, XMFLOAT3{ -1.0f, 0.0f, 0.0f });

    // Face +Y
    AppendFaceIndices(4 * vertsPerFace,
        XMFLOAT3{ +1.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, -1.0f }, XMFLOAT3{ 0.0f, +1.0f, 0.0f });

    // Face -Y
    AppendFaceIndices(5 * vertsPerFace,
        XMFLOAT3{ +1.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, +1.0f }, XMFLOAT3{ 0.0f, -1.0f, 0.0f });

    return indices;
}

void kfe::KEFCubeSceneObject::Impl::BindShadowMapSRV(KFEResourceHeap* heap,
    KFEShadowMap* shadowMap) noexcept
{
    if (!m_pDevice) return;
    if (!shadowMap) return;

    if (!heap || !shadowMap)
        return;

    if (m_baseSrvIndex == KFE_INVALID_INDEX)
        return;

    const std::uint32_t shadowSlot = m_baseSrvIndex + static_cast<std::uint32_t>(EModelTextureSlot::ShadowMap);
    if (!heap->IsValidIndex(shadowSlot))
        return;

    D3D12_CPU_DESCRIPTOR_HANDLE dstCpu = heap->GetHandle(shadowSlot);

    std::uint32_t handle = shadowMap->GetHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE srcCpu = heap->GetHandle(handle);

    m_pDevice->GetNative()->CopyDescriptorsSimple(
        1,
        dstCpu,
        srcCpu,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

JsonLoader kfe::KEFCubeSceneObject::Impl::GetJsonData() const noexcept
{
    JsonLoader root{};
    root["SubDivisionLevel"] = std::to_string(m_subdivisionLevel);

    // Texture paths
    const auto getPath = [this](EModelTextureSlot tex) -> const std::string&
        {
            return m_srvs[static_cast<std::size_t>(tex)].TexturePath;
        };

    root["BaseColor"]       = getPath(EModelTextureSlot::BaseColor);
    root["Normal"]          = getPath(EModelTextureSlot::Normal);
    root["ORM"]             = getPath(EModelTextureSlot::ORM);
    root["Emissive"]        = getPath(EModelTextureSlot::Emissive);

    root["Roughness"]       = getPath(EModelTextureSlot::Roughness);
    root["Metallic"]        = getPath(EModelTextureSlot::Metallic);
    root["Occlusion"]       = getPath(EModelTextureSlot::Occlusion);

    root["Opacity"]         = getPath(EModelTextureSlot::Opacity);

    root["Height"]          = getPath(EModelTextureSlot::Height);
    root["Displacement"]    = getPath(EModelTextureSlot::Displacement);

    root["Specular"]        = getPath(EModelTextureSlot::Specular);
    root["Glossiness"]      = getPath(EModelTextureSlot::Glossiness);

    root["DetailNormal"]    = getPath(EModelTextureSlot::DetailNormal);

    auto& meta = root["TextureMeta"];

    // BaseColor
    meta["BaseColor"]["IsTextureAttached"] = m_metaInformation.BaseColor.IsTextureAttached;
    meta["BaseColor"]["UvTilingX"] = m_metaInformation.BaseColor.UvTilingX;
    meta["BaseColor"]["UvTilingY"] = m_metaInformation.BaseColor.UvTilingY;
    meta["BaseColor"]["Strength"] = m_metaInformation.BaseColor.Strength;

    // Normal
    meta["Normal"]["IsTextureAttached"] = m_metaInformation.Normal.IsTextureAttached;
    meta["Normal"]["NormalStrength"] = m_metaInformation.Normal.NormalStrength;
    meta["Normal"]["UvTilingX"] = m_metaInformation.Normal.UvTilingX;
    meta["Normal"]["UvTilingY"] = m_metaInformation.Normal.UvTilingY;

    // ORM
    meta["ORM"]["IsTextureAttached"] = m_metaInformation.ORM.IsTextureAttached;
    meta["ORM"]["IsMixed"] = m_metaInformation.ORM.IsMixed;
    meta["ORM"]["UvTilingX"] = m_metaInformation.ORM.UvTilingX;
    meta["ORM"]["UvTilingY"] = m_metaInformation.ORM.UvTilingY;

    // Emissive
    meta["Emissive"]["IsTextureAttached"] = m_metaInformation.Emissive.IsTextureAttached;
    meta["Emissive"]["EmissiveIntensity"] = m_metaInformation.Emissive.EmissiveIntensity;
    meta["Emissive"]["UvTilingX"] = m_metaInformation.Emissive.UvTilingX;
    meta["Emissive"]["UvTilingY"] = m_metaInformation.Emissive.UvTilingY;

    // Opacity
    meta["Opacity"]["IsTextureAttached"] = m_metaInformation.Opacity.IsTextureAttached;
    meta["Opacity"]["AlphaMultiplier"] = m_metaInformation.Opacity.AlphaMultiplier;
    meta["Opacity"]["AlphaCutoff"] = m_metaInformation.Opacity.AlphaCutoff;

    // Height / Parallax
    meta["Height"]["IsTextureAttached"] = m_metaInformation.Height.IsTextureAttached;
    meta["Height"]["HeightScale"] = m_metaInformation.Height.HeightScale;
    meta["Height"]["ParallaxMinLayers"] = m_metaInformation.Height.ParallaxMinLayers;
    meta["Height"]["ParallaxMaxLayers"] = m_metaInformation.Height.ParallaxMaxLayers;

    // Displacement
    meta["Displacement"]["IsTextureAttached"] = m_metaInformation.Displacement.IsTextureAttached;
    meta["Displacement"]["DisplacementScale"] = m_metaInformation.Displacement.DisplacementScale;
    meta["Displacement"]["UvTilingX"] = m_metaInformation.Displacement.UvTilingX;
    meta["Displacement"]["UvTilingY"] = m_metaInformation.Displacement.UvTilingY;

    // Specular
    meta["Specular"]["IsTextureAttached"] = m_metaInformation.Specular.IsTextureAttached;
    meta["Specular"]["Strength"] = m_metaInformation.Specular.Strength;
    meta["Specular"]["UvTilingX"] = m_metaInformation.Specular.UvTilingX;
    meta["Specular"]["UvTilingY"] = m_metaInformation.Specular.UvTilingY;

    // Glossiness
    meta["Glossiness"]["IsTextureAttached"] = m_metaInformation.Glossiness.IsTextureAttached;
    meta["Glossiness"]["Strength"] = m_metaInformation.Glossiness.Strength;
    meta["Glossiness"]["UvTilingX"] = m_metaInformation.Glossiness.UvTilingX;
    meta["Glossiness"]["UvTilingY"] = m_metaInformation.Glossiness.UvTilingY;

    // Detail Normal
    meta["DetailNormal"]["IsTextureAttached"] = m_metaInformation.DetailNormal.IsTextureAttached;
    meta["DetailNormal"]["NormalStrength"] = m_metaInformation.DetailNormal.NormalStrength;
    meta["DetailNormal"]["UvTilingX"] = m_metaInformation.DetailNormal.UvTilingX;
    meta["DetailNormal"]["UvTilingY"] = m_metaInformation.DetailNormal.UvTilingY;

    // Singulars
    meta["Singular"]["IsOcclusionAttached"] = m_metaInformation.Singular.IsOcclusionAttached;
    meta["Singular"]["IsRoughnessAttached"] = m_metaInformation.Singular.IsRoughnessAttached;
    meta["Singular"]["IsMetallicAttached"] = m_metaInformation.Singular.IsMetallicAttached;

    meta["Singular"]["OcclusionStrength"] = m_metaInformation.Singular.OcclusionStrength;
    meta["Singular"]["RoughnessValue"] = m_metaInformation.Singular.RoughnessValue;
    meta["Singular"]["MetallicValue"] = m_metaInformation.Singular.MetallicValue;

    meta["Singular"]["OcclusionTilingX"] = m_metaInformation.Singular.OcclusionTilingX;
    meta["Singular"]["OcclusionTilingY"] = m_metaInformation.Singular.OcclusionTilingY;
    meta["Singular"]["RoughnessTilingX"] = m_metaInformation.Singular.RoughnessTilingX;
    meta["Singular"]["RoughnessTilingY"] = m_metaInformation.Singular.RoughnessTilingY;
    meta["Singular"]["MetallicTilingX"] = m_metaInformation.Singular.MetallicTilingX;
    meta["Singular"]["MetallicTilingY"] = m_metaInformation.Singular.MetallicTilingY;

    // Mip debug
    meta["MipDebug"]["ForcedMipLevel"] = m_metaInformation.ForcedMipLevel;
    meta["MipDebug"]["UseForcedMip"] = m_metaInformation.UseForcedMip;

    return root;
}

void kfe::KEFCubeSceneObject::Impl::LoadFromJson(const JsonLoader& loader) noexcept
{
    if (loader.Has("SubDivisionLevel"))
    {
        m_subdivisionLevel = loader["SubDivisionLevel"].AsUInt();
        m_bDirtyGeometry = true;
    }

    // Texture paths
    auto loadPath = [&](const char* key, EModelTextureSlot slot, auto&& setter)
        {
            if (!loader.Contains(key))
                return;

            const std::string path = loader[key].GetValue();
            if (!path.empty())
                setter(path);
        };

    loadPath("BaseColor", EModelTextureSlot::BaseColor, [&](const std::string& p) { SetBaseColorTexture(p); });
    loadPath("Normal", EModelTextureSlot::Normal, [&](const std::string& p)       { SetNormalMap(p); });
    loadPath("ORM", EModelTextureSlot::ORM, [&](const std::string& p)             { SetORMTexture(p); });
    loadPath("Emissive", EModelTextureSlot::Emissive, [&](const std::string& p)   { SetEmissiveTexture(p); });

    loadPath("Roughness", EModelTextureSlot::Roughness, [&](const std::string& p) { SetRoughnessTexture(p); });
    loadPath("Metallic", EModelTextureSlot::Metallic, [&](const std::string& p) { SetMetallicTexture(p); });
    loadPath("Occlusion", EModelTextureSlot::Occlusion, [&](const std::string& p) { SetOcclusionTexture(p); });

    loadPath("Opacity", EModelTextureSlot::Opacity, [&](const std::string& p) { SetOpacityTexture(p); });

    loadPath("Height", EModelTextureSlot::Height, [&](const std::string& p) { SetHeightMap(p); });
    loadPath("Displacement", EModelTextureSlot::Displacement, [&](const std::string& p) { SetDisplacementTexture(p); });

    loadPath("Specular", EModelTextureSlot::Specular, [&](const std::string& p) { SetSpecularTexture(p); });
    loadPath("Glossiness", EModelTextureSlot::Glossiness, [&](const std::string& p) { SetGlossinessTexture(p); });

    loadPath("DetailNormal", EModelTextureSlot::DetailNormal, [&](const std::string& p) { SetDetailNormalMap(p); });

    // Meta information
    if (loader.Contains("TextureMeta"))
    {
        const JsonLoader& metaRoot = loader["TextureMeta"];

        auto loadFloat = [](const JsonLoader& obj, const char* key, float& outValue) noexcept
            {
                if (!obj.Contains(key))
                    return;

                const std::string s = obj[key].GetValue();
                if (s.empty())
                    return;

                outValue = std::stof(s);
            };

        // BaseColor
        if (metaRoot.Contains("BaseColor"))
        {
            const JsonLoader& b = metaRoot["BaseColor"];
            loadFloat(b, "IsTextureAttached", m_metaInformation.BaseColor.IsTextureAttached);
            loadFloat(b, "UvTilingX", m_metaInformation.BaseColor.UvTilingX);
            loadFloat(b, "UvTilingY", m_metaInformation.BaseColor.UvTilingY);
            loadFloat(b, "Strength", m_metaInformation.BaseColor.Strength);
        }

        // Normal
        if (metaRoot.Contains("Normal"))
        {
            const JsonLoader& n = metaRoot["Normal"];
            loadFloat(n, "IsTextureAttached", m_metaInformation.Normal.IsTextureAttached);
            loadFloat(n, "NormalStrength", m_metaInformation.Normal.NormalStrength);
            loadFloat(n, "UvTilingX", m_metaInformation.Normal.UvTilingX);
            loadFloat(n, "UvTilingY", m_metaInformation.Normal.UvTilingY);
        }

        // ORM
        if (metaRoot.Contains("ORM"))
        {
            const JsonLoader& o = metaRoot["ORM"];
            loadFloat(o, "IsTextureAttached", m_metaInformation.ORM.IsTextureAttached);
            loadFloat(o, "IsMixed", m_metaInformation.ORM.IsMixed);
            loadFloat(o, "UvTilingX", m_metaInformation.ORM.UvTilingX);
            loadFloat(o, "UvTilingY", m_metaInformation.ORM.UvTilingY);
        }

        // Emissive
        if (metaRoot.Contains("Emissive"))
        {
            const JsonLoader& e = metaRoot["Emissive"];
            loadFloat(e, "IsTextureAttached", m_metaInformation.Emissive.IsTextureAttached);
            loadFloat(e, "EmissiveIntensity", m_metaInformation.Emissive.EmissiveIntensity);
            loadFloat(e, "UvTilingX", m_metaInformation.Emissive.UvTilingX);
            loadFloat(e, "UvTilingY", m_metaInformation.Emissive.UvTilingY);
        }

        // Opacity
        if (metaRoot.Contains("Opacity"))
        {
            const JsonLoader& a = metaRoot["Opacity"];
            loadFloat(a, "IsTextureAttached", m_metaInformation.Opacity.IsTextureAttached);
            loadFloat(a, "AlphaMultiplier", m_metaInformation.Opacity.AlphaMultiplier);
            loadFloat(a, "AlphaCutoff", m_metaInformation.Opacity.AlphaCutoff);
        }

        // Height
        if (metaRoot.Contains("Height"))
        {
            const JsonLoader& h = metaRoot["Height"];
            loadFloat(h, "IsTextureAttached", m_metaInformation.Height.IsTextureAttached);
            loadFloat(h, "HeightScale", m_metaInformation.Height.HeightScale);
            loadFloat(h, "ParallaxMinLayers", m_metaInformation.Height.ParallaxMinLayers);
            loadFloat(h, "ParallaxMaxLayers", m_metaInformation.Height.ParallaxMaxLayers);
        }

        // Displacement
        if (metaRoot.Contains("Displacement"))
        {
            const JsonLoader& d = metaRoot["Displacement"];
            loadFloat(d, "IsTextureAttached", m_metaInformation.Displacement.IsTextureAttached);
            loadFloat(d, "DisplacementScale", m_metaInformation.Displacement.DisplacementScale);
            loadFloat(d, "UvTilingX", m_metaInformation.Displacement.UvTilingX);
            loadFloat(d, "UvTilingY", m_metaInformation.Displacement.UvTilingY);
        }

        // Specular
        if (metaRoot.Contains("Specular"))
        {
            const JsonLoader& s = metaRoot["Specular"];
            loadFloat(s, "IsTextureAttached", m_metaInformation.Specular.IsTextureAttached);
            loadFloat(s, "Strength", m_metaInformation.Specular.Strength);
            loadFloat(s, "UvTilingX", m_metaInformation.Specular.UvTilingX);
            loadFloat(s, "UvTilingY", m_metaInformation.Specular.UvTilingY);
        }

        // Glossiness
        if (metaRoot.Contains("Glossiness"))
        {
            const JsonLoader& g = metaRoot["Glossiness"];
            loadFloat(g, "IsTextureAttached", m_metaInformation.Glossiness.IsTextureAttached);
            loadFloat(g, "Strength", m_metaInformation.Glossiness.Strength);
            loadFloat(g, "UvTilingX", m_metaInformation.Glossiness.UvTilingX);
            loadFloat(g, "UvTilingY", m_metaInformation.Glossiness.UvTilingY);
        }

        // DetailNormal
        if (metaRoot.Contains("DetailNormal"))
        {
            const JsonLoader& dn = metaRoot["DetailNormal"];
            loadFloat(dn, "IsTextureAttached", m_metaInformation.DetailNormal.IsTextureAttached);
            loadFloat(dn, "NormalStrength", m_metaInformation.DetailNormal.NormalStrength);
            loadFloat(dn, "UvTilingX", m_metaInformation.DetailNormal.UvTilingX);
            loadFloat(dn, "UvTilingY", m_metaInformation.DetailNormal.UvTilingY);
        }

        // Singular
        if (metaRoot.Contains("Singular"))
        {
            const JsonLoader& s = metaRoot["Singular"];

            loadFloat(s, "IsOcclusionAttached", m_metaInformation.Singular.IsOcclusionAttached);
            loadFloat(s, "IsRoughnessAttached", m_metaInformation.Singular.IsRoughnessAttached);
            loadFloat(s, "IsMetallicAttached", m_metaInformation.Singular.IsMetallicAttached);

            loadFloat(s, "OcclusionStrength", m_metaInformation.Singular.OcclusionStrength);
            loadFloat(s, "RoughnessValue", m_metaInformation.Singular.RoughnessValue);
            loadFloat(s, "MetallicValue", m_metaInformation.Singular.MetallicValue);

            loadFloat(s, "OcclusionTilingX", m_metaInformation.Singular.OcclusionTilingX);
            loadFloat(s, "OcclusionTilingY", m_metaInformation.Singular.OcclusionTilingY);
            loadFloat(s, "RoughnessTilingX", m_metaInformation.Singular.RoughnessTilingX);
            loadFloat(s, "RoughnessTilingY", m_metaInformation.Singular.RoughnessTilingY);
            loadFloat(s, "MetallicTilingX", m_metaInformation.Singular.MetallicTilingX);
            loadFloat(s, "MetallicTilingY", m_metaInformation.Singular.MetallicTilingY);
        }

        // MipDebug
        if (metaRoot.Contains("MipDebug"))
        {
            const JsonLoader& dbg = metaRoot["MipDebug"];
            loadFloat(dbg, "ForcedMipLevel", m_metaInformation.ForcedMipLevel);
            loadFloat(dbg, "UseForcedMip", m_metaInformation.UseForcedMip);
        }
    }

    m_bTextureDirty = true;
}

void kfe::KEFCubeSceneObject::Impl::ImguiViewHeader(float dt)
{
    if (!ImGui::CollapsingHeader("Geometry", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    int subdivision = static_cast<int>(m_subdivisionLevel);

    if (ImGui::SliderInt("Subdivision Level", &subdivision, 1, 64))
    {
        const std::uint32_t newValue =
            static_cast<std::uint32_t>(std::max(1, subdivision));

        if (newValue != m_subdivisionLevel)
        {
            m_subdivisionLevel = newValue;
            m_bDirtyGeometry = true;
        }
    }

    const std::uint32_t vertsPerFace = (m_subdivisionLevel + 1u) * (m_subdivisionLevel + 1u);
    const std::uint32_t totalVerts   = 6u * vertsPerFace;
    const std::uint32_t totalTris    = 12u * m_subdivisionLevel * m_subdivisionLevel;

    ImGui::TextDisabled("Vertices : %u", totalVerts);
    ImGui::TextDisabled("Triangles: %u", totalTris);
}


void kfe::KEFCubeSceneObject::Impl::ImguiView(float)
{
    if (!ImGui::CollapsingHeader("Material Settings", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    // Mip debug
    ImGui::TextUnformatted("Texture Mip Debug");
    ImGui::Separator();

    bool forceMip = (m_metaInformation.UseForcedMip > 0.5f);
    if (ImGui::Checkbox("Force Mip Level", &forceMip))
        m_metaInformation.UseForcedMip = forceMip ? 1.0f : 0.0f;

    ImGui::SliderFloat("Forced Mip", &m_metaInformation.ForcedMipLevel, 0.0f, 10.0f, "%.1f");
    ImGui::Separator();

    // Helpers
    auto CopyToBuf = [](char(&buf)[260], const std::string& s)
        {
            std::memset(buf, 0, sizeof(buf));
            const size_t maxCopy = sizeof(buf) - 1;
            const size_t count = std::min(s.size(), maxCopy);
            s.copy(buf, count);
            buf[count] = '\0';
        };

    auto DrawEnable = [](const char* label, float& flag)
        {
            bool enabled = (flag > 0.5f);
            if (ImGui::Checkbox(label, &enabled))
                flag = enabled ? 1.0f : 0.0f;
        };

    auto DrawUvTiling = [](const char* label, float& x, float& y)
        {
            float uv[2] = { x, y };
            if (ImGui::DragFloat2(label, uv, 0.01f, 0.01f, 256.0f))
            {
                x = uv[0];
                y = uv[1];
            }
        };

    auto EditTextureNode =
        [&](const char* nodeLabel,
            EModelTextureSlot slot,
            const char* pathLabel,
            auto setter,
            auto drawMetaControls)
        {
            if (!ImGui::TreeNode(nodeLabel))
                return;

            std::string& texturePath =
                m_srvs[static_cast<std::size_t>(slot)].TexturePath;

            char buf[260];
            CopyToBuf(buf, texturePath);

            if (ImGui::InputText(pathLabel, buf, sizeof(buf)))
                texturePath.assign(buf);

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload =
                    ImGui::AcceptDragDropPayload(kfe::KFEAssetPanel::kPayloadType))
                {
                    kfe::KFEAssetPanel::PayloadHeader hdr{};
                    std::string pathUtf8;

                    if (kfe::KFEAssetPanel::ParsePayload(payload, hdr, pathUtf8))
                    {
                        texturePath = pathUtf8;
                        setter(pathUtf8);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (ImGui::Button("Apply Path"))
                setter(std::string{ buf });

            ImGui::Separator();
            drawMetaControls();
            ImGui::TreePop();
        };

    // Texture Binding UI
    if (!ImGui::TreeNode("Texture Binding"))
        return;

    // BaseColor
    EditTextureNode(
        "BaseColor (Albedo)",
        EModelTextureSlot::BaseColor,
        "BaseColor Path",
        [this](const std::string& p) { SetBaseColorTexture(p); },
        [&]()
        {
            DrawEnable("Enabled", m_metaInformation.BaseColor.IsTextureAttached);
            ImGui::SliderFloat("Strength", &m_metaInformation.BaseColor.Strength, 0.0f, 4.0f);
            DrawUvTiling("UV Tiling", m_metaInformation.BaseColor.UvTilingX, m_metaInformation.BaseColor.UvTilingY);
        });

    // Normal
    EditTextureNode(
        "Normal Map",
        EModelTextureSlot::Normal,
        "Normal Path",
        [this](const std::string& p) { SetNormalMap(p); },
        [&]()
        {
            DrawEnable("Enabled", m_metaInformation.Normal.IsTextureAttached);
            ImGui::SliderFloat("Normal Strength", &m_metaInformation.Normal.NormalStrength, 0.0f, 4.0f);
            DrawUvTiling("UV Tiling", m_metaInformation.Normal.UvTilingX, m_metaInformation.Normal.UvTilingY);
        });

    // ORM
    EditTextureNode(
        "ORM (AO/Rough/Metal)",
        EModelTextureSlot::ORM,
        "ORM Path",
        [this](const std::string& p) { SetORMTexture(p); },
        [&]()
        {
            DrawEnable("Enabled", m_metaInformation.ORM.IsTextureAttached);

            bool mixed = (m_metaInformation.ORM.IsMixed > 0.5f);
            if (ImGui::Checkbox("IsMixed (packed)", &mixed))
                m_metaInformation.ORM.IsMixed = mixed ? 1.0f : 0.0f;

            DrawUvTiling("UV Tiling", m_metaInformation.ORM.UvTilingX, m_metaInformation.ORM.UvTilingY);
        });

    // Occlusion (separate)
    EditTextureNode(
        "Occlusion (AO) [Separate]",
        EModelTextureSlot::Occlusion,
        "Occlusion Path",
        [this](const std::string& p) { SetOcclusionTexture(p); },
        [&]()
        {
            DrawEnable("Enabled", m_metaInformation.Singular.IsOcclusionAttached);
            ImGui::SliderFloat("Strength", &m_metaInformation.Singular.OcclusionStrength, 0.0f, 4.0f);
            DrawUvTiling("UV Tiling", m_metaInformation.Singular.OcclusionTilingX, m_metaInformation.Singular.OcclusionTilingY);
        });

    // Roughness (separate)
    EditTextureNode(
        "Roughness [Separate]",
        EModelTextureSlot::Roughness,
        "Roughness Path",
        [this](const std::string& p) { SetRoughnessTexture(p); },
        [&]()
        {
            DrawEnable("Enabled", m_metaInformation.Singular.IsRoughnessAttached);
            ImGui::SliderFloat("Value", &m_metaInformation.Singular.RoughnessValue, 0.0f, 1.0f);
            DrawUvTiling("UV Tiling", m_metaInformation.Singular.RoughnessTilingX, m_metaInformation.Singular.RoughnessTilingY);
        });

    // Metallic (separate)
    EditTextureNode(
        "Metallic [Separate]",
        EModelTextureSlot::Metallic,
        "Metallic Path",
        [this](const std::string& p) { SetMetallicTexture(p); },
        [&]()
        {
            DrawEnable("Enabled", m_metaInformation.Singular.IsMetallicAttached);
            ImGui::SliderFloat("Value", &m_metaInformation.Singular.MetallicValue, 0.0f, 1.0f);
            DrawUvTiling("UV Tiling", m_metaInformation.Singular.MetallicTilingX, m_metaInformation.Singular.MetallicTilingY);
        });

    // Emissive
    EditTextureNode(
        "Emissive",
        EModelTextureSlot::Emissive,
        "Emissive Path",
        [this](const std::string& p) { SetEmissiveTexture(p); },
        [&]()
        {
            DrawEnable("Enabled", m_metaInformation.Emissive.IsTextureAttached);
            ImGui::SliderFloat("Intensity", &m_metaInformation.Emissive.EmissiveIntensity, 0.0f, 50.0f);
            DrawUvTiling("UV Tiling", m_metaInformation.Emissive.UvTilingX, m_metaInformation.Emissive.UvTilingY);
        });

    // Opacity
    EditTextureNode(
        "Opacity (Alpha / Cutout)",
        EModelTextureSlot::Opacity,
        "Opacity Path",
        [this](const std::string& p) { SetOpacityTexture(p); },
        [&]()
        {
            DrawEnable("Enabled", m_metaInformation.Opacity.IsTextureAttached);
            ImGui::SliderFloat("Alpha Multiplier", &m_metaInformation.Opacity.AlphaMultiplier, 0.0f, 2.0f);
            ImGui::SliderFloat("Alpha Cutoff", &m_metaInformation.Opacity.AlphaCutoff, 0.0f, 1.0f);
        });

    // Height (Parallax)
    EditTextureNode(
        "Height (Parallax)",
        EModelTextureSlot::Height,
        "Height Path",
        [this](const std::string& p) { SetHeightMap(p); },
        [&]()
        {
            DrawEnable("Enabled", m_metaInformation.Height.IsTextureAttached);
            ImGui::SliderFloat("Height Scale", &m_metaInformation.Height.HeightScale, 0.0f, 0.25f);
            ImGui::SliderFloat("Min Layers", &m_metaInformation.Height.ParallaxMinLayers, 1.0f, 128.0f);
            ImGui::SliderFloat("Max Layers", &m_metaInformation.Height.ParallaxMaxLayers, 1.0f, 256.0f);
        });

    // Displacement
    EditTextureNode(
        "Displacement",
        EModelTextureSlot::Displacement,
        "Displacement Path",
        [this](const std::string& p) { SetDisplacementTexture(p); },
        [&]()
        {
            DrawEnable("Enabled", m_metaInformation.Displacement.IsTextureAttached);
            ImGui::SliderFloat("Scale", &m_metaInformation.Displacement.DisplacementScale, 0.0f, 1.0f);
            DrawUvTiling("UV Tiling", m_metaInformation.Displacement.UvTilingX, m_metaInformation.Displacement.UvTilingY);
        });

    // Specular
    EditTextureNode(
        "Specular",
        EModelTextureSlot::Specular,
        "Specular Path",
        [this](const std::string& p) { SetSpecularTexture(p); },
        [&]()
        {
            DrawEnable("Enabled", m_metaInformation.Specular.IsTextureAttached);
            ImGui::SliderFloat("Strength", &m_metaInformation.Specular.Strength, 0.0f, 4.0f);
            DrawUvTiling("UV Tiling", m_metaInformation.Specular.UvTilingX, m_metaInformation.Specular.UvTilingY);
        });

    // Glossiness
    EditTextureNode(
        "Glossiness",
        EModelTextureSlot::Glossiness,
        "Glossiness Path",
        [this](const std::string& p) { SetGlossinessTexture(p); },
        [&]()
        {
            DrawEnable("Enabled", m_metaInformation.Glossiness.IsTextureAttached);
            ImGui::SliderFloat("Strength", &m_metaInformation.Glossiness.Strength, 0.0f, 4.0f);
            DrawUvTiling("UV Tiling", m_metaInformation.Glossiness.UvTilingX, m_metaInformation.Glossiness.UvTilingY);
        });

    // Detail Normal
    EditTextureNode(
        "Detail Normal",
        EModelTextureSlot::DetailNormal,
        "DetailNormal Path",
        [this](const std::string& p) { SetDetailNormalMap(p); },
        [&]()
        {
            DrawEnable("Enabled", m_metaInformation.DetailNormal.IsTextureAttached);
            ImGui::SliderFloat("Normal Strength", &m_metaInformation.DetailNormal.NormalStrength, 0.0f, 4.0f);
            DrawUvTiling("UV Tiling", m_metaInformation.DetailNormal.UvTilingX, m_metaInformation.DetailNormal.UvTilingY);
        });

    ImGui::Separator();

    if (ImGui::TreeNode("Singular AO / Roughness / Metallic (Overrides)"))
    {
        DrawEnable("Occlusion Enabled", m_metaInformation.Singular.IsOcclusionAttached);
        ImGui::SliderFloat("Occlusion Strength", &m_metaInformation.Singular.OcclusionStrength, 0.0f, 4.0f);
        DrawUvTiling("Occlusion UV", m_metaInformation.Singular.OcclusionTilingX, m_metaInformation.Singular.OcclusionTilingY);

        ImGui::Separator();

        DrawEnable("Roughness Enabled", m_metaInformation.Singular.IsRoughnessAttached);
        ImGui::SliderFloat("Roughness Value", &m_metaInformation.Singular.RoughnessValue, 0.0f, 1.0f);
        DrawUvTiling("Roughness UV", m_metaInformation.Singular.RoughnessTilingX, m_metaInformation.Singular.RoughnessTilingY);

        ImGui::Separator();

        DrawEnable("Metallic Enabled", m_metaInformation.Singular.IsMetallicAttached);
        ImGui::SliderFloat("Metallic Value", &m_metaInformation.Singular.MetallicValue, 0.0f, 1.0f);
        DrawUvTiling("Metallic UV", m_metaInformation.Singular.MetallicTilingX, m_metaInformation.Singular.MetallicTilingY);

        ImGui::TreePop();
    }

    ImGui::TreePop(); // Texture Binding

    m_bTextureDirty = true;
}

#pragma endregion
