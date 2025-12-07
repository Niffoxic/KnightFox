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

#include "engine/render_manager/api/buffer/index_buffer.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"

#include <d3d12.h>
#include <cstring>

#pragma region Impl_Declaration

class kfe::KFEIndexBuffer::Impl 
{
public:
	 Impl() = default;
	~Impl() = default;

	NODISCARD bool Initialize(_In_ const KFE_INDEX_BUFFER_CREATE_DESC& desc);

	NODISCARD bool Destroy		() noexcept;
	NODISCARD bool IsInitialized() const noexcept;

	NODISCARD KFEBuffer*			  GetBuffer()       noexcept;
	NODISCARD const KFEBuffer*		  GetBuffer() const noexcept;
	NODISCARD D3D12_INDEX_BUFFER_VIEW GetView () const noexcept;

	NODISCARD DXGI_FORMAT   GetFormat		 () const noexcept;
	NODISCARD std::uint64_t GetOffsetInBytes () const noexcept;
	NODISCARD std::uint32_t GetIndexCount	 () const noexcept;
	NODISCARD std::uint32_t GetIndexSizeBytes() const noexcept;

private:
    NODISCARD std::uint32_t GetIndexSizeBytes(const DXGI_FORMAT format) const noexcept;

private:
	D3D12_INDEX_BUFFER_VIEW m_indexView{};
	KFEBuffer*				m_pResourceBuffer{ nullptr };
	KFEDevice*				m_pDevice	     { nullptr };
	DXGI_FORMAT				m_format		 { DXGI_FORMAT_UNKNOWN };
	std::uint64_t			m_offsetInBytes  { 0u };
	std::uint32_t			m_indexCount	 { 0u };

	bool m_bInitialized{ false };
};

#pragma endregion

#pragma region Class_Implementation

kfe::KFEIndexBuffer::KFEIndexBuffer() noexcept
	: m_impl(std::make_unique<kfe::KFEIndexBuffer::Impl>())
{}

kfe::KFEIndexBuffer::~KFEIndexBuffer() noexcept = default;

kfe::KFEIndexBuffer::KFEIndexBuffer				   (KFEIndexBuffer&& other) noexcept = default;
kfe::KFEIndexBuffer& kfe::KFEIndexBuffer::operator=(KFEIndexBuffer&& other) noexcept = default;

_Use_decl_annotations_
std::string kfe::KFEIndexBuffer::GetName() const noexcept
{
    return "KFEIndexBuffer";
}

std::string kfe::KFEIndexBuffer::GetDescription() const noexcept
{
	return "Index Buffer View: Wraps ID3D12Resource for indexed drawing.";
}

_Use_decl_annotations_
bool kfe::KFEIndexBuffer::Initialize(const KFE_INDEX_BUFFER_CREATE_DESC& desc)
{
	return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFEIndexBuffer::Destroy() noexcept
{
	return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFEIndexBuffer::IsInitialized() const noexcept
{
	return m_impl->IsInitialized();
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEIndexBuffer::GetBuffer() noexcept
{
	return m_impl->GetBuffer();
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEIndexBuffer::GetBuffer() const noexcept
{
	return m_impl->GetBuffer();
}

_Use_decl_annotations_
D3D12_INDEX_BUFFER_VIEW kfe::KFEIndexBuffer::GetView() const noexcept
{
	return m_impl->GetView();
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFEIndexBuffer::GetFormat() const noexcept
{
	return m_impl->GetFormat();
}

_Use_decl_annotations_
std::uint64_t kfe::KFEIndexBuffer::GetOffsetInBytes() const noexcept
{
	return m_impl->GetOffsetInBytes();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEIndexBuffer::GetIndexCount() const noexcept
{
	return m_impl->GetIndexCount();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEIndexBuffer::GetIndexSizeBytes() const noexcept
{
	return m_impl->GetIndexSizeBytes();
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFEIndexBuffer::Impl::Initialize(const KFE_INDEX_BUFFER_CREATE_DESC& desc)
{
    // Validate device
    if (!desc.Device || !desc.Device->GetNative())
    {
        LOG_ERROR("KFEIndexBuffer::Impl::Initialize: Device or native device is null.");
        return false;
    }

    // Validate underlying buffer
    if (!desc.ResourceBuffer || !desc.ResourceBuffer->GetNative())
    {
        LOG_ERROR("KFEIndexBuffer::Impl::Initialize: ResourceBuffer or its native resource is null.");
        return false;
    }

    if (desc.Format == DXGI_FORMAT_UNKNOWN)
    {
        LOG_ERROR("KFEIndexBuffer::Impl::Initialize: DXGI_FORMAT must not be DXGI_FORMAT_UNKNOWN.");
        return false;
    }

    // Determine index size from format
    const std::uint32_t indexSize = GetIndexSizeBytes(desc.Format);
    if (indexSize == 0u)
    {
        LOG_ERROR("KFEIndexBuffer::Impl::Initialize: Unsupported index format: {}", static_cast<int>(desc.Format));
        return false;
    }

    // If already initialized and destroy and re initialize
    if (IsInitialized())
    {
        LOG_WARNING("KFEIndexBuffer::Impl::Initialize: Already initialized. Destroying and recreating.");
        if (!Destroy())
        {
            LOG_ERROR("KFEIndexBuffer::Impl::Initialize: Destroy failed; cannot reinitialize.");
            return false;
        }
    }

    m_pDevice           = desc.Device;
    m_pResourceBuffer   = desc.ResourceBuffer;
    m_offsetInBytes     = desc.OffsetInBytes;
    m_format            = desc.Format;

    // Get resource info
    ID3D12Resource* resource    = m_pResourceBuffer->GetNative();
    const auto      gpuVA       = resource->GetGPUVirtualAddress();
    const auto      bufferSize  = m_pResourceBuffer->GetSizeInBytes();

    if (m_offsetInBytes >= bufferSize)
    {
        LOG_ERROR(
            "KFEIndexBuffer::Impl::Initialize: OffsetInBytes ({}) is >= buffer size ({}).",
            m_offsetInBytes, bufferSize);
        return false;
    }

    const std::uint64_t usableSize = bufferSize - m_offsetInBytes;
    if (usableSize < indexSize)
    {
        LOG_ERROR(
            "KFEIndexBuffer::Impl::Initialize: Usable size ({}) is smaller than a single index ({} bytes).",
            usableSize, indexSize);
        return false;
    }

    // Setup view
    m_indexView.BufferLocation  = gpuVA + m_offsetInBytes;
    m_indexView.SizeInBytes     = static_cast<UINT>(usableSize);
    m_indexView.Format          = m_format;
    m_indexCount                = static_cast<std::uint32_t>(usableSize / indexSize);

    m_bInitialized = true;
    return true;
}

_Use_decl_annotations_
bool kfe::KFEIndexBuffer::Impl::Destroy() noexcept
{
    m_indexView.BufferLocation  = 0u;
    m_indexView.SizeInBytes     = 0u;
    m_indexView.Format          = DXGI_FORMAT_UNKNOWN;
    m_pResourceBuffer           = nullptr;
    m_pDevice                   = nullptr;
    m_format                    = DXGI_FORMAT_UNKNOWN;
    m_offsetInBytes             = 0u;
    m_indexCount                = 0u;
    m_bInitialized              = false;
    return true;
}

_Use_decl_annotations_
bool kfe::KFEIndexBuffer::Impl::IsInitialized() const noexcept
{
    return m_bInitialized;
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEIndexBuffer::Impl::GetBuffer() noexcept
{
    return m_pResourceBuffer;
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEIndexBuffer::Impl::GetBuffer() const noexcept
{
    return m_pResourceBuffer;
}

_Use_decl_annotations_
D3D12_INDEX_BUFFER_VIEW kfe::KFEIndexBuffer::Impl::GetView() const noexcept
{
    return m_indexView;
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFEIndexBuffer::Impl::GetFormat() const noexcept
{
    return m_format;
}

_Use_decl_annotations_
std::uint64_t kfe::KFEIndexBuffer::Impl::GetOffsetInBytes() const noexcept
{
    return m_offsetInBytes;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEIndexBuffer::Impl::GetIndexCount() const noexcept
{
    return m_indexCount;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEIndexBuffer::Impl::GetIndexSizeBytes() const noexcept
{
    return GetIndexSizeBytes(m_format);
}

std::uint32_t kfe::KFEIndexBuffer::Impl::GetIndexSizeBytes(const DXGI_FORMAT format) const noexcept
{
    switch (format)
    {
    case DXGI_FORMAT_R16_UINT: return 2u;
    case DXGI_FORMAT_R32_UINT: return 4u;
    default:                   return 0u;
    }
}

#pragma endregion
