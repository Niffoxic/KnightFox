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
#include "engine/render_manager/api/texture/texture_dsv.h"
#include "engine/render_manager/api/texture/texture.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/heap/heap_dsv.h"

#include "engine/utils/logger.h"
#include "engine/system/common_types.h"

#include <d3d12.h>

#pragma region Impl_Declaration

class kfe::KFETextureDSV::Impl
{
public:
     Impl() = default;
    ~Impl()
    {
        if (!Destroy())
        {
            LOG_ERROR("Failed to Destroy DSV!");
        }
    }

    NODISCARD bool Initialize(_In_ const KFE_DSV_CREATE_DESC& desc);

    NODISCARD bool Destroy     ()       noexcept;
    NODISCARD bool IsInitialize() const noexcept;

    NODISCARD KFEDSVHeap* GetAttachedHeap() const noexcept;
    NODISCARD KFETexture* GetTexture     () const noexcept;

    NODISCARD std::uint32_t GetDescriptorIndex() const noexcept;
    NODISCARD bool          HasValidDescriptor() const noexcept;

    NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept;

    NODISCARD DXGI_FORMAT         GetFormat       () const noexcept;
    NODISCARD D3D12_DSV_DIMENSION GetViewDimension() const noexcept;
    NODISCARD D3D12_DSV_FLAGS     GetFlags        () const noexcept;

    NODISCARD std::uint32_t GetMipSlice       () const noexcept;
    NODISCARD std::uint32_t GetFirstArraySlice() const noexcept;
    NODISCARD std::uint32_t GetArraySize      () const noexcept;

private:
    void CacheAndValidateInput(_In_ const KFE_DSV_CREATE_DESC& desc);
    void ResetState();

private:
    bool        m_bInitialized{ false };
    KFEDevice*  m_pDevice     { nullptr };
    KFEDSVHeap* m_pHeap       { nullptr };
    KFETexture* m_pTexture    { nullptr };

    DXGI_FORMAT         m_format        { DXGI_FORMAT_UNKNOWN };
    D3D12_DSV_DIMENSION m_viewDimension { D3D12_DSV_DIMENSION_UNKNOWN };
    D3D12_DSV_FLAGS     m_flags         { D3D12_DSV_FLAG_NONE };

    std::uint32_t m_mipSlice        { 0u };
    std::uint32_t m_firstArraySlice { 0u };
    std::uint32_t m_arraySize       { 0u };
    std::uint32_t m_descriptorIndex { KFE_INVALID_INDEX };
};

#pragma endregion

#pragma region DSV_Implementation

kfe::KFETextureDSV::KFETextureDSV() noexcept
	: m_impl(std::make_unique<kfe::KFETextureDSV::Impl>())
{
}

kfe::KFETextureDSV::~KFETextureDSV() noexcept = default;

kfe::KFETextureDSV::KFETextureDSV(KFETextureDSV&&) noexcept = default;
kfe::KFETextureDSV& kfe::KFETextureDSV::operator=(KFETextureDSV&&) noexcept = default;

_Use_decl_annotations_
bool kfe::KFETextureDSV::Initialize(const KFE_DSV_CREATE_DESC& desc)
{
	return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFETextureDSV::Destroy() noexcept
{
	return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFETextureDSV::IsInitialize() const noexcept
{
	return m_impl->IsInitialize();
}

_Use_decl_annotations_
kfe::KFEDSVHeap* kfe::KFETextureDSV::GetAttachedHeap() const noexcept
{
	return m_impl->GetAttachedHeap();
}

_Use_decl_annotations_
kfe::KFETexture* kfe::KFETextureDSV::GetTexture() const noexcept
{
	return m_impl->GetTexture();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureDSV::GetDescriptorIndex() const noexcept
{
	return m_impl->GetDescriptorIndex();
}

_Use_decl_annotations_
bool kfe::KFETextureDSV::HasValidDescriptor() const noexcept
{
	return m_impl->HasValidDescriptor();
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFETextureDSV::GetCPUHandle() const noexcept
{
	return m_impl->GetCPUHandle();
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFETextureDSV::GetFormat() const noexcept
{
	return m_impl->GetFormat();
}

_Use_decl_annotations_
D3D12_DSV_DIMENSION kfe::KFETextureDSV::GetViewDimension() const noexcept
{
	return m_impl->GetViewDimension();
}

_Use_decl_annotations_
D3D12_DSV_FLAGS kfe::KFETextureDSV::GetFlags() const noexcept
{
	return m_impl->GetFlags();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureDSV::GetMipSlice() const noexcept
{
	return m_impl->GetMipSlice();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureDSV::GetFirstArraySlice() const noexcept
{
	return m_impl->GetFirstArraySlice();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureDSV::GetArraySize() const noexcept
{
	return m_impl->GetArraySize();
}

_Use_decl_annotations_
std::string kfe::KFETextureDSV::GetName() const noexcept
{
	return "KFETextureDSV";
}

_Use_decl_annotations_
std::string kfe::KFETextureDSV::GetDescription() const noexcept
{
	return "KFETextureDSV: D3D12 Depth Stencil View wrapper";
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFETextureDSV::Impl::Initialize(_In_ const KFE_DSV_CREATE_DESC& desc)
{
	if (m_bInitialized)
	{
		LOG_WARNING("KFETextureDSV::Impl::Initialize: Already initialized. Destroying previous state.");
		if (!Destroy())
		{
			LOG_ERROR("KFETextureDSV::Impl::Initialize: Failed to destroy previous state.");
			return false;
		}
	}

	ResetState();
	CacheAndValidateInput(desc);

	if (!m_bInitialized)
	{
		return false;
	}

	if (m_pDevice == nullptr || m_pTexture == nullptr || m_pHeap == nullptr)
	{
		LOG_ERROR("KFETextureDSV::Impl::Initialize: Cached pointers are invalid.");
		ResetState();
		return false;
	}

	auto* nativeDevice = m_pDevice->GetNative();
	if (nativeDevice == nullptr)
	{
		LOG_ERROR("KFETextureDSV::Impl::Initialize: Native device is null.");
		ResetState();
		return false;
	}

	if (!HasValidDescriptor())
	{
		LOG_ERROR("KFETextureDSV::Impl::Initialize: Invalid descriptor index after validation.");
		ResetState();
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};
	dsv.Format		  = m_format;
	dsv.ViewDimension = m_viewDimension;
	dsv.Flags		  = m_flags;

	switch (m_viewDimension)
	{
	case D3D12_DSV_DIMENSION_TEXTURE1D:
		dsv.Texture1D.MipSlice = m_mipSlice;
		break;

	case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
		dsv.Texture1DArray.MipSlice			= m_mipSlice;
		dsv.Texture1DArray.FirstArraySlice	= m_firstArraySlice;
		dsv.Texture1DArray.ArraySize		= m_arraySize;
		break;

	case D3D12_DSV_DIMENSION_TEXTURE2D:
		dsv.Texture2D.MipSlice = m_mipSlice;
		break;

	case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
		dsv.Texture2DArray.MipSlice			= m_mipSlice;
		dsv.Texture2DArray.FirstArraySlice	= m_firstArraySlice;
		dsv.Texture2DArray.ArraySize		= m_arraySize;
		break;

	case D3D12_DSV_DIMENSION_TEXTURE2DMS:
		break;

	case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
		dsv.Texture2DMSArray.FirstArraySlice = m_firstArraySlice;
		dsv.Texture2DMSArray.ArraySize		 = m_arraySize;
		break;

	case D3D12_DSV_DIMENSION_UNKNOWN:
	default:
		LOG_ERROR("KFETextureDSV::Impl::Initialize: Unsupported or UNKNOWN DSV dimension.");
		ResetState();
		return false;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE handle = m_pHeap->GetHandle(m_descriptorIndex);

	nativeDevice->CreateDepthStencilView(
		m_pTexture->GetNative(),
		&dsv,
		handle
	);

	LOG_SUCCESS("KFETextureDSV::Impl::Initialize: DSV created. HeapIdx = {}, Format = {}, Dimension = {}",
		m_descriptorIndex,
		static_cast<int>(m_format),
		static_cast<int>(m_viewDimension));

	return true;
}

_Use_decl_annotations_
bool kfe::KFETextureDSV::Impl::Destroy() noexcept
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
			LOG_WARNING("KFETextureDSV::Impl::Destroy: Failed to free descriptor index {}.", m_descriptorIndex);
		}
	}

	ResetState();

	LOG_SUCCESS("KFETextureDSV::Impl::Destroy: DSV destroyed and state reset.");
	return true;
}

_Use_decl_annotations_
bool kfe::KFETextureDSV::Impl::IsInitialize() const noexcept
{
	return m_bInitialized;
}

_Use_decl_annotations_
kfe::KFEDSVHeap* kfe::KFETextureDSV::Impl::GetAttachedHeap() const noexcept
{
	return m_pHeap;
}

_Use_decl_annotations_
kfe::KFETexture* kfe::KFETextureDSV::Impl::GetTexture() const noexcept
{
	return m_pTexture;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureDSV::Impl::GetDescriptorIndex() const noexcept
{
	return m_descriptorIndex;
}

_Use_decl_annotations_
bool kfe::KFETextureDSV::Impl::HasValidDescriptor() const noexcept
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
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFETextureDSV::Impl::GetCPUHandle() const noexcept
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
DXGI_FORMAT kfe::KFETextureDSV::Impl::GetFormat() const noexcept
{
	return m_format;
}

_Use_decl_annotations_
D3D12_DSV_DIMENSION kfe::KFETextureDSV::Impl::GetViewDimension() const noexcept
{
	return m_viewDimension;
}

_Use_decl_annotations_
D3D12_DSV_FLAGS kfe::KFETextureDSV::Impl::GetFlags() const noexcept
{
	return m_flags;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureDSV::Impl::GetMipSlice() const noexcept
{
	return m_mipSlice;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureDSV::Impl::GetFirstArraySlice() const noexcept
{
	return m_firstArraySlice;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureDSV::Impl::GetArraySize() const noexcept
{
	return m_arraySize;
}

_Use_decl_annotations_
void kfe::KFETextureDSV::Impl::CacheAndValidateInput(const KFE_DSV_CREATE_DESC& desc)
{
	ResetState();

	if (desc.Device == nullptr)
	{
		LOG_ERROR("KFETextureDSV::Impl::Initialize: Device is null.");
		return;
	}

	if (desc.Heap == nullptr)
	{
		LOG_ERROR("KFETextureDSV::Impl::Initialize: DSV heap is null.");
		return;
	}

	if (desc.Texture == nullptr)
	{
		LOG_ERROR("KFETextureDSV::Impl::Initialize: Texture is null.");
		return;
	}

	m_pDevice	= desc.Device;
	m_pHeap		= desc.Heap;
	m_pTexture	= desc.Texture;

	m_format = (desc.Format != DXGI_FORMAT_UNKNOWN)
		? desc.Format
		: m_pTexture->GetFormat();

	m_viewDimension = desc.ViewDimension;
	m_flags			= desc.Flags;

	m_mipSlice			= desc.MipSlice;
	m_firstArraySlice	= desc.FirstArraySlice;
	m_arraySize			= desc.ArraySize;

	std::uint32_t index = desc.DescriptorIndex;

	if (index == KFE_INVALID_INDEX)
	{
		index = m_pHeap->Allocate();
		if (index == KFE_INVALID_INDEX)
		{
			LOG_ERROR("KFETextureDSV::Impl::Initialize: Failed to allocate descriptor from DSV heap.");
			ResetState();
			return;
		}
	}
	else
	{
		if (!m_pHeap->IsValidIndex(index))
		{
			LOG_ERROR("KFETextureDSV::Impl::Initialize: Provided descriptor index {} is invalid for this heap.", index);
			ResetState();
			return;
		}
	}

	m_descriptorIndex = index;
	m_bInitialized	  = true;

	LOG_SUCCESS("KFETextureDSV::Impl::Initialize: DSV cached. HeapIdx = {}, Format = {}, Dimension = {}",
		m_descriptorIndex,
		static_cast<int>(m_format),
		static_cast<int>(m_viewDimension));
}

void kfe::KFETextureDSV::Impl::ResetState()
{
	m_pDevice	= nullptr;
	m_pHeap		= nullptr;
	m_pTexture	= nullptr;

	m_format		= DXGI_FORMAT_UNKNOWN;
	m_viewDimension = D3D12_DSV_DIMENSION_UNKNOWN;
	m_flags			= D3D12_DSV_FLAG_NONE;

	m_mipSlice			= 0u;
	m_firstArraySlice	= 0u;
	m_arraySize			= 0u;

	m_descriptorIndex = KFE_INVALID_INDEX;
	m_bInitialized	  = false;
}

#pragma endregion
