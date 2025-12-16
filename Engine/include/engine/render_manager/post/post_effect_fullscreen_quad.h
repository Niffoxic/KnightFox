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
#include "engine/render_manager/components/camera.h"
#include "engine/windows_manager/windows_manager.h"


namespace kfe
{
    struct alignas(16) FullQuadPostEffect_CB
    {
        // Basic grade
        float Exposure{ 1.0f };
        float Gamma{ 2.2f };
        float Contrast{ 1.0f };
        float Saturation{ 1.0f };

        float Invert{ 0.0f };
        float Grayscale{ 0.0f };
        float Time{ 0.0f };
        float Fade{ 0.0f };

        // Resolution
        float Resolution[2]{ 0.0f, 0.0f };
        float InvResolution[2]{ 0.0f, 0.0f };

        // Vignette / blur / sharpen
        float Vignette{ 0.0f };
        float VignettePower{ 2.0f };
        float BlurStrength{ 0.0f };
        float SharpenStrength{ 0.0f };

        float GrainStrength{ 0.0f };
        float ChromAbStrength{ 0.0f };
        float ScanlineStrength{ 0.0f };
        float DitherStrength{ 0.0f };

        // Mouse
        float MousePosPixels[2]{ 0.0f, 0.0f };
        float MousePosUV[2]{ 0.5f, 0.5f };

        float MouseButtons[3]{ 0.0f, 0.0f, 0.0f };
        float MouseWheel{ 0.0f };

        // Color grading
        float Temperature{ 0.0f };
        float Tint{ 0.0f };
        float HueShift{ 0.0f };
        float TonemapType{ 2.0f };

        float WhitePoint{ 1.0f };
        float BloomStrength{ 0.0f };
        float BloomThreshold{ 1.0f };
        float BloomKnee{ 0.5f };

        // Lens and presentation
        float LensDistortion{ 0.0f };
        float Letterbox{ 0.0f };
        float LetterboxSoftness{ 0.2f };
        float RadialBlurStrength{ 0.0f };

        float RadialBlurRadius{ 0.25f };
        float _Pad0[3]{ 0.0f, 0.0f, 0.0f };
    };
    static_assert(sizeof(FullQuadPostEffect_CB) % 16 == 0);

    class KFE_API KFEPostEffect_FullscreenQuad final : public IKFEPostEffect
    {
    public:
        KFEPostEffect_FullscreenQuad() = default;
        ~KFEPostEffect_FullscreenQuad() override { Destroy(); }

        std::string GetName() const noexcept override { return "KFEPostEffect_FullscreenQuad"; }
        std::string GetDescription() const noexcept override { return "Simple post: draw fullscreen quad sampling SceneColor."; }

        // IKFEPostEffect
        void Update(const KFEWindows* window);
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
