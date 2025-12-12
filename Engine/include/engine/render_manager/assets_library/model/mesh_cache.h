// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  File      : mesh_cache.h
 *  Purpose   : Simple cache that maps model path -> GPU mesh.
 *  -----------------------------------------------------------------------------
 */
#pragma once

#include "EngineAPI.h"
#include "geometry.h"
#include "gpu_mesh.h"
#include "assimp_importer.h"

#include "engine/system/interface/interface_singleton.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace kfe
{
    class KFEDevice;
    class KFEGraphicsCommandList;

    struct KFE_MESH_CACHE_ENTRY
    {
        std::unique_ptr<import::ImportedScene> SceneCPU;
        std::unique_ptr<KFEMeshGeometry>       MeshCPU;
        std::unique_ptr<KFEGpuMesh>            MeshGPU;

        bool IsValid() const noexcept
        {
            return MeshGPU != nullptr && MeshGPU->IsValid();
        }
    };

    class KFE_API KFEMeshCache final: public ISingleton<KFEMeshCache>
    {
    public:
         KFEMeshCache();
        ~KFEMeshCache();

        KFEMeshCache(const KFEMeshCache&)     = delete;
        KFEMeshCache(KFEMeshCache&&) noexcept = delete;

        KFEMeshCache& operator=(const KFEMeshCache&)     = delete;
        KFEMeshCache& operator=(KFEMeshCache&&) noexcept = delete;

        _Use_decl_annotations_
        bool GetOrCreate(const std::string& path,
            KFEDevice* device,
            KFEGraphicsCommandList* cmdList,
            KFEGpuMesh*& outMesh) noexcept;

        void Clear() noexcept;

    private:
        std::unordered_map<std::string, KFE_MESH_CACHE_ENTRY> m_cache;
        import::AssimpImporter                                m_importer;
    };
}
