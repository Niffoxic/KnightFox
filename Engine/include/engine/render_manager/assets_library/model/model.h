// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  File      : model.h
 *  -----------------------------------------------------------------------------
 */
#pragma once

#include "EngineAPI.h"
#include "mesh_cache.h"

#include <DirectXMath.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <array>

#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/buffer/constant_buffer.h"
#include "engine/render_manager/api/commands/graphics_list.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"
#include "engine/utils/logger.h"
#include "engine/render_manager/api/texture/texture.h"
#include "engine/render_manager/api/texture/texture_srv.h"
#include "engine/render_manager/assets_library/texture_library.h"
#include "engine/utils/helpers.h"

namespace kfe
{
    struct KFEModelNode
    {
    public:
        std::string                                Name;
        std::vector<std::uint32_t>                 MeshIndices;
        std::vector<std::unique_ptr<KFEModelNode>> Children;

    public:

        void SetMatrixFromImport(const DirectX::XMFLOAT4X4& m,
            const DirectX::XMFLOAT3& pos,
            const DirectX::XMFLOAT3& rotDeg,
            const DirectX::XMFLOAT3& scale) noexcept
        {
            m_localMatrix = m;
            m_position    = pos;
            m_rotation    = rotDeg;
            m_scale       = scale;
            m_pivot       = { 0.0f, 0.0f, 0.0f };
            m_dirty       = false;
        }

        void SetPivotZero() noexcept
        {
            m_pivot = { 0.0f, 0.0f, 0.0f };
            MarkDirty();
        }

        bool HasMeshes  () const noexcept { return !MeshIndices.empty(); }
        bool HasChildren() const noexcept { return !Children.empty(); }

        bool IsEnabled() const noexcept { return m_enabled; }
        void SetEnabled(bool v) noexcept { m_enabled = v; }

        // Getters
        const DirectX::XMFLOAT3& GetPosition() const noexcept { return m_position; }
        const DirectX::XMFLOAT3& GetRotation() const noexcept { return m_rotation; } //~ degrees
        const DirectX::XMFLOAT3& GetScale() const noexcept { return m_scale; }
        const DirectX::XMFLOAT3& GetPivot() const noexcept { return m_pivot; }

        void SetPosition(const DirectX::XMFLOAT3& p) noexcept
        {
            m_position = p;
            MarkDirty();
        }

        void SetRotation(const DirectX::XMFLOAT3& rDeg) noexcept
        {
            m_rotation = rDeg;
            MarkDirty();
        }

        void SetScale(const DirectX::XMFLOAT3& s) noexcept
        {
            m_scale = s;
            MarkDirty();
        }

        void SetPivot(const DirectX::XMFLOAT3& p) noexcept
        {
            m_pivot = p;
            MarkDirty();
        }

        void SetTRS(const DirectX::XMFLOAT3& p,
            const DirectX::XMFLOAT3& rDeg,
            const DirectX::XMFLOAT3& s) noexcept
        {
            m_position = p;
            m_rotation = rDeg;
            m_scale = s;
            MarkDirty();
        }

        //~ Position
        void AddPositionX(float v) noexcept { m_position.x += v; MarkDirty(); }
        void AddPositionY(float v) noexcept { m_position.y += v; MarkDirty(); }
        void AddPositionZ(float v) noexcept { m_position.z += v; MarkDirty(); }

        void AddPosition(const DirectX::XMFLOAT3& dp) noexcept
        {
            m_position.x += dp.x;
            m_position.y += dp.y;
            m_position.z += dp.z;
            MarkDirty();
        }

        //~ Rotation
        void AddRotationX(float vDeg) noexcept { m_rotation.x += vDeg; MarkDirty(); }
        void AddRotationY(float vDeg) noexcept { m_rotation.y += vDeg; MarkDirty(); }
        void AddRotationZ(float vDeg) noexcept { m_rotation.z += vDeg; MarkDirty(); }

        void AddRotation(const DirectX::XMFLOAT3& drDeg) noexcept
        {
            m_rotation.x += drDeg.x;
            m_rotation.y += drDeg.y;
            m_rotation.z += drDeg.z;
            MarkDirty();
        }

        //~ Scale
        void AddScaleX(float v) noexcept { m_scale.x += v; MarkDirty(); }
        void AddScaleY(float v) noexcept { m_scale.y += v; MarkDirty(); }
        void AddScaleZ(float v) noexcept { m_scale.z += v; MarkDirty(); }

        void AddScale(const DirectX::XMFLOAT3& ds) noexcept
        {
            m_scale.x += ds.x;
            m_scale.y += ds.y;
            m_scale.z += ds.z;
            MarkDirty();
        }

        // Utilities
        void ResetTransform() noexcept
        {
            m_position = { 0.0f, 0.0f, 0.0f };
            m_rotation = { 0.0f, 0.0f, 0.0f };
            m_scale = { 1.0f, 1.0f, 1.0f };
            m_pivot = { 0.0f, 0.0f, 0.0f };
            MarkDirty();
        }

        // Matrix
        DirectX::XMMATRIX GetMatrix() const noexcept
        {
            RebuildMatrixIfDirty();
            return DirectX::XMLoadFloat4x4(&m_localMatrix);
        }

        const DirectX::XMFLOAT4X4& GetMatrixF4x4() const noexcept
        {
            RebuildMatrixIfDirty();
            return m_localMatrix;
        }

    private:
        void MarkDirty() const noexcept { m_dirty = true; }

        void RebuildMatrixIfDirty() const noexcept
        {
            if (!m_dirty)
                return;

            using namespace DirectX;

            const float rx = XMConvertToRadians(m_rotation.x);
            const float ry = XMConvertToRadians(m_rotation.y);
            const float rz = XMConvertToRadians(m_rotation.z);

            const XMMATRIX TpivotNeg = XMMatrixTranslation(-m_pivot.x, -m_pivot.y, -m_pivot.z);
            const XMMATRIX S = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
            const XMMATRIX R = XMMatrixRotationRollPitchYaw(rx, ry, rz);
            const XMMATRIX TpivotPos = XMMatrixTranslation(m_pivot.x, m_pivot.y, m_pivot.z);
            const XMMATRIX T = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);

            const XMMATRIX M = TpivotNeg * S * R * TpivotPos * T;

            XMStoreFloat4x4(&m_localMatrix, M);
            m_dirty = false;
        }

    private:
        DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 m_rotation{ 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 m_scale{ 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT3 m_pivot{ 0.0f, 0.0f, 0.0f };

        bool m_enabled{ true };

        mutable bool m_dirty{ true };

        mutable DirectX::XMFLOAT4X4 m_localMatrix
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
    };

    // A light submesh descriptor
    enum class EModelTextureSlot : std::uint32_t
    {
        BaseColor = 0,
        Normal,
        ORM,
        Emissive,
        Opacity,
        Height,

        Occlusion,
        Roughness,
        Metallic,

        DyeMask,

        Count
    };

    struct ModelTextureMetaInformation
    {
        struct BaseColorTexture
        {
            float IsTextureAttached{ 0.0f };
            float UvTilingX{ 1.0f };
            float UvTilingY{ 1.0f };
            float Strength{ 1.0f };
        } BaseColor;

        struct NormalTexture
        {
            float IsTextureAttached{ 0.0f };
            float NormalStrength{ 1.0f };
            float UvTilingX{ 1.0f };
            float UvTilingY{ 1.0f };
        } Normal;

        struct ORMTexture
        {
            float IsTextureAttached{ 0.0f };
            float IsMixed{ 1.0f };
            float UvTilingX{ 1.0f };
            float UvTilingY{ 1.0f };
        } ORM;

        struct EmissiveTexture
        {
            float IsTextureAttached{ 0.0f };
            float EmissiveIntensity{ 1.0f };
            float UvTilingX{ 1.0f };
            float UvTilingY{ 1.0f };
        } Emissive;

        struct OpacityTexture
        {
            float IsTextureAttached{ 0.0f };
            float AlphaMultiplier{ 1.0f };
            float AlphaCutoff{ 0.5f };
            float _Pad0{ 0.0f };
        } Opacity;

        struct HeightTexture
        {
            float IsTextureAttached{ 0.0f };
            float HeightScale{ 0.05f };
            float ParallaxMinLayers{ 8.0f };
            float ParallaxMaxLayers{ 32.0f };
        } Height;

        struct SingularOccRoughMetal
        {
            float IsOcclusionAttached{ 0.0f };
            float IsRoughnessAttached{ 0.0f };
            float IsMetallicAttached{ 0.0f };
            float _Pad0{ 0.0f };

            float OcclusionStrength{ 1.0f };
            float RoughnessValue{ 1.0f };
            float MetallicValue{ 0.0f };
            float _Pad1{ 0.0f };

            float OcclusionTilingX{ 1.0f };
            float OcclusionTilingY{ 1.0f };
            float RoughnessTilingX{ 1.0f };
            float RoughnessTilingY{ 1.0f };

            float MetallicTilingX{ 1.0f };
            float MetallicTilingY{ 1.0f };
            float _Pad2{ 0.0f };
            float _Pad3{ 0.0f };
        } Singular;

        struct Dye
        {
            float IsEnabled{ 0.0f };
            float Strength{ 1.0f };
            float _Pad0{ 0.0f };
            float _Pad1{ 0.0f };

            float Color[3]{ 1.0f, 1.0f, 1.0f };
            float _Pad2{ 0.0f };
        } Dye;

        float ForcedMipLevel{ 0.0f };
        float UseForcedMip{ 0.0f };
        float _Pad0{ 0.0f };
        float _Pad1{ 0.0f };
    };

    struct KFEModelSubmesh
    {
        std::uint32_t                      CacheMeshIndex = 0u;
        std::unique_ptr<KFEBuffer>         ConstantBuffer{ nullptr };
        std::unique_ptr<KFEConstantBuffer> CBView        { nullptr };

        std::unique_ptr<KFEBuffer>         MetaCB    { nullptr };
        std::unique_ptr<KFEConstantBuffer> MetaCBView{ nullptr };
        bool m_bMetaDirty{ true };

        ModelTextureMetaInformation m_textureMetaInformation{};

        struct SrvData
        {
            std::string    TexturePath{};
            KFETextureSRV* TextureSrv    { nullptr };
            std::uint32_t  ResourceHandle{ KFE_INVALID_INDEX };
            std::uint32_t  ReservedSlot  { KFE_INVALID_INDEX };
            bool           Dirty         { false };

            void Reset() noexcept
            {
                TexturePath.clear();
                TextureSrv     = nullptr;
                ResourceHandle = KFE_INVALID_INDEX;
                ReservedSlot   = KFE_INVALID_INDEX;
                Dirty          = false;
            }
        };
        std::array<SrvData, static_cast<std::size_t>(EModelTextureSlot::Count)> m_srvs;
        std::uint32_t m_baseSrvIndex{ KFE_INVALID_INDEX };
        bool          m_bTextureDirty{ true };

        KFEModelSubmesh() noexcept
        {
            for (auto& e : m_srvs)
                e.Reset();
        }

        bool AllocateReserveSolt(KFEResourceHeap* heap) noexcept
        {
            if (!heap)
                return false;

            const std::size_t count = static_cast<std::size_t>(EModelTextureSlot::Count);

            //~ already allocated
            if (m_baseSrvIndex != KFE_INVALID_INDEX)
                return true;

            //~ Allocate one contiguous block
            const std::uint32_t base = heap->Allocate(static_cast<std::uint32_t>(count));
            if (base == KFE_INVALID_INDEX)
                return false;

            m_baseSrvIndex = base;

            //~ Assign contiguous slots
            for (std::size_t i = 0; i < count; ++i)
            {
                auto& d = m_srvs[i];
                d.ReservedSlot = base + static_cast<std::uint32_t>(i);
                d.Dirty = true;
            }

            m_bTextureDirty = true;
            return true;
        }

        void FreeReserveSlot(KFEResourceHeap* heap) noexcept
        {
            if (!heap)
                return;

            for (auto& d : m_srvs)
            {
                if (d.ReservedSlot != KFE_INVALID_INDEX)
                {
                    heap->Free(d.ReservedSlot);
                    d.ReservedSlot = KFE_INVALID_INDEX;
                }

                d.TextureSrv = nullptr;
                d.ResourceHandle = KFE_INVALID_INDEX;
                d.Dirty = false;
            }

            m_baseSrvIndex = KFE_INVALID_INDEX;
            m_bTextureDirty = false;
        }

        bool BindTextureFromPath(KFEGraphicsCommandList* cmdList,
            KFEDevice* device,
            KFEResourceHeap* heap) noexcept
        {
            if (!m_bTextureDirty)
                return true;

            if (!cmdList || !device || !heap)
                return false;

            auto& pool = KFEImagePool::Instance();

            const std::size_t count = static_cast<std::size_t>(EModelTextureSlot::Count);

            std::size_t   firstValidIndex    = static_cast<std::size_t>(-1);
            std::uint32_t firstValidResource = KFE_INVALID_INDEX;

            for (std::size_t i = 0; i < count; ++i)
            {
                auto& data = m_srvs[i];

                if (!data.Dirty)
                    continue;

                if (data.ReservedSlot == KFE_INVALID_INDEX)
                {
                    LOG_ERROR("ModelSubmesh SRV slot {} has no ReservedSlot allocated!", i);
                    data.Dirty = false;
                    continue;
                }

                if (data.TexturePath.empty())
                {
                    data.TextureSrv = nullptr;
                    data.ResourceHandle = KFE_INVALID_INDEX;
                    data.Dirty = false;
                    continue;
                }

                if (!kfe_helpers::IsFile(data.TexturePath))
                {
                    LOG_ERROR("Texture '{}' does not exist!", data.TexturePath);
                    data.TextureSrv     = nullptr;
                    data.ResourceHandle = KFE_INVALID_INDEX;
                    data.Dirty          = false;
                    continue;
                }

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

                const D3D12_CPU_DESCRIPTOR_HANDLE src = heap->GetHandle(data.ResourceHandle);
                const D3D12_CPU_DESCRIPTOR_HANDLE dst = heap->GetHandle(data.ReservedSlot);

                device->GetNative()->CopyDescriptorsSimple(
                    1,
                    dst,
                    src,
                    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                if (firstValidResource == KFE_INVALID_INDEX)
                {
                    firstValidResource = data.ResourceHandle;
                    firstValidIndex = i;
                }

                data.Dirty = false;
            }

            if (firstValidResource != KFE_INVALID_INDEX)
            {
                const D3D12_CPU_DESCRIPTOR_HANDLE firstSrc = heap->GetHandle(firstValidResource);

                for (std::size_t i = 0; i < count; ++i)
                {
                    auto& data = m_srvs[i];

                    if (data.ReservedSlot == KFE_INVALID_INDEX)
                        continue;

                    if (data.ResourceHandle != KFE_INVALID_INDEX)
                        continue;

                    const D3D12_CPU_DESCRIPTOR_HANDLE dst = heap->GetHandle(data.ReservedSlot);

                    device->GetNative()->CopyDescriptorsSimple(
                        1,
                        dst,
                        firstSrc,
                        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                    data.ResourceHandle = firstValidResource;
                    data.TextureSrv = m_srvs[firstValidIndex].TextureSrv;
                }
            }

            m_bTextureDirty = false;
            return true;
        }

        const std::string& GetTexturePath(EModelTextureSlot tex) const noexcept
        {
            return m_srvs[static_cast<std::size_t>(tex)].TexturePath;
        }

        const std::string& GetBaseColorPath() const noexcept { return GetTexturePath(EModelTextureSlot::BaseColor); }
        const std::string& GetNormalPath()    const noexcept { return GetTexturePath(EModelTextureSlot::Normal); }
        const std::string& GetORMPath()       const noexcept { return GetTexturePath(EModelTextureSlot::ORM); }
        const std::string& GetEmissivePath()  const noexcept { return GetTexturePath(EModelTextureSlot::Emissive); }
        const std::string& GetOpacityPath()   const noexcept { return GetTexturePath(EModelTextureSlot::Opacity); }
        const std::string& GetHeightPath()    const noexcept { return GetTexturePath(EModelTextureSlot::Height); }

        const std::string& GetOcclusionPath() const noexcept { return GetTexturePath(EModelTextureSlot::Occlusion); }
        const std::string& GetRoughnessPath() const noexcept { return GetTexturePath(EModelTextureSlot::Roughness); }
        const std::string& GetMetallicPath()  const noexcept { return GetTexturePath(EModelTextureSlot::Metallic); }

        const std::string& GetDyeMaskPath()   const noexcept { return GetTexturePath(EModelTextureSlot::DyeMask); }

        //~ Helpers
        bool HasTexture(EModelTextureSlot tex) const noexcept
        {
            return !m_srvs[static_cast<std::size_t>(tex)].TexturePath.empty();
        }

        bool IsSlotDirty(EModelTextureSlot tex) const noexcept
        {
            return m_srvs[static_cast<std::size_t>(tex)].Dirty;
        }

        std::uint32_t GetReservedSlot(EModelTextureSlot tex) const noexcept
        {
            return m_srvs[static_cast<std::size_t>(tex)].ReservedSlot;
        }

        std::uint32_t GetResourceHandle(EModelTextureSlot tex) const noexcept
        {
            return m_srvs[static_cast<std::size_t>(tex)].ResourceHandle;
        }

        KFETextureSRV* GetTextureSrv(EModelTextureSlot tex) const noexcept
        {
            return m_srvs[static_cast<std::size_t>(tex)].TextureSrv;
        }

        std::uint32_t GetBaseSrvIndex() const noexcept { return m_baseSrvIndex; }

        //~ set textures
        void SetTexture(EModelTextureSlot tex, const std::string& path) noexcept
        {
            auto index          = static_cast<std::size_t>(tex);
            auto& data          = m_srvs[index];
            data.TexturePath    = path;
            data.TextureSrv     = nullptr;
            data.ResourceHandle = KFE_INVALID_INDEX;
            data.Dirty          = true;
            m_bTextureDirty     = true;
        }

        void ClearTexture(EModelTextureSlot tex) noexcept
        {
            const std::size_t index = static_cast<std::size_t>(tex);
            auto& data = m_srvs[index];

            data.TexturePath.clear();
            data.TextureSrv = nullptr;
            data.ResourceHandle = KFE_INVALID_INDEX;
            data.Dirty = true;

            m_bTextureDirty = true;
        }

        void SetBaseColor(const std::string& p) noexcept { SetTexture(EModelTextureSlot::BaseColor, p); }
        void SetNormal   (const std::string& p) noexcept { SetTexture(EModelTextureSlot::Normal, p); }
        void SetORM      (const std::string& p) noexcept { SetTexture(EModelTextureSlot::ORM, p); }
        void SetEmissive (const std::string& p) noexcept { SetTexture(EModelTextureSlot::Emissive, p); }
        void SetOpacity  (const std::string& p) noexcept { SetTexture(EModelTextureSlot::Opacity, p); }
        void SetHeight   (const std::string& p) noexcept { SetTexture(EModelTextureSlot::Height, p); }

        void SetOcclusion(const std::string& p) noexcept { SetTexture(EModelTextureSlot::Occlusion, p); }
        void SetRoughness(const std::string& p) noexcept { SetTexture(EModelTextureSlot::Roughness, p); }
        void SetMetallic (const std::string& p) noexcept { SetTexture(EModelTextureSlot::Metallic, p); }
        void SetDyeMask  (const std::string& p) noexcept { SetTexture(EModelTextureSlot::DyeMask, p); }
    };

    /// <summary>
    /// Builds a hierarchy of KFEModelNode with transforms and mesh indices
    /// </summary>
    class KFE_API KFEModel
    {
    public:
         KFEModel() = default;
        ~KFEModel() = default;

        KFEModel(const KFEModel&) = delete;
        KFEModel& operator=(const KFEModel&) = delete;
        KFEModel(KFEModel&&) noexcept = default;
        KFEModel& operator=(KFEModel&&) noexcept = default;

        NODISCARD
        bool Initialize(const std::string&      path,
                        KFEDevice*              device,
                        KFEGraphicsCommandList* cmdList,
                        KFEResourceHeap* heap) noexcept;

        void Reset  ()       noexcept;
        bool IsValid() const noexcept;

        const KFEModelNode*                 GetRootNode  () const noexcept;
        const std::vector<KFEModelSubmesh>& GetSubmeshes () const noexcept;
        const KFE_MESH_CACHE_SHARE*         GetCacheShare() const noexcept;

        std::uint32_t GetSubmeshCount () const noexcept;

        std::vector<KFEModelSubmesh>& GetSubmeshesMutable() noexcept { return m_submeshes; }

    private:
        void BuildFromShare() noexcept;
        void BuildNodeRecursive(const import::ImportedNode& src,
                                KFEModelNode& dst) noexcept;

    private:
        KFE_MESH_CACHE_SHARE* m_pShare = nullptr;

        std::vector<KFEModelSubmesh>  m_submeshes;
        std::unique_ptr<KFEModelNode> m_root;
    };
}
