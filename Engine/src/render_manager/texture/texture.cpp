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
#include "engine/render_manager/texture/texture.h"
#include "engine/render_manager/components/device.h"
#include "engine/utils/logger.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#pragma region Impl_Declaration

class kfe::KFETexture::Impl
{
public:
	 Impl() = default;
	~Impl()
	{
		if (!Destroy())
		{
			LOG_ERROR("Faile do Destroy Texture Resource!");
		}
	}

    NODISCARD bool Initialize(_In_ const KFE_TEXTURE_CREATE_DESC& desc);
    
    NODISCARD bool Destroy      ()       noexcept;
    NODISCARD bool IsInitialized() const noexcept;

    NODISCARD ID3D12Resource* GetNative() const noexcept;

    NODISCARD D3D12_RESOURCE_DIMENSION GetDimension       () const noexcept;
    NODISCARD std::uint32_t            GetWidth           () const noexcept;
    NODISCARD std::uint32_t            GetHeight          () const noexcept;
    NODISCARD std::uint16_t            GetDepthOrArraySize() const noexcept;
    NODISCARD std::uint16_t            GetMipLevels       () const noexcept;
    NODISCARD DXGI_FORMAT              GetFormat          () const noexcept;
    NODISCARD D3D12_HEAP_TYPE          GetHeapType        () const noexcept;
    NODISCARD D3D12_RESOURCE_STATES    GetInitialState    () const noexcept;
    NODISCARD D3D12_RESOURCE_FLAGS     GetResourceFlags   () const noexcept;

    NODISCARD bool IsRenderTarget() const noexcept;
    NODISCARD bool IsDepthStencil() const noexcept;
    NODISCARD bool IsTexture1D   () const noexcept;
    NODISCARD bool IsTexture2D   () const noexcept;
    NODISCARD bool IsTexture3D   () const noexcept;

private:
	void CacheFromDesc(_In_ const D3D12_RESOURCE_DESC& desc) noexcept;

	void				ResetCachedDesc() noexcept;
	D3D12_RESOURCE_DESC GetResourceDesc() const noexcept;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource{ nullptr };

	//~ properties queries
	D3D12_RESOURCE_DIMENSION m_dimension	 = static_cast<D3D12_RESOURCE_DIMENSION>(0);
	std::uint32_t            m_width		 = 0u;
	std::uint32_t            m_height		 = 0u;
	std::uint16_t            m_depthOrArray  = 0u;
	std::uint16_t            m_mipLevels	 = 0u;
	DXGI_FORMAT              m_format		 = static_cast<DXGI_FORMAT>			 (0);
	D3D12_HEAP_TYPE          m_heapType		 = static_cast<D3D12_HEAP_TYPE>		 (0);
	D3D12_RESOURCE_STATES    m_initialState  = static_cast<D3D12_RESOURCE_STATES>(0);
	D3D12_RESOURCE_FLAGS     m_resourceFlags = static_cast<D3D12_RESOURCE_FLAGS> (0);

	bool m_bInitialized = false;
};

#pragma endregion

#pragma region Texture_Implementation

kfe::KFETexture::KFETexture()
	: m_impl(std::make_unique<kfe::KFETexture::Impl>())
{}

kfe::KFETexture::~KFETexture() = default;

kfe::KFETexture::KFETexture				   (KFETexture&& other) noexcept = default;
kfe::KFETexture& kfe::KFETexture::operator=(KFETexture&& other) noexcept = default;

_Use_decl_annotations_
bool kfe::KFETexture::Initialize(const KFE_TEXTURE_CREATE_DESC& desc)
{
	return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFETexture::Destroy() noexcept
{
	return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFETexture::IsInitialized() const noexcept
{
	return m_impl->IsInitialized();
}

_Use_decl_annotations_
ID3D12Resource* kfe::KFETexture::GetNative() const noexcept
{
	return m_impl->GetNative();
}

_Use_decl_annotations_
D3D12_RESOURCE_DIMENSION kfe::KFETexture::GetDimension() const noexcept
{
	return m_impl->GetDimension();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETexture::GetWidth() const noexcept
{
	return m_impl->GetWidth();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETexture::GetHeight() const noexcept
{
	return m_impl->GetHeight();
}

_Use_decl_annotations_
std::uint16_t kfe::KFETexture::GetDepthOrArraySize() const noexcept
{
	return m_impl->GetDepthOrArraySize();
}

_Use_decl_annotations_
std::uint16_t kfe::KFETexture::GetMipLevels() const noexcept
{
	return m_impl->GetMipLevels();
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFETexture::GetFormat() const noexcept
{
	return m_impl->GetFormat();
}

_Use_decl_annotations_
D3D12_HEAP_TYPE kfe::KFETexture::GetHeapType() const noexcept
{
	return m_impl->GetHeapType();
}

_Use_decl_annotations_
D3D12_RESOURCE_STATES kfe::KFETexture::GetInitialState() const noexcept
{
	return m_impl->GetInitialState();
}

_Use_decl_annotations_
D3D12_RESOURCE_FLAGS kfe::KFETexture::GetResourceFlags() const noexcept
{
	return m_impl->GetResourceFlags();
}

_Use_decl_annotations_
bool kfe::KFETexture::IsRenderTarget() const noexcept
{
	return m_impl->IsRenderTarget();
}

_Use_decl_annotations_
bool kfe::KFETexture::IsDepthStencil() const noexcept
{
	return m_impl->IsDepthStencil();
}

_Use_decl_annotations_
bool kfe::KFETexture::IsTexture1D() const noexcept
{
	return m_impl->IsTexture1D();
}

_Use_decl_annotations_
bool kfe::KFETexture::IsTexture2D() const noexcept
{
	return m_impl->IsTexture2D();
}

_Use_decl_annotations_
bool kfe::KFETexture::IsTexture3D() const noexcept
{
	return m_impl->IsTexture3D();
}

_Use_decl_annotations_
std::string kfe::KFETexture::GetName() const noexcept
{
	return "KFETexture";
}

_Use_decl_annotations_
std::string kfe::KFETexture::GetDescription() const noexcept
{
	return "KFETexture: D3D12 Texture Resource wrapper!";
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFETexture::Impl::Initialize(const KFE_TEXTURE_CREATE_DESC& desc)
{
    if (m_bInitialized)
    {
        LOG_WARNING("KFETexture::Impl::Initialize: Texture already initialized. Skipping re-initialization.");
        return true;
    }

    if (desc.Device == nullptr)
    {
        LOG_ERROR("KFETexture::Impl::Initialize: Invalid descriptor. Device is null.");
        return false;
    }

    auto* nativeDevice = desc.Device->GetNative();
    if (nativeDevice == nullptr)
    {
        LOG_ERROR("KFETexture::Impl::Initialize: Native D3D12 device is null.");
        return false;
    }

    if (desc.Dimension == static_cast<D3D12_RESOURCE_DIMENSION>(0))
    {
        LOG_ERROR("KFETexture::Impl::Initialize: Invalid texture dimension (0).");
        return false;
    }

    if (desc.Width == 0u)
    {
        LOG_ERROR("KFETexture::Impl::Initialize: Width is 0. Cannot create zero-width texture.");
        return false;
    }

    if (desc.Height == 0u && desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE1D)
    {
        LOG_ERROR("KFETexture::Impl::Initialize: Height is 0 for non-1D texture.");
        return false;
    }

    if (desc.DepthOrArraySize == 0u)
    {
        LOG_ERROR("KFETexture::Impl::Initialize: DepthOrArraySize is 0.");
        return false;
    }

    if (desc.MipLevels == 0u)
    {
        LOG_ERROR("KFETexture::Impl::Initialize: MipLevels is 0.");
        return false;
    }

    if (desc.Format == static_cast<DXGI_FORMAT>(0))
    {
        LOG_WARNING("KFETexture::Impl::Initialize: Format is 0. Make sure this is intended.");
    }

    // Heap properties
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type                  = desc.HeapType;
    heapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.VisibleNodeMask       = 1u;
    heapProps.CreationNodeMask      = 1u;

    // Resource description
    D3D12_RESOURCE_DESC texDesc{};
    texDesc.Dimension        = desc.Dimension;
    texDesc.Alignment        = 0u;
    texDesc.Width            = static_cast<UINT64>(desc.Width);
    texDesc.Height           = desc.Height;
    texDesc.DepthOrArraySize = desc.DepthOrArraySize;
    texDesc.MipLevels        = desc.MipLevels;
    texDesc.Format           = desc.Format;
    texDesc.SampleDesc.Count   = desc.SampleDesc.Count;
    texDesc.SampleDesc.Quality = desc.SampleDesc.Quality;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags  = desc.ResourceFlags;

    //~ Create resource
    HRESULT hr = nativeDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        desc.InitialState,
        desc.ClearValue,
        IID_PPV_ARGS(m_pResource.ReleaseAndGetAddressOf())
    );

    if (FAILED(hr))
    {
        LOG_ERROR("KFETexture::Impl::Initialize: CreateCommittedResource failed. HRESULT=0x{:08X}", static_cast<unsigned>(hr));

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            auto* nativeDevice = desc.Device ? desc.Device->GetNative() : nullptr;
            if (nativeDevice)
            {
                HRESULT reason = nativeDevice->GetDeviceRemovedReason();
                LOG_ERROR("KFETexture::Impl::Initialize: Device removed. Reason=0x{:08X}", static_cast<unsigned>(reason));
            }
        }

        m_pResource.Reset();
        ResetCachedDesc();
        m_bInitialized = false;
        return false;
    }

    // properties
    m_heapType      = desc.HeapType;
    m_initialState  = desc.InitialState;
    m_resourceFlags = desc.ResourceFlags;

    CacheFromDesc(texDesc);
    m_bInitialized = true;

    LOG_SUCCESS("KFETexture::Impl::Initialize: Created texture {}x{} (mips: {}, format: {})",
        desc.Width, desc.Height,
        desc.MipLevels, static_cast<int>(desc.Format));

    return true;
}

_Use_decl_annotations_
bool kfe::KFETexture::Impl::Destroy() noexcept
{
    if (!m_bInitialized && !m_pResource)
    {
        return true;
    }

    m_pResource.Reset();
    ResetCachedDesc();
    m_bInitialized = false;

    return true;
}

_Use_decl_annotations_
bool kfe::KFETexture::Impl::IsInitialized() const noexcept
{
    return m_bInitialized && (m_pResource != nullptr);
}

_Use_decl_annotations_
ID3D12Resource* kfe::KFETexture::Impl::GetNative() const noexcept
{
    return m_pResource.Get();
}

_Use_decl_annotations_
D3D12_RESOURCE_DIMENSION kfe::KFETexture::Impl::GetDimension() const noexcept
{
    return m_dimension;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETexture::Impl::GetWidth() const noexcept
{
    return m_width;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETexture::Impl::GetHeight() const noexcept
{
    return m_height;
}

_Use_decl_annotations_
std::uint16_t kfe::KFETexture::Impl::GetDepthOrArraySize() const noexcept
{
    return m_depthOrArray;
}

_Use_decl_annotations_
std::uint16_t kfe::KFETexture::Impl::GetMipLevels() const noexcept
{
    return m_mipLevels;
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFETexture::Impl::GetFormat() const noexcept
{
    return m_format;
}

_Use_decl_annotations_
D3D12_HEAP_TYPE kfe::KFETexture::Impl::GetHeapType() const noexcept
{
    return m_heapType;
}

_Use_decl_annotations_
D3D12_RESOURCE_STATES kfe::KFETexture::Impl::GetInitialState() const noexcept
{
    return m_initialState;
}

_Use_decl_annotations_
D3D12_RESOURCE_FLAGS kfe::KFETexture::Impl::GetResourceFlags() const noexcept
{
    return m_resourceFlags;
}

_Use_decl_annotations_
bool kfe::KFETexture::Impl::IsRenderTarget() const noexcept
{
    return (m_resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0;
}

_Use_decl_annotations_
bool kfe::KFETexture::Impl::IsDepthStencil() const noexcept
{
    return (m_resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0;
}

_Use_decl_annotations_
bool kfe::KFETexture::Impl::IsTexture1D() const noexcept
{
    return m_dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D;
}

_Use_decl_annotations_
bool kfe::KFETexture::Impl::IsTexture2D() const noexcept
{
    return m_dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

_Use_decl_annotations_
bool kfe::KFETexture::Impl::IsTexture3D() const noexcept
{
    return m_dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D;
}

_Use_decl_annotations_
void kfe::KFETexture::Impl::CacheFromDesc(const D3D12_RESOURCE_DESC& desc) noexcept
{
    m_dimension     = desc.Dimension;
    m_width         = static_cast<std::uint32_t>(desc.Width);
    m_height        = desc.Height;
    m_depthOrArray  = desc.DepthOrArraySize;
    m_mipLevels     = desc.MipLevels;
    m_format        = desc.Format;
}

void kfe::KFETexture::Impl::ResetCachedDesc() noexcept
{
    m_dimension     = static_cast<D3D12_RESOURCE_DIMENSION>(0);
    m_width         = 0u;
    m_height        = 0u;
    m_depthOrArray  = 0u;
    m_mipLevels     = 0u;
    m_format        = static_cast<DXGI_FORMAT>          (0);
    m_heapType      = static_cast<D3D12_HEAP_TYPE>      (0);
    m_initialState  = static_cast<D3D12_RESOURCE_STATES>(0);
    m_resourceFlags = static_cast<D3D12_RESOURCE_FLAGS> (0);
}

D3D12_RESOURCE_DESC kfe::KFETexture::Impl::GetResourceDesc() const noexcept
{
    if (m_pResource)
    {
        return m_pResource->GetDesc();
    }

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension          = static_cast<D3D12_RESOURCE_DIMENSION>(0);
    desc.Alignment          = 0u;
    desc.Width              = 0u;
    desc.Height             = 0u;
    desc.DepthOrArraySize   = 0u;
    desc.MipLevels          = 0u;
    desc.Format             = static_cast<DXGI_FORMAT>(0);
    desc.SampleDesc.Count   = 1u;
    desc.SampleDesc.Quality = 0u;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = static_cast<D3D12_RESOURCE_FLAGS>(0);
    return desc;
}

#pragma endregion
