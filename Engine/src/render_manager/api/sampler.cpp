// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */
#include "pch.h"
#include "engine/render_manager/api/sampler.h"

 //~ Heaps
#include "engine/render_manager/api/heap/heap_sampler.h"

//~ Device
#include "engine/render_manager/api/components/device.h"

//~ Utils
#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"

#include <d3d12.h>

#pragma region Impl_Declaration

class kfe::KFESampler::Impl
{
public:
    Impl() = default;

    ~Impl()
    {
        if (!Destroy())
        {
            LOG_ERROR("KFESampler::Impl::~Impl: Failed to destroy sampler cleanly.");
        }
    }

    NODISCARD bool Initialize(_In_ const KFE_SAMPLER_CREATE_DESC& desc);

    NODISCARD bool Destroy() noexcept;
    NODISCARD bool IsInitialize() const noexcept;

    NODISCARD KFESamplerHeap* GetAttachedHeap() const noexcept;

    NODISCARD std::uint32_t               GetDescriptorIndex() const noexcept;
    NODISCARD bool                        HasValidDescriptor() const noexcept;
    NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept;

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

private:
    NODISCARD bool CacheValidateInput(const KFE_SAMPLER_CREATE_DESC& desc);
    void Reset();

private:
    bool            m_bInitialized{ false };
    KFEDevice* m_pDevice{ nullptr };
    KFESamplerHeap* m_pHeap{ nullptr };

    D3D12_FILTER               m_filter{};
    D3D12_TEXTURE_ADDRESS_MODE m_addressU{};
    D3D12_TEXTURE_ADDRESS_MODE m_addressV{};
    D3D12_TEXTURE_ADDRESS_MODE m_addressW{};
    float                      m_mipLODBias{ 0.0f };
    std::uint32_t              m_maxAnisotropy{ 1u };
    D3D12_COMPARISON_FUNC      m_comparisonFunc{};
    float                      m_borderColor[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
    float                      m_minLOD{ 0.0f };
    float                      m_maxLOD{ D3D12_FLOAT32_MAX };

    std::uint32_t              m_descriptorIndex{ KFE_INVALID_INDEX };
};

#pragma endregion

#pragma region Public_Implementation

kfe::KFESampler::KFESampler() noexcept
    : m_impl(std::make_unique<kfe::KFESampler::Impl>())
{
}

kfe::KFESampler::~KFESampler() noexcept = default;

kfe::KFESampler::KFESampler(KFESampler&&) noexcept = default;

kfe::KFESampler& kfe::KFESampler::operator=(KFESampler&&) noexcept = default;

_Use_decl_annotations_
bool kfe::KFESampler::Initialize(const KFE_SAMPLER_CREATE_DESC& desc)
{
    return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFESampler::Destroy() noexcept
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFESampler::IsInitialize() const noexcept
{
    return m_impl->IsInitialize();
}

_Use_decl_annotations_
kfe::KFESamplerHeap* kfe::KFESampler::GetAttachedHeap() const noexcept
{
    return m_impl->GetAttachedHeap();
}

_Use_decl_annotations_
std::uint32_t kfe::KFESampler::GetDescriptorIndex() const noexcept
{
    return m_impl->GetDescriptorIndex();
}

_Use_decl_annotations_
bool kfe::KFESampler::HasValidDescriptor() const noexcept
{
    return m_impl->HasValidDescriptor();
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFESampler::GetCPUHandle() const noexcept
{
    return m_impl->GetCPUHandle();
}

_Use_decl_annotations_
D3D12_FILTER kfe::KFESampler::GetFilter() const noexcept
{
    return m_impl->GetFilter();
}

_Use_decl_annotations_
D3D12_TEXTURE_ADDRESS_MODE kfe::KFESampler::GetAddressU() const noexcept
{
    return m_impl->GetAddressU();
}

_Use_decl_annotations_
D3D12_TEXTURE_ADDRESS_MODE kfe::KFESampler::GetAddressV() const noexcept
{
    return m_impl->GetAddressV();
}

_Use_decl_annotations_
D3D12_TEXTURE_ADDRESS_MODE kfe::KFESampler::GetAddressW() const noexcept
{
    return m_impl->GetAddressW();
}

_Use_decl_annotations_
float kfe::KFESampler::GetMipLODBias() const noexcept
{
    return m_impl->GetMipLODBias();
}

_Use_decl_annotations_
std::uint32_t kfe::KFESampler::GetMaxAnisotropy() const noexcept
{
    return m_impl->GetMaxAnisotropy();
}

_Use_decl_annotations_
D3D12_COMPARISON_FUNC kfe::KFESampler::GetComparisonFunc() const noexcept
{
    return m_impl->GetComparisonFunc();
}

_Use_decl_annotations_
const float* kfe::KFESampler::GetBorderColor() const noexcept
{
    return m_impl->GetBorderColor();
}

_Use_decl_annotations_
float kfe::KFESampler::GetMinLOD() const noexcept
{
    return m_impl->GetMinLOD();
}

_Use_decl_annotations_
float kfe::KFESampler::GetMaxLOD() const noexcept
{
    return m_impl->GetMaxLOD();
}

std::string kfe::KFESampler::GetName() const noexcept
{
    return "KFESampler";
}

std::string kfe::KFESampler::GetDescription() const noexcept
{
    return "KFESampler: D3D12 Sampler State";
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFESampler::Impl::Initialize(const KFE_SAMPLER_CREATE_DESC& desc)
{
    // Validate + cache everything + allocate descriptor
    if (!CacheValidateInput(desc))
    {
        return false;
    }

    if (m_pDevice == nullptr)
    {
        LOG_ERROR("KFESampler::Impl::Initialize: Cached device is null.");
        Reset();
        return false;
    }

    if (m_pHeap == nullptr)
    {
        LOG_ERROR("KFESampler::Impl::Initialize: Cached sampler heap is null.");
        Reset();
        return false;
    }

    auto* nativeDevice = m_pDevice->GetNative();
    if (nativeDevice == nullptr)
    {
        LOG_ERROR("KFESampler::Impl::Initialize: Native device is null.");
        Reset();
        return false;
    }

    D3D12_SAMPLER_DESC sampler{};
    sampler.Filter = m_filter;
    sampler.AddressU = m_addressU;
    sampler.AddressV = m_addressV;
    sampler.AddressW = m_addressW;
    sampler.MipLODBias = m_mipLODBias;
    sampler.MaxAnisotropy = m_maxAnisotropy;
    sampler.ComparisonFunc = m_comparisonFunc;
    sampler.MinLOD = m_minLOD;
    sampler.MaxLOD = m_maxLOD;
    sampler.BorderColor[0] = m_borderColor[0];
    sampler.BorderColor[1] = m_borderColor[1];
    sampler.BorderColor[2] = m_borderColor[2];
    sampler.BorderColor[3] = m_borderColor[3];

    if (!HasValidDescriptor())
    {
        LOG_ERROR("KFESampler::Impl::Initialize: Descriptor index is not valid after CacheValidateInput.");
        Reset();
        return false;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_pHeap->GetHandle(m_descriptorIndex);

    nativeDevice->CreateSampler(
        &sampler,
        cpuHandle
    );

    LOG_SUCCESS("KFESampler::Impl::Initialize: Sampler created. HeapIdx = {}", m_descriptorIndex);
    return true;
}

_Use_decl_annotations_
bool kfe::KFESampler::Impl::Destroy() noexcept
{
    if (!m_bInitialized)
    {
        return true;
    }

    if (m_pHeap != nullptr &&
        m_descriptorIndex != KFE_INVALID_INDEX &&
        m_pHeap->IsValidIndex(m_descriptorIndex))
    {
        if (!m_pHeap->Free(m_descriptorIndex))
        {
            LOG_WARNING("KFESampler::Impl::Destroy: Failed to free descriptor index {}.", m_descriptorIndex);
        }
    }

    Reset();

    LOG_SUCCESS("KFESampler::Impl::Destroy: Sampler destroyed and state reset.");
    return true;
}

_Use_decl_annotations_
bool kfe::KFESampler::Impl::IsInitialize() const noexcept
{
    return m_bInitialized;
}

_Use_decl_annotations_
kfe::KFESamplerHeap* kfe::KFESampler::Impl::GetAttachedHeap() const noexcept
{
    return m_pHeap;
}

_Use_decl_annotations_
std::uint32_t kfe::KFESampler::Impl::GetDescriptorIndex() const noexcept
{
    return m_descriptorIndex;
}

_Use_decl_annotations_
bool kfe::KFESampler::Impl::HasValidDescriptor() const noexcept
{
    if (m_pHeap == nullptr)
    {
        return false;
    }

    if (m_descriptorIndex == KFE_INVALID_INDEX)
    {
        return false;
    }

    return m_pHeap->IsValidIndex(m_descriptorIndex);
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFESampler::Impl::GetCPUHandle() const noexcept
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};
    handle.ptr = 0;

    if (!HasValidDescriptor())
    {
        return handle;
    }

    return m_pHeap->GetHandle(m_descriptorIndex);
}

_Use_decl_annotations_
D3D12_FILTER kfe::KFESampler::Impl::GetFilter() const noexcept
{
    return m_filter;
}

_Use_decl_annotations_
D3D12_TEXTURE_ADDRESS_MODE kfe::KFESampler::Impl::GetAddressU() const noexcept
{
    return m_addressU;
}

_Use_decl_annotations_
D3D12_TEXTURE_ADDRESS_MODE kfe::KFESampler::Impl::GetAddressV() const noexcept
{
    return m_addressV;
}

_Use_decl_annotations_
D3D12_TEXTURE_ADDRESS_MODE kfe::KFESampler::Impl::GetAddressW() const noexcept
{
    return m_addressW;
}

_Use_decl_annotations_
float kfe::KFESampler::Impl::GetMipLODBias() const noexcept
{
    return m_mipLODBias;
}

_Use_decl_annotations_
std::uint32_t kfe::KFESampler::Impl::GetMaxAnisotropy() const noexcept
{
    return m_maxAnisotropy;
}

_Use_decl_annotations_
D3D12_COMPARISON_FUNC kfe::KFESampler::Impl::GetComparisonFunc() const noexcept
{
    return m_comparisonFunc;
}

_Use_decl_annotations_
const float* kfe::KFESampler::Impl::GetBorderColor() const noexcept
{
    return m_borderColor;
}

_Use_decl_annotations_
float kfe::KFESampler::Impl::GetMinLOD() const noexcept
{
    return m_minLOD;
}

_Use_decl_annotations_
float kfe::KFESampler::Impl::GetMaxLOD() const noexcept
{
    return m_maxLOD;
}

_Use_decl_annotations_
bool kfe::KFESampler::Impl::CacheValidateInput(const KFE_SAMPLER_CREATE_DESC& desc)
{
    if (m_bInitialized)
    {
        LOG_WARNING("KFESampler::Impl::Initialize: Already initialized. Destroying previous state.");
        if (!Destroy())
        {
            LOG_ERROR("KFESampler::Impl::Initialize: Failed to destroy previous state.");
            return false;
        }
    }

    if (desc.Device == nullptr)
    {
        LOG_ERROR("KFESampler::Impl::Initialize: Device is null.");
        return false;
    }

    if (desc.Heap == nullptr)
    {
        LOG_ERROR("KFESampler::Impl::Initialize: Sampler heap is null.");
        return false;
    }

    m_pDevice = desc.Device;
    m_pHeap = desc.Heap;

    m_filter = desc.Filter;
    m_addressU = desc.AddressU;
    m_addressV = desc.AddressV;
    m_addressW = desc.AddressW;
    m_mipLODBias = desc.MipLODBias;
    m_maxAnisotropy = (desc.MaxAnisotropy == 0u) ? 1u : desc.MaxAnisotropy;
    m_comparisonFunc = desc.ComparisonFunc;

    m_borderColor[0] = desc.BorderColor[0];
    m_borderColor[1] = desc.BorderColor[1];
    m_borderColor[2] = desc.BorderColor[2];
    m_borderColor[3] = desc.BorderColor[3];

    m_minLOD = desc.MinLOD;
    m_maxLOD = (desc.MaxLOD < desc.MinLOD) ? D3D12_FLOAT32_MAX : desc.MaxLOD;

    std::uint32_t index = desc.DescriptorIndex;

    if (index == KFE_INVALID_INDEX)
    {
        index = m_pHeap->Allocate();
        if (index == KFE_INVALID_INDEX)
        {
            LOG_ERROR("KFESampler::Impl::Initialize: Failed to allocate descriptor from sampler heap.");
            Reset();
            return false;
        }
    }
    else
    {
        if (!m_pHeap->IsValidIndex(index))
        {
            LOG_ERROR("KFESampler::Impl::Initialize: Provided descriptor index {} is invalid for this heap.", index);
            Reset();
            return false;
        }
    }

    m_descriptorIndex = index;
    m_bInitialized = true;

    LOG_SUCCESS("KFESampler::Impl::Initialize: Sampler cached. HeapIdx = {}", m_descriptorIndex);
    return true;
}

void kfe::KFESampler::Impl::Reset()
{
    m_pDevice = nullptr;
    m_pHeap = nullptr;
    m_filter = static_cast<D3D12_FILTER>(0);
    m_addressU = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(0);
    m_addressV = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(0);
    m_addressW = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(0);
    m_mipLODBias = 0.0f;
    m_maxAnisotropy = 1u;
    m_comparisonFunc = static_cast<D3D12_COMPARISON_FUNC>(0);
    m_borderColor[0] = 0.0f;
    m_borderColor[1] = 0.0f;
    m_borderColor[2] = 0.0f;
    m_borderColor[3] = 0.0f;
    m_minLOD = 0.0f;
    m_maxLOD = D3D12_FLOAT32_MAX;
    m_descriptorIndex = KFE_INVALID_INDEX;
    m_bInitialized = false;
}

#pragma endregion
