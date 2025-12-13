// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  File      : model.cpp
 *  -----------------------------------------------------------------------------
 */

#include "pch.h"

#include "engine/render_manager/assets_library/model/model.h"
#include "engine/utils/logger.h"

#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/commands/graphics_list.h"
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"

#include <cstring>
#include <cmath>

namespace kfe
{
    using namespace DirectX;

    static void DecomposeMatrixToTRS(const XMFLOAT4X4& m,
        XMFLOAT3& outPos,
        XMFLOAT3& outRotDeg,
        XMFLOAT3& outScale) noexcept
    {
        const XMMATRIX M = XMLoadFloat4x4(&m);

        XMVECTOR s{}, rQuat{}, t{};
        if (!XMMatrixDecompose(&s, &rQuat, &t, M))
        {
            outPos = { 0.0f, 0.0f, 0.0f };
            outRotDeg = { 0.0f, 0.0f, 0.0f };
            outScale = { 1.0f, 1.0f, 1.0f };
            return;
        }

        XMStoreFloat3(&outScale, s);
        XMStoreFloat3(&outPos, t);

        XMFLOAT4 q{};
        XMStoreFloat4(&q, rQuat);

        const float xx = q.x * q.x;
        const float yy = q.y * q.y;
        const float zz = q.z * q.z;

        const float sinp = 2.0f * (q.w * q.x - q.y * q.z);
        float pitch = 0.0f;
        if (std::fabs(sinp) >= 1.0f)
            pitch = std::copysign(XM_PIDIV2, sinp);
        else
            pitch = std::asin(sinp);

        const float siny_cosp = 2.0f * (q.w * q.y + q.z * q.x);
        const float cosy_cosp = 1.0f - 2.0f * (yy + xx);
        const float yaw = std::atan2(siny_cosp, cosy_cosp);

        const float sinr_cosp = 2.0f * (q.w * q.z + q.x * q.y);
        const float cosr_cosp = 1.0f - 2.0f * (zz + xx);
        const float roll = std::atan2(sinr_cosp, cosr_cosp);

        outRotDeg =
        {
            XMConvertToDegrees(pitch),
            XMConvertToDegrees(yaw),
            XMConvertToDegrees(roll)
        };
    }

    _Use_decl_annotations_
        bool KFEModel::Initialize(const std::string& path,
            KFEDevice* device,
            KFEGraphicsCommandList* cmdList,
            KFEResourceHeap* heap) noexcept
    {
        Reset();

        if (path.empty())
        {
            LOG_ERROR("Empty path");
            return false;
        }

        if (!device || !cmdList)
        {
            LOG_ERROR("Device or CommandList is null for '{}'", path);
            return false;
        }

        KFE_MESH_CACHE_SHARE* share = nullptr;
        if (!KFEMeshCache::Instance().GetOrCreate(path, device, cmdList, heap, share))
        {
            LOG_ERROR("Failed to get mesh cache share for '{}'", path);
            return false;
        }

        if (!share || !share->IsValid())
        {
            LOG_ERROR("Invalid mesh cache share for '{}'", path);
            return false;
        }

        m_pShare = share;

        //~ Build submesh table and node hierarchy view
        BuildFromShare();

        if (!IsValid())
        {
            LOG_ERROR("Model built from '{}' is not valid", path);
            Reset();
            return false;
        }

        return true;
    }

    void KFEModel::Reset() noexcept
    {
        m_pShare = nullptr;
        m_submeshes.clear();
        m_root.reset();
    }

    bool KFEModel::IsValid() const noexcept
    {
        return m_pShare != nullptr && m_root != nullptr && !m_submeshes.empty();
    }

    const KFEModelNode* KFEModel::GetRootNode() const noexcept
    {
        return m_root.get();
    }

    const std::vector<KFEModelSubmesh>& KFEModel::GetSubmeshes() const noexcept
    {
        return m_submeshes;
    }

    const KFE_MESH_CACHE_SHARE* KFEModel::GetCacheShare() const noexcept
    {
        return m_pShare;
    }

    std::uint32_t KFEModel::GetSubmeshCount() const noexcept
    {
        return static_cast<std::uint32_t>(m_submeshes.size());
    }

    std::string KFEModel::GetSubmeshName(std::uint32_t submeshIndex) const noexcept
    {
        if (submeshIndex >= m_submeshes.size())
            return {};

        const auto& s = m_submeshes[submeshIndex];

        if (!s.Name.empty())
            return s.Name;

        return "Mesh_" + std::to_string(submeshIndex);
    }

    void kfe::KFEModel::BuildFromShare() noexcept
    {
        if (!m_pShare || !m_pShare->Entry || !m_pShare->Entry->SceneCPU)
            return;

        const import::ImportedScene& scene = *m_pShare->Entry->SceneCPU;

        const auto meshCount = scene.Meshes.size();
        m_submeshes.resize(meshCount);

        for (std::size_t i = 0; i < meshCount; ++i)
        {
            KFEModelSubmesh& dst = m_submeshes[i];
            dst.CacheMeshIndex = static_cast<std::uint32_t>(i);

            if (!scene.Meshes[i].Name.empty())
                dst.Name = scene.Meshes[i].Name;
            else
                dst.Name = "Mesh_" + std::to_string(i);
        }

        m_root = std::make_unique<KFEModelNode>();
        BuildNodeRecursive(scene.RootNode, *m_root);
    }

    void KFEModel::BuildNodeRecursive(const import::ImportedNode& src,
        KFEModelNode& dst) noexcept
    {
        dst.Name = src.Name;

        static_assert(sizeof(import::Float4x4) == sizeof(XMFLOAT4X4),
            "Imported Float4x4 must match size of XMFLOAT4X4 for memcpy.");

        XMFLOAT4X4 importedLocal{};
        std::memcpy(&importedLocal, &src.LocalTransform, sizeof(XMFLOAT4X4));

        XMFLOAT3 pos{}, rotDeg{}, scale{};
        DecomposeMatrixToTRS(importedLocal, pos, rotDeg, scale);

        dst.SetEnabled(true);
        dst.SetPivot({ 0.0f, 0.0f, 0.0f });

        //~ Important: set the imported cached matrix + TRS without making it dirty again
        dst.SetMatrixFromImport(importedLocal, pos, rotDeg, scale);

        //~ Mesh indices
        dst.MeshIndices.clear();
        dst.MeshIndices.reserve(src.MeshIndices.size());

        for (std::uint32_t meshIndex : src.MeshIndices)
        {
            if (meshIndex < m_submeshes.size())
            {
                dst.MeshIndices.push_back(meshIndex);

                auto& sm = m_submeshes[meshIndex];

                const bool isGeneric =
                    sm.Name.empty() ||
                    sm.Name.rfind("Mesh_", 0) == 0;

                if (isGeneric && !src.Name.empty())
                    sm.Name = src.Name;
            }
            else
            {
                LOG_WARNING("KFEModel::BuildNodeRecursive: Node '{}' references invalid mesh index {}",
                    src.Name, meshIndex);
            }
        }

        //~ Recurse into children
        dst.Children.clear();
        dst.Children.reserve(src.Children.size());

        for (const auto& childSrc : src.Children)
        {
            auto childDst = std::make_unique<KFEModelNode>();
            BuildNodeRecursive(childSrc, *childDst);
            dst.Children.emplace_back(std::move(childDst));
        }
    }
}
