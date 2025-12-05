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

#include "engine/render_manager/buffer/raw_buffer.h"
#include "engine/render_manager/buffer/buffer.h"
#include "engine/render_manager/heap/heap_cbv_srv_uav.h"
#include "engine/render_manager/components/device.h"
#include "engine/utils/logger.h"

#include <d3d12.h>
#include <limits>
#include <vector>
#include <cstdint>

namespace
{
    constexpr std::uint32_t KFE_INVALID_DESCRIPTOR_INDEX =
        std::numeric_limits<std::uint32_t>::max();
}

#pragma region Impl_Declaration

class kfe::KFERawBuffer::Impl
{
public:
    Impl() = default;
    ~Impl()
    {
        if (!Destroy())
        {
            LOG_ERROR("KFERawBuffer::Impl::~Impl: Failed to destroy raw buffer cleanly.");
        }
    }

    NODISCARD bool Initialize(_In_ const KFE_RAW_BUFFER_CREATE_DESC& desc);

    NODISCARD bool Destroy      () noexcept;
    NODISCARD bool IsInitialized() const noexcept;

    NODISCARD       KFEBuffer* GetBuffer()       noexcept;
    NODISCARD const KFEBuffer* GetBuffer() const noexcept;

    NODISCARD       KFEResourceHeap* GetResourceHeap()       noexcept;
    NODISCARD const KFEResourceHeap* GetResourceHeap() const noexcept;

    NODISCARD std::uint64_t GetOffsetInBytes() const noexcept;
    NODISCARD std::uint64_t GetSizeInBytes  () const noexcept;

    NODISCARD std::uint32_t CreateSRV(_In_ const KFE_RAW_SRV_DESC& desc);
    NODISCARD std::uint32_t CreateUAV(_In_ const KFE_RAW_UAV_DESC& desc);

    NODISCARD std::uint32_t GetNumSRVs() const noexcept;
    NODISCARD std::uint32_t GetNumUAVs() const noexcept;

    NODISCARD std::uint32_t GetSRVDescriptorIndex(std::uint32_t viewIndex) const noexcept;
    NODISCARD std::uint32_t GetUAVDescriptorIndex(std::uint32_t viewIndex) const noexcept;

private:
    // Helpers
    NODISCARD bool ValidateSRVRange(_In_ const KFE_RAW_SRV_DESC& desc,
        _Out_ std::uint64_t& byteOffset,
        _Out_ std::uint64_t& byteSize) const noexcept;

    NODISCARD bool ValidateUAVRange(_In_ const KFE_RAW_UAV_DESC& desc,
        _Out_ std::uint64_t& byteOffset,
        _Out_ std::uint64_t& byteSize) const noexcept;

private:
    KFEDevice*       m_pDevice        { nullptr };
    KFEBuffer*       m_pResourceBuffer{ nullptr };
    KFEResourceHeap* m_pResourceHeap  { nullptr };

    std::uint64_t     m_offsetInBytes{ 0u };
    std::uint64_t     m_sizeInBytes  { 0u };

    std::vector<std::uint32_t> m_srvDescriptorIndices{};
    std::vector<std::uint32_t> m_uavDescriptorIndices{};

    bool m_bInitialized{ false };
};

#pragma endregion

#pragma region Class_Implementation

kfe::KFERawBuffer::KFERawBuffer() noexcept
    : m_impl(std::make_unique<kfe::KFERawBuffer::Impl>())
{
}

kfe::KFERawBuffer::~KFERawBuffer() noexcept = default;

kfe::KFERawBuffer::KFERawBuffer(KFERawBuffer&& other) noexcept = default;
kfe::KFERawBuffer& kfe::KFERawBuffer::operator=(KFERawBuffer&& other) noexcept = default;

_Use_decl_annotations_
std::string kfe::KFERawBuffer::GetName() const noexcept
{
    return "KFERawBuffer";
}

_Use_decl_annotations_
std::string kfe::KFERawBuffer::GetDescription() const noexcept
{
    return "Raw (byte address) buffer wrapper: manages RAW SRV/UAV views.";
}

_Use_decl_annotations_
bool kfe::KFERawBuffer::Initialize(const KFE_RAW_BUFFER_CREATE_DESC& desc)
{
    return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFERawBuffer::Destroy() noexcept
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFERawBuffer::IsInitialized() const noexcept
{
    return m_impl->IsInitialized();
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFERawBuffer::GetBuffer() noexcept
{
    return m_impl->GetBuffer();
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFERawBuffer::GetBuffer() const noexcept
{
    return m_impl->GetBuffer();
}

_Use_decl_annotations_
kfe::KFEResourceHeap* kfe::KFERawBuffer::GetResourceHeap() noexcept
{
    return m_impl->GetResourceHeap();
}

_Use_decl_annotations_
const kfe::KFEResourceHeap* kfe::KFERawBuffer::GetResourceHeap() const noexcept
{
    return m_impl->GetResourceHeap();
}

_Use_decl_annotations_
std::uint64_t kfe::KFERawBuffer::GetOffsetInBytes() const noexcept
{
    return m_impl->GetOffsetInBytes();
}

_Use_decl_annotations_
std::uint64_t kfe::KFERawBuffer::GetSizeInBytes() const noexcept
{
    return m_impl->GetSizeInBytes();
}

_Use_decl_annotations_
std::uint32_t kfe::KFERawBuffer::CreateSRV(_In_ const KFE_RAW_SRV_DESC& desc)
{
    return m_impl->CreateSRV(desc);
}

_Use_decl_annotations_
std::uint32_t kfe::KFERawBuffer::CreateUAV(_In_ const KFE_RAW_UAV_DESC& desc)
{
    return m_impl->CreateUAV(desc);
}

_Use_decl_annotations_
std::uint32_t kfe::KFERawBuffer::GetNumSRVs() const noexcept
{
    return m_impl->GetNumSRVs();
}

_Use_decl_annotations_
std::uint32_t kfe::KFERawBuffer::GetNumUAVs() const noexcept
{
    return m_impl->GetNumUAVs();
}

_Use_decl_annotations_
std::uint32_t kfe::KFERawBuffer::GetSRVDescriptorIndex(std::uint32_t viewIndex) const noexcept
{
    return m_impl->GetSRVDescriptorIndex(viewIndex);
}

_Use_decl_annotations_
std::uint32_t kfe::KFERawBuffer::GetUAVDescriptorIndex(std::uint32_t viewIndex) const noexcept
{
    return m_impl->GetUAVDescriptorIndex(viewIndex);
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFERawBuffer::Impl::Initialize(const KFE_RAW_BUFFER_CREATE_DESC& desc)
{
    // Validate device
    if (!desc.Device || !desc.Device->GetNative())
    {
        LOG_ERROR("KFERawBuffer::Impl::Initialize: Device or native device is null.");
        return false;
    }

    // Validate buffer
    if (!desc.ResourceBuffer || !desc.ResourceBuffer->GetNative())
    {
        LOG_ERROR("KFERawBuffer::Impl::Initialize: ResourceBuffer or its native resource is null.");
        return false;
    }

    // Validate heap
    if (!desc.ResourceHeap)
    {
        LOG_ERROR("KFERawBuffer::Impl::Initialize: ResourceHeap is null.");
        return false;
    }

    // If already initialized, destroy and recreate
    if (IsInitialized())
    {
        LOG_WARNING("KFERawBuffer::Impl::Initialize: Already initialized. Destroying and recreating.");
        if (!Destroy())
        {
            LOG_ERROR("KFERawBuffer::Impl::Initialize: Destroy failed; cannot reinitialize.");
            return false;
        }
    }

    m_pDevice           = desc.Device;
    m_pResourceBuffer   = desc.ResourceBuffer;
    m_pResourceHeap     = desc.ResourceHeap;

    const std::uint64_t bufferSize = m_pResourceBuffer->GetSizeInBytes();

    m_offsetInBytes = desc.OffsetInBytes;

    if (m_offsetInBytes >= bufferSize)
    {
        LOG_ERROR(
            "KFERawBuffer::Impl::Initialize: OffsetInBytes ({}) is >= buffer size ({}).",
            m_offsetInBytes, bufferSize);
        return false;
    }

    if (desc.SizeInBytes == 0u)
    {
        // Use remaining bytes from offset
        m_sizeInBytes = bufferSize - m_offsetInBytes;
    }
    else
    {
        m_sizeInBytes = desc.SizeInBytes;
        if (m_offsetInBytes + m_sizeInBytes > bufferSize)
        {
            LOG_ERROR(
                "KFERawBuffer::Impl::Initialize: Range [offset={}, offset+size={}] exceeds buffer size ({}).",
                m_offsetInBytes,
                m_offsetInBytes + m_sizeInBytes,
                bufferSize);
            return false;
        }
    }

    // RAW views require 4 bytes alignment
    if ((m_offsetInBytes % 4u) != 0u)
    {
        LOG_WARNING(
            "KFERawBuffer::Impl::Initialize: OffsetInBytes ({}) is not 4-byte aligned. "
            "RAW SRV/UAV FirstElement alignment may be problematic.",
            m_offsetInBytes);
    }

    m_srvDescriptorIndices.clear();
    m_uavDescriptorIndices.clear();

    LOG_SUCCESS(
        "KFERawBuffer::Impl::Initialize: Initialized raw buffer. Offset={}, Size={}, BufferSize={}",
        m_offsetInBytes,
        m_sizeInBytes,
        bufferSize);

    m_bInitialized = true;
    return true;
}

_Use_decl_annotations_
bool kfe::KFERawBuffer::Impl::Destroy() noexcept
{
    if (m_pResourceHeap)
    {
        for (auto idx : m_srvDescriptorIndices)
        {
            if (idx != KFE_INVALID_DESCRIPTOR_INDEX)
            {
                if (!m_pResourceHeap->Free(idx))
                {
                    LOG_ERROR("KFERawBuffer::Impl::Destroy: Failed to free SRV descriptor index {}.", idx);
                }
            }
        }

        for (auto idx : m_uavDescriptorIndices)
        {
            if (idx != KFE_INVALID_DESCRIPTOR_INDEX)
            {
                if (!m_pResourceHeap->Free(idx))
                {
                    LOG_ERROR("KFERawBuffer::Impl::Destroy: Failed to free UAV descriptor index {}.", idx);
                }
            }
        }
    }

    m_srvDescriptorIndices.clear();
    m_uavDescriptorIndices.clear();

    m_pDevice           = nullptr;
    m_pResourceBuffer   = nullptr;
    m_pResourceHeap     = nullptr;
    m_offsetInBytes     = 0u;
    m_sizeInBytes       = 0u;
    m_bInitialized      = false;

    return true;
}

_Use_decl_annotations_
bool kfe::KFERawBuffer::Impl::IsInitialized() const noexcept
{
    return m_bInitialized;
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFERawBuffer::Impl::GetBuffer() noexcept
{
    return m_pResourceBuffer;
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFERawBuffer::Impl::GetBuffer() const noexcept
{
    return m_pResourceBuffer;
}

_Use_decl_annotations_
kfe::KFEResourceHeap* kfe::KFERawBuffer::Impl::GetResourceHeap() noexcept
{
    return m_pResourceHeap;
}

_Use_decl_annotations_
const kfe::KFEResourceHeap* kfe::KFERawBuffer::Impl::GetResourceHeap() const noexcept
{
    return m_pResourceHeap;
}

_Use_decl_annotations_
std::uint64_t kfe::KFERawBuffer::Impl::GetOffsetInBytes() const noexcept
{
    return m_offsetInBytes;
}

_Use_decl_annotations_
std::uint64_t kfe::KFERawBuffer::Impl::GetSizeInBytes() const noexcept
{
    return m_sizeInBytes;
}

_Use_decl_annotations_
std::uint32_t kfe::KFERawBuffer::Impl::CreateSRV(_In_ const KFE_RAW_SRV_DESC& desc)
{
    if (!IsInitialized())
    {
        LOG_ERROR("KFERawBuffer::Impl::CreateSRV: Raw buffer not initialized.");
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    std::uint64_t byteOffset = 0u;
    std::uint64_t byteSize   = 0u;

    if (!ValidateSRVRange(desc, byteOffset, byteSize))
    {
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    // RAW SRV requires 4 bytes alignment and size multiple of 4
    if ((byteOffset % 4u) != 0u || (byteSize % 4u) != 0u)
    {
        LOG_ERROR(
            "KFERawBuffer::Impl::CreateSRV: RAW SRV range must be 4-byte aligned and multiple of 4. "
            "ByteOffset={}, ByteSize={}.",
            byteOffset, byteSize);
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    const std::uint32_t descriptorIndex = m_pResourceHeap->Allocate();
    if (descriptorIndex == KFE_INVALID_DESCRIPTOR_INDEX)
    {
        LOG_ERROR("KFERawBuffer::Impl::CreateSRV: Failed to allocate descriptor from ResourceHeap.");
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    const KFE_CPU_DESCRIPTOR_HANDLE temp = m_pResourceHeap->GetHandle(descriptorIndex);
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};
    handle.ptr = temp.ptr;

    ID3D12Resource* resource = m_pResourceBuffer->GetNative();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format                  = DXGI_FORMAT_R32_TYPELESS;
    srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.Flags            = D3D12_BUFFER_SRV_FLAG_RAW;

    const UINT firstElement = static_cast<UINT>(byteOffset / 4u);
    const UINT numElements  = static_cast<UINT>(byteSize / 4u);

    srvDesc.Buffer.FirstElement         = firstElement;
    srvDesc.Buffer.NumElements          = numElements;
    srvDesc.Buffer.StructureByteStride  = 0u;

    m_pDevice->GetNative()->CreateShaderResourceView(
        resource,
        &srvDesc,
        handle);

    m_srvDescriptorIndices.push_back(descriptorIndex);

    LOG_SUCCESS(
        "KFERawBuffer::Impl::CreateSRV: Created RAW SRV. ViewOffset={}, ViewSize={}, FirstElement={}, NumElements={}, DescriptorIndex={}.",
        byteOffset, byteSize, firstElement, numElements, descriptorIndex);

    return descriptorIndex;
}

_Use_decl_annotations_
std::uint32_t kfe::KFERawBuffer::Impl::CreateUAV(_In_ const KFE_RAW_UAV_DESC& desc)
{
    if (!IsInitialized())
    {
        LOG_ERROR("KFERawBuffer::Impl::CreateUAV: Raw buffer not initialized.");
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    std::uint64_t byteOffset = 0u;
    std::uint64_t byteSize   = 0u;

    if (!ValidateUAVRange(desc, byteOffset, byteSize))
    {
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    // RAW UAV requires 4 byte alignments and size multiple of 4
    if ((byteOffset % 4u) != 0u || (byteSize % 4u) != 0u)
    {
        LOG_ERROR(
            "KFERawBuffer::Impl::CreateUAV: RAW UAV range must be 4-byte aligned and multiple of 4. "
            "ByteOffset={}, ByteSize={}.",
            byteOffset, byteSize);
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    const std::uint32_t descriptorIndex = m_pResourceHeap->Allocate();
    if (descriptorIndex == KFE_INVALID_DESCRIPTOR_INDEX)
    {
        LOG_ERROR("KFERawBuffer::Impl::CreateUAV: Failed to allocate descriptor from ResourceHeap.");
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    KFE_CPU_DESCRIPTOR_HANDLE temp = m_pResourceHeap->GetHandle(descriptorIndex);
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};
    handle.ptr = temp.ptr;

    ID3D12Resource* resource = m_pResourceBuffer->GetNative();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format          = DXGI_FORMAT_R32_TYPELESS;
    uavDesc.ViewDimension   = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.Flags    = D3D12_BUFFER_UAV_FLAG_RAW;

    const UINT firstElement = static_cast<UINT>(byteOffset / 4u);
    const UINT numElements  = static_cast<UINT>(byteSize / 4u);

    uavDesc.Buffer.FirstElement         = firstElement;
    uavDesc.Buffer.NumElements          = numElements;
    uavDesc.Buffer.StructureByteStride  = 0u;
    uavDesc.Buffer.CounterOffsetInBytes = 0u;

    if (desc.HasCounter)
    {
        LOG_WARNING(
            "KFERawBuffer::Impl::CreateUAV: HasCounter=true but no separate counter resource is bound. "
            "CounterOffsetInBytes ({}) is ignored by CreateUnorderedAccessView.",
            desc.CounterOffsetInBytes);
    }

    m_pDevice->GetNative()->CreateUnorderedAccessView(
        resource,
        nullptr,
        &uavDesc,
        handle);

    m_uavDescriptorIndices.push_back(descriptorIndex);

    LOG_SUCCESS(
        "KFERawBuffer::Impl::CreateUAV: Created RAW UAV. ViewOffset={}, ViewSize={}, FirstElement={}, NumElements={}, DescriptorIndex={}.",
        byteOffset, byteSize, firstElement, numElements, descriptorIndex);

    return descriptorIndex;
}

_Use_decl_annotations_
std::uint32_t kfe::KFERawBuffer::Impl::GetNumSRVs() const noexcept
{
    return static_cast<std::uint32_t>(m_srvDescriptorIndices.size());
}

_Use_decl_annotations_
std::uint32_t kfe::KFERawBuffer::Impl::GetNumUAVs() const noexcept
{
    return static_cast<std::uint32_t>(m_uavDescriptorIndices.size());
}

_Use_decl_annotations_
std::uint32_t kfe::KFERawBuffer::Impl::GetSRVDescriptorIndex(std::uint32_t viewIndex) const noexcept
{
    if (viewIndex >= m_srvDescriptorIndices.size())
    {
        LOG_ERROR(
            "KFERawBuffer::Impl::GetSRVDescriptorIndex: viewIndex {} is out of range [0, {}).",
            viewIndex, m_srvDescriptorIndices.size());
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    return m_srvDescriptorIndices[viewIndex];
}

_Use_decl_annotations_
std::uint32_t kfe::KFERawBuffer::Impl::GetUAVDescriptorIndex(std::uint32_t viewIndex) const noexcept
{
    if (viewIndex >= m_uavDescriptorIndices.size())
    {
        LOG_ERROR(
            "KFERawBuffer::Impl::GetUAVDescriptorIndex: viewIndex {} is out of range [0, {}).",
            viewIndex, m_uavDescriptorIndices.size());
        return KFE_INVALID_DESCRIPTOR_INDEX;
    }

    return m_uavDescriptorIndices[viewIndex];
}

_Use_decl_annotations_
bool kfe::KFERawBuffer::Impl::ValidateSRVRange(
    _In_ const KFE_RAW_SRV_DESC& desc,
    _Out_ std::uint64_t& byteOffset,
    _Out_ std::uint64_t& byteSize) const noexcept
{
    byteOffset = desc.FirstByteOffset;
    if (byteOffset >= m_sizeInBytes)
    {
        LOG_ERROR(
            "KFERawBuffer::Impl::ValidateSRVRange: FirstByteOffset ({}) is >= raw buffer size ({}).",
            byteOffset, m_sizeInBytes);
        return false;
    }

    if (desc.NumBytes == 0u)
    {
        byteSize = m_sizeInBytes - byteOffset;
    }
    else
    {
        byteSize = desc.NumBytes;
        if (byteOffset + byteSize > m_sizeInBytes)
        {
            LOG_ERROR(
                "KFERawBuffer::Impl::ValidateSRVRange: Range [offset={}, offset+size={}] exceeds raw buffer size ({}).",
                byteOffset,
                byteOffset + byteSize,
                m_sizeInBytes);
            return false;
        }
    }

    byteOffset += m_offsetInBytes;
    return true;
}

_Use_decl_annotations_
bool kfe::KFERawBuffer::Impl::ValidateUAVRange(
    _In_ const KFE_RAW_UAV_DESC& desc,
    _Out_ std::uint64_t& byteOffset,
    _Out_ std::uint64_t& byteSize) const noexcept
{
    byteOffset = desc.FirstByteOffset;
    if (byteOffset >= m_sizeInBytes)
    {
        LOG_ERROR(
            "KFERawBuffer::Impl::ValidateUAVRange: FirstByteOffset ({}) is >= raw buffer size ({}).",
            byteOffset, m_sizeInBytes);
        return false;
    }

    if (desc.NumBytes == 0u)
    {
        byteSize = m_sizeInBytes - byteOffset;
    }
    else
    {
        byteSize = desc.NumBytes;
        if (byteOffset + byteSize > m_sizeInBytes)
        {
            LOG_ERROR(
                "KFERawBuffer::Impl::ValidateUAVRange: Range [offset={}, offset+size={}] exceeds raw buffer size ({}).",
                byteOffset,
                byteOffset + byteSize,
                m_sizeInBytes);
            return false;
        }
    }

    byteOffset += m_offsetInBytes;
    return true;
}

#pragma endregion
