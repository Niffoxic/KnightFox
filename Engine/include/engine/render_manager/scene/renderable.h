// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */

#pragma once
#include "EngineAPI.h"
#include "engine/core.h"
#include <DirectXMath.h>
#include <d3d12.h>

#include "engine/render_manager/api/commands/graphics_list.h"
#include "engine/render_manager/api/buffer/index_buffer.h"
#include "engine/render_manager/api/buffer/vertex_buffer.h"

namespace kfe 
{
    enum class ERenderLayer : std::uint8_t
    {
        Opaque = 0,
        Transparent,
        Skybox,
        UI,
        Debug,
        Count
    };

    struct KFEBounds
    {
        DirectX::XMFLOAT3 Center{ 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 Extents{ 0.5f, 0.5f, 0.5f };
        float Radius{ 1.0f };
    };

    struct KFETransformNode
    {
        DirectX::XMFLOAT4X4 LocalMatrix{};
        DirectX::XMFLOAT4X4 WorldMatrix{};
        bool bDirty{ true };

        KFETransformNode()
        {
            using namespace DirectX;
            XMStoreFloat4x4(&LocalMatrix, XMMatrixIdentity());
            XMStoreFloat4x4(&WorldMatrix, XMMatrixIdentity());
        }

        void MarkDirty() noexcept { bDirty = true; }

        void UpdateWorld(_In_opt_ const DirectX::XMFLOAT4X4* parentWorld = nullptr) noexcept
        {
            using namespace DirectX;
            if (parentWorld)
            {
                const XMMATRIX local = XMLoadFloat4x4(&LocalMatrix);
                const XMMATRIX parent = XMLoadFloat4x4(parentWorld);
                const XMMATRIX world = XMMatrixMultiply(local, parent);
                XMStoreFloat4x4(&WorldMatrix, world);
            }
            else
            {
                WorldMatrix = LocalMatrix;
            }

            bDirty = false;
        }
    };

    struct KFEGeometryData
    {
        KFEVertexBuffer VertexView{};
        KFEIndexBuffer  IndexView {};

        std::uint32_t            IndexCount{ 0u };
        D3D_PRIMITIVE_TOPOLOGY   Topology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST };

        NODISCARD bool IsValid() const noexcept
        {
            return  VertexView.IsInitialized() &&
                    IndexView.IsInitialized();
        }
    };

	class KFE_API IKFERenderable : public IKFEObject
	{
	public:
        std::string GetName() const noexcept override
        {
            return "IKFERenderable";
        }

        std::string GetDescription() const noexcept override
        {
            return "Test Renderable";
        }

        void SetDebugName(_In_ const std::string& name) { m_debugName = name; }
        NODISCARD const std::string& GetDebugName() const noexcept { return m_debugName; }

        void SetVisible(bool visible) noexcept { m_visible = visible; }
        NODISCARD bool IsVisible() const noexcept { return m_visible; }

        void SetRenderLayer(ERenderLayer layer) noexcept { m_layer = layer; }
        NODISCARD ERenderLayer GetRenderLayer() const noexcept { return m_layer; }

        // Attach geometry / transform
        void SetGeometry(_In_opt_ KFEGeometryData* geometry) noexcept { m_geometry = geometry; }
        NODISCARD const KFEGeometryData* GetGeometry() const noexcept { return m_geometry; }

        void SetTransform(_In_opt_ KFETransformNode* transform) noexcept { m_transform = transform; }
        NODISCARD const KFETransformNode* GetTransform() const noexcept { return m_transform; }

        // Bounds (for future culling)
        void SetBounds(_In_ const KFEBounds& bounds) noexcept { m_bounds = bounds; }
        NODISCARD const KFEBounds& GetBounds() const noexcept { return m_bounds; }

        void RecordDraw(_In_ KFEGraphicsCommandList* gfxList) const
        {
            if (!gfxList)
            {
                return;
            }

            if (!m_visible)
            {
                return;
            }

            if (!m_geometry || !m_geometry->IsValid())
            {
                return;
            }

            ID3D12GraphicsCommandList* cmd = gfxList->GetNative();
            if (!cmd)
            {
                return;
            }

            auto vertex = m_geometry->VertexView.GetView();
            auto index =  m_geometry->IndexView.GetView();
            cmd->IASetPrimitiveTopology(m_geometry->Topology);
            cmd->IASetVertexBuffers(0u, 1u, &vertex);
            cmd->IASetIndexBuffer(&index);
            cmd->DrawIndexedInstanced(m_geometry->IndexCount, 1u, 0u, 0, 0u);
        }

    private:
        std::string       m_debugName{};
        bool              m_visible{ true };
        ERenderLayer      m_layer{ ERenderLayer::Opaque };

        KFEGeometryData*  m_geometry{ nullptr };
        KFETransformNode* m_transform{ nullptr };
        KFEBounds         m_bounds{};
    };
} // namespace kfe
