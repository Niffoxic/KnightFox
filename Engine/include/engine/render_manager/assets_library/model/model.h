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

#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/buffer/constant_buffer.h"

namespace kfe
{
    class KFEDevice;
    class KFEGraphicsCommandList;
    class KFEVertexBuffer;
    class KFEResourceHeap;
    class KFEIndexBuffer;

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
    struct KFEModelSubmesh
    {
        std::uint32_t CacheMeshIndex = 0u;
        std::unique_ptr<KFEBuffer>         ConstantBuffer{ nullptr };
        std::unique_ptr<KFEConstantBuffer> CBView{ nullptr };

        std::unique_ptr<KFEBuffer>         MetaCB{ nullptr };
        std::unique_ptr<KFEConstantBuffer> MetaCBView{ nullptr };
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
