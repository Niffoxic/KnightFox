// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  File      : mesh_geometry.h
 *  Purpose   : Engine-owned CPU mesh representation built from ImportedMesh.
 *  -----------------------------------------------------------------------------
 */
#pragma once

#include "EngineAPI.h"
#include "assimp_importer.h"

#include <cstdint>
#include <string>
#include <vector>

#include <DirectXMath.h>

namespace kfe
{
    // Final CPU-side vertex layout for the renderer.
    // This is what you will eventually upload to your D3D12 vertex buffers.
    struct KFEMeshVertex
    {
        DirectX::XMFLOAT3 Position{};
        DirectX::XMFLOAT3 Normal{};
        DirectX::XMFLOAT3 Tangent{};
        DirectX::XMFLOAT3 Bitangent{};
        DirectX::XMFLOAT2 UV0{};
        DirectX::XMFLOAT2 UV1{};

        bool HasNormal = false;
        bool HasTangent = false;
        bool HasUV0 = false;
        bool HasUV1 = false;
    };

    class KFE_API KFEMeshGeometry
    {
    public:
        KFEMeshGeometry();
        ~KFEMeshGeometry();

        KFEMeshGeometry(const KFEMeshGeometry&) = delete;
        KFEMeshGeometry& operator=(const KFEMeshGeometry&) = delete;
        KFEMeshGeometry(KFEMeshGeometry&&) noexcept;
        KFEMeshGeometry& operator=(KFEMeshGeometry&&) noexcept;

        bool BuildFromImportedMesh(const kfe::import::ImportedMesh& src) noexcept;

        void Clear() noexcept;

        // Accessors
        const std::string& GetName()          const noexcept;
        std::uint32_t      GetMaterialIndex() const noexcept;

        const std::vector<KFEMeshVertex>& GetVertices() const noexcept;
        const std::vector<std::uint32_t>& GetIndices()  const noexcept;

        const DirectX::XMFLOAT3& GetAABBMin() const noexcept;
        const DirectX::XMFLOAT3& GetAABBMax() const noexcept;

        bool IsValid() const noexcept;

    private:
        std::string                m_name;
        std::uint32_t              m_materialIndex = 0u;
        std::vector<KFEMeshVertex> m_vertices;
        std::vector<std::uint32_t> m_indices;

        DirectX::XMFLOAT3          m_aabbMin;
        DirectX::XMFLOAT3          m_aabbMax;
    };
}
