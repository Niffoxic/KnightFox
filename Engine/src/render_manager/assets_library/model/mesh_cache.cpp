// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  File      : mesh_cache.cpp
 *  Purpose   : Mesh cache (CPU ImportedScene + CPU Geometry + GPU Meshes).
 *              No textures, no materials.
 *  -----------------------------------------------------------------------------
 */

#include "pch.h"

#include "engine/render_manager/assets_library/model/mesh_cache.h"
#include "engine/utils/logger.h"

#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/commands/graphics_list.h"
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"

namespace kfe
{
    KFEMeshCache::KFEMeshCache() = default;
    KFEMeshCache::~KFEMeshCache() = default;

    void KFEMeshCache::Clear() noexcept
    {
        m_cache.clear();
        m_shares.clear();
    }

    bool KFEMeshCache::BuildEntryCPU(const std::string& path, KFE_MESH_CACHE_ENTRY& entry) noexcept
    {
        entry.SceneCPU.reset();
        entry.MeshesCPU.clear();
        entry.MeshesGPU.clear();

        std::unique_ptr<import::ImportedScene> importedScene =
            std::make_unique<import::ImportedScene>();

        std::string errorMsg;
        if (!m_importer.LoadFromFile(path, *importedScene, errorMsg))
        {
            LOG_ERROR("Failed to import '{}': {}", path, errorMsg);
            return false;
        }

        if (importedScene->Meshes.empty())
        {
            LOG_ERROR("Imported scene '{}' has no meshes", path);
            return false;
        }

        entry.SceneCPU = std::move(importedScene);

        const auto& srcMeshes = entry.SceneCPU->Meshes;

        entry.MeshesCPU.reserve(srcMeshes.size());

        for (std::size_t i = 0; i < srcMeshes.size(); ++i)
        {
            const auto& importedMesh = srcMeshes[i];

            auto geom = std::make_unique<KFEMeshGeometry>();
            if (!geom->BuildFromImportedMesh(importedMesh))
            {
                LOG_ERROR("Failed to build geometry for mesh[{}] '{}' in '{}'",
                    i, importedMesh.Name, path);
                return false;
            }

            entry.MeshesCPU.emplace_back(std::move(geom));
        }

        LOG_INFO("Built CPU meshes for '{}' (meshes={})",
            path,
            static_cast<std::uint32_t>(entry.MeshesCPU.size()));

        return true;
    }

    bool KFEMeshCache::BuildEntryGPU(
        KFEDevice* device,
        KFEGraphicsCommandList* cmdList,
        KFEResourceHeap* resourceHeap,
        const std::string& path,
        KFE_MESH_CACHE_ENTRY& entry) noexcept
    {
        (void)resourceHeap;

        if (!device || !cmdList || !cmdList->GetNative())
        {
            LOG_ERROR("Invalid device/cmdList for '{}'", path);
            return false;
        }

        if (!entry.SceneCPU || entry.MeshesCPU.empty())
        {
            LOG_ERROR("CPU entry not ready for '{}'", path);
            return false;
        }

        entry.MeshesGPU.clear();
        entry.MeshesGPU.reserve(entry.MeshesCPU.size());

        const auto& srcMeshes = entry.SceneCPU->Meshes;

        for (std::size_t i = 0; i < entry.MeshesCPU.size(); ++i)
        {
            KFEMeshGeometry* geom = entry.MeshesCPU[i].get();
            if (!geom || !geom->IsValid())
            {
                LOG_ERROR("Invalid geometry for mesh[{}] in '{}'", i, path);
                return false;
            }

            auto gpuMesh = std::make_unique<KFEGpuMesh>();

            KFE_GPU_MESH_BUILD_DESC buildDesc{};
            buildDesc.Device = device;
            buildDesc.CommandList = cmdList;
            buildDesc.Geometry = geom;
            buildDesc.DebugName = srcMeshes[i].Name.c_str();

            if (!gpuMesh->Build(buildDesc))
            {
                LOG_ERROR("Failed GPU build for mesh[{}] '{}' in '{}'",
                    i, srcMeshes[i].Name, path);
                return false;
            }

            entry.MeshesGPU.emplace_back(std::move(gpuMesh));
        }

        LOG_INFO("Built GPU meshes for '{}' (meshes={})",
            path,
            static_cast<std::uint32_t>(entry.MeshesGPU.size()));

        return true;
    }

    _Use_decl_annotations_
        bool KFEMeshCache::GetOrCreate(
            const std::string& path,
            KFEDevice* device,
            KFEGraphicsCommandList* cmdList,
            KFEResourceHeap* resourceHeap,
            KFE_MESH_CACHE_SHARE*& outShare) noexcept
    {
        outShare = nullptr;

        if (path.empty())
        {
            LOG_ERROR("Empty path");
            return false;
        }

        if (!device || !cmdList || !resourceHeap)
        {
            LOG_ERROR("Device/CmdList/ResourceHeap null for '{}'", path);
            return false;
        }

        //~ Cache hit
        {
            auto it = m_cache.find(path);
            if (it != m_cache.end())
            {
                KFE_MESH_CACHE_ENTRY& entry = it->second;
                if (entry.IsValid())
                {
                    auto itShare = m_shares.find(path);
                    if (itShare == m_shares.end())
                    {
                        KFE_MESH_CACHE_SHARE share{};
                        share.Entry = &entry;
                        share.MeshCount = static_cast<std::uint32_t>(entry.MeshesGPU.size());

                        auto [shareIt, _] = m_shares.insert_or_assign(path, share);
                        outShare = &shareIt->second;
                    }
                    else
                    {
                        outShare = &itShare->second;
                    }

                    return outShare && outShare->IsValid();
                }

                LOG_WARNING("Existing entry for '{}' invalid, rebuilding", path);
            }
        }

        KFE_MESH_CACHE_ENTRY entry{};

        if (!BuildEntryCPU(path, entry))
        {
            LOG_ERROR("BuildEntryCPU failed for '{}'", path);
            return false;
        }

        if (!BuildEntryGPU(device, cmdList, resourceHeap, path, entry))
        {
            LOG_ERROR("BuildEntryGPU failed for '{}'", path);
            return false;
        }

        auto [itInserted, _] = m_cache.insert_or_assign(path, std::move(entry));
        KFE_MESH_CACHE_ENTRY* finalEntry = &itInserted->second;

        KFE_MESH_CACHE_SHARE share{};
        share.Entry = finalEntry;
        share.MeshCount = static_cast<std::uint32_t>(finalEntry->MeshesGPU.size());

        auto [shareIt, __] = m_shares.insert_or_assign(path, share);
        outShare = &shareIt->second;

        LOG_INFO("KFEMeshCache::GetOrCreate: Cached '{}' (meshes={})",
            path,
            share.MeshCount);

        return outShare && outShare->IsValid();
    }
}
