#include "pch.h"
#include "engine/render_manager/assets_library/model/mesh_cache.h"

#include "engine/utils/logger.h"

#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/commands/graphics_list.h"

kfe::KFEMeshCache::KFEMeshCache () = default;
kfe::KFEMeshCache::~KFEMeshCache() = default;

_Use_decl_annotations_
bool kfe::KFEMeshCache::GetOrCreate(
    const std::string& path,
    KFEDevice* device,
    KFEGraphicsCommandList* cmdList,
    KFEGpuMesh*& outMesh) noexcept
{
    outMesh = nullptr;

    if (path.empty())
    {
        LOG_ERROR("KFEMeshCache::GetOrCreate: Empty path");
        return false;
    }

    if (!device || !cmdList)
    {
        LOG_ERROR("KFEMeshCache::GetOrCreate: Device or CommandList is null for path '{}'", path);
        return false;
    }

    auto it = m_cache.find(path);
    if (it != m_cache.end())
    {
        KFE_MESH_CACHE_ENTRY& entry = it->second;
        if (entry.IsValid())
        {
            outMesh = entry.MeshGPU.get();
            return true;
        }
        else
        {
            LOG_WARNING("KFEMeshCache::GetOrCreate: Existing cache entry for '{}' is invalid, rebuilding", path);
        }
    }

    // Import scene
    std::unique_ptr<import::ImportedScene> importedScene =
        std::make_unique<import::ImportedScene>();

    std::string errorMsg;
    if (!m_importer.LoadFromFile(path, *importedScene, errorMsg))
    {
        LOG_ERROR("KFEMeshCache::GetOrCreate: Failed to import '{}': {}", path, errorMsg);
        return false;
    }

    if (importedScene->Meshes.empty())
    {
        LOG_ERROR("KFEMeshCache::GetOrCreate: Imported scene '{}' has no meshes", path);
        return false;
    }

    // For now only handle mesh 0
    import::ImportedMesh& importedMesh = importedScene->Meshes[0];

    std::unique_ptr<KFEMeshGeometry> geom = std::make_unique<KFEMeshGeometry>();
    if (!geom->BuildFromImportedMesh(importedMesh))
    {
        LOG_ERROR("KFEMeshCache::GetOrCreate: Failed to build KFEMeshGeometry from mesh[0] of '{}'", path);
        return false;
    }

    // Build GPU mesh from KFEMeshGeometry
    std::unique_ptr<KFEGpuMesh> gpuMesh = std::make_unique<KFEGpuMesh>();

    KFE_GPU_MESH_BUILD_DESC buildDesc{};
    buildDesc.Device = device;
    buildDesc.CommandList = cmdList;
    buildDesc.Geometry = geom.get();
    buildDesc.DebugName = path.c_str();

    if (!gpuMesh->Build(buildDesc))
    {
        LOG_ERROR("KFEMeshCache::GetOrCreate: Failed to build KFEGpuMesh for '{}'", path);
        return false;
    }

    // Insert/replace cache entry
    KFE_MESH_CACHE_ENTRY entry{};
    entry.SceneCPU = std::move(importedScene);
    entry.MeshCPU = std::move(geom);
    entry.MeshGPU = std::move(gpuMesh);

    auto [iterInserted, _] = m_cache.insert_or_assign(path, std::move(entry));

    outMesh = iterInserted->second.MeshGPU.get();

    LOG_INFO("KFEMeshCache::GetOrCreate: Cached GPU mesh for '{}' (verts={}, indices={})",
        path,
        outMesh->GetVertexCount(),
        outMesh->GetIndexCount());

    return true;
}

void kfe::KFEMeshCache::Clear() noexcept
{
	m_cache.clear();
}
