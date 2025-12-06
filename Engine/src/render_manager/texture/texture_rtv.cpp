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
#include "engine/render_manager/texture/texture_rtv.h"
#include "engine/render_manager/heap/heap_rtv.h"
#include "engine/render_manager/components/device.h"
#include "engine/render_manager/texture/texture.h"

#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"

#include <d3d12.h>

#pragma region Impl_Declaration

class kfe::KFETextureRTV::Impl 
{
public:
	Impl() = default;
    ~Impl() 
    {
         if (!Destroy())
         {
             LOG_ERROR("KFETextureRTV::Impl::~Impl: Failed to destroy RTV cleanly.");
         }
    }

	NODISCARD bool Initialize(const KFE_RTV_CREATE_DESC& desc);
	
	NODISCARD bool Destroy	   ()		noexcept;
	NODISCARD bool IsInitialize() const noexcept;

	NODISCARD KFERTVHeap* GetAttachedHeap() const noexcept;
	NODISCARD KFETexture* GetTexture	 () const noexcept;

	NODISCARD std::uint32_t				  GetDescriptorIndex() const noexcept;
	NODISCARD bool						  HasValidDescriptor() const noexcept;
	NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle		() const noexcept;

	NODISCARD DXGI_FORMAT         GetFormat		  () const noexcept;
	NODISCARD D3D12_RTV_DIMENSION GetViewDimension() const noexcept;

	NODISCARD std::uint32_t GetMipSlice		  () const noexcept;
	NODISCARD std::uint32_t GetFirstArraySlice() const noexcept;
	NODISCARD std::uint32_t GetArraySize	  () const noexcept;
	NODISCARD std::uint32_t GetPlaneSlice	  () const noexcept;

private:
    NODISCARD bool CacheValidateInput(const KFE_RTV_CREATE_DESC& desc);
    void Reset();

private:
    bool        m_bInitialized{ false };
    KFEDevice*  m_pDevice     { nullptr };
    KFERTVHeap* m_pHeap       { nullptr };
    KFETexture* m_pTexture    { nullptr };
    DXGI_FORMAT m_format      { DXGI_FORMAT::DXGI_FORMAT_UNKNOWN };
    
    //~ configurations
    D3D12_RTV_DIMENSION m_viewDimension{};
    std::uint32_t       m_mipSlice       { 0u };
    std::uint32_t       m_firstArraySlice{ 0u };
    std::uint32_t       m_arraySize      { 0u };
    std::uint32_t       m_planeSlice     { 0u };
    std::uint32_t       m_descriptorIndex{ 0u };
};

#pragma endregion

#pragma region RTV_Implementation

kfe::KFETextureRTV::KFETextureRTV() noexcept
	: m_impl(std::make_unique<kfe::KFETextureRTV::Impl>())
{}

kfe::KFETextureRTV::~KFETextureRTV() noexcept = default;

kfe::KFETextureRTV::KFETextureRTV				 (KFETextureRTV&&) noexcept = default;
kfe::KFETextureRTV& kfe::KFETextureRTV::operator=(KFETextureRTV&&) noexcept = default;

_Use_decl_annotations_
bool kfe::KFETextureRTV::Initialize(const KFE_RTV_CREATE_DESC& desc)
{
	return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFETextureRTV::Destroy() noexcept 
{
	return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFETextureRTV::IsInitialize() const noexcept
{
	return m_impl->IsInitialize();
}

_Use_decl_annotations_
kfe::KFERTVHeap* kfe::KFETextureRTV::GetAttachedHeap() const noexcept
{
	return m_impl->GetAttachedHeap();
}

_Use_decl_annotations_
kfe::KFETexture* kfe::KFETextureRTV::GetTexture() const noexcept
{
	return m_impl->GetTexture();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureRTV::GetDescriptorIndex() const noexcept
{
	return m_impl->GetDescriptorIndex();
}

_Use_decl_annotations_
bool kfe::KFETextureRTV::HasValidDescriptor() const noexcept
{
	return m_impl->HasValidDescriptor();
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFETextureRTV::GetCPUHandle() const noexcept
{
	return m_impl->GetCPUHandle();
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFETextureRTV::GetFormat() const noexcept
{
	return m_impl->GetFormat();
}

_Use_decl_annotations_
D3D12_RTV_DIMENSION kfe::KFETextureRTV::GetViewDimension() const noexcept
{
	return m_impl->GetViewDimension();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureRTV::GetMipSlice() const noexcept
{
	return m_impl->GetMipSlice();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureRTV::GetFirstArraySlice() const noexcept
{
	return m_impl->GetFirstArraySlice();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureRTV::GetArraySize() const noexcept
{
	return m_impl->GetArraySize();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureRTV::GetPlaneSlice() const noexcept
{
	return m_impl->GetPlaneSlice();
}

std::string kfe::KFETextureRTV::GetName() const noexcept
{
	return "KFETextureRTV";
}

std::string kfe::KFETextureRTV::GetDescription() const noexcept
{
	return "KFETextureRTV: RTV D3D12";
}

#pragma endregion

#pragma region Impl_Implementation


_Use_decl_annotations_
bool kfe::KFETextureRTV::Impl::Initialize(const KFE_RTV_CREATE_DESC& desc)
{
	// Validate + cache everything + allocate descriptor
	if (!CacheValidateInput(desc))
	{
		return false;
	}

	if (m_pDevice == nullptr)
	{
		LOG_ERROR("KFETextureRTV::Impl::Initialize: Cached device is null.");
		Reset();
		return false;
	}

	if (m_pTexture == nullptr)
	{
		LOG_ERROR("KFETextureRTV::Impl::Initialize: Cached texture is null.");
		Reset();
		return false;
	}

	if (m_pHeap == nullptr)
	{
		LOG_ERROR("KFETextureRTV::Impl::Initialize: Cached RTV heap is null.");
		Reset();
		return false;
	}

	auto* nativeDevice = m_pDevice->GetNative();
	if (nativeDevice == nullptr)
	{
		LOG_ERROR("KFETextureRTV::Impl::Initialize: Native device is null.");
		Reset();
		return false;
	}

	D3D12_RENDER_TARGET_VIEW_DESC view{};
	view.Format = m_format;
	view.ViewDimension = m_viewDimension;

	switch (m_viewDimension)
	{
	case D3D12_RTV_DIMENSION_TEXTURE1D:
		view.Texture1D.MipSlice = m_mipSlice;
		break;

	case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
		view.Texture1DArray.MipSlice		= m_mipSlice;
		view.Texture1DArray.FirstArraySlice = m_firstArraySlice;
		view.Texture1DArray.ArraySize		= m_arraySize;
		break;

	case D3D12_RTV_DIMENSION_TEXTURE2D:
		view.Texture2D.MipSlice   = m_mipSlice;
		view.Texture2D.PlaneSlice = m_planeSlice;
		break;

	case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
		view.Texture2DArray.MipSlice		= m_mipSlice;
		view.Texture2DArray.FirstArraySlice = m_firstArraySlice;
		view.Texture2DArray.ArraySize		= m_arraySize;
		view.Texture2DArray.PlaneSlice		= m_planeSlice;
		break;

	case D3D12_RTV_DIMENSION_TEXTURE2DMS:
		break;

	case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
		view.Texture2DMSArray.FirstArraySlice	= m_firstArraySlice;
		view.Texture2DMSArray.ArraySize			= m_arraySize;
		break;

	case D3D12_RTV_DIMENSION_TEXTURE3D:
		view.Texture3D.MipSlice		= m_mipSlice;
		view.Texture3D.FirstWSlice	= m_firstArraySlice;
		view.Texture3D.WSize		= m_arraySize;
		break;

	case D3D12_RTV_DIMENSION_BUFFER:
		LOG_WARNING("KFETextureRTV::Impl::Initialize: D3D12_RTV_DIMENSION_BUFFER not fully handled.");
		break;

	case D3D12_RTV_DIMENSION_UNKNOWN:
	default:
		LOG_ERROR("KFETextureRTV::Impl::Initialize: Unsupported or UNKNOWN RTV view dimension.");
		Reset();
		return false;
	}

	if (!HasValidDescriptor())
	{
		LOG_ERROR("KFETextureRTV::Impl::Initialize: Descriptor index is not valid after CacheValidateInput.");
		Reset();
		return false;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_pHeap->GetHandle(m_descriptorIndex);

	nativeDevice->CreateRenderTargetView(
		m_pTexture->GetNative(),
		&view,
		cpuHandle
	);

	return true;
}

_Use_decl_annotations_
bool kfe::KFETextureRTV::Impl::Destroy() noexcept
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
			LOG_WARNING("KFETextureRTV::Impl::Destroy: Failed to free descriptor index {}.", m_descriptorIndex);
		}
	}

	Reset();
	m_bInitialized = false;

	LOG_SUCCESS("KFETextureRTV::Impl::Destroy: RTV destroyed and state reset.");
	return true;
}

_Use_decl_annotations_
bool kfe::KFETextureRTV::Impl::IsInitialize() const noexcept
{
	return m_bInitialized;
}

_Use_decl_annotations_
kfe::KFERTVHeap* kfe::KFETextureRTV::Impl::GetAttachedHeap() const noexcept
{
	return m_pHeap;
}

_Use_decl_annotations_
kfe::KFETexture* kfe::KFETextureRTV::Impl::GetTexture() const noexcept
{
	return m_pTexture;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureRTV::Impl::GetDescriptorIndex() const noexcept
{
	return m_descriptorIndex;
}

_Use_decl_annotations_
bool kfe::KFETextureRTV::Impl::HasValidDescriptor() const noexcept
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
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFETextureRTV::Impl::GetCPUHandle() const noexcept
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
DXGI_FORMAT kfe::KFETextureRTV::Impl::GetFormat() const noexcept
{
	return m_format;
}

_Use_decl_annotations_
D3D12_RTV_DIMENSION kfe::KFETextureRTV::Impl::GetViewDimension() const noexcept
{
	return m_viewDimension;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureRTV::Impl::GetMipSlice() const noexcept
{
	return m_mipSlice;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureRTV::Impl::GetFirstArraySlice() const noexcept
{
	return m_firstArraySlice;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureRTV::Impl::GetArraySize() const noexcept
{
	return m_arraySize;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureRTV::Impl::GetPlaneSlice() const noexcept
{
	return m_planeSlice;
}

_Use_decl_annotations_
bool kfe::KFETextureRTV::Impl::CacheValidateInput(const KFE_RTV_CREATE_DESC& desc)
{
	if (m_bInitialized)
	{
		LOG_WARNING("KFETextureRTV::Impl::Initialize: Already initialized. Destroying previous state.");
		if (!Destroy())
		{
			LOG_ERROR("KFETextureRTV::Impl::Initialize: Failed to destroy previous state.");
			return false;
		}
	}

	if (desc.Device == nullptr)
	{
		LOG_ERROR("KFETextureRTV::Impl::Initialize: Device is null.");
		return false;
	}

	if (desc.Heap == nullptr)
	{
		LOG_ERROR("KFETextureRTV::Impl::Initialize: RTV heap is null.");
		return false;
	}

	if (desc.Texture == nullptr)
	{
		LOG_ERROR("KFETextureRTV::Impl::Initialize: Texture is null.");
		return false;
	}

	m_pDevice = desc.Device;
	m_pHeap		= desc.Heap;
	m_pTexture	= desc.Texture;

	// View parameters
	m_format = (desc.Format != DXGI_FORMAT_UNKNOWN)
		? desc.Format
		: m_pTexture->GetFormat();

	m_viewDimension		= desc.ViewDimension;
	m_mipSlice			= desc.MipSlice;
	m_firstArraySlice	= desc.FirstArraySlice;
	m_arraySize			= desc.ArraySize;
	m_planeSlice		= desc.PlaneSlice;

	std::uint32_t index = desc.DescriptorIndex;

	if (index == KFE_INVALID_INDEX)
	{
		index = m_pHeap->Allocate();
		if (index == KFE_INVALID_INDEX)
		{
			LOG_ERROR("KFETextureRTV::Impl::Initialize: Failed to allocate descriptor from RTV heap.");
			Reset();
			return false;
		}
	}
	else
	{
		if (!m_pHeap->IsValidIndex(index))
		{
			LOG_ERROR("KFETextureRTV::Impl::Initialize: Provided descriptor index {} is invalid for this heap.", index);
			Reset();
			return false;
		}
	}

	m_descriptorIndex = index;
	m_bInitialized	  = true;

	LOG_SUCCESS("KFETextureRTV::Impl::Initialize: RTV cached. HeapIdx = {}, Format = {}, Dimension = {}",
		m_descriptorIndex,
		static_cast<int>(m_format),
		static_cast<int>(m_viewDimension));

	return true;
}

void kfe::KFETextureRTV::Impl::Reset()
{
	m_pDevice		  = nullptr;
	m_pHeap			  = nullptr;
	m_pTexture		  = nullptr;
	m_format		  = DXGI_FORMAT_UNKNOWN;
	m_viewDimension   = D3D12_RTV_DIMENSION_UNKNOWN;
	m_mipSlice		  = 0u;
	m_firstArraySlice = 0u;
	m_arraySize		  = 0u;
	m_planeSlice	  = 0u;
	m_descriptorIndex = KFE_INVALID_INDEX;
	m_bInitialized	  = false;
}

#pragma endregion
