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

enum   D3D12_FILTER;
enum   D3D12_TEXTURE_ADDRESS_MODE;
enum   D3D12_COMPARISON_FUNC;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

namespace kfe
{
    class KFEDevice;
    class KFESamplerHeap;

    typedef struct _KFE_SAMPLER_CREATE_DESC
    {
        _In_ KFEDevice* Device;
        _In_ KFESamplerHeap* Heap;

        D3D12_FILTER               Filter;
        D3D12_TEXTURE_ADDRESS_MODE AddressU;
        D3D12_TEXTURE_ADDRESS_MODE AddressV;
        D3D12_TEXTURE_ADDRESS_MODE AddressW;
        float                      MipLODBias;
        std::uint32_t              MaxAnisotropy;
        D3D12_COMPARISON_FUNC      ComparisonFunc;
        float                      BorderColor[4];
        float                      MinLOD;
        float                      MaxLOD;

        // If set to INVALID the sampler will allocate a new slot
        std::uint32_t DescriptorIndex;

    } KFE_SAMPLER_CREATE_DESC;

    class KFE_API KFESampler final : public IKFEObject
    {
    public:
        KFESampler() noexcept;
        ~KFESampler() noexcept;

        KFESampler(const KFESampler&) = delete;
        KFESampler& operator=(const KFESampler&) = delete;

        KFESampler(KFESampler&&) noexcept;
        KFESampler& operator=(KFESampler&&) noexcept;

        NODISCARD bool Initialize(_In_ const KFE_SAMPLER_CREATE_DESC& desc);

        NODISCARD bool Destroy() noexcept;
        NODISCARD bool IsInitialize() const noexcept;

        NODISCARD KFESamplerHeap* GetAttachedHeap() const noexcept;

        NODISCARD std::uint32_t               GetDescriptorIndex() const noexcept;
        NODISCARD bool                        HasValidDescriptor() const noexcept;
        NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept;

        // View parameters
        NODISCARD D3D12_FILTER               GetFilter() const noexcept;
        NODISCARD D3D12_TEXTURE_ADDRESS_MODE GetAddressU() const noexcept;
        NODISCARD D3D12_TEXTURE_ADDRESS_MODE GetAddressV() const noexcept;
        NODISCARD D3D12_TEXTURE_ADDRESS_MODE GetAddressW() const noexcept;
        NODISCARD float                      GetMipLODBias() const noexcept;
        NODISCARD std::uint32_t              GetMaxAnisotropy() const noexcept;
        NODISCARD D3D12_COMPARISON_FUNC      GetComparisonFunc() const noexcept;
        NODISCARD const float* GetBorderColor() const noexcept;
        NODISCARD float                      GetMinLOD() const noexcept;
        NODISCARD float                      GetMaxLOD() const noexcept;

        // Inherited via IKFEObject
        std::string GetName() const noexcept override;
        std::string GetDescription() const noexcept override;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace kfe
