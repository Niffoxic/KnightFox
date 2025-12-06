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

#include "engine/render_manager/buffer/structured_buffer.h"
#include "engine/render_manager/buffer/buffer.h"
#include "engine/render_manager/heap/heap_cbv_srv_uav.h"
#include "engine/render_manager/components/device.h"
#include "engine/utils/logger.h"

#include <d3d12.h>
#include <limits>
#include <utility>

namespace
{
    constexpr std::uint32_t KFE_INVALID_DESCRIPTOR_INDEX =
        std::numeric_limits<std::uint32_t>::max();
}

#pragma region Impl_Declaration

class kfe::KFEStructuredBuffer::Impl
{
public:
    Impl() = default;
    ~Impl()
    {
        if (!Destroy())
        {
            LOG_ERROR("KFEStructuredBuffer::Impl::~Impl: Failed to destroy structured buffer cleanly.");
        }
    }

    NODISCARD bool Initialize(_In_ const KFE_STRUCTURED_BUFFER_CREATE_DESC& desc);

    NODISCARD bool Destroy      ()       noexcept;
    NODISCARD bool IsInitialized() const noexcept;

    NODISCARD       KFEBuffer* GetBuffer()       noexcept;
    NODISCARD const KFEBuffer* GetBuffer() const noexcept;
    NODISCARD       KFEResourceHeap* GetResourceHeap()       noexcept;
    NODISCARD const KFEResourceHeap* GetResourceHeap() const noexcept;

    NODISCARD std::uint32_t GetElementStride() const noexcept;
    NODISCARD std::uint32_t GetElementCount () const noexcept;
    NODISCARD std::uint64_t GetOffsetInBytes() const noexcept;

    NODISCARD std::uint32_t CreateSRV(_In_ const KFE_STRUCTURED_SRV_DESC& desc);
    NODISCARD std::uint32_t CreateUAV(_In_ const KFE_STRUCTURED_UAV_DESC& desc);

    NODISCARD std::uint32_t GetNumSRVs() const noexcept;
    NODISCARD std::uint32_t GetNumUAVs() const noexcept;

    NODISCARD std::uint32_t GetSRVDescriptorIndex(std::uint32_t viewIndex) const noexcept;
    NODISCARD std::uint32_t GetUAVDescriptorIndex(std::uint32_t viewIndex) const noexcept;

private:
    NODISCARD bool ValidateSRVRange(
        _In_  const KFE_STRUCTURED_SRV_DESC& desc,
        _Out_ std::uint32_t& firstElement,
        _Out_ std::uint32_t& numElements) const noexcept;

    NODISCARD bool ValidateUAVRange(
        _In_  const KFE_STRUCTURED_UAV_DESC& desc,
        _Out_ std::uint32_t& firstElement,
        _Out_ std::uint32_t& numElements) const noexcept;

private:
    KFEDevice*       m_pDevice        { nullptr };
    KFEBuffer*       m_pResourceBuffer{ nullptr };
    KFEResourceHeap* m_pResourceHeap  { nullptr };

    std::uint32_t     m_elementStride{ 0u };
    std::uint32_t     m_elementCount { 0u };
    std::uint64_t     m_offsetInBytes{ 0u };

    std::vector<std::uint32_t> m_srvDescriptorIndices;
    std::vector<std::uint32_t> m_uavDescriptorIndices;

    bool              m_bInitialized{ false };
};

#pragma endregion

#pragma region Class_Implementation

kfe::KFEStructuredBuffer::KFEStructuredBuffer() noexcept
    : m_impl(std::make_unique<kfe::KFEStructuredBuffer::Impl>())
{
}

kfe::KFEStructuredBuffer::~KFEStructuredBuffer() noexcept = default;

kfe::KFEStructuredBuffer::KFEStructuredBuffer(KFEStructuredBuffer&& other) noexcept = default;
kfe::KFEStructuredBuffer& kfe::KFEStructuredBuffer::operator=(KFEStructuredBuffer&& other) noexcept = default;

_Use_decl_annotations_
std::string kfe::KFEStructuredBuffer::GetName() const noexcept
{
    return "KFEStructuredBuffer";
}

_Use_decl_annotations_
std::string kfe::KFEStructuredBuffer::GetDescription() const noexcept
{
    return "Structured buffer wrapper: manages structured SRVs and UAVs over an existing KFEBuffer.";
}

_Use_decl_annotations_
bool kfe::KFEStructuredBuffer::Initialize(const KFE_STRUCTURED_BUFFER_CREATE_DESC& desc)
{
    return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFEStructuredBuffer::Destroy() noexcept
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFEStructuredBuffer::IsInitialized() const noexcept
{
    return m_impl->IsInitialized();
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEStructuredBuffer::GetBuffer() noexcept
{
    return m_impl->GetBuffer();
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEStructuredBuffer::GetBuffer() const noexcept
{
    return m_impl->GetBuffer();
}

_Use_decl_annotations_
kfe::KFEResourceHeap* kfe::KFEStructuredBuffer::GetResourceHeap() noexcept
{
    return m_impl->GetResourceHeap();
}

_Use_decl_annotations_
const kfe::KFEResourceHeap* kfe::KFEStructuredBuffer::GetResourceHeap() const noexcept
{
    return m_impl->GetResourceHeap();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::GetElementStride() const noexcept
{
    return m_impl->GetElementStride();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::GetElementCount() const noexcept
{
    return m_impl->GetElementCount();
}

_Use_decl_annotations_
std::uint64_t kfe::KFEStructuredBuffer::GetOffsetInBytes() const noexcept
{
    return m_impl->GetOffsetInBytes();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::CreateSRV(_In_ const KFE_STRUCTURED_SRV_DESC& desc)
{
    return m_impl->CreateSRV(desc);
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::CreateUAV(_In_ const KFE_STRUCTURED_UAV_DESC& desc)
{
    return m_impl->CreateUAV(desc);
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::GetNumSRVs() const noexcept
{
    return m_impl->GetNumSRVs();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::GetNumUAVs() const noexcept
{
    return m_impl->GetNumUAVs();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::GetSRVDescriptorIndex(std::uint32_t viewIndex) const noexcept
{
    return m_impl->GetSRVDescriptorIndex(viewIndex);
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::GetUAVDescriptorIndex(std::uint32_t viewIndex) const noexcept
{
    return m_impl->GetUAVDescriptorIndex(viewIndex);
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFEStructuredBuffer::Impl::Initialize(const KFE_STRUCTURED_BUFFER_CREATE_DESC& desc)
{
    // Validate device
    if (!desc.Device || !desc.Device->GetNative())
    {
        LOG_ERROR("KFEStructuredBuffer::Impl::Initialize: Device or native device is null.");
        return false;
    }

    // Validate buffer
    if (!desc.ResourceBuffer || !desc.ResourceBuffer->GetNative())
    {
        LOG_ERROR("KFEStructuredBuffer::Impl::Initialize: ResourceBuffer or its native resource is null.");
        return false;
    }

    // Validate heap
    if (!desc.ResourceHeap)
    {
        LOG_ERROR("KFEStructuredBuffer::Impl::Initialize: ResourceHeap is null.");
        return false;
    }

    if (desc.ElementStride == 0u)
    {
        LOG_ERROR("KFEStructuredBuffer::Impl::Initialize: ElementStride must be > 0.");
        return false;
    }

    if (desc.ElementCount == 0u)
    {
        LOG_ERROR("KFEStructuredBuffer::Impl::Initialize: ElementCount must be > 0.");
        return false;
    }

    // If already initialized, destroy and re-init
    if (IsInitialized())
    {
        LOG_WARNING("KFEStructuredBuffer::Impl::Initialize: Already initialized. Destroying and recreating.");
        if (!Destroy())
        {
            LOG_ERROR("KFEStructuredBuffer::Impl::Initialize: Destroy failed; cannot reinitialize.");
            return false;
        }
    }

    m_pDevice           = desc.Device;
    m_pResourceBuffer   = desc.ResourceBuffer;
    m_pResourceHeap     = desc.ResourceHeap;
    m_elementStride     = desc.ElementStride;
    m_elementCount      = desc.ElementCount;
    m_offsetInBytes     = desc.OffsetInBytes;

    const std::uint64_t bufferSize = m_pResourceBuffer->GetSizeInBytes();
    const std::uint64_t totalBytes =
        static_cast<std::uint64_t>(m_elementStride) *
        static_cast<std::uint64_t>(m_elementCount);

    if (totalBytes == 0u)
    {
        LOG_ERROR(
            "KFEStructuredBuffer::Impl::Initialize: Computed totalBytes == 0; "
            "ElementStride={}, ElementCount={}.",
            m_elementStride, m_elementCount);
        return false;
    }

    if (m_offsetInBytes >= bufferSize)
    {
        LOG_ERROR(
            "KFEStructuredBuffer::Impl::Initialize: OffsetInBytes ({}) is >= buffer size ({}).",
            m_offsetInBytes, bufferSize);
        return false;
    }

    if (m_offsetInBytes + totalBytes > bufferSize)
    {
        LOG_ERROR(
            "KFEStructuredBuffer::Impl::Initialize: Range [offset={}, offset+size={}] exceeds "
            "buffer size ({}). ElementStride={}, ElementCount={}.",
            m_offsetInBytes,
            m_offsetInBytes + totalBytes,
            bufferSize,
            m_elementStride,
            m_elementCount);
        return false;
    }

    m_srvDescriptorIndices.clear();
    m_uavDescriptorIndices.clear();

    LOG_SUCCESS(
        "KFEStructuredBuffer::Impl::Initialize: Initialized structured buffer. "
        "Stride={}, Count={}, Offset={}, BufferSize={}",
        m_elementStride, m_elementCount, m_offsetInBytes, bufferSize);

    m_bInitialized = true;
    return true;
}

_Use_decl_annotations_
bool kfe::KFEStructuredBuffer::Impl::Destroy() noexcept
{
    if (m_pResourceHeap)
    {
        for (auto idx : m_srvDescriptorIndices)
        {
            if (idx != KFE_INVALID_DESCRIPTOR_INDEX)
            {
                if (!m_pResourceHeap->Free(idx))
                {
                    LOG_ERROR("KFEStructuredBuffer::Impl::Destroy: Failed to free SRV descriptor index {}.", idx);
                }
            }
        }

        for (auto idx : m_uavDescriptorIndices)
        {
            if (idx != KFE_INVALID_DESCRIPTOR_INDEX)
            {
                if (!m_pResourceHeap->Free(idx))
                {
                    LOG_ERROR("KFEStructuredBuffer::Impl::Destroy: Failed to free UAV descriptor index {}.", idx);
                }
            }
        }
    }

    m_srvDescriptorIndices.clear();
    m_uavDescriptorIndices.clear();

    m_pDevice           = nullptr;
    m_pResourceBuffer   = nullptr;
    m_pResourceHeap     = nullptr;
    m_elementStride     = 0u;
    m_elementCount      = 0u;
    m_offsetInBytes     = 0u;
    m_bInitialized      = false;

    return true;
}

_Use_decl_annotations_
bool kfe::KFEStructuredBuffer::Impl::IsInitialized() const noexcept
{
    return m_bInitialized;
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEStructuredBuffer::Impl::GetBuffer() noexcept
{
    return m_pResourceBuffer;
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEStructuredBuffer::Impl::GetBuffer() const noexcept
{
    return m_pResourceBuffer;
}

_Use_decl_annotations_
kfe::KFEResourceHeap* kfe::KFEStructuredBuffer::Impl::GetResourceHeap() noexcept
{
    return m_pResourceHeap;
}

_Use_decl_annotations_
const kfe::KFEResourceHeap* kfe::KFEStructuredBuffer::Impl::GetResourceHeap() const noexcept
{
    return m_pResourceHeap;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::Impl::GetElementStride() const noexcept
{
    return m_elementStride;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::Impl::GetElementCount() const noexcept
{
    return m_elementCount;
}

_Use_decl_annotations_
std::uint64_t kfe::KFEStructuredBuffer::Impl::GetOffsetInBytes() const noexcept
{
    return m_offsetInBytes;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::Impl::CreateSRV(_In_ const KFE_STRUCTURED_SRV_DESC& desc)
{
    if (!IsInitialized())
    {
        LOG_ERROR("KFEStructuredBuffer::Impl::CreateSRV: Structured buffer not initialized.");
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    std::uint32_t firstElement = 0u;
    std::uint32_t numElements = 0u;

    if (!ValidateSRVRange(desc, firstElement, numElements))
    {
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    const std::uint32_t descriptorIndex = m_pResourceHeap->Allocate();
    if (descriptorIndex == KFE_INVALID_DESCRIPTOR_INDEX)
    {
        LOG_ERROR("KFEStructuredBuffer::Impl::CreateSRV: Failed to allocate descriptor.");
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE handle = m_pResourceHeap->GetHandle(descriptorIndex);

    ID3D12Resource* resource = m_pResourceBuffer->GetNative();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format                      = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension               = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping     = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement         = firstElement;
    srvDesc.Buffer.NumElements          = numElements;
    srvDesc.Buffer.StructureByteStride  = m_elementStride;
    srvDesc.Buffer.Flags                = D3D12_BUFFER_SRV_FLAG_NONE;

    m_pDevice->GetNative()->CreateShaderResourceView(
        resource,
        &srvDesc,
        handle);

    m_srvDescriptorIndices.push_back(descriptorIndex);

    LOG_SUCCESS(
        "KFEStructuredBuffer::Impl::CreateSRV: Created structured SRV. FirstElement={}, NumElements={}, DescriptorIndex={}.",
        firstElement, numElements, descriptorIndex);

    return descriptorIndex;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::Impl::CreateUAV(_In_ const KFE_STRUCTURED_UAV_DESC& desc)
{
    if (!IsInitialized())
    {
        LOG_ERROR("KFEStructuredBuffer::Impl::CreateUAV: Structured buffer not initialized.");
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    std::uint32_t firstElement = 0u;
    std::uint32_t numElements = 0u;

    if (!ValidateUAVRange(desc, firstElement, numElements))
    {
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    const std::uint32_t descriptorIndex = m_pResourceHeap->Allocate();
    if (descriptorIndex == KFE_INVALID_DESCRIPTOR_INDEX)
    {
        LOG_ERROR("KFEStructuredBuffer::Impl::CreateUAV: Failed to allocate descriptor.");
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE handle = m_pResourceHeap->GetHandle(descriptorIndex);

    ID3D12Resource* resource = m_pResourceBuffer->GetNative();
    if (!resource)
    {
        LOG_ERROR("KFEStructuredBuffer::Impl::CreateUAV: Resource buffer native pointer is null.");
        m_pResourceHeap->Free(descriptorIndex);
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    const D3D12_RESOURCE_DESC resDesc = resource->GetDesc();
    if ((resDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
    {
        LOG_ERROR(
            "KFEStructuredBuffer::Impl::CreateUAV: Resource does not have D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS set. "
            "Aborting UAV creation. DescriptorIndex={}.",
            descriptorIndex);

        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = firstElement;
    uavDesc.Buffer.NumElements = numElements;
    uavDesc.Buffer.StructureByteStride = m_elementStride;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    uavDesc.Buffer.CounterOffsetInBytes = 0u;

    if (desc.HasCounter)
    {
        LOG_WARNING("KFEStructuredBuffer::Impl::CreateUAV: HasCounter=true but counter buffer support is not implemented yet.");
    }

    m_pDevice->GetNative()->CreateUnorderedAccessView(
        resource,
        nullptr,
        &uavDesc,
        handle);

    m_uavDescriptorIndices.push_back(descriptorIndex);

    LOG_SUCCESS(
        "KFEStructuredBuffer::Impl::CreateUAV: Created structured UAV. FirstElement={}, NumElements={}, DescriptorIndex={}.",
        firstElement, numElements, descriptorIndex);

    return descriptorIndex;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::Impl::GetNumSRVs() const noexcept
{
    return static_cast<std::uint32_t>(m_srvDescriptorIndices.size());
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::Impl::GetNumUAVs() const noexcept
{
    return static_cast<std::uint32_t>(m_uavDescriptorIndices.size());
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::Impl::GetSRVDescriptorIndex(std::uint32_t viewIndex) const noexcept
{
    if (viewIndex >= m_srvDescriptorIndices.size())
    {
        LOG_ERROR(
            "KFEStructuredBuffer::Impl::GetSRVDescriptorIndex: viewIndex {} is out of range [0, {}).",
            viewIndex, m_srvDescriptorIndices.size());
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    return m_srvDescriptorIndices[viewIndex];
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStructuredBuffer::Impl::GetUAVDescriptorIndex(std::uint32_t viewIndex) const noexcept
{
    if (viewIndex >= m_uavDescriptorIndices.size())
    {
        LOG_ERROR(
            "KFEStructuredBuffer::Impl::GetUAVDescriptorIndex: viewIndex {} is out of range [0, {}).",
            viewIndex, m_uavDescriptorIndices.size());
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    return m_uavDescriptorIndices[viewIndex];
}

_Use_decl_annotations_
bool kfe::KFEStructuredBuffer::Impl::ValidateSRVRange(
    _In_  const KFE_STRUCTURED_SRV_DESC& desc,
    _Out_ std::uint32_t& firstElement,
    _Out_ std::uint32_t& numElements) const noexcept
{
    firstElement = desc.FirstElement;

    if (firstElement >= m_elementCount)
    {
        LOG_ERROR(
            "KFEStructuredBuffer::Impl::ValidateSRVRange: FirstElement ({}) is >= ElementCount ({}).",
            firstElement, m_elementCount);
        return false;
    }

    if (desc.NumElements == 0u)
    {
        numElements = m_elementCount - firstElement;
    }
    else
    {
        numElements = desc.NumElements;
        if (firstElement + numElements > m_elementCount)
        {
            LOG_ERROR(
                "KFEStructuredBuffer::Impl::ValidateSRVRange: Range [FirstElement={}, FirstElement+NumElements={}] "
                "exceeds ElementCount ({}).",
                firstElement,
                firstElement + numElements,
                m_elementCount);
            return false;
        }
    }

    return true;
}

_Use_decl_annotations_
bool kfe::KFEStructuredBuffer::Impl::ValidateUAVRange(
    _In_  const KFE_STRUCTURED_UAV_DESC& desc,
    _Out_ std::uint32_t& firstElement,
    _Out_ std::uint32_t& numElements) const noexcept
{
    firstElement = desc.FirstElement;

    if (firstElement >= m_elementCount)
    {
        LOG_ERROR(
            "KFEStructuredBuffer::Impl::ValidateUAVRange: FirstElement ({}) is >= ElementCount ({}).",
            firstElement, m_elementCount);
        return false;
    }

    if (desc.NumElements == 0u)
    {
        numElements = m_elementCount - firstElement;
    }
    else
    {
        numElements = desc.NumElements;
        if (firstElement + numElements > m_elementCount)
        {
            LOG_ERROR(
                "KFEStructuredBuffer::Impl::ValidateUAVRange: Range [FirstElement={}, FirstElement+NumElements={}] "
                "exceeds ElementCount ({}).",
                firstElement,
                firstElement + numElements,
                m_elementCount);
            return false;
        }
    }

    return true;
}

#pragma endregion
