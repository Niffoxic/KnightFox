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
#include <unordered_set>
#include <array>

#include "engine/render_manager/api/frame_cb.h"
#include "engine/render_manager/shadow/shadow_map.h"

static void UpdateMetaCB(_Inout_ kfe::KFEModelSubmesh& sm) noexcept
{
    if (!sm.MetaConstantBuffer.IsInitialized())
        return;

    auto* dst = static_cast<kfe::ModelTextureMetaInformation*>(sm.MetaConstantBuffer.GetMappedData());
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

    JsonLoader GetJsonData               () const noexcept;
    JsonLoader GetChildTransformation    () const noexcept;
    JsonLoader GetChildMetaInformation   () const noexcept;
    JsonLoader GetChildTextureInformation() const noexcept;
    void       LoadFromJson            (const JsonLoader& loader) noexcept;
    void       LoadChildTransformations(const JsonLoader& loader) noexcept;
    void       LoadChildMetaInformation(const JsonLoader& loader) noexcept;
    void       LoadChildTextureInformation(const JsonLoader& loader) noexcept;

    void ImguiViewHeader(float deltaTime);
    void ImguiView                (float deltaTime);
    void ImguiChildTransformation (float deltaTime);
    void ImguiTextureMetaConfig   (float deltaTime);

private:
    bool BuildGeometry      (_In_ const KFE_BUILD_OBJECT_DESC& desc);

    //~ Build Constant buffer for submeshes
    void ApplyChildTransformationsFromCache() noexcept;
    void ApplyChildMetaInformationFromCache() noexcept;
    void ApplyChildTextureInformationFromCache() noexcept;
    bool BuildSubmeshConstantBuffers(const KFE_BUILD_OBJECT_DESC& desc);
    void BuildSubmeshCBDataCacheFromModel() noexcept;
    void CacheNodeMeshesRecursive(const KFEModelNode& node) noexcept;

    //~ Constant buffer updates
    void UpdateSubmeshConstantBuffers(const KFE_UPDATE_OBJECT_DESC& desc);
    void UpdateCBDataForNodeMeshes(const KFEModelNode& node) noexcept;

    void RenderNodeRecursive(
        ID3D12GraphicsCommandList* cmdList,
        const KFE_RENDER_OBJECT_DESC& desc,
        const KFEModelNode& node,
        const DirectX::XMMATRIX& parentWorld);

public:
    bool      m_bTextureDirty{ true };
    bool      m_bModelDirty  { false };
    bool      m_bMetaDirty   { true };

private:
    KFEMeshSceneObject* m_pObject{ nullptr };
    float               m_nTimeLived{ 0.0f };
    bool                m_bBuild{ false };

    //~ Constant buffers
    KFEFrameConstantBuffer m_metaFrameCB{};

    //~ Pipeline
    KFEDevice*       m_pDevice      { nullptr };
    KFEResourceHeap* m_pResourceHeap{ nullptr };

    //~ Model
    KFEModel    m_mesh     {};
    std::string m_modelPath{ "assets/defaults/default_3d.obj" };
    std::unordered_map<std::uint32_t, KFE_COMMON_CB_GPU>                   m_cbData{};
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

std::string kfe::KFEMeshSceneObject::GetName() const noexcept
{
    return "KFEMeshSceneObject";
}

std::string kfe::KFEMeshSceneObject::GetDescription() const noexcept
{
    return "A Mesh Object that can be used for rendering debug cube for colliders";
}

void kfe::KFEMeshSceneObject::ChildMainPass(const KFE_RENDER_OBJECT_DESC& desc)
{
    m_impl->Render(desc);
}

void kfe::KFEMeshSceneObject::ChildShadowPass(const KFE_RENDER_OBJECT_DESC& desc)
{
}

void kfe::KFEMeshSceneObject::ChildUpdate(const KFE_UPDATE_OBJECT_DESC& desc)
{
    m_impl->Update(desc);
}

_Use_decl_annotations_
bool kfe::KFEMeshSceneObject::ChildBuild(const KFE_BUILD_OBJECT_DESC& desc)
{
    if (m_impl->Build(desc))
    {
        return true;
    }
    return false;
}

bool kfe::KFEMeshSceneObject::ChildDestroy()
{
    return m_impl->Destroy();
}

void kfe::KFEMeshSceneObject::ChildImguiViewHeader(float deltaTime)
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

        m_impl->ImguiViewHeader(deltaTime);
    }
}

void kfe::KFEMeshSceneObject::ChildImguiViewBody(float deltaTime)
{
    m_impl->ImguiView(deltaTime);
}

JsonLoader kfe::KFEMeshSceneObject::ChildGetJsonData() const
{
    return m_impl->GetJsonData();
}

void kfe::KFEMeshSceneObject::ChildLoadFromJson(const JsonLoader& loader)
{
    m_impl->LoadFromJson(loader);
}

#pragma endregion

#pragma region Impl_body

void kfe::KFEMeshSceneObject::Impl::Update(const KFE_UPDATE_OBJECT_DESC& desc)
{
    m_nTimeLived += desc.DeltaTime;
    UpdateSubmeshConstantBuffers(desc);
}

_Use_decl_annotations_
bool kfe::KFEMeshSceneObject::Impl::Build(const KFE_BUILD_OBJECT_DESC& desc)
{
    m_pDevice       = desc.Device;
    m_pResourceHeap = desc.ResourceHeap;

    if (!m_pDevice || !m_pResourceHeap)
    {
        LOG_ERROR("One or more required pointers are null.");
        return false;
    }

    if (!BuildGeometry(desc))
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
    auto& subs = m_mesh.GetSubmeshesMutable();
    for (auto& sm : subs)
    {
        sm.FreeReserveSlot(m_pResourceHeap);
    }
    m_mesh.Reset();
    return true;
}

void kfe::KFEMeshSceneObject::Impl::Render(_In_ const KFE_RENDER_OBJECT_DESC& desc)
{
    if (m_bModelDirty)
    {
        if (m_modelPath.empty()) return;
        KFE_BUILD_OBJECT_DESC builder{};
        builder.Device = m_pDevice;
        builder.CommandList = desc.CommandList;
        builder.ResourceHeap = m_pResourceHeap;
        if (!BuildGeometry(builder))
        {
            LOG_ERROR("Failed to rebuild model.");
            m_bModelDirty = false; // probably failed!
            return;
        }
    }

    if (!m_bBuild) return;
    if (!m_mesh.IsValid()) return;

    auto* cmdListObj = desc.CommandList;
    if (!cmdListObj)
        return;

    ID3D12GraphicsCommandList* cmdList = cmdListObj;

    if (m_pResourceHeap)
    {
        auto& subs = m_mesh.GetSubmeshesMutable();
        for (auto& sm : subs)
        {
            sm.BindTextureFromPath(desc.CommandList, m_pDevice, m_pResourceHeap);
        }
    }
    else 
    {
        LOG_ERROR("There's no resource heap allocated for model!");
        return;
    }

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

    //~ Free Resources
    auto& subs = m_mesh.GetSubmeshesMutable();
    for (auto& sm : subs)
    {
        sm.FreeReserveSlot(m_pResourceHeap);
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

    for (auto& sm : subs)
    {
        if (!sm.AllocateReserveSlot(desc.ResourceHeap))
        {
            LOG_ERROR("Failed to allocate srv slots for model!");
            return false;
        }
    }
    m_bBuild = true;
    LOG_SUCCESS("Model Built!");
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
    const std::size_t   slotCount = static_cast<std::size_t>(EModelTextureSlot::Count);

    for (const auto& kv : m_cbTexLazy)
    {
        const std::uint32_t idx = kv.first;
        if (idx >= subCount)
            continue;

        const auto& slots = kv.second;

        if (slots.size() < slotCount)
        {
            LOG_WARNING(
                "Submesh {} cache has {} slots, expected {}. Skipping.",
                idx, slots.size(), slotCount);
            continue;
        }

        auto& sm = subs[idx];

        sm.SetBaseColor(slots[static_cast<std::size_t>(EModelTextureSlot::BaseColor)]);
        sm.SetNormal(slots[static_cast<std::size_t>(EModelTextureSlot::Normal)]);
        sm.SetORM(slots[static_cast<std::size_t>(EModelTextureSlot::ORM)]);
        sm.SetEmissive(slots[static_cast<std::size_t>(EModelTextureSlot::Emissive)]);

        sm.SetRoughness(slots[static_cast<std::size_t>(EModelTextureSlot::Roughness)]);
        sm.SetMetallic(slots[static_cast<std::size_t>(EModelTextureSlot::Metallic)]);
        sm.SetOcclusion(slots[static_cast<std::size_t>(EModelTextureSlot::Occlusion)]);

        sm.SetOpacity(slots[static_cast<std::size_t>(EModelTextureSlot::Opacity)]);

        sm.SetHeight(slots[static_cast<std::size_t>(EModelTextureSlot::Height)]);
        sm.SetDisplacement(slots[static_cast<std::size_t>(EModelTextureSlot::Displacement)]);

        sm.SetSpecular(slots[static_cast<std::size_t>(EModelTextureSlot::Specular)]);
        sm.SetGlossiness(slots[static_cast<std::size_t>(EModelTextureSlot::Glossiness)]);

        sm.SetDetailNormal(slots[static_cast<std::size_t>(EModelTextureSlot::DetailNormal)]);
        sm.SetShadowMap(slots[static_cast<std::size_t>(EModelTextureSlot::ShadowMap)]);

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

    KFE_FRAME_CONSTANT_BUFFER_DESC buffer{};
    buffer.Device       = desc.Device;
    buffer.FrameCount   = 2u;
    buffer.ResourceHeap = desc.ResourceHeap;
    buffer.SizeInBytes  = sizeof(KFE_COMMON_CB_GPU);

    for (std::size_t i = 0; i < submeshes.size(); ++i)
    {
        auto& sm = submeshes[i];
        buffer.SizeInBytes = sizeof(KFE_COMMON_CB_GPU);
        if (!sm.ConstantBuffer.Initialize(buffer))
        {
            LOG_ERROR("Failed to build submesh CB buffer (i={})", i);
            return false;
        }

        buffer.SizeInBytes = sizeof(ModelTextureMetaInformation);
        if (!sm.MetaConstantBuffer.Initialize(buffer))
        {
            LOG_ERROR("Failed to build submesh Meta CB buffer (i={})", i);
            return false;
        }
    }

    LOG_SUCCESS("Per submesh constant buffers created! Count={}", static_cast<std::uint32_t>(submeshes.size()));
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

    using namespace DirectX;

    KFE_COMMON_CB_GPU init{};

    const XMMATRIX I = XMMatrixIdentity();

    // Matrices
    XMStoreFloat4x4(&init.WorldT, I);
    XMStoreFloat4x4(&init.WorldInvTransposeT, I);

    XMStoreFloat4x4(&init.ViewT, I);
    XMStoreFloat4x4(&init.ProjT, I);
    XMStoreFloat4x4(&init.ViewProjT, I);

    XMStoreFloat4x4(&init.OrthoT, I);

    // Camera
    init.CameraPosWS = { 0.0f, 0.0f, 0.0f };
    init.CameraNear = 0.1f;

    init.CameraForwardWS = { 0.0f, 0.0f, 1.0f };
    init.CameraFar = 1000.0f;

    init.CameraRightWS = { 1.0f, 0.0f, 0.0f };
    init._PadCamRight = 0.0f;

    init.CameraUpWS = { 0.0f, 1.0f, 0.0f };
    init._PadCamUp = 0.0f;

    // Object
    init.ObjectPosWS = { 0.0f, 0.0f, 0.0f };
    init._PadObjPos = 0.0f;

    init.PlayerPosWS = { 0.0f, 0.0f, 0.0f };
    init._PadPlayerPos = 0.0f;

    // Render target
    init.Resolution = { 1.0f, 1.0f };
    init.InvResolution = { 1.0f, 1.0f };

    init.MousePosPixels = { 0.0f, 0.0f };
    init.MousePosNDC = { 0.0f, 0.0f };

    // Time
    init.Time = 0.0f;
    init.DeltaTime = 0.0f;
    init._PadTime0 = 0.0f;
    init._PadTime1 = 0.0f;

    // Lights / flags
    init.NumTotalLights = m_pObject->m_lightManager.GetPackedCount();
    init.RenderFlags = 0u;
    init._PadFlags0 = 0u;
    init._PadFlags1 = 0u;

    for (std::uint32_t i = 0u; i < submeshCount; ++i)
        m_cbData.emplace(i, init);
}

void kfe::KFEMeshSceneObject::Impl::CacheNodeMeshesRecursive(const KFEModelNode& node) noexcept
{
    using namespace DirectX;

    const XMMATRIX nodeLocal = node.GetMatrix();
    const XMMATRIX nodeLocalT = XMMatrixTranspose(nodeLocal);

    const XMMATRIX I = XMMatrixIdentity();

    for (std::uint32_t meshIndex : node.MeshIndices)
    {
        auto it = m_cbData.find(meshIndex);

        if (it == m_cbData.end())
        {
            KFE_COMMON_CB_GPU cb{};

            // Matrices
            XMStoreFloat4x4(&cb.WorldT, nodeLocalT);

            // World inverse transpose (already transposed for *_T fields)
            {
                const XMMATRIX Winv = XMMatrixInverse(nullptr, nodeLocal);
                const XMMATRIX WIT = XMMatrixTranspose(Winv);
                XMStoreFloat4x4(&cb.WorldInvTransposeT, WIT);
            }

            XMStoreFloat4x4(&cb.ViewT, I);
            XMStoreFloat4x4(&cb.ProjT, I);
            XMStoreFloat4x4(&cb.ViewProjT, I);
            XMStoreFloat4x4(&cb.OrthoT, I);

            // Camera
            cb.CameraPosWS = { 0.0f, 0.0f, 0.0f };
            cb.CameraNear = 0.1f;

            cb.CameraForwardWS = { 0.0f, 0.0f, 1.0f };
            cb.CameraFar = 1000.0f;

            cb.CameraRightWS = { 1.0f, 0.0f, 0.0f };
            cb._PadCamRight = 0.0f;

            cb.CameraUpWS = { 0.0f, 1.0f, 0.0f };
            cb._PadCamUp = 0.0f;

            // Object / player
            cb.ObjectPosWS = { 0.0f, 0.0f, 0.0f };
            cb._PadObjPos = 0.0f;

            cb.PlayerPosWS = { 0.0f, 0.0f, 0.0f };
            cb._PadPlayerPos = 0.0f;

            // Render target / viewport
            cb.Resolution = { 1.0f, 1.0f };
            cb.InvResolution = { 1.0f, 1.0f };
            cb.MousePosPixels = { 0.0f, 0.0f };
            cb.MousePosNDC = { 0.0f, 0.0f };

            // Time
            cb.Time = 0.0f;
            cb.DeltaTime = 0.0f;
            cb._PadTime0 = 0.0f;
            cb._PadTime1 = 0.0f;

            // Lights / flags
            cb.NumTotalLights = 0u;
            cb.RenderFlags = 0u;
            cb._PadFlags0 = 0u;
            cb._PadFlags1 = 0u;

            m_cbData.emplace(meshIndex, cb);
        }
        else
        {
            XMStoreFloat4x4(&it->second.WorldT, nodeLocalT);

            {
                const XMMATRIX Winv = XMMatrixInverse(nullptr, nodeLocal);
                const XMMATRIX WIT = XMMatrixTranspose(Winv);
                XMStoreFloat4x4(&it->second.WorldInvTransposeT, WIT);
            }
        }
    }

    for (const auto& child : node.Children)
    {
        if (child)
            CacheNodeMeshesRecursive(*child);
    }
}

void kfe::KFEMeshSceneObject::Impl::UpdateSubmeshConstantBuffers(const KFE_UPDATE_OBJECT_DESC& desc)
{
    if (!m_bBuild)       return;
    if (!m_mesh.IsValid()) return;

    using namespace DirectX;

    const auto& submeshes = m_mesh.GetSubmeshes();

    for (const auto& sm : submeshes)
    {
        if (!sm.ConstantBuffer.IsInitialized())
            continue;

        auto* cv = static_cast<KFE_COMMON_CB_GPU*>(sm.ConstantBuffer.GetMappedData());
        if (!cv)
            continue;

        XMStoreFloat4x4(&cv->ViewT, desc.ViewMatrixT);
        XMStoreFloat4x4(&cv->ProjT, desc.PerpectiveMatrixT);
        XMStoreFloat4x4(&cv->OrthoT, desc.OrthographicMatrixT);

        // ViewProjT = ViewT * ProjT
        {
            const XMMATRIX VP_T = XMMatrixMultiply(desc.ViewMatrixT, desc.PerpectiveMatrixT);
            XMStoreFloat4x4(&cv->ViewProjT, VP_T);
        }

        // World + WorldInvTranspose
        if (m_pObject)
        {
            const XMMATRIX W = m_pObject->GetWorldMatrix();
            const XMMATRIX WT = XMMatrixTranspose(W);
            XMStoreFloat4x4(&cv->WorldT, WT);

            // WorldInvTransposeT
            const XMMATRIX Winv = XMMatrixInverse(nullptr, W);
            const XMMATRIX WIT = XMMatrixTranspose(Winv);
            XMStoreFloat4x4(&cv->WorldInvTransposeT, WIT);

            cv->ObjectPosWS = m_pObject->Transform.Position;
            cv->_PadObjPos = 0.0f;
        }
        else
        {
            const XMMATRIX I = XMMatrixIdentity();
            XMStoreFloat4x4(&cv->WorldT, I);
            XMStoreFloat4x4(&cv->WorldInvTransposeT, I);

            cv->ObjectPosWS = { 0.0f, 0.0f, 0.0f };
            cv->_PadObjPos = 0.0f;
        }

        // Camera
        cv->CameraPosWS = desc.CameraPosition;
        cv->CameraNear = desc.ZNear;

        cv->CameraForwardWS = desc.CameraForwardWS;
        cv->CameraFar = desc.ZFar;

        cv->CameraRightWS = desc.CameraRightWS;
        cv->_PadCamRight = 0.0f;

        cv->CameraUpWS = desc.CameraUpWS;
        cv->_PadCamUp = 0.0f;

        // Player
        cv->PlayerPosWS = desc.PlayerPosition;
        cv->_PadPlayerPos = 0.0f;

        // Render target / viewport
        cv->Resolution = desc.Resolution;
        cv->InvResolution =
        {
            (desc.Resolution.x != 0.0f) ? (1.0f / desc.Resolution.x) : 0.0f,
            (desc.Resolution.y != 0.0f) ? (1.0f / desc.Resolution.y) : 0.0f
        };

        cv->MousePosPixels = desc.MousePosition;
        cv->MousePosNDC =
        {
            (desc.Resolution.x != 0.0f) ? ((desc.MousePosition.x / desc.Resolution.x) * 2.0f - 1.0f) : 0.0f,
            (desc.Resolution.y != 0.0f) ? (1.0f - (desc.MousePosition.y / desc.Resolution.y) * 2.0f) : 0.0f
        };

        // Time
        cv->Time = desc.Time;
        cv->DeltaTime = desc.DeltaTime;
        cv->_PadTime0 = 0.0f;
        cv->_PadTime1 = 0.0f;

        // Lights / flags
        const std::uint32_t packedLights =
            (m_pObject) ? m_pObject->m_lightManager.GetPackedCount() : 0u;

        cv->NumTotalLights = packedLights;
        cv->RenderFlags = 0u;
        cv->_PadFlags0 = 0u;
        cv->_PadFlags1 = 0u;
    }
}

void kfe::KFEMeshSceneObject::Impl::UpdateCBDataForNodeMeshes(const KFEModelNode& node) noexcept
{
    using namespace DirectX;

    const XMMATRIX local = node.GetMatrix();
    const XMMATRIX localT = XMMatrixTranspose(local);

    const XMMATRIX I = XMMatrixIdentity();

    for (std::uint32_t meshIndex : node.MeshIndices)
    {
        auto it = m_cbData.find(meshIndex);

        if (it == m_cbData.end())
        {
            KFE_COMMON_CB_GPU cb{};

            // Matrices
            XMStoreFloat4x4(&cb.WorldT, localT);

            // Inverse transpose for normals
            {
                const XMMATRIX Linv = XMMatrixInverse(nullptr, local);
                const XMMATRIX LIT = XMMatrixTranspose(Linv);
                XMStoreFloat4x4(&cb.WorldInvTransposeT, LIT);
            }

            XMStoreFloat4x4(&cb.ViewT, I);
            XMStoreFloat4x4(&cb.ProjT, I);
            XMStoreFloat4x4(&cb.ViewProjT, I);
            XMStoreFloat4x4(&cb.OrthoT, I);

            // Camera defaults
            cb.CameraPosWS = { 0.0f, 0.0f, 0.0f };
            cb.CameraNear = 0.1f;

            cb.CameraForwardWS = { 0.0f, 0.0f, 1.0f };
            cb.CameraFar = 1000.0f;

            cb.CameraRightWS = { 1.0f, 0.0f, 0.0f };
            cb._PadCamRight = 0.0f;

            cb.CameraUpWS = { 0.0f, 1.0f, 0.0f };
            cb._PadCamUp = 0.0f;

            // Object
            cb.ObjectPosWS = { 0.0f, 0.0f, 0.0f };
            cb._PadObjPos = 0.0f;

            cb.PlayerPosWS = { 0.0f, 0.0f, 0.0f };
            cb._PadPlayerPos = 0.0f;

            // Render target
            cb.Resolution = { 1.0f, 1.0f };
            cb.InvResolution = { 1.0f, 1.0f };
            cb.MousePosPixels = { 0.0f, 0.0f };
            cb.MousePosNDC = { 0.0f, 0.0f };

            // Time defaults
            cb.Time = 0.0f;
            cb.DeltaTime = 0.0f;
            cb._PadTime0 = 0.0f;
            cb._PadTime1 = 0.0f;

            // Lights
            cb.NumTotalLights = m_pObject->m_lightManager.GetPackedCount();
            cb.RenderFlags    = 0u;
            cb._PadFlags0     = 0u;
            cb._PadFlags1     = 0u;

            m_cbData.emplace(meshIndex, cb);
        }
        else
        {
            // Update world derived data only
            XMStoreFloat4x4(&it->second.WorldT, localT);

            const XMMATRIX Linv = XMMatrixInverse(nullptr, local);
            const XMMATRIX LIT  = XMMatrixTranspose(Linv);
            XMStoreFloat4x4(&it->second.WorldInvTransposeT, LIT);
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

    // Node hierarchy world (CPU-space, NOT transposed)
    const XMMATRIX local     = node.GetMatrix();
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
        // Per submesh b0
        if (sm.ConstantBuffer.IsInitialized())
        {
            sm.ConstantBuffer.Step();
            XMMATRIX cachedLocal = XMMatrixIdentity();

            auto it = m_cbData.find(meshIndex);
            if (it != m_cbData.end())
            {
                const XMMATRIX cachedLocalT = XMLoadFloat4x4(&it->second.WorldT);
                cachedLocal = XMMatrixTranspose(cachedLocalT);
            }

            const XMMATRIX finalWorld = cachedLocal * nodeWorld;

            if (auto* cv = static_cast<KFE_COMMON_CB_GPU*>(sm.ConstantBuffer.GetMappedData()))
            {
                // Store transposed for GPU
                XMStoreFloat4x4(&cv->WorldT, XMMatrixTranspose(finalWorld));

                // Inverse transpose for normals
                const XMMATRIX Winv = XMMatrixInverse(nullptr, finalWorld);
                const XMMATRIX WIT = XMMatrixTranspose(Winv);
                XMStoreFloat4x4(&cv->WorldInvTransposeT, WIT);

                cv->ObjectPosWS = m_pObject->Transform.Position;
            }

            cmdList->SetGraphicsRootConstantBufferView(
                0u,
                sm.ConstantBuffer.GetView()->GetGPUVirtualAddress());
        }

        //SRV table update
        sm.BindTextureFromPath(cmdList, m_pDevice, m_pResourceHeap);

        if (sm.GetBaseSrvIndex() != KFE_INVALID_INDEX)
        {
            cmdList->SetGraphicsRootDescriptorTable(
                1u,
                m_pResourceHeap->GetGPUHandle(sm.GetBaseSrvIndex()));
        }

        // b1 (meta) update + bind CBV
        if (sm.MetaConstantBuffer.IsInitialized())
        {
            sm.MetaConstantBuffer.Step();
            UpdateMetaCB(sm);

            cmdList->SetGraphicsRootConstantBufferView(
                2u,
                sm.MetaConstantBuffer.GetView()->GetGPUVirtualAddress());
        }

        const D3D12_VERTEX_BUFFER_VIEW vb = vbView->GetView();
        const D3D12_INDEX_BUFFER_VIEW  ib = ibView->GetView();

        cmdList->IASetVertexBuffers(0u, 1u, &vb);
        cmdList->IASetIndexBuffer(&ib);

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

JsonLoader kfe::KFEMeshSceneObject::Impl::GetJsonData() const noexcept
{
    JsonLoader root{};
    // Basic properties
    root["ModelPath"]          = m_modelPath;
    root["Submesh"]            = GetChildTransformation();
    root["MetaInformation"]    = GetChildMetaInformation();
    root["TextureInformation"] = GetChildTextureInformation();
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

            //~ Displacement
            dst["Displacement"]["IsTextureAttached"] = meta.Displacement.IsTextureAttached;
            dst["Displacement"]["DisplacementScale"] = meta.Displacement.DisplacementScale;
            dst["Displacement"]["UvTilingX"] = meta.Displacement.UvTilingX;
            dst["Displacement"]["UvTilingY"] = meta.Displacement.UvTilingY;

            //~ Specular
            dst["Specular"]["IsTextureAttached"] = meta.Specular.IsTextureAttached;
            dst["Specular"]["Strength"] = meta.Specular.Strength;
            dst["Specular"]["UvTilingX"] = meta.Specular.UvTilingX;
            dst["Specular"]["UvTilingY"] = meta.Specular.UvTilingY;

            //~ Glossiness
            dst["Glossiness"]["IsTextureAttached"] = meta.Glossiness.IsTextureAttached;
            dst["Glossiness"]["Strength"] = meta.Glossiness.Strength;
            dst["Glossiness"]["UvTilingX"] = meta.Glossiness.UvTilingX;
            dst["Glossiness"]["UvTilingY"] = meta.Glossiness.UvTilingY;

            //~ DetailNormal
            dst["DetailNormal"]["IsTextureAttached"] = meta.DetailNormal.IsTextureAttached;
            dst["DetailNormal"]["NormalStrength"] = meta.DetailNormal.NormalStrength;
            dst["DetailNormal"]["UvTilingX"] = meta.DetailNormal.UvTilingX;
            dst["DetailNormal"]["UvTilingY"] = meta.DetailNormal.UvTilingY;

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

            WritePath(dst, "Roughness", sm.GetRoughnessPath());
            WritePath(dst, "Metallic", sm.GetMetallicPath());
            WritePath(dst, "Occlusion", sm.GetOcclusionPath());

            WritePath(dst, "Opacity", sm.GetOpacityPath());

            WritePath(dst, "Height", sm.GetHeightPath());
            WritePath(dst, "Displacement", sm.GetDisplacementPath());

            WritePath(dst, "Specular", sm.GetSpecularPath());
            WritePath(dst, "Glossiness", sm.GetGlossinessPath());

            WritePath(dst, "DetailNormal", sm.GetDetailNormalPath());
            WritePath(dst, "ShadowMap", sm.GetShadowMapPath());
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

            //~ Displacement
            if (src.Has("Displacement"))
            {
                const auto& d = src["Displacement"];
                meta.Displacement.IsTextureAttached = ReadFloat(d, "IsTextureAttached", meta.Displacement.IsTextureAttached);
                meta.Displacement.DisplacementScale = ReadFloat(d, "DisplacementScale", meta.Displacement.DisplacementScale);
                meta.Displacement.UvTilingX = ReadFloat(d, "UvTilingX", meta.Displacement.UvTilingX);
                meta.Displacement.UvTilingY = ReadFloat(d, "UvTilingY", meta.Displacement.UvTilingY);
            }

            //~ Specular
            if (src.Has("Specular"))
            {
                const auto& s = src["Specular"];
                meta.Specular.IsTextureAttached = ReadFloat(s, "IsTextureAttached", meta.Specular.IsTextureAttached);
                meta.Specular.Strength = ReadFloat(s, "Strength", meta.Specular.Strength);
                meta.Specular.UvTilingX = ReadFloat(s, "UvTilingX", meta.Specular.UvTilingX);
                meta.Specular.UvTilingY = ReadFloat(s, "UvTilingY", meta.Specular.UvTilingY);
            }

            //~ Glossiness
            if (src.Has("Glossiness"))
            {
                const auto& g = src["Glossiness"];
                meta.Glossiness.IsTextureAttached = ReadFloat(g, "IsTextureAttached", meta.Glossiness.IsTextureAttached);
                meta.Glossiness.Strength = ReadFloat(g, "Strength", meta.Glossiness.Strength);
                meta.Glossiness.UvTilingX = ReadFloat(g, "UvTilingX", meta.Glossiness.UvTilingX);
                meta.Glossiness.UvTilingY = ReadFloat(g, "UvTilingY", meta.Glossiness.UvTilingY);
            }

            //~ DetailNormal
            if (src.Has("DetailNormal"))
            {
                const auto& dn = src["DetailNormal"];
                meta.DetailNormal.IsTextureAttached = ReadFloat(dn, "IsTextureAttached", meta.DetailNormal.IsTextureAttached);
                meta.DetailNormal.NormalStrength = ReadFloat(dn, "NormalStrength", meta.DetailNormal.NormalStrength);
                meta.DetailNormal.UvTilingX = ReadFloat(dn, "UvTilingX", meta.DetailNormal.UvTilingX);
                meta.DetailNormal.UvTilingY = ReadFloat(dn, "UvTilingY", meta.DetailNormal.UvTilingY);
            }

            //~ Singular (Occlusion/Roughness/Metallic)
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

            //~ Global
            if (src.Has("Global"))
            {
                const auto& g = src["Global"];
                meta.ForcedMipLevel = ReadFloat(g, "ForcedMipLevel", meta.ForcedMipLevel);
                meta.UseForcedMip = ReadFloat(g, "UseForcedMip", meta.UseForcedMip);
            }
        };

    for (std::uint32_t i = 0;; ++i)
    {
        const std::string idx = std::to_string(i);
        if (!metaRoot->Has(idx))
            break;

        const JsonLoader& entry = (*metaRoot)[idx];

        if (!entry.Has("Meta"))
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
        std::array<std::string, static_cast<std::size_t>(EModelTextureSlot::Count)>& slots) noexcept
        {
            slots.fill(std::string{});

            ReadPath(texNode, "BaseColor", slots[static_cast<std::size_t>(EModelTextureSlot::BaseColor)]);
            ReadPath(texNode, "Normal", slots[static_cast<std::size_t>(EModelTextureSlot::Normal)]);
            ReadPath(texNode, "ORM", slots[static_cast<std::size_t>(EModelTextureSlot::ORM)]);
            ReadPath(texNode, "Emissive", slots[static_cast<std::size_t>(EModelTextureSlot::Emissive)]);

            ReadPath(texNode, "Roughness", slots[static_cast<std::size_t>(EModelTextureSlot::Roughness)]);
            ReadPath(texNode, "Metallic", slots[static_cast<std::size_t>(EModelTextureSlot::Metallic)]);
            ReadPath(texNode, "Occlusion", slots[static_cast<std::size_t>(EModelTextureSlot::Occlusion)]);

            ReadPath(texNode, "Opacity", slots[static_cast<std::size_t>(EModelTextureSlot::Opacity)]);

            ReadPath(texNode, "Height", slots[static_cast<std::size_t>(EModelTextureSlot::Height)]);
            ReadPath(texNode, "Displacement", slots[static_cast<std::size_t>(EModelTextureSlot::Displacement)]);

            ReadPath(texNode, "Specular", slots[static_cast<std::size_t>(EModelTextureSlot::Specular)]);
            ReadPath(texNode, "Glossiness", slots[static_cast<std::size_t>(EModelTextureSlot::Glossiness)]);

            ReadPath(texNode, "DetailNormal", slots[static_cast<std::size_t>(EModelTextureSlot::DetailNormal)]);
            ReadPath(texNode, "ShadowMap", slots[static_cast<std::size_t>(EModelTextureSlot::ShadowMap)]);
        };

    // If mesh isn't built yet: cache lazily
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

            std::array<std::string, static_cast<std::size_t>(EModelTextureSlot::Count)> slots{};
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

        std::array<std::string, static_cast<std::size_t>(EModelTextureSlot::Count)> slots{};
        FillSlots(entry["Textures"], slots);

        auto& sm = subs[i];

        sm.SetBaseColor(slots[static_cast<std::size_t>(EModelTextureSlot::BaseColor)]);
        sm.SetNormal(slots[static_cast<std::size_t>(EModelTextureSlot::Normal)]);
        sm.SetORM(slots[static_cast<std::size_t>(EModelTextureSlot::ORM)]);
        sm.SetEmissive(slots[static_cast<std::size_t>(EModelTextureSlot::Emissive)]);

        sm.SetRoughness(slots[static_cast<std::size_t>(EModelTextureSlot::Roughness)]);
        sm.SetMetallic(slots[static_cast<std::size_t>(EModelTextureSlot::Metallic)]);
        sm.SetOcclusion(slots[static_cast<std::size_t>(EModelTextureSlot::Occlusion)]);

        sm.SetOpacity(slots[static_cast<std::size_t>(EModelTextureSlot::Opacity)]);

        sm.SetHeight(slots[static_cast<std::size_t>(EModelTextureSlot::Height)]);
        sm.SetDisplacement(slots[static_cast<std::size_t>(EModelTextureSlot::Displacement)]);

        sm.SetSpecular(slots[static_cast<std::size_t>(EModelTextureSlot::Specular)]);
        sm.SetGlossiness(slots[static_cast<std::size_t>(EModelTextureSlot::Glossiness)]);

        sm.SetDetailNormal(slots[static_cast<std::size_t>(EModelTextureSlot::DetailNormal)]);
        sm.SetShadowMap(slots[static_cast<std::size_t>(EModelTextureSlot::ShadowMap)]);

        sm.m_bTextureDirty = true;
    }
}

void kfe::KFEMeshSceneObject::Impl::ImguiViewHeader(float deltaTime)
{
    (void)deltaTime;

    ImGui::SeparatorText("Mesh");

    auto EditModelPath = [&](const char* label)
        {
            char buf[260]{};

            const size_t maxCopy = sizeof(buf) - 1;
            const size_t count = std::min(m_modelPath.size(), maxCopy);

            m_modelPath.copy(buf, count);
            buf[count] = '\0';

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);
            ImGui::SameLine();

            ImGui::SetNextItemWidth(-80.0f);
            if (ImGui::InputText("##ModelPath", buf, sizeof(buf)))
            {
                m_modelPath = std::string{ buf };
                m_bModelDirty = true;
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
                        m_modelPath = std::move(pathUtf8);
                        LOG_INFO("Model path set to: {}", m_modelPath);
                        m_bModelDirty = true;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::SameLine();
            if (ImGui::Button("X##ClearModelPath"))
            {
                m_modelPath.clear();
                m_bModelDirty = true;
            }

            ImGui::SameLine();
            if (ImGui::Button("Apply##ApplyModelPath"))
            {
                m_bModelDirty = true;
            }
        };

    EditModelPath("Model Path");

    if (m_bModelDirty)
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Pending: model reload");
}

void kfe::KFEMeshSceneObject::Impl::ImguiView(float dt)
{
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

    // Pick a nicer display root (skip dummy nodes)
    KFEModelNode* displayRoot = root;
    while (displayRoot &&
        displayRoot->Children.size() == 1 &&
        !displayRoot->HasMeshes())
    {
        displayRoot = displayRoot->Children[0].get();
    }
    if (!displayRoot)
        displayRoot = root;

    // -----------------------------
    // Selection stored as a "path"
    // Example: "0/2/1" (child indices)
    // -----------------------------
    static std::string s_selectedPath;

    auto NodeLabel = [](const KFEModelNode* n) -> const char*
        {
            if (!n) return "<null>";
            return n->Name.empty() ? "(unnamed)" : n->Name.c_str();
        };

    auto MakePathFromPointers = [&](KFEModelNode* baseRoot, KFEModelNode* target) -> std::string
        {
            // Find path by DFS and record indices
            std::vector<int> stack;

            auto Dfs = [&](auto&& self, KFEModelNode* node) -> bool
                {
                    if (!node) return false;
                    if (node == target) return true;

                    for (int i = 0; i < (int)node->Children.size(); ++i)
                    {
                        stack.push_back(i);
                        if (self(self, node->Children[i].get()))
                            return true;
                        stack.pop_back();
                    }
                    return false;
                };

            stack.clear();
            if (!Dfs(Dfs, baseRoot))
                return {};

            // Build "0/2/1"
            std::string out;
            out.reserve(stack.size() * 3);
            for (size_t i = 0; i < stack.size(); ++i)
            {
                if (i) out.push_back('/');
                out += std::to_string(stack[i]);
            }
            return out;
        };

    auto FindByPath = [&](KFEModelNode* baseRoot, const std::string& path) -> KFEModelNode*
        {
            if (!baseRoot)
                return nullptr;

            if (path.empty())
                return baseRoot;

            KFEModelNode* cur = baseRoot;

            size_t start = 0;
            while (start < path.size())
            {
                size_t slash = path.find('/', start);
                const size_t end = (slash == std::string::npos) ? path.size() : slash;

                const std::string token = path.substr(start, end - start);
                if (token.empty())
                    return nullptr;

                const int idx = std::atoi(token.c_str());
                if (idx < 0 || idx >= (int)cur->Children.size())
                    return nullptr;

                cur = cur->Children[idx].get();
                if (!cur)
                    return nullptr;

                if (slash == std::string::npos)
                    break;
                start = slash + 1;
            }

            return cur;
        };

    // Initialize selection if empty
    if (s_selectedPath.empty())
        s_selectedPath = ""; // empty = displayRoot itself

    // Reacquire selected pointer every frame (SAFE even after rebuild)
    KFEModelNode* selected = FindByPath(displayRoot, s_selectedPath);
    if (!selected)
    {
        // Selection path is no longer valid -> fallback
        selected = displayRoot;
        s_selectedPath.clear();
    }

    auto DrawNodeTreeRecursive = [&](auto&& self, KFEModelNode* node, const std::string& basePath) -> void
        {
            if (!node)
                return;

            ImGui::PushID(node);

            const bool isSelected = (basePath == s_selectedPath);

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanFullWidth;

            if (isSelected)
                flags |= ImGuiTreeNodeFlags_Selected;

            const bool hasChildren = node->HasChildren();
            if (!hasChildren)
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

            // Use label safely
            const bool opened = ImGui::TreeNodeEx(NodeLabel(node), flags);

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                s_selectedPath = basePath;

            if (hasChildren && opened)
            {
                for (int i = 0; i < (int)node->Children.size(); ++i)
                {
                    auto* child = node->Children[i].get();
                    if (!child) continue;

                    std::string childPath = basePath;
                    if (!childPath.empty()) childPath.push_back('/');
                    childPath += std::to_string(i);

                    self(self, child, childPath);
                }
                ImGui::TreePop();
            }

            ImGui::PopID();
        };

    auto EditNodeTRS = [&](KFEModelNode* node) -> void
        {
            if (!node)
                return;

            ImGui::PushID(node);

            ImGui::TextUnformatted("Selected: ");
            ImGui::SameLine();
            ImGui::TextUnformatted(NodeLabel(node));

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

    // Tree starts at displayRoot. Root path = "".
    DrawNodeTreeRecursive(DrawNodeTreeRecursive, displayRoot, std::string{});

    ImGui::NextColumn();

    ImGui::TextDisabled("Transform Editor");
    ImGui::Separator();

    // Reacquire again (paranoid safety if selection changed this frame)
    selected = FindByPath(displayRoot, s_selectedPath);
    if (!selected)
    {
        selected = displayRoot;
        s_selectedPath.clear();
    }

    EditNodeTRS(selected);

    ImGui::Columns(1);
}

void kfe::KFEMeshSceneObject::Impl::ImguiTextureMetaConfig(float dt)
{
    (void)dt;

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

    auto DrawDisplacementPair = [&](kfe::KFEModelSubmesh& sm) -> bool
        {
            bool metaDirty = false;

            if (ImGui::TreeNode("Displacement"))
            {
                if (ImGui::TreeNode("Displacement Texture"))
                {
                    EditPath("Path",
                        sm.m_srvs[static_cast<std::size_t>(EModelTextureSlot::Displacement)].TexturePath,
                        [&sm](const std::string& p) { sm.SetDisplacement(p); });

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Meta Displacement"))
                {
                    auto& d = sm.m_textureMetaInformation.Displacement;

                    metaDirty |= CheckboxFloat01("Attached", &d.IsTextureAttached);
                    metaDirty |= ImGui::DragFloat("Displacement Scale", &d.DisplacementScale, 0.001f, 0.0f, 0.5f);
                    metaDirty |= ImGui::DragFloat2("UV Tiling", &d.UvTilingX, 0.01f, 0.01f, 100.0f);

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            return metaDirty;
        };

    auto DrawSpecGlossPair = [&](kfe::KFEModelSubmesh& sm) -> bool
        {
            bool metaDirty = false;

            if (ImGui::TreeNode("Specular / Glossiness"))
            {
                if (ImGui::TreeNode("Specular Texture"))
                {
                    EditPath("Path",
                        sm.m_srvs[static_cast<std::size_t>(EModelTextureSlot::Specular)].TexturePath,
                        [&sm](const std::string& p) { sm.SetSpecular(p); });

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Glossiness Texture"))
                {
                    EditPath("Path",
                        sm.m_srvs[static_cast<std::size_t>(EModelTextureSlot::Glossiness)].TexturePath,
                        [&sm](const std::string& p) { sm.SetGlossiness(p); });

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Meta Specular / Glossiness"))
                {
                    auto& s = sm.m_textureMetaInformation.Specular;
                    auto& g = sm.m_textureMetaInformation.Glossiness;

                    metaDirty |= CheckboxFloat01("Specular Attached", &s.IsTextureAttached);
                    metaDirty |= ImGui::DragFloat("Specular Strength", &s.Strength, 0.01f, 0.0f, 10.0f);
                    metaDirty |= ImGui::DragFloat2("Specular UV Tiling", &s.UvTilingX, 0.01f, 0.01f, 100.0f);

                    metaDirty |= CheckboxFloat01("Glossiness Attached", &g.IsTextureAttached);
                    metaDirty |= ImGui::DragFloat("Glossiness Strength", &g.Strength, 0.01f, 0.0f, 10.0f);
                    metaDirty |= ImGui::DragFloat2("Glossiness UV Tiling", &g.UvTilingX, 0.01f, 0.01f, 100.0f);

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            return metaDirty;
        };

    auto DrawDetailNormalPair = [&](kfe::KFEModelSubmesh& sm) -> bool
        {
            bool metaDirty = false;

            if (ImGui::TreeNode("Detail Normal"))
            {
                if (ImGui::TreeNode("Detail Normal Texture"))
                {
                    EditPath("Path",
                        sm.m_srvs[static_cast<std::size_t>(EModelTextureSlot::DetailNormal)].TexturePath,
                        [&sm](const std::string& p) { sm.SetDetailNormal(p); });

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Meta Detail Normal"))
                {
                    auto& dn = sm.m_textureMetaInformation.DetailNormal;

                    metaDirty |= CheckboxFloat01("Attached", &dn.IsTextureAttached);
                    metaDirty |= ImGui::DragFloat("Normal Strength", &dn.NormalStrength, 0.01f, 0.0f, 5.0f);
                    metaDirty |= ImGui::DragFloat2("UV Tiling", &dn.UvTilingX, 0.01f, 0.01f, 100.0f);

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            return metaDirty;
        };

    auto DrawShadowMapPath = [&](kfe::KFEModelSubmesh& sm) -> bool
        {
            bool metaDirty = false;

            if (ImGui::TreeNode("Shadow Map"))
            {
                if (ImGui::TreeNode("Shadow Map SRV Path (debug)"))
                {
                    EditPath("Path",
                        sm.m_srvs[static_cast<std::size_t>(EModelTextureSlot::ShadowMap)].TexturePath,
                        [&sm](const std::string& p) { sm.SetShadowMap(p); });

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

                    // (Optional but you *do* have these in the struct — might as well expose them)
                    metaDirty |= ImGui::DragFloat2("Occlusion UV Tiling", &s.OcclusionTilingX, 0.01f, 0.01f, 100.0f);
                    metaDirty |= ImGui::DragFloat2("Roughness UV Tiling", &s.RoughnessTilingX, 0.01f, 0.01f, 100.0f);
                    metaDirty |= ImGui::DragFloat2("Metallic UV Tiling", &s.MetallicTilingX, 0.01f, 0.01f, 100.0f);

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
        metaDirty |= DrawDisplacementPair(sm);

        metaDirty |= DrawSplitORMPair(sm);

        metaDirty |= DrawSpecGlossPair(sm);
        metaDirty |= DrawDetailNormalPair(sm);

        metaDirty |= DrawShadowMapPath(sm);

        metaDirty |= DrawGlobalMeta(sm);

        if (metaDirty)
            sm.m_bMetaDirty = true;

        ImGui::PopID();
    }
}

#pragma endregion

