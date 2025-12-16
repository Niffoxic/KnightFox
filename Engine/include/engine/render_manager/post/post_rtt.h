// This is a personal academic project. Dear PVS-Studio, please check it.

#pragma once

#include "EngineAPI.h"
#include "engine/core.h"
#include <cstdint>
#include <memory>
#include <d3d12.h>
#include <dxgi1_6.h>

#include "engine/system/common_types.h"

namespace kfe
{
    class KFEDevice;
    class KFETexture;
    class KFETextureRTV;
    class KFETextureSRV;
    class KFERTVHeap;
    class KFEResourceHeap;

    enum class KFE_RT_DRAW_STATE : std::uint8_t
    {
        Unknown = 0,
        RenderTarget,   //~ We want to render into it (RTV)
        ShaderResource  //~ We want to sample it (SRV)
    };

    struct KFE_RT_TEXTURE_CREATE_DESC
    {
        KFEDevice*       Device{ nullptr };
        KFERTVHeap*      RTVHeap{ nullptr };
        KFEResourceHeap* SRVHeap{ nullptr };

        std::uint32_t Width { 0u };
        std::uint32_t Height{ 0u };

        DXGI_FORMAT Format{ DXGI_FORMAT_R8G8B8A8_UNORM };
        std::uint16_t MipLevels{ 1u };

        DXGI_SAMPLE_DESC SampleDesc{ 1u, 0u };

        D3D12_RESOURCE_FLAGS ResourceFlags{ D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET };

        D3D12_RESOURCE_STATES InitialState{ D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE };

        const D3D12_CLEAR_VALUE* ClearValue{ nullptr };

        std::uint32_t RTVDescriptorIndex{ KFE_INVALID_INDEX };
        std::uint32_t SRVDescriptorIndex{ KFE_INVALID_INDEX };

        D3D12_RTV_DIMENSION RTVViewDimension{ D3D12_RTV_DIMENSION_TEXTURE2D };
        D3D12_SRV_DIMENSION SRVViewDimension{ D3D12_SRV_DIMENSION_TEXTURE2D };
    };

    class KFE_API KFERenderTargetTexture final
    {
    public:
        KFERenderTargetTexture() noexcept;
        ~KFERenderTargetTexture() noexcept;

        KFERenderTargetTexture(KFERenderTargetTexture&&) noexcept;
        KFERenderTargetTexture& operator=(KFERenderTargetTexture&&) noexcept;

        KFERenderTargetTexture(const KFERenderTargetTexture&) = delete;
        KFERenderTargetTexture& operator=(const KFERenderTargetTexture&) = delete;

        NODISCARD bool Initialize(_In_ const KFE_RT_TEXTURE_CREATE_DESC& desc);
        NODISCARD bool Destroy() noexcept;

        NODISCARD bool IsInitialized() const noexcept;

        //~ State helpers
        NODISCARD KFE_RT_DRAW_STATE GetDrawState() const noexcept;
        NODISCARD D3D12_RESOURCE_STATES GetResourceState() const noexcept;

        void SetDrawState(KFE_RT_DRAW_STATE state) noexcept;

        NODISCARD bool Transition(_In_ ID3D12GraphicsCommandList* cmd,
            _In_ KFE_RT_DRAW_STATE target) noexcept;

        NODISCARD bool SetAsRenderTarget(_In_ ID3D12GraphicsCommandList* cmd,
            _In_opt_ const D3D12_CPU_DESCRIPTOR_HANDLE* dsv) noexcept;

        NODISCARD bool BindSRV(_In_ ID3D12GraphicsCommandList* cmd,
            _In_ std::uint32_t rootParamIndex) noexcept;

        //~ Accessors
        NODISCARD KFETexture*    GetTexture () const noexcept;
        NODISCARD KFETextureRTV* GetRTV     () const noexcept;
        NODISCARD KFETextureSRV* GetSRV     () const noexcept;

        NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPU() const noexcept;

        NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPU() const noexcept;

        NODISCARD std::uint32_t GetRTVIndex() const noexcept;
        NODISCARD std::uint32_t GetSRVIndex() const noexcept;

        std::string GetName       () const noexcept;
        std::string GetDescription() const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
