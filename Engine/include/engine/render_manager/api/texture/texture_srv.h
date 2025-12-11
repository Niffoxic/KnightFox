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

enum   DXGI_FORMAT;
enum   D3D12_SRV_DIMENSION;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

namespace kfe
{
    class KFEDevice;
    class KFETexture;
    class KFEResourceHeap;

    typedef struct _KFE_SRV_CREATE_DESC
    {
        _In_ KFEDevice*   Device;
        _In_ KFEResourceHeap* Heap;
        _In_ KFETexture*  Texture;

        DXGI_FORMAT         Format;
        D3D12_SRV_DIMENSION ViewDimension;

        std::uint32_t MostDetailedMip;
        std::uint32_t MipLevels;
        std::uint32_t FirstArraySlice;
        std::uint32_t ArraySize;
        std::uint32_t PlaneSlice;

        float         ResourceMinLODClamp;
        std::uint32_t Shader4ComponentMapping;

        // If set to INVALID the SRV will allocate a new slot
        std::uint32_t DescriptorIndex;

    } KFE_SRV_CREATE_DESC;

    class KFE_API KFETextureSRV final : public IKFEObject
    {
    public:
        KFETextureSRV() noexcept;
        ~KFETextureSRV() noexcept;

        KFETextureSRV(const KFETextureSRV&) = delete;
        KFETextureSRV& operator=(const KFETextureSRV&) = delete;

        KFETextureSRV(KFETextureSRV&&) noexcept;
        KFETextureSRV& operator=(KFETextureSRV&&) noexcept;

        NODISCARD bool Initialize(_In_ const KFE_SRV_CREATE_DESC& desc);

        NODISCARD bool Destroy() const noexcept;
        NODISCARD bool IsInitialize() const noexcept;

        NODISCARD KFEResourceHeap* GetAttachedHeap() const noexcept;
        NODISCARD KFETexture* GetTexture() const noexcept;

        NODISCARD std::uint32_t                  GetDescriptorIndex() const noexcept;
        NODISCARD bool                           HasValidDescriptor() const noexcept;
        NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE    GetCPUHandle() const noexcept;

        // View parameters
        NODISCARD DXGI_FORMAT         GetFormat() const noexcept;
        NODISCARD D3D12_SRV_DIMENSION GetViewDimension() const noexcept;
        NODISCARD std::uint32_t       GetMostDetailedMip() const noexcept;
        NODISCARD std::uint32_t       GetMipLevels() const noexcept;
        NODISCARD std::uint32_t       GetFirstArraySlice() const noexcept;
        NODISCARD std::uint32_t       GetArraySize() const noexcept;
        NODISCARD std::uint32_t       GetPlaneSlice() const noexcept;
        NODISCARD float               GetResourceMinLODClamp() const noexcept;
        NODISCARD std::uint32_t       GetShader4ComponentMapping() const noexcept;

        // Inherited via IKFEObject
        std::string GetName() const noexcept override;
        std::string GetDescription() const noexcept override;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace kfe
