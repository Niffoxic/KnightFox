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

#include <d3d12.h>

namespace kfe
{
    class KFEDevice;
    class KFEDSVHeap;
    class KFETexture;

    typedef struct _KFE_DSV_CREATE_DESC
    {
        _In_ KFEDevice*  Device;
        _In_ KFEDSVHeap* Heap;
        _In_ KFETexture* Texture;

        DXGI_FORMAT         Format;
        D3D12_DSV_DIMENSION ViewDimension;

        std::uint32_t MipSlice;
        std::uint32_t FirstArraySlice;
        std::uint32_t ArraySize;

        D3D12_DSV_FLAGS Flags;

        std::uint32_t DescriptorIndex;

    } KFE_DSV_CREATE_DESC;

    class KFE_API KFETextureDSV final : public IKFEObject
    {
    public:
         KFETextureDSV() noexcept;
        ~KFETextureDSV() noexcept override;

        KFETextureDSV(const KFETextureDSV&) = delete;
        KFETextureDSV& operator=(const KFETextureDSV&) = delete;

        KFETextureDSV           (KFETextureDSV&&) noexcept;
        KFETextureDSV& operator=(KFETextureDSV&&) noexcept;

        NODISCARD bool Initialize(_In_ const KFE_DSV_CREATE_DESC& desc);
        
        NODISCARD bool Destroy     ()       noexcept;
        NODISCARD bool IsInitialize() const noexcept;

        NODISCARD KFEDSVHeap* GetAttachedHeap() const noexcept;
        NODISCARD KFETexture* GetTexture     () const noexcept;

        NODISCARD std::uint32_t GetDescriptorIndex() const noexcept;
        NODISCARD bool          HasValidDescriptor() const noexcept;

        NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept;

        NODISCARD DXGI_FORMAT           GetFormat       () const noexcept;
        NODISCARD D3D12_DSV_DIMENSION   GetViewDimension() const noexcept;
        NODISCARD D3D12_DSV_FLAGS       GetFlags        () const noexcept;

        NODISCARD std::uint32_t GetMipSlice       () const noexcept;
        NODISCARD std::uint32_t GetFirstArraySlice() const noexcept;
        NODISCARD std::uint32_t GetArraySize      () const noexcept;

        NODISCARD std::string GetName       () const noexcept override;
        NODISCARD std::string GetDescription() const noexcept override;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace kfe
