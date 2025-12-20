// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  File      : mesh_geometry.cpp
 *  -----------------------------------------------------------------------------
 */

#include "pch.h"

#include "engine/render_manager/assets_library/model/geometry.h"
#include "engine/utils/logger.h"

namespace kfe
{
    using namespace DirectX;

    KFEMeshGeometry::KFEMeshGeometry()
        : m_aabbMin(1e30f, 1e30f, 1e30f)
        , m_aabbMax(-1e30f, -1e30f, -1e30f)
    {
    }

    KFEMeshGeometry::~KFEMeshGeometry()
    {
        Clear();
    }

    KFEMeshGeometry::KFEMeshGeometry(KFEMeshGeometry&& other) noexcept
        : m_name(std::move(other.m_name))
        , m_vertices(std::move(other.m_vertices))
        , m_indices(std::move(other.m_indices))
        , m_aabbMin(other.m_aabbMin)
        , m_aabbMax(other.m_aabbMax)
    {
        other.m_aabbMin = XMFLOAT3(1e30f, 1e30f, 1e30f);
        other.m_aabbMax = XMFLOAT3(-1e30f, -1e30f, -1e30f);
    }

    KFEMeshGeometry& KFEMeshGeometry::operator=(KFEMeshGeometry&& other) noexcept
    {
        if (this != &other)
        {
            m_name = std::move(other.m_name);
            m_vertices = std::move(other.m_vertices);
            m_indices = std::move(other.m_indices);
            m_aabbMin = other.m_aabbMin;
            m_aabbMax = other.m_aabbMax;

            other.m_aabbMin = XMFLOAT3(1e30f, 1e30f, 1e30f);
            other.m_aabbMax = XMFLOAT3(-1e30f, -1e30f, -1e30f);
        }
        return *this;
    }

    void KFEMeshGeometry::Clear() noexcept
    {
        m_name.clear();
        m_vertices.clear();
        m_indices.clear();

        m_aabbMin = XMFLOAT3(1e30f, 1e30f, 1e30f);
        m_aabbMax = XMFLOAT3(-1e30f, -1e30f, -1e30f);
    }

    bool KFEMeshGeometry::BuildFromImportedMesh(const kfe::import::ImportedMesh& src) noexcept
    {
        Clear();

        if (src.Vertices.empty() || src.Indices.empty())
        {
            LOG_WARNING("KFEMeshGeometry::BuildFromImportedMesh: Source mesh '{}' is empty",
                src.Name);
            return false;
        }

        m_name = src.Name;

        m_aabbMin = XMFLOAT3(src.AABBMin.x, src.AABBMin.y, src.AABBMin.z);
        m_aabbMax = XMFLOAT3(src.AABBMax.x, src.AABBMax.y, src.AABBMax.z);

        m_vertices.resize(src.Vertices.size());
        m_indices = src.Indices;

        for (std::size_t i = 0; i < src.Vertices.size(); ++i)
        {
            const kfe::import::ImportedVertex& inV = src.Vertices[i];
            KFEMeshVertex& outV = m_vertices[i];

            //~ Position
            outV.Position = XMFLOAT3(inV.Position.x, inV.Position.y, inV.Position.z);

            //~ Normal
            outV.HasNormal = inV.HasNormal;
            outV.Normal = inV.HasNormal
                ? XMFLOAT3(inV.Normal.x, inV.Normal.y, inV.Normal.z)
                : XMFLOAT3(0.0f, 1.0f, 0.0f);

            //~ Tangent / Bitangent
            outV.HasTangent = inV.HasTangent;
            if (inV.HasTangent)
            {
                outV.Tangent = XMFLOAT3(inV.Tangent.x, inV.Tangent.y, inV.Tangent.z);
                outV.Bitangent = XMFLOAT3(inV.Bitangent.x, inV.Bitangent.y, inV.Bitangent.z);
            }
            else
            {
                outV.Tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);
                outV.Bitangent = XMFLOAT3(0.0f, 0.0f, 1.0f);
            }

            //~ UVs (geometry only, no texture meaning)
            outV.HasUV0 = inV.HasUV[0];
            outV.UV0 = inV.HasUV[0]
                ? XMFLOAT2(inV.UV[0].x, inV.UV[0].y)
                : XMFLOAT2(0.0f, 0.0f);

            outV.HasUV1 = inV.HasUV[1];
            outV.UV1 = inV.HasUV[1]
                ? XMFLOAT2(inV.UV[1].x, inV.UV[1].y)
                : XMFLOAT2(0.0f, 0.0f);
        }

        LOG_INFO("KFEMeshGeometry::BuildFromImportedMesh: Built mesh '{}' (verts={}, indices={})",
            m_name, m_vertices.size(), m_indices.size());

        return true;
    }

    const std::string& KFEMeshGeometry::GetName() const noexcept
    {
        return m_name;
    }

    const std::vector<KFEMeshVertex>& KFEMeshGeometry::GetVertices() const noexcept
    {
        return m_vertices;
    }

    const std::vector<std::uint32_t>& KFEMeshGeometry::GetIndices() const noexcept
    {
        return m_indices;
    }

    const XMFLOAT3& KFEMeshGeometry::GetAABBMin() const noexcept
    {
        return m_aabbMin;
    }

    const XMFLOAT3& KFEMeshGeometry::GetAABBMax() const noexcept
    {
        return m_aabbMax;
    }

    bool KFEMeshGeometry::IsValid() const noexcept
    {
        return !m_vertices.empty() && !m_indices.empty();
    }

    std::vector<D3D12_INPUT_ELEMENT_DESC> KFEMeshGeometry::GetInputLayout() noexcept
    {
        const UINT offsetPosition = 0;
        const UINT offsetNormal = offsetPosition + sizeof(XMFLOAT3);
        const UINT offsetTangent = offsetNormal + sizeof(XMFLOAT3);
        const UINT offsetBitangent = offsetTangent + sizeof(XMFLOAT3);
        const UINT offsetUV0 = offsetBitangent + sizeof(XMFLOAT3);
        const UINT offsetUV1 = offsetUV0 + sizeof(XMFLOAT2);

        return
        {
            { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetPosition,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetNormal,    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetTangent,   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetBitangent, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetUV0,       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",  1, DXGI_FORMAT_R32G32_FLOAT,    0, offsetUV1,       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };
    }
}
