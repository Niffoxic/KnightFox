#include "pch.h"

#include "engine/render_manager/assets_library/model/gpu_mesh.h"
#include "engine/utils/logger.h"

#include "engine/render_manager/api/buffer/staging_buffer.h"
#include "engine/render_manager/api/buffer/vertex_buffer.h"
#include "engine/render_manager/api/buffer/index_buffer.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/commands/graphics_list.h"

namespace kfe
{
    using namespace DirectX;

    KFEGpuMesh::KFEGpuMesh() = default;

    KFEGpuMesh::~KFEGpuMesh()
    {
        Destroy();
    }

    KFEGpuMesh::KFEGpuMesh(KFEGpuMesh&& other) noexcept
        : m_name(std::move(other.m_name))
        , m_vertexCount(other.m_vertexCount)
        , m_indexCount(other.m_indexCount)
        , m_materialIndex(other.m_materialIndex)
        , m_pVBStaging(std::move(other.m_pVBStaging))
        , m_pIBStaging(std::move(other.m_pIBStaging))
        , m_pVertexView(std::move(other.m_pVertexView))
        , m_pIndexView(std::move(other.m_pIndexView))
    {
        other.m_vertexCount   = 0u;
        other.m_indexCount    = 0u;
        other.m_materialIndex = 0u;
    }

    KFEGpuMesh& KFEGpuMesh::operator=(KFEGpuMesh&& other) noexcept
    {
        if (this != &other)
        {
            Destroy();

            m_name = std::move(other.m_name);
            m_vertexCount = other.m_vertexCount;
            m_indexCount = other.m_indexCount;
            m_materialIndex = other.m_materialIndex;

            m_pVBStaging = std::move(other.m_pVBStaging);
            m_pIBStaging = std::move(other.m_pIBStaging);
            m_pVertexView = std::move(other.m_pVertexView);
            m_pIndexView = std::move(other.m_pIndexView);

            other.m_vertexCount = 0u;
            other.m_indexCount = 0u;
            other.m_materialIndex = 0u;
        }
        return *this;
    }

    void KFEGpuMesh::Destroy() noexcept
    {
        m_pVertexView.reset();
        m_pIndexView.reset();
        m_pVBStaging.reset();
        m_pIBStaging.reset();

        m_vertexCount = 0u;
        m_indexCount = 0u;
        m_materialIndex = 0u;
        m_name.clear();
    }

    _Use_decl_annotations_
    bool KFEGpuMesh::Build(const KFE_GPU_MESH_BUILD_DESC& desc) noexcept
    {
        Destroy();

        if (!desc.Device || !desc.CommandList || !desc.Geometry)
        {
            LOG_ERROR("KFEGpuMesh::Build: Invalid build descriptor (null Device/CommandList/Geometry)");
            return false;
        }

        const KFEMeshGeometry& geom = *desc.Geometry;

        if (!geom.IsValid())
        {
            LOG_ERROR("KFEGpuMesh::Build: Geometry is not valid for mesh");
            return false;
        }

        const auto& vertices = geom.GetVertices();
        const auto& indices  = geom.GetIndices();

        m_vertexCount   = static_cast<std::uint32_t>(vertices.size());
        m_indexCount    = static_cast<std::uint32_t>(indices.size());
        m_materialIndex = geom.GetMaterialIndex();

        if (m_vertexCount == 0u || m_indexCount == 0u)
        {
            LOG_ERROR("KFEGpuMesh::Build: Geometry has zero vertices or indices");
            Destroy();
            return false;
        }

        m_name = geom.GetName();
        if (desc.DebugName && desc.DebugName[0] != '\0')
        {
            m_name = desc.DebugName;
        }

        // Calculate buffer sizes
        const std::uint32_t vbSize = static_cast<std::uint32_t>(vertices.size() * sizeof(KFEMeshVertex));
        const std::uint32_t ibSize = static_cast<std::uint32_t>(indices.size() * sizeof(std::uint32_t));

        auto* cmdListNative = desc.CommandList->GetNative();
        if (!cmdListNative)
        {
            LOG_ERROR("KFEGpuMesh::Build: CommandList native pointer is null");
            Destroy();
            return false;
        }

        // Vertex staging buffer
        m_pVBStaging = std::make_unique<KFEStagingBuffer>();

        KFE_STAGING_BUFFER_CREATE_DESC vbDesc{};
        vbDesc.Device = desc.Device;
        vbDesc.SizeInBytes = vbSize;

        if (!m_pVBStaging->Initialize(vbDesc))
        {
            LOG_ERROR("KFEGpuMesh::Build: Failed to initialize Vertex Staging Buffer for '{}'", m_name);
            Destroy();
            return false;
        }

        if (!m_pVBStaging->WriteBytes(vertices.data(), vbSize, 0u))
        {
            LOG_ERROR("KFEGpuMesh::Build: Failed to write Vertex Staging Buffer for '{}'", m_name);
            Destroy();
            return false;
        }

        if (!m_pVBStaging->RecordUploadToDefault(cmdListNative, vbSize, 0u, 0u))
        {
            LOG_ERROR("KFEGpuMesh::Build: Failed to record upload for Vertex Staging Buffer '{}'", m_name);
            Destroy();
            return false;
        }

        // Index staging buffer
        m_pIBStaging = std::make_unique<KFEStagingBuffer>();

        KFE_STAGING_BUFFER_CREATE_DESC ibDesc{};
        ibDesc.Device      = desc.Device;
        ibDesc.SizeInBytes = ibSize;

        if (!m_pIBStaging->Initialize(ibDesc))
        {
            LOG_ERROR("KFEGpuMesh::Build: Failed to initialize Index Staging Buffer for '{}'", m_name);
            Destroy();
            return false;
        }

        if (!m_pIBStaging->WriteBytes(indices.data(), ibSize, 0u))
        {
            LOG_ERROR("KFEGpuMesh::Build: Failed to write Index Staging Buffer for '{}'", m_name);
            Destroy();
            return false;
        }

        if (!m_pIBStaging->RecordUploadToDefault(cmdListNative, ibSize, 0u, 0u))
        {
            LOG_ERROR("KFEGpuMesh::Build: Failed to record upload for Index Staging Buffer '{}'", m_name);
            Destroy();
            return false;
        }

        // Get default buffers
        KFEBuffer* vbDefault = m_pVBStaging->GetDefaultBuffer();
        KFEBuffer* ibDefault = m_pIBStaging->GetDefaultBuffer();

        if (!vbDefault || !vbDefault->GetNative())
        {
            LOG_ERROR("KFEGpuMesh::Build: VB default buffer is null for '{}'", m_name);
            Destroy();
            return false;
        }

        if (!ibDefault || !ibDefault->GetNative())
        {
            LOG_ERROR("KFEGpuMesh::Build: IB default buffer is null for '{}'", m_name);
            Destroy();
            return false;
        }

        // Resource barriers
        D3D12_RESOURCE_BARRIER barriers[2]{};

        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[0].Transition.pResource = vbDefault->GetNative();
        barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[1].Transition.pResource = ibDefault->GetNative();
        barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;

        cmdListNative->ResourceBarrier(2u, barriers);

        // Create vertex buffer view
        m_pVertexView = std::make_unique<KFEVertexBuffer>();

        KFE_VERTEX_BUFFER_CREATE_DESC vbViewDesc{};
        vbViewDesc.DebugName = m_name.c_str();
        vbViewDesc.Device = desc.Device;
        vbViewDesc.OffsetInBytes = 0u;
        vbViewDesc.ResourceBuffer = vbDefault;
        vbViewDesc.StrideInBytes = sizeof(KFEMeshVertex);

        if (!m_pVertexView->Initialize(vbViewDesc))
        {
            LOG_ERROR("KFEGpuMesh::Build: Failed to build Vertex View for '{}'", m_name);
            Destroy();
            return false;
        }

        // Create index buffer view
        m_pIndexView = std::make_unique<KFEIndexBuffer>();

        KFE_INDEX_BUFFER_CREATE_DESC ibViewDesc{};
        ibViewDesc.Device = desc.Device;
        ibViewDesc.Format = DXGI_FORMAT_R32_UINT;
        ibViewDesc.OffsetInBytes = 0u;
        ibViewDesc.ResourceBuffer = ibDefault;

        if (!m_pIndexView->Initialize(ibViewDesc))
        {
            LOG_ERROR("KFEGpuMesh::Build: Failed to build Index View for '{}'", m_name);
            Destroy();
            return false;
        }

        LOG_INFO("KFEGpuMesh::Build: Built GPU mesh '{}' (verts={}, indices={})",
            m_name, m_vertexCount, m_indexCount);

        return true;
    }

    const std::string& KFEGpuMesh::GetName() const noexcept
    {
        return m_name;
    }

    std::uint32_t KFEGpuMesh::GetVertexCount() const noexcept
    {
        return m_vertexCount;
    }

    std::uint32_t KFEGpuMesh::GetIndexCount() const noexcept
    {
        return m_indexCount;
    }

    std::uint32_t KFEGpuMesh::GetMaterialIndex() const noexcept
    {
        return m_materialIndex;
    }

    const KFEVertexBuffer* KFEGpuMesh::GetVertexBufferView() const noexcept
    {
        return m_pVertexView.get();
    }

    const KFEIndexBuffer* KFEGpuMesh::GetIndexBufferView() const noexcept
    {
        return m_pIndexView.get();
    }

    bool KFEGpuMesh::IsValid() const noexcept
    {
        return (m_pVertexView != nullptr) &&
            (m_pIndexView != nullptr) &&
            (m_vertexCount > 0u) &&
            (m_indexCount > 0u);
    }
}
