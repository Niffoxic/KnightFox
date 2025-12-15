// This is a personal academic project. Dear PVS-Studio, please check it.

#pragma once
#include "EngineAPI.h"
#include "engine/system/interface/interface_post_effect.h"

#include <wrl/client.h>

#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"
#include "engine/render_manager/api/pso.h"
#include "engine/render_manager/api/root_signature.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/buffer/vertex_buffer.h"
#include "engine/render_manager/api/buffer/index_buffer.h"
#include "engine/render_manager/api/frame_cb.h"


namespace kfe
{
    struct alignas(16) FullQuadPostEffect_CB
    {
        float Exposure{ 1.0f };
        float Invert{ 0.0f };
        float Time{ 0.0f };
        float _Pad{ 0.0f };
    };

    class KFE_API KFEPostEffect_FullscreenQuad final : public IKFEPostEffect
    {
    public:
        KFEPostEffect_FullscreenQuad() = default;
        ~KFEPostEffect_FullscreenQuad() override { Destroy(); }

        std::string GetName() const noexcept override { return "KFEPostEffect_FullscreenQuad"; }
        std::string GetDescription() const noexcept override { return "Simple post: draw fullscreen quad sampling SceneColor."; }

        // IKFEPostEffect
        bool Initialize(_In_ const KFE_POST_EFFECT_INIT_DESC& desc) override;
        void Destroy() noexcept override;

        void Render(_In_ const KFE_POST_EFFECT_RENDER_DESC& desc) override;
        void ImguiView(float deltaTime) override;

        std::string GetPEClassName() const noexcept override { return "KFEPostEffect_FullscreenQuad"; }

        JsonLoader GetJsonData() const override;
        void LoadFromJson(const JsonLoader& loader) override;

        bool IsInitialized() const noexcept override { return m_initialized; }

    private:
        bool CreatePSO(DXGI_FORMAT outputFormat);

    private:
        KFEDevice* m_device{ nullptr };
        KFEResourceHeap* m_resourceHeap{ nullptr };
        bool       m_initialized{ false };

        std::uint32_t m_reservedIndex{ 0u };
        FullQuadPostEffect_CB m_cbData{};

        KFERootSignature m_root{};
        KFEPipelineState m_pso{};

        KFEBuffer       m_vertexBuffer{};
        KFEVertexBuffer m_viewVertex{};

        KFEBuffer      m_indexBuffer{};
        KFEIndexBuffer m_viewIndex{};
        KFEFrameConstantBuffer m_constantBuffer{};

        std::string m_vsPath = "shaders/post/fullscreen_post_vs.hlsl";
        std::string m_psPath = "shaders/post/fullscreen_post_ps.hlsl";
    };
}
