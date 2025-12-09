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

#include "engine/render_manager/api/buffer/constant_buffer.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"

#include <d3d12.h>
#include <limits>
#include <cstring>

#pragma region Impl_Declaration

class kfe::KFEConstantBuffer::Impl
{
public:
    Impl() = default;
    ~Impl()
    {
        if (!Destroy())
        {
            LOG_ERROR("KFEConstantBuffer::Impl::~Impl: Failed to destroy constant buffer cleanly.");
        }
    }

    NODISCARD bool Initialize(_In_ const KFE_CONSTANT_BUFFER_CREATE_DESC& desc);

    NODISCARD bool Destroy      () noexcept;
    NODISCARD bool IsInitialized() const noexcept;

    NODISCARD       KFEBuffer* GetBuffer()       noexcept;
    NODISCARD const KFEBuffer* GetBuffer() const noexcept;

    NODISCARD       KFEResourceHeap* GetResourceHeap()       noexcept;
    NODISCARD const KFEResourceHeap* GetResourceHeap() const noexcept;

    NODISCARD std::uint32_t GetSizeInBytes       () const noexcept;
    NODISCARD std::uint32_t GetAlignedSizeInBytes() const noexcept;
    NODISCARD std::uint64_t GetOffsetInBytes     () const noexcept;

    NODISCARD std::uint32_t GetCBVDescriptorIndex() const noexcept;
    NODISCARD std::uint64_t GetGPUVirtualAddress () const noexcept;

    NODISCARD       void* GetMappedData()       noexcept;
    NODISCARD const void* GetMappedData() const noexcept;

private:
    KFEDevice*       m_pDevice        { nullptr };
    KFEBuffer*       m_pResourceBuffer{ nullptr };
    KFEResourceHeap* m_pResourceHeap  { nullptr };

    std::uint32_t     m_sizeInBytes       { 0u };
    std::uint32_t     m_alignedSizeInBytes{ 0u };
    std::uint64_t     m_offsetInBytes     { 0u };

    std::uint32_t     m_cbvDescriptorIndex{ KFE_INVALID_INDEX };

    void* m_mappedBase{ nullptr };
    bool  m_bInitialized{ false };
};
#pragma endregion

#pragma region Class_Implementation

kfe::KFEConstantBuffer::KFEConstantBuffer() noexcept
    : m_impl(std::make_unique<kfe::KFEConstantBuffer::Impl>())
{}

kfe::KFEConstantBuffer::~KFEConstantBuffer() noexcept = default;

kfe::KFEConstantBuffer::KFEConstantBuffer(KFEConstantBuffer&& other) noexcept = default;
kfe::KFEConstantBuffer& kfe::KFEConstantBuffer::operator=(KFEConstantBuffer&& other) noexcept = default;

_Use_decl_annotations_
std::string kfe::KFEConstantBuffer::GetName() const noexcept
{
    return "KFEConstantBuffer";
}

_Use_decl_annotations_
std::string kfe::KFEConstantBuffer::GetDescription() const noexcept
{
    return "Constant Buffer View: Wraps KFEBuffer + CBV descriptor in a CBV/SRV/UAV heap.";
}

_Use_decl_annotations_
bool kfe::KFEConstantBuffer::Initialize(const KFE_CONSTANT_BUFFER_CREATE_DESC& desc)
{
    return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFEConstantBuffer::Destroy() noexcept
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFEConstantBuffer::IsInitialized() const noexcept
{
    return m_impl->IsInitialized();
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEConstantBuffer::GetBuffer() noexcept
{
    return m_impl->GetBuffer();
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEConstantBuffer::GetBuffer() const noexcept
{
    return m_impl->GetBuffer();
}

_Use_decl_annotations_
kfe::KFEResourceHeap* kfe::KFEConstantBuffer::GetResourceHeap() noexcept
{
    return m_impl->GetResourceHeap();
}

_Use_decl_annotations_
const kfe::KFEResourceHeap* kfe::KFEConstantBuffer::GetResourceHeap() const noexcept
{
    return m_impl->GetResourceHeap();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEConstantBuffer::GetSizeInBytes() const noexcept
{
    return m_impl->GetSizeInBytes();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEConstantBuffer::GetAlignedSizeInBytes() const noexcept
{
    return m_impl->GetAlignedSizeInBytes();
}

_Use_decl_annotations_
std::uint64_t kfe::KFEConstantBuffer::GetOffsetInBytes() const noexcept
{
    return m_impl->GetOffsetInBytes();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEConstantBuffer::GetCBVDescriptorIndex() const noexcept
{
    return m_impl->GetCBVDescriptorIndex();
}

_Use_decl_annotations_
std::uint64_t kfe::KFEConstantBuffer::GetGPUVirtualAddress() const noexcept
{
    return m_impl->GetGPUVirtualAddress();
}

_Use_decl_annotations_
void* kfe::KFEConstantBuffer::GetMappedData() noexcept
{
    return m_impl->GetMappedData();
}

_Use_decl_annotations_
const void* kfe::KFEConstantBuffer::GetMappedData() const noexcept
{
    return m_impl->GetMappedData();
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFEConstantBuffer::Impl::Initialize(const KFE_CONSTANT_BUFFER_CREATE_DESC& desc)
{
    // Validate device
    if (!desc.Device || !desc.Device->GetNative())
    {
        LOG_ERROR("KFEConstantBuffer::Impl::Initialize: Device or native device is null.");
        return false;
    }

    // Validate buffer
    if (!desc.ResourceBuffer || !desc.ResourceBuffer->GetNative())
    {
        LOG_ERROR("KFEConstantBuffer::Impl::Initialize: ResourceBuffer or its native resource is null.");
        return false;
    }

    // Validate heap
    if (!desc.ResourceHeap)
    {
        LOG_ERROR("KFEConstantBuffer::Impl::Initialize: ResourceHeap is null.");
        return false;
    }

    if (desc.SizeInBytes == 0u)
    {
        LOG_ERROR("KFEConstantBuffer::Impl::Initialize: SizeInBytes must be > 0.");
        return false;
    }

    // If already initialized and recreate
    if (IsInitialized())
    {
        LOG_WARNING("KFEConstantBuffer::Impl::Initialize: Already initialized. Destroying and recreating.");
        if (!Destroy())
        {
            LOG_ERROR("KFEConstantBuffer::Impl::Initialize: Destroy failed; cannot reinitialize.");
            return false;
        }
    }

    m_pDevice            = desc.Device;
    m_pResourceBuffer    = desc.ResourceBuffer;
    m_pResourceHeap      = desc.ResourceHeap;
    m_sizeInBytes        = desc.SizeInBytes;
    m_alignedSizeInBytes = kfe_helpers::AlignTo256(desc.SizeInBytes);
    m_offsetInBytes      = desc.OffsetInBytes;

    const std::uint64_t bufferSize = m_pResourceBuffer->GetSizeInBytes();

    // Validate offset range
    if (m_offsetInBytes >= bufferSize)
    {
        LOG_ERROR(
            "KFEConstantBuffer::Impl::Initialize: OffsetInBytes ({}) is >= buffer size ({}).",
            m_offsetInBytes, bufferSize);
        return false;
    }

    if (m_offsetInBytes + m_alignedSizeInBytes > bufferSize)
    {
        LOG_ERROR(
            "KFEConstantBuffer::Impl::Initialize: Range [offset={}, offset+alignedSize={}] exceeds "
            "buffer size ({}). RequestedSize={}, AlignedSize={}.",
            m_offsetInBytes,
            m_offsetInBytes + m_alignedSizeInBytes,
            bufferSize,
            m_sizeInBytes,
            m_alignedSizeInBytes);
        return false;
    }

    // Constant buffer GPU virtual address must be 256-byte aligned
    if ((m_offsetInBytes % 256u) != 0u)
    {
        LOG_ERROR(
            "KFEConstantBuffer::Impl::Initialize: OffsetInBytes ({}) is not 256-byte aligned. "
            "Constant buffers require 256-byte alignment.",
            m_offsetInBytes);
        return false;
    }

    // Allocate CBV descriptor from heap and create the CBV
    m_cbvDescriptorIndex = KFE_INVALID_INDEX;

    const std::uint32_t cbvIndex = m_pResourceHeap->Allocate();
    if (cbvIndex == KFE_INVALID_INDEX)
    {
        LOG_ERROR("KFEConstantBuffer::Impl::Initialize: Failed to allocate CBV descriptor from heap.");
        return false;
    }

    m_cbvDescriptorIndex = cbvIndex;

    const auto temp = m_pResourceHeap->GetHandle(cbvIndex);
    D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle{};
    cbvHandle.ptr = temp.ptr;

    ID3D12Resource* resource               = m_pResourceBuffer->GetNative();
    const D3D12_GPU_VIRTUAL_ADDRESS baseVA = resource->GetGPUVirtualAddress();

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
    cbvDesc.BufferLocation = baseVA + m_offsetInBytes;
    cbvDesc.SizeInBytes    = m_alignedSizeInBytes;

    m_pDevice->GetNative()->CreateConstantBufferView(&cbvDesc, cbvHandle);

    // Query mapped base pointer from KFEBuffer
    m_mappedBase = m_pResourceBuffer->GetMappedData();

    if (!m_mappedBase && m_pResourceBuffer->IsUploadHeap())
    {
        LOG_WARNING(
            "KFEConstantBuffer::Impl::Initialize: Underlying buffer is an UPLOAD heap, but "
            "GetMappedData() returned nullptr. CPU-side writes via GetMappedData() will not be possible.");
    }

    LOG_SUCCESS(
        "KFEConstantBuffer::Impl::Initialize: Initialized constant buffer. "
        "Size={}, AlignedSize={}, Offset={}, BufferSize={}, DescriptorIndex={}.",
        m_sizeInBytes,
        m_alignedSizeInBytes,
        m_offsetInBytes,
        bufferSize,
        m_cbvDescriptorIndex);

    m_bInitialized = true;
    return true;
}

_Use_decl_annotations_
bool kfe::KFEConstantBuffer::Impl::Destroy() noexcept
{
    // Free CBV descriptor back to the heap
    if (m_pResourceHeap && m_cbvDescriptorIndex != KFE_INVALID_INDEX)
    {
        if (!m_pResourceHeap->Free(m_cbvDescriptorIndex))
        {
            LOG_ERROR(
                "KFEConstantBuffer::Impl::Destroy: Failed to free CBV descriptor index {}.",
                m_cbvDescriptorIndex);
        }
    }

    m_cbvDescriptorIndex = KFE_INVALID_INDEX;

    m_pDevice           = nullptr;
    m_pResourceBuffer   = nullptr;
    m_pResourceHeap     = nullptr;

    m_sizeInBytes        = 0u;
    m_alignedSizeInBytes = 0u;
    m_offsetInBytes      = 0u;

    m_mappedBase = nullptr;
    m_bInitialized = false;

    return true;
}

_Use_decl_annotations_
bool kfe::KFEConstantBuffer::Impl::IsInitialized() const noexcept
{
    return m_bInitialized;
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEConstantBuffer::Impl::GetBuffer() noexcept
{
    return m_pResourceBuffer;
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEConstantBuffer::Impl::GetBuffer() const noexcept
{
    return m_pResourceBuffer;
}

_Use_decl_annotations_
kfe::KFEResourceHeap* kfe::KFEConstantBuffer::Impl::GetResourceHeap() noexcept
{
    return m_pResourceHeap;
}

_Use_decl_annotations_
const kfe::KFEResourceHeap* kfe::KFEConstantBuffer::Impl::GetResourceHeap() const noexcept
{
    return m_pResourceHeap;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEConstantBuffer::Impl::GetSizeInBytes() const noexcept
{
    return m_sizeInBytes;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEConstantBuffer::Impl::GetAlignedSizeInBytes() const noexcept
{
    return m_alignedSizeInBytes;
}

_Use_decl_annotations_
std::uint64_t kfe::KFEConstantBuffer::Impl::GetOffsetInBytes() const noexcept
{
    return m_offsetInBytes;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEConstantBuffer::Impl::GetCBVDescriptorIndex() const noexcept
{
    return m_cbvDescriptorIndex;
}

_Use_decl_annotations_
std::uint64_t kfe::KFEConstantBuffer::Impl::GetGPUVirtualAddress() const noexcept
{
    if (!m_pResourceBuffer || !m_pResourceBuffer->GetNative())
    {
        return 0ull;
    }

    return static_cast<std::uint64_t>(
        m_pResourceBuffer->GetNative()->GetGPUVirtualAddress() + m_offsetInBytes);
}

_Use_decl_annotations_
void* kfe::KFEConstantBuffer::Impl::GetMappedData() noexcept
{
    if (!m_mappedBase)
    {
        return nullptr;
    }

    auto* base = static_cast<std::uint8_t*>(m_mappedBase);
    return static_cast<void*>(base + m_offsetInBytes);
}

_Use_decl_annotations_
const void* kfe::KFEConstantBuffer::Impl::GetMappedData() const noexcept
{
    if (!m_mappedBase)
    {
        return nullptr;
    }

    auto* base = static_cast<const std::uint8_t*>(m_mappedBase);
    return static_cast<const void*>(base + m_offsetInBytes);
}

#pragma endregion
