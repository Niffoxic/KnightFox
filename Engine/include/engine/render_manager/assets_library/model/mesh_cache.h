// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  File      : mesh_cache.h
 *  -----------------------------------------------------------------------------
 */
#pragma once

#include "EngineAPI.h"
#include "geometry.h"
#include "gpu_mesh.h"
#include "assimp_importer.h"
#include "engine/system/common_types.h"
#include "engine/system/interface/interface_singleton.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace kfe
{
    class KFEDevice;
    class KFEGraphicsCommandList;
    class KFEResourceHeap;

    struct KFE_MESH_CACHE_ENTRY;

    struct KFE_MESH_CACHE_SHARE
    {
        KFE_MESH_CACHE_ENTRY* Entry = nullptr;
        std::uint32_t         MeshCount = 0u;

        bool IsValid() const noexcept
        {
            return Entry != nullptr && MeshCount > 0u;
        }
    };

    struct KFE_MESH_CACHE_ENTRY
    {
        std::unique_ptr<import::ImportedScene>        SceneCPU;
        std::vector<std::unique_ptr<KFEMeshGeometry>> MeshesCPU;
        std::vector<std::unique_ptr<KFEGpuMesh>>      MeshesGPU;

        bool IsValid() const noexcept
        {
            if (MeshesGPU.empty())
                return false;

            for (const auto& m : MeshesGPU)
            {
                if (!m || !m->IsValid())
                    return false;
            }

            return true;
        }
    };

    class KFE_API KFEMeshCache final : public ISingleton<KFEMeshCache>
    {
    public:
        KFEMeshCache();
        ~KFEMeshCache();

        KFEMeshCache(const KFEMeshCache&) = delete;
        KFEMeshCache(KFEMeshCache&&) noexcept = delete;

        KFEMeshCache& operator=(const KFEMeshCache&) = delete;
        KFEMeshCache& operator=(KFEMeshCache&&) noexcept = delete;

        _Use_decl_annotations_
        bool GetOrCreate(const std::string& path,
            KFEDevice* device,
            KFEGraphicsCommandList* cmdList,
            KFEResourceHeap* resourceHeap,
            KFE_MESH_CACHE_SHARE*& outShare) noexcept;

        void Clear() noexcept;

    private:
        //~ Build helpers
        bool BuildEntryCPU(const std::string& path,
            KFE_MESH_CACHE_ENTRY& entry) noexcept;

        bool BuildEntryGPU(KFEDevice* device,
            KFEGraphicsCommandList* cmdList,
            KFEResourceHeap* resourceHeap,
            const std::string& path,
            KFE_MESH_CACHE_ENTRY& entry) noexcept;

    private:
        std::unordered_map<std::string, KFE_MESH_CACHE_ENTRY> m_cache;
        std::unordered_map<std::string, KFE_MESH_CACHE_SHARE> m_shares;
        import::AssimpImporter                                m_importer;
    };
}
