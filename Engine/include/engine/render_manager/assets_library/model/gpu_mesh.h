// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  File      : gpu_mesh.h
 *  Purpose   : GPU-side mesh built from KFEMeshGeometry.
 *  -----------------------------------------------------------------------------
 */
#pragma once

#include "EngineAPI.h"
#include "engine/core.h"

#include "geometry.h"

#include <cstdint>
#include <memory>
#include <string>

namespace kfe
{
    class KFEDevice;
    class KFEGraphicsCommandList;
    class KFEStagingBuffer;
    class KFEVertexBuffer;
    class KFEIndexBuffer;
    class KFEBuffer;

    struct KFE_GPU_MESH_BUILD_DESC
    {
        KFEDevice* Device = nullptr;
        KFEGraphicsCommandList* CommandList = nullptr;
        const KFEMeshGeometry* Geometry = nullptr;
        const char* DebugName = nullptr;
    };

    class KFE_API KFEGpuMesh
    {
    public:
        KFEGpuMesh();
        ~KFEGpuMesh();

        KFEGpuMesh(const KFEGpuMesh&) = delete;
        KFEGpuMesh& operator=(const KFEGpuMesh&) = delete;
        KFEGpuMesh(KFEGpuMesh&&) noexcept;
        KFEGpuMesh& operator=(KFEGpuMesh&&) noexcept;

        NODISCARD bool Build(const KFE_GPU_MESH_BUILD_DESC& desc) noexcept;
        void Destroy() noexcept;

        // Accessors
        const std::string& GetName          () const noexcept;
        std::uint32_t      GetVertexCount   () const noexcept;
        std::uint32_t      GetIndexCount    () const noexcept;
        std::uint32_t      GetMaterialIndex () const noexcept;

        const KFEVertexBuffer* GetVertexBufferView() const noexcept;
        const KFEIndexBuffer*  GetIndexBufferView () const noexcept;

        bool IsValid() const noexcept;

    private:
        std::string   m_name{ "No Name" };
        std::uint32_t m_vertexCount   = 0u;
        std::uint32_t m_indexCount    = 0u;
        std::uint32_t m_materialIndex = 0u;

        std::unique_ptr<KFEStagingBuffer> m_pVBStaging;
        std::unique_ptr<KFEStagingBuffer> m_pIBStaging;
        std::unique_ptr<KFEVertexBuffer>  m_pVertexView;
        std::unique_ptr<KFEIndexBuffer>   m_pIndexView;
    };
}
