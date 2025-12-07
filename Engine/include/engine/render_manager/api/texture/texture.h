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

#include <cstdint>
#include <memory>
#include <string>

struct ID3D12Resource;

enum   D3D12_RESOURCE_DIMENSION;
enum   D3D12_RESOURCE_FLAGS;
enum   D3D12_HEAP_TYPE;
enum   D3D12_RESOURCE_STATES;
enum   DXGI_FORMAT;

struct DXGI_SAMPLE_DESC;
struct D3D12_CLEAR_VALUE;

namespace kfe
{
    class KFEDevice;

    typedef struct _KFE_TEXTURE_CREATE_DESC
    {
        KFEDevice* Device = nullptr;

        D3D12_RESOURCE_DIMENSION Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(0);

        std::uint32_t Width            = 0u;
        std::uint32_t Height           = 1u;
        std::uint16_t DepthOrArraySize = 1u;
        std::uint16_t MipLevels        = 1u;

        DXGI_FORMAT       Format     = static_cast<DXGI_FORMAT>(0);
        DXGI_SAMPLE_DESC SampleDesc  = static_cast<DXGI_SAMPLE_DESC>(0);

        D3D12_RESOURCE_FLAGS ResourceFlags = static_cast<D3D12_RESOURCE_FLAGS>(0);
        D3D12_HEAP_TYPE      HeapType      = static_cast<D3D12_HEAP_TYPE>(0);

        D3D12_RESOURCE_STATES    InitialState = static_cast<D3D12_RESOURCE_STATES>(0);
        const D3D12_CLEAR_VALUE* ClearValue   = nullptr;

    } KFE_TEXTURE_CREATE_DESC;

    /// <summary>
    /// Texture Resource: DirectX 12 Texture resource wrapper
    /// </summary>
    class KFE_API KFETexture final : public IKFEObject
    {
    public:
         KFETexture();
        ~KFETexture();

        KFETexture           (const KFETexture&) = delete;
        KFETexture& operator=(const KFETexture&) = delete;

        KFETexture           (KFETexture&& other) noexcept;
        KFETexture& operator=(KFETexture&& other) noexcept;

        NODISCARD bool Initialize(_In_ const KFE_TEXTURE_CREATE_DESC& desc);
        
        NODISCARD bool Destroy      ()       noexcept;
        NODISCARD bool IsInitialized() const noexcept;
        
        NODISCARD ID3D12Resource* GetNative() const noexcept;

        ///~ Query texture layout and properties
        NODISCARD D3D12_RESOURCE_DIMENSION GetDimension         () const noexcept;
        NODISCARD std::uint32_t            GetWidth             () const noexcept;
        NODISCARD std::uint32_t            GetHeight            () const noexcept;
        NODISCARD std::uint16_t            GetDepthOrArraySize  () const noexcept;
        NODISCARD std::uint16_t            GetMipLevels         () const noexcept;
        NODISCARD DXGI_FORMAT              GetFormat            () const noexcept;
        NODISCARD D3D12_HEAP_TYPE          GetHeapType          () const noexcept;
        NODISCARD D3D12_RESOURCE_STATES    GetInitialState      () const noexcept;
        NODISCARD D3D12_RESOURCE_FLAGS     GetResourceFlags     () const noexcept;

        //~ helpers
        NODISCARD bool IsRenderTarget() const noexcept;
        NODISCARD bool IsDepthStencil() const noexcept;
        NODISCARD bool IsTexture1D   () const noexcept;
        NODISCARD bool IsTexture2D   () const noexcept;
        NODISCARD bool IsTexture3D   () const noexcept;

        //~ Inherited via IKFEObject
        NODISCARD std::string GetName       () const noexcept override;
        NODISCARD std::string GetDescription() const noexcept override;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace kfe
