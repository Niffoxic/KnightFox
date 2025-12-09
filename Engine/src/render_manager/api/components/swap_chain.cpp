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
#include "engine/render_manager/api/components/swap_chain.h"

#include "engine/render_manager/api/components/factory.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/components/monitor.h"
#include "engine/render_manager/api/queue/graphics_queue.h"

#include "engine/windows_manager/windows_manager.h"
#include "engine/utils/logger.h"

//~ RTVs
#include "engine/render_manager/api/heap/heap_rtv.h"

#pragma region Impl_Declaration

class kfe::KFESwapChain::Impl
{
public:
	 Impl() = default;
	 ~Impl() = default;

	NODISCARD bool Initialize(_In_ const KFE_SWAP_CHAIN_CREATE_DESC& desc);
	
	NODISCARD bool Destroy () noexcept;
	NODISCARD bool Recreate();
	NODISCARD bool Present ();
	NODISCARD bool Present(std::uint32_t syncInterval, std::uint32_t flags);

	NODISCARD bool SetScreenState(const EScreenState state);
	NODISCARD bool SetResolution (const KFE_WinSizeU& winSize);
	NODISCARD bool SetTearing	 (_In_ const bool tearing);

	void AttachFactory		(_In_opt_ KFEFactory*      factory		) noexcept;
	void AttachMonitor		(_In_opt_ KFEMonitor*	   monitor		) noexcept;
	void AttachGraphicsQueue(_In_opt_ KFEGraphicsCmdQ* graphicsQueue) noexcept;
	void AttachWindows		(_In_opt_ KFEWindows*	   windows		) noexcept;

	// Buffer / format configuration setters (call recreate afterward)
	void SetBufferCount		  (_In_ std::uint16_t	 bufferCount	 ) noexcept;
	void SetBackBufferFormat  (_In_ DXGI_FORMAT		 backBufferFormat) noexcept;
	void SetDepthStencilFormat(_In_ DXGI_FORMAT		 depthFormat	 ) noexcept;
	void SetSwapEffect		  (_In_ DXGI_SWAP_EFFECT swapEffect		 ) noexcept;
	void SetBufferUsage		  (_In_ std::uint32_t	 bufferUsage	 ) noexcept;
	void SetSwapChainFlags	  (_In_ std::uint32_t	 flags			 ) noexcept;
	void SetVSyncEnabled	  (_In_ bool			 enableVSync	 ) noexcept;
	void SetWindowState		  (_In_ EScreenState	 state			 ) noexcept;

	//~ Getters
	NODISCARD bool                IsInitialize			 () const noexcept;
	NODISCARD std::uint16_t       GetBufferCount		 () const noexcept;
	NODISCARD DXGI_FORMAT         GetBufferFormat		 () const noexcept;
	NODISCARD DXGI_FORMAT         GetDepthStencilFormat  () const noexcept;
	NODISCARD const KFE_WinSizeU& GetResolution			 () const noexcept;
	NODISCARD float               GetAspectRatio		 () const noexcept;
	NODISCARD bool                IsVSyncEnabled		 () const noexcept;
	NODISCARD bool                IsTearingOn			 () const noexcept;
	NODISCARD std::uint32_t       GetCurrentBackBufferIdx() const noexcept;
	NODISCARD IDXGISwapChain4*	  GetNative				 () const noexcept;
	NODISCARD bool				  IsReadyToCreate		 () const noexcept;

	//~ RTV
	NODISCARD bool			HasRTVs	  () const noexcept;
	NODISCARD KFERTVHeap*	GetRTVHeap() const noexcept;

	NODISCARD std::uint32_t				  GetBackBufferRTVIndex (_In_ std::uint32_t bufferIndex) const noexcept;
	NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRTVHandle(_In_ std::uint32_t bufferIndex) const noexcept;

	NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTVHandle() const noexcept;

	NODISCARD KFE_SWAP_CHAIN_DATA GetAndMarkBackBufferData(
		_In_ ID3D12Fence*  fence,
		_In_ std::uint64_t fenceValue
	) noexcept;

private:
	NODISCARD bool BuildRTVs(
		_In_opt_ KFERTVHeap* rtvHeap,
		_In_opt_ KFEDevice*  pDevice);
	
	NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandleInternal(std::uint32_t bufferIndex) const noexcept;
	
	NODISCARD std::uint32_t FindFreeBackBufferIndex(
		_In_ ID3D12Fence* fence,
		_In_ std::uint64_t fenceValue,
		bool forceWaitIfNone
	) noexcept;

	void UpdateAspectRatio() noexcept;

private:
	//~ Externals
	KFEMonitor*		 m_pMonitor		 { nullptr };
	KFEFactory*		 m_pFactory		 { nullptr };
	KFEGraphicsCmdQ* m_pGraphicsQueue{ nullptr };
	KFEWindows*		 m_pWindows		 { nullptr };

	//~ Com resources
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_pSwapChain;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_ppResources{};
	//~ Configurations
	std::uint16_t      m_nBufferCount		{			    2u			    };
	DXGI_FORMAT        m_eBackBufferFormat	{   DXGI_FORMAT_R8G8B8A8_UNORM  };
	DXGI_FORMAT        m_eDepthStencilFormat{		DXGI_FORMAT_D32_FLOAT   };
	DXGI_SWAP_EFFECT   m_eSwapEffect		{ DXGI_SWAP_EFFECT_FLIP_DISCARD };

	std::uint32_t      m_nBufferUsage   { 0u };
	std::uint32_t      m_nSwapChainFlags{ 0u };
	std::uint32_t      m_nCurrentBackBufferIdx{ 0 };

	KFE_WinSizeU       m_windowSize  { 1280u, 720u };
	EScreenState       m_eWindowState{ EScreenState::Windowed };

	bool               m_bEnableVSync { true };
	bool               m_bAllowTearing{ false };
	bool               m_bInitialized { false };
	bool               m_bDirty		  { false };

	//~ RTVs
	KFERTVHeap*					m_pRTVHeap		{ nullptr };
	bool						m_bRTVsBuilt	{ false };
	std::uint32_t				m_rtvBaseIndex	{ KFE_INVALID_INDEX };
	std::vector<std::uint32_t>	m_backBufferRTVIndices;
	std::vector<std::uint64_t>	m_backBufferFenceValues;
};
#pragma endregion

#pragma region SwapChain_Implementation

kfe::KFESwapChain::KFESwapChain() noexcept
	: m_impl(std::make_unique<kfe::KFESwapChain::Impl>())
{}

kfe::KFESwapChain::~KFESwapChain() noexcept = default;

std::string kfe::KFESwapChain::GetName() const noexcept
{
	return "KFESwapChain";
}

std::string kfe::KFESwapChain::GetDescription() const noexcept
{
	return "Description";
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Initialize(const KFE_SWAP_CHAIN_CREATE_DESC& desc)
{
	return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Destroy() noexcept
{
	return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Recreate()
{
	return m_impl->Recreate();
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Present()
{
	return m_impl->Present();
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Present(std::uint32_t syncInterval, std::uint32_t flags)
{
	return m_impl->Present(syncInterval, flags);
}

_Use_decl_annotations_
kfe::KFE_SWAP_CHAIN_DATA kfe::KFESwapChain::GetAndMarkBackBufferData(ID3D12Fence* fence, std::uint64_t fenceValue) noexcept
{
	return m_impl->GetAndMarkBackBufferData(fence, fenceValue);
}

_Use_decl_annotations_
bool kfe::KFESwapChain::HasRTVs() const noexcept
{
	return m_impl->HasRTVs();
}

_Use_decl_annotations_
kfe::KFERTVHeap* kfe::KFESwapChain::GetRTVHeap() const noexcept
{
	return m_impl->GetRTVHeap();
}

_Use_decl_annotations_
std::uint32_t kfe::KFESwapChain::GetBackBufferRTVIndex(std::uint32_t bufferIndex) const noexcept
{
	return m_impl->GetBackBufferRTVIndex(bufferIndex);
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFESwapChain::GetBackBufferRTVHandle(std::uint32_t bufferIndex) const noexcept
{
	return m_impl->GetBackBufferRTVHandle(bufferIndex);
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFESwapChain::GetCurrentBackBufferRTVHandle() const noexcept
{
	return m_impl->GetCurrentBackBufferRTVHandle();
}

_Use_decl_annotations_
bool kfe::KFESwapChain::SetScreenState(EScreenState state)
{
	return m_impl->SetScreenState(state);
}

_Use_decl_annotations_
bool kfe::KFESwapChain::SetResolution(const KFE_WinSizeU& winSize)
{
	return m_impl->SetResolution(winSize);
}

_Use_decl_annotations_
bool kfe::KFESwapChain::SetTearing(const bool tearing)
{
	return m_impl->SetTearing(tearing);
}

_Use_decl_annotations_
void kfe::KFESwapChain::AttachFactory(KFEFactory* factory) noexcept
{
	return m_impl->AttachFactory(factory);
}

_Use_decl_annotations_
void kfe::KFESwapChain::AttachMonitor(KFEMonitor* monitor) noexcept
{
	return m_impl->AttachMonitor(monitor);
}

_Use_decl_annotations_
void kfe::KFESwapChain::AttachGraphicsQueue(KFEGraphicsCmdQ* graphicsQueue) noexcept
{
	return m_impl->AttachGraphicsQueue(graphicsQueue);
}

_Use_decl_annotations_
void kfe::KFESwapChain::AttachWindows(KFEWindows* windows) noexcept
{
	return m_impl->AttachWindows(windows);
}

_Use_decl_annotations_
void kfe::KFESwapChain::SetBufferCount(std::uint16_t bufferCount) noexcept
{
	return m_impl->SetBufferCount(bufferCount);
}

_Use_decl_annotations_
void kfe::KFESwapChain::SetBackBufferFormat(DXGI_FORMAT backBufferFormat) noexcept
{
	return m_impl->SetBackBufferFormat(backBufferFormat);
}

_Use_decl_annotations_
void kfe::KFESwapChain::SetDepthStencilFormat(DXGI_FORMAT depthFormat) noexcept
{
	return m_impl->SetDepthStencilFormat(depthFormat);
}

_Use_decl_annotations_
void kfe::KFESwapChain::SetSwapEffect(DXGI_SWAP_EFFECT swapEffect) noexcept
{
	return m_impl->SetSwapEffect(swapEffect);
}

_Use_decl_annotations_
void kfe::KFESwapChain::SetBufferUsage(std::uint32_t bufferUsage) noexcept
{
	return m_impl->SetBufferUsage(bufferUsage);
}

_Use_decl_annotations_
void kfe::KFESwapChain::SetSwapChainFlags(std::uint32_t flags) noexcept
{
	return m_impl->SetSwapChainFlags(flags);
}

_Use_decl_annotations_
void kfe::KFESwapChain::SetVSyncEnabled(bool enableVSync) noexcept
{
	return m_impl->SetVSyncEnabled(enableVSync);
}

_Use_decl_annotations_
void kfe::KFESwapChain::SetWindowState(EScreenState state) noexcept
{
	return m_impl->SetWindowState(state);
}

_Use_decl_annotations_
bool kfe::KFESwapChain::IsInitialize() const noexcept
{
	return m_impl->IsInitialize();
}

_Use_decl_annotations_
std::uint16_t kfe::KFESwapChain::GetBufferCount() const noexcept
{
	return m_impl->GetBufferCount();
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFESwapChain::GetBufferFormat() const noexcept
{
	return m_impl->GetBufferFormat();
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFESwapChain::GetDepthStencilFormat() const noexcept
{
	return m_impl->GetDepthStencilFormat();
}

_Use_decl_annotations_
const KFE_WinSizeU& kfe::KFESwapChain::GetResolution() const noexcept
{
	return m_impl->GetResolution();
}

_Use_decl_annotations_
float kfe::KFESwapChain::GetAspectRatio() const noexcept
{
	return m_impl->GetAspectRatio();
}

_Use_decl_annotations_
bool kfe::KFESwapChain::IsVSyncEnabled() const noexcept
{
	return m_impl->IsVSyncEnabled();
}

_Use_decl_annotations_
bool kfe::KFESwapChain::IsTearingOn() const noexcept
{
	return m_impl->IsTearingOn();
}

_Use_decl_annotations_
std::uint32_t kfe::KFESwapChain::GetCurrentBackBufferIdx() const noexcept
{
	return m_impl->GetCurrentBackBufferIdx();
}

_Use_decl_annotations_
IDXGISwapChain4* kfe::KFESwapChain::GetNative() const noexcept
{
	return m_impl->GetNative();
}

_Use_decl_annotations_
bool kfe::KFESwapChain::IsReadyToCreate() const noexcept
{
	return m_impl->IsReadyToCreate();
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFESwapChain::Impl::Initialize(const KFE_SWAP_CHAIN_CREATE_DESC& desc)
{
	//~ Destroy existing swapchain if already initialized
	if (m_bInitialized)
	{
		LOG_WARNING("Already initialized, attempting to destroy existing swapchain.");
		if (!Destroy())
		{
			LOG_ERROR("Failed to destroy existing swapchain before re-initialization.");
			return false;
		}
		m_bInitialized = false;
	}

	//~ Basic validation checks
	if (desc.Factory == nullptr)
	{
		LOG_ERROR("Invalid KFEFactory pointer(nullptr).");
		return false;
	}

	if (desc.GraphicsQueue == nullptr)
	{
		LOG_ERROR("Invalid KFEGraphicsCmdQ pointer (nullptr).");
		return false;
	}

	if (desc.Windows == nullptr)
	{
		LOG_ERROR("Invalid KFEWindows pointer (nullptr).");
		return false;
	}

	//~ Setup Configurations
	m_pFactory			  = desc.Factory;
	m_pGraphicsQueue	  = desc.GraphicsQueue;
	m_pWindows			  = desc.Windows;
	m_pMonitor			  = desc.Monitor;
	m_nBufferCount		  = desc.BufferCount;
	m_eBackBufferFormat   = desc.BackBufferFormat;
	m_eDepthStencilFormat = desc.DepthStencilFormat;
	m_eSwapEffect		  = desc.SwapEffect;
	m_nBufferUsage		  = desc.BufferUsage;
	m_nSwapChainFlags	  = desc.SwapChainFlags;
	m_windowSize		  = desc.WindowSize;
	m_eWindowState		  = desc.WindowState;
	m_bEnableVSync		  = desc.EnableVSync;

	//~ check tearing
	const bool tearingSupported = desc.Factory->IsTearingSupported();
	m_bAllowTearing = desc.AllowTearing && tearingSupported;

	if (desc.AllowTearing && !tearingSupported)
	{
		LOG_WARNING("Tearing requested but not supported by this system. Disabling tearing.");
	}

	if (m_bAllowTearing)
	{
		m_nSwapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	}

	//~ Get and check native pointers
	auto* pNativeFactory = m_pFactory->GetNative();
	auto* pCmdQueue		 = m_pGraphicsQueue->GetNative();

	if (pNativeFactory == nullptr)
	{
		LOG_ERROR("Native factory pointer is nullptr.");
		return false;
	}

	if (pCmdQueue == nullptr)
	{
		LOG_ERROR("Native graphics command queue pointer is nullptr.");
		return false;
	}

	HWND hwnd = m_pWindows->GetWindowsHandle();
	if (hwnd == nullptr)
	{
		LOG_ERROR("Invalid window handle (nullptr).");
		return false;
	}

	if (m_windowSize.Width == 0 || m_windowSize.Height == 0)
	{
		LOG_ERROR("Invalid window size: {}x{}.",
			m_windowSize.Width, m_windowSize.Height);
		return false;
	}

	if (m_nBufferCount == 0)
	{
		LOG_ERROR("BufferCount is zero, must be >= 1.");
		return false;
	}

	if (m_eBackBufferFormat == DXGI_FORMAT_UNKNOWN)
	{
		LOG_ERROR("BackBufferFormat is DXGI_FORMAT_UNKNOWN.");
		return false;
	}

	LOG_INFO(
		"Creating swapchain - Size: {}x{}, Buffers: {}, Format: {}, VSync: {}, Tearing: {}, Windowed: {}.",
		m_windowSize.Width,
		m_windowSize.Height,
		m_nBufferCount,
		static_cast<int>(m_eBackBufferFormat),
		m_bEnableVSync,
		m_bAllowTearing,
		(m_eWindowState == EScreenState::Windowed));

	//~ Fill Swapchain desc
	DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
	swapchainDesc.BufferDesc.Width  = m_windowSize.Width;
	swapchainDesc.BufferDesc.Height = m_windowSize.Height;
	swapchainDesc.BufferDesc.Format = m_eBackBufferFormat;

	// Let DXGI decide refresh
	swapchainDesc.BufferDesc.RefreshRate.Numerator   = 0u;
	swapchainDesc.BufferDesc.RefreshRate.Denominator = 1u;

	swapchainDesc.BufferDesc.Scaling		  = DXGI_MODE_SCALING_UNSPECIFIED;
	swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	swapchainDesc.SampleDesc.Count   = 1u;
	swapchainDesc.SampleDesc.Quality = 0u;

	swapchainDesc.BufferUsage  = m_nBufferUsage;
	swapchainDesc.BufferCount  = m_nBufferCount;
	swapchainDesc.OutputWindow = hwnd;
	swapchainDesc.Windowed     = (m_eWindowState == EScreenState::Windowed);

	swapchainDesc.SwapEffect = m_eSwapEffect;
	swapchainDesc.Flags		 = m_nSwapChainFlags;

	//~ Create Initial Swap Chain
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChainTemp;

	const HRESULT hrCreate = pNativeFactory->CreateSwapChain(
							 static_cast<IUnknown*>(pCmdQueue),
							 &swapchainDesc,
							 swapChainTemp.GetAddressOf());

	if (FAILED(hrCreate))
	{
		LOG_ERROR(
			"CreateSwapChain failed. HRESULT = 0x{:08X}.",
			static_cast<unsigned long>(hrCreate));

		m_pSwapChain.Reset();
		m_bInitialized = false;
		return false;
	}

	//~ Switch to IDXGISwapChain4
	const HRESULT hrCast = swapChainTemp.As(&m_pSwapChain);
	if (FAILED(hrCast) || !m_pSwapChain)
	{
		LOG_ERROR(
			"Failed to query IDXGISwapChain4. HRESULT = 0x{:08X}.",
			static_cast<unsigned long>(hrCast));

		m_pSwapChain.Reset();
		m_bInitialized = false;
		return false;
	}

	//~ Initialize State
	m_nCurrentBackBufferIdx = m_pSwapChain->GetCurrentBackBufferIndex();
	m_bInitialized			= true;
	m_bDirty				= false;

	LOG_SUCCESS(
		"Swapchain created successfully. CurrentBackBufferIdx = {}.",
		m_nCurrentBackBufferIdx);

	if (desc.RtvHeap != nullptr)
	{
		if (!BuildRTVs(desc.RtvHeap, desc.Device))
		{
			LOG_ERROR("Failed to build RTVs for backbuffers.");
		}
		LOG_SUCCESS("Swap Chain RTVs are built!");
	}

	return true;
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Impl::Destroy() noexcept
{
	if (!m_bInitialized && !m_pSwapChain)
	{
		LOG_WARNING("Called on an uninitialized swapchain.");
		return true;
	}

	// Free RTV descriptors
	if (m_pRTVHeap && m_pRTVHeap->IsInitialized() && m_bRTVsBuilt)
	{
		for (std::uint32_t i = 0; i < m_backBufferRTVIndices.size(); ++i)
		{
			const std::uint32_t idx = m_backBufferRTVIndices[i];
			if (idx != KFE_INVALID_INDEX && m_pRTVHeap->IsValidIndex(idx))
			{
				if (!m_pRTVHeap->Free(idx))
				{
					LOG_WARNING("Failed to free RTV descriptor index {} for backbuffer {}.", idx, i);
				}
			}
		}

		m_backBufferRTVIndices.clear();
		m_backBufferFenceValues.clear();
		
		m_pRTVHeap		= nullptr;
		m_bRTVsBuilt	= false;
		m_rtvBaseIndex	= KFE_INVALID_INDEX;

		LOG_INFO("Freed RTV descriptors for all backbuffers.");
	}

	if (m_pSwapChain)
	{
		LOG_INFO("Releasing swapchain.");
		m_pSwapChain.Reset();
	}

	m_bInitialized			= false;
	m_bDirty				= false;
	m_nCurrentBackBufferIdx = 0u;

	LOG_SUCCESS("Swapchain destroyed successfully.");
	return true;
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Impl::Recreate()
{
	//~ Must have enough state to rebuild a swapchain
	if (!IsReadyToCreate())
	{
		LOG_ERROR("KFESwapChain::Impl::Recreate: Not enough information to recreate the swapchain.");
		return false;
	}

	LOG_INFO("KFESwapChain::Impl::Recreate: Recreating swapchain...");

	//~ Destroy the existing one
	if (!Destroy())
	{
		LOG_ERROR("KFESwapChain::Impl::Recreate: Failed to destroy existing swapchain before recreation.");
		return false;
	}

	//~ Build Config
	KFE_SWAP_CHAIN_CREATE_DESC desc = {};
	desc.Monitor		= m_pMonitor;
	desc.Factory		= m_pFactory;
	desc.GraphicsQueue	= m_pGraphicsQueue;
	desc.Windows		= m_pWindows;

	desc.BufferCount		= m_nBufferCount;
	desc.BackBufferFormat	= m_eBackBufferFormat;
	desc.DepthStencilFormat = m_eDepthStencilFormat;

	desc.WindowSize = m_windowSize;
	desc.SwapEffect = m_eSwapEffect;

	desc.BufferUsage	= m_nBufferUsage;
	desc.SwapChainFlags = m_nSwapChainFlags;

	desc.EnableVSync  = m_bEnableVSync;
	desc.AllowTearing = m_bAllowTearing;
	desc.WindowState  = m_eWindowState;

	//~ Attempt initialization
	if (!Initialize(desc))
	{
		LOG_ERROR("KFESwapChain::Impl::Recreate: Swapchain initialization failed.");
		return false;
	}

	m_bDirty = false;

	LOG_SUCCESS("KFESwapChain::Impl::Recreate: Swapchain recreated successfully. BackBufferIdx = {}",
		m_nCurrentBackBufferIdx);

	return true;
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Impl::Present()
{
	if (!m_bInitialized || !m_pSwapChain)
	{
		LOG_ERROR("KFESwapChain::Impl::Present: Swapchain is not initialized.");
		return false;
	}

	UINT syncInterval = m_bEnableVSync ? 1u : 0u;
	UINT flags		  = 0u;

	if (!m_bEnableVSync && m_bAllowTearing)
	{
		flags |= DXGI_PRESENT_ALLOW_TEARING;
	}

	const HRESULT hr = m_pSwapChain->Present(syncInterval, flags);
	if (FAILED(hr))
	{
		LOG_ERROR(
			"KFESwapChain::Impl::Present: Present failed. HRESULT = 0x{:08X}, SyncInterval = {}, Flags = 0x{:08X}.",
			static_cast<unsigned long>(hr),
			syncInterval,
			flags);
		return false;
	}

	m_nCurrentBackBufferIdx = m_pSwapChain->GetCurrentBackBufferIndex();

	return true;
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Impl::Present(std::uint32_t syncInterval, std::uint32_t flags)
{
	if (!m_bInitialized || !m_pSwapChain)
	{
		LOG_ERROR("KFESwapChain::Impl::Present(custom): Swapchain is not initialized.");
		return false;
	}

	if ((flags & DXGI_PRESENT_ALLOW_TEARING) != 0u && syncInterval != 0u)
	{
		LOG_WARNING(
			"KFESwapChain::Impl::Present(custom): DXGI_PRESENT_ALLOW_TEARING used with non-zero SyncInterval ({}). "
			"Behavior may be undefined, recommended SyncInterval = 0 when using tearing.",
			syncInterval);
	}

	const HRESULT hr =	m_pSwapChain->Present(static_cast<UINT>(syncInterval),
						static_cast<UINT>(flags));

	if (FAILED(hr))
	{
		LOG_ERROR(
			"KFESwapChain::Impl::Present(custom): Present failed. HRESULT = 0x{:08X}, SyncInterval = {}, Flags = 0x{:08X}.",
			static_cast<unsigned long>(hr),
			syncInterval,
			flags);
		return false;
	}

	//~ Update current backbuffer index after successful Present
	m_nCurrentBackBufferIdx = m_pSwapChain->GetCurrentBackBufferIndex();

	return true;
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Impl::SetScreenState(const EScreenState state)
{
	if (m_eWindowState == state)
	{
		return true;
	}

	m_eWindowState = state;
	m_bDirty	   = true;
	return true;
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Impl::SetResolution(const KFE_WinSizeU& winSize)
{
	if (winSize.Width == 0 || winSize.Height == 0)
	{
		return false;
	}

	if (m_windowSize.Width  == winSize.Width &&
		m_windowSize.Height == winSize.Height)
	{
		return true;
	}

	m_windowSize = winSize;
	m_bDirty     = true;
	return true;
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Impl::SetTearing(const bool tearing)
{
	if (m_bAllowTearing == tearing)
	{
		return true;
	}

	m_bAllowTearing = tearing;
	m_bDirty		= true;
	return true;
}

_Use_decl_annotations_
void kfe::KFESwapChain::Impl::AttachFactory(KFEFactory* factory) noexcept
{
	if (m_pFactory == factory)
	{
		return;
	}

	m_pFactory = factory;
	m_bDirty   = true;
}

_Use_decl_annotations_
void kfe::KFESwapChain::Impl::AttachMonitor(KFEMonitor* monitor) noexcept
{
	if (m_pMonitor == monitor)
	{
		return;
	}

	m_pMonitor = monitor;
	m_bDirty   = true;
}

_Use_decl_annotations_
void kfe::KFESwapChain::Impl::AttachGraphicsQueue(KFEGraphicsCmdQ* graphicsQueue) noexcept
{
	if (m_pGraphicsQueue == graphicsQueue)
	{
		return;
	}

	m_pGraphicsQueue = graphicsQueue;
	m_bDirty		 = true;
}

_Use_decl_annotations_
void kfe::KFESwapChain::Impl::AttachWindows(KFEWindows* windows) noexcept
{
	if (m_pWindows == windows)
	{
		return;
	}

	m_pWindows = windows;
	m_bDirty   = true;
}

_Use_decl_annotations_
void kfe::KFESwapChain::Impl::SetBufferCount(std::uint16_t bufferCount) noexcept
{
	if (bufferCount == 0 || m_nBufferCount == bufferCount)
	{
		return;
	}

	m_nBufferCount = bufferCount;
	m_bDirty       = true;
}

_Use_decl_annotations_
void kfe::KFESwapChain::Impl::SetBackBufferFormat(DXGI_FORMAT backBufferFormat) noexcept
{
	if (m_eBackBufferFormat == backBufferFormat)
	{
		return;
	}

	m_eBackBufferFormat = backBufferFormat;
	m_bDirty			= true;
}

_Use_decl_annotations_
void kfe::KFESwapChain::Impl::SetDepthStencilFormat(DXGI_FORMAT depthFormat) noexcept
{
	if (m_eDepthStencilFormat == depthFormat)
	{
		return;
	}

	m_eDepthStencilFormat = depthFormat;
	m_bDirty			  = true;
}

_Use_decl_annotations_
void kfe::KFESwapChain::Impl::SetSwapEffect(DXGI_SWAP_EFFECT swapEffect) noexcept
{
	if (m_eSwapEffect == swapEffect)
	{
		return;
	}

	m_eSwapEffect = swapEffect;
	m_bDirty	  = true;
}

_Use_decl_annotations_
void kfe::KFESwapChain::Impl::SetBufferUsage(std::uint32_t bufferUsage) noexcept
{
	if (m_nBufferUsage == bufferUsage)
	{
		return;
	}

	m_nBufferUsage = bufferUsage;
	m_bDirty	   = true;
}

_Use_decl_annotations_
void kfe::KFESwapChain::Impl::SetSwapChainFlags(std::uint32_t flags) noexcept
{
	if (m_nSwapChainFlags == flags)
	{
		return;
	}

	m_nSwapChainFlags = flags;
	m_bDirty		  = true;
}

_Use_decl_annotations_
void kfe::KFESwapChain::Impl::SetVSyncEnabled(bool enableVSync) noexcept
{
	if (m_bEnableVSync == enableVSync)
	{
		return;
	}

	m_bEnableVSync = enableVSync;
	m_bDirty	   = true;
}

_Use_decl_annotations_
void kfe::KFESwapChain::Impl::SetWindowState(EScreenState state) noexcept
{
	if (m_eWindowState == state)
	{
		return;
	}

	m_eWindowState = state;
	m_bDirty	   = true;
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Impl::IsInitialize() const noexcept
{
	return m_bInitialized;
}

_Use_decl_annotations_
std::uint16_t kfe::KFESwapChain::Impl::GetBufferCount() const noexcept
{
	return m_nBufferCount;
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFESwapChain::Impl::GetBufferFormat() const noexcept
{
	return m_eBackBufferFormat;
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFESwapChain::Impl::GetDepthStencilFormat() const noexcept
{
	return m_eDepthStencilFormat;
}

_Use_decl_annotations_
const KFE_WinSizeU& kfe::KFESwapChain::Impl::GetResolution() const noexcept
{
	return m_windowSize;
}

_Use_decl_annotations_
float kfe::KFESwapChain::Impl::GetAspectRatio() const noexcept
{
	if (m_windowSize.Height == 0)
	{
		return 0.0f;
	}

	return	static_cast<float>(m_windowSize.Width) /
			static_cast<float>(m_windowSize.Height);
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Impl::IsVSyncEnabled() const noexcept
{
	return m_bEnableVSync;
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Impl::IsTearingOn() const noexcept
{
	return m_bAllowTearing;
}

_Use_decl_annotations_
std::uint32_t kfe::KFESwapChain::Impl::GetCurrentBackBufferIdx() const noexcept
{
	return m_nCurrentBackBufferIdx;
}

_Use_decl_annotations_
IDXGISwapChain4* kfe::KFESwapChain::Impl::GetNative() const noexcept
{
	return m_pSwapChain.Get();
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Impl::IsReadyToCreate() const noexcept
{
	const bool hasCoreObjects =
		(m_pFactory != nullptr)		  &&
		(m_pGraphicsQueue != nullptr) &&
		(m_pWindows != nullptr);

	const bool hasValidSize =
		(m_windowSize.Width > 0) &&
		(m_windowSize.Height > 0);

	const bool hasBuffers = (m_nBufferCount > 0);
	const bool hasFormat  = (m_eBackBufferFormat != DXGI_FORMAT_UNKNOWN);

	return	hasCoreObjects &&
			hasValidSize   &&
			hasBuffers	   &&
			hasFormat;
}

_Use_decl_annotations_
kfe::KFE_SWAP_CHAIN_DATA kfe::KFESwapChain::Impl::GetAndMarkBackBufferData(ID3D12Fence* fence, std::uint64_t fenceValue) noexcept
{
	KFE_SWAP_CHAIN_DATA data{};
	data.BufferHandle.ptr = 0;
	data.BufferIndex	  = KFE_INVALID_INDEX;
	data.BufferResource   = nullptr;

	// Validate RTVs
	if (!HasRTVs())
	{
		LOG_ERROR("GetAndMarkBackBufferData: RTVs not built.");
		return data;
	}

	if (!m_pSwapChain)
	{
		LOG_ERROR("GetAndMarkBackBufferData: Swapchain is null.");
		return data;
	}

	// Get current swapchain backbuffer index
	std::uint32_t index = m_pSwapChain->GetCurrentBackBufferIndex();
	if (index >= m_nBufferCount)
	{
		LOG_ERROR("GetAndMarkBackBufferData: Backbuffer index {} out of range.", index);
		return data;
	}

	// Fence tracking (optional)
	if (fence != nullptr)
	{
		m_backBufferFenceValues[index] = fenceValue;
	}

	// Update internal current index
	m_nCurrentBackBufferIdx = index;

	// Get RTV descriptor handle
	const std::uint32_t rtvIndex = m_backBufferRTVIndices[index];
	if (!m_pRTVHeap->IsValidIndex(rtvIndex))
	{
		LOG_ERROR("GetAndMarkBackBufferData: Invalid RTV descriptor index {}.", rtvIndex);
		return data;
	}

	data.BufferHandle = m_pRTVHeap->GetHandle(rtvIndex);
	data.BufferIndex  = index;

	// Get the backbuffer resource
	if (index < m_ppResources.size() && m_ppResources[index])
	{
		data.BufferResource = m_ppResources[index].Get();
	}
	else
	{
		LOG_ERROR("GetAndMarkBackBufferData: Missing backbuffer resource at {}.", index);
	}

	return data;
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Impl::BuildRTVs(
	KFERTVHeap* rtvHeap,
	KFEDevice* pDevice)
{
	m_backBufferRTVIndices .clear();
	m_backBufferFenceValues.clear();

	m_bRTVsBuilt	= false;
	m_pRTVHeap		= nullptr;
	m_rtvBaseIndex	= KFE_INVALID_INDEX;

	if (!m_pSwapChain)
	{
		LOG_ERROR("SwapChain is null. Cannot build RTVs.");
		return false;
	}

	if (rtvHeap == nullptr)
	{
		LOG_WARNING("RTV heap is null. Skipping RTV creation.");
		return false;
	}

	if (!rtvHeap->IsInitialized())
	{
		LOG_ERROR("RTV heap is not initialized.");
		return false;
	}

	if (m_nBufferCount == 0)
	{
		LOG_ERROR("BufferCount is 0.");
		return false;
	}

	if (!pDevice || !pDevice->GetNative())
	{
		LOG_ERROR("Device or native device is null!");
		return false;
	}

	//~ Check that heap has enough space
	const std::uint32_t remaining = rtvHeap->GetRemaining();
	if (remaining < m_nBufferCount)
	{
		LOG_ERROR(
			"RTV heap does not have enough remaining descriptors. "
			"Required = {}, Remaining = {}",
			m_nBufferCount, remaining);
		return false;
	}

	m_pRTVHeap = rtvHeap;

	m_backBufferRTVIndices .resize(m_nBufferCount, KFE_INVALID_INDEX);
	m_backBufferFenceValues.resize(m_nBufferCount, 0ull);

	ID3D12Device* nativeDevice = pDevice->GetNative();

	//~ Create RTVs
	for (std::uint32_t i = 0; i < m_nBufferCount; ++i)
	{
		// Allocate descriptor from heap
		const std::uint32_t descriptorIndex = rtvHeap->Allocate();
		if (!rtvHeap->IsValidIndex(descriptorIndex))
		{
			LOG_ERROR("Failed to allocate RTV descriptor for backbuffer {}.", i);

			// Rollback any allocated descriptors so far
			for (std::uint32_t j = 0; j < i; ++j)
			{
				const std::uint32_t idx = m_backBufferRTVIndices[j];
				if (idx != KFE_INVALID_INDEX && rtvHeap->IsValidIndex(idx))
				{
					if (rtvHeap->Free(idx)) LOG_ERROR("Failed to Free Handle!");
				}
			}

			m_backBufferRTVIndices .clear();
			m_backBufferFenceValues.clear();
			m_pRTVHeap	   = nullptr;
			m_rtvBaseIndex = KFE_INVALID_INDEX;
			return false;
		}

		m_backBufferRTVIndices[i] = descriptorIndex;

		// Get the backbuffer resource
		Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
		const HRESULT hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
		if (FAILED(hr) || !backBuffer)
		{
			LOG_ERROR("Failed to get backbuffer {}. HRESULT = 0x{:08X}", i, hr);

			// Rollback descriptors on errors
			for (std::uint32_t j = 0; j <= i; ++j)
			{
				const std::uint32_t idx = m_backBufferRTVIndices[j];
				if (idx != KFE_INVALID_INDEX && rtvHeap->IsValidIndex(idx))
				{
					if (rtvHeap->Free(idx)) LOG_ERROR("Failed to Free Handle!");
				}
			}

			m_backBufferRTVIndices .clear();
			m_backBufferFenceValues.clear();
			m_pRTVHeap	   = nullptr;
			m_rtvBaseIndex = KFE_INVALID_INDEX;
			return false;
		}

		// RTV description
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format				 = m_eBackBufferFormat;
		rtvDesc.ViewDimension		 = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice	 = 0u;
		rtvDesc.Texture2D.PlaneSlice = 0u;

		const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = rtvHeap->GetHandle(descriptorIndex);

		nativeDevice->CreateRenderTargetView(
			backBuffer.Get(),
			&rtvDesc,
			cpuHandle
		);

		m_ppResources.emplace_back(std::move(backBuffer));

		LOG_INFO("Built RTV for backbuffer{} at descriptor index{}.", i, descriptorIndex);
	}

	m_bRTVsBuilt = true;

	m_rtvBaseIndex = m_backBufferRTVIndices.empty()
		? KFE_INVALID_INDEX
		: m_backBufferRTVIndices.front();

	LOG_SUCCESS("Successfully built RTVs for {} backbuffers.", m_nBufferCount);

	return true;
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE
kfe::KFESwapChain::Impl::GetRTVHandleInternal(
	std::uint32_t bufferIndex) const noexcept
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle{};
	handle.ptr = 0;

	if (!HasRTVs() || !m_pRTVHeap)
		return handle;

	const std::uint32_t rtvIndex = GetBackBufferRTVIndex(bufferIndex);
	if (rtvIndex == KFE_INVALID_INDEX)
		return handle;

	return m_pRTVHeap->GetHandle(rtvIndex);
}

_Use_decl_annotations_
std::uint32_t kfe::KFESwapChain::Impl::FindFreeBackBufferIndex(
	ID3D12Fence* fence,
	std::uint64_t fenceValue,
	bool forceWaitIfNone) noexcept
{
	if (!HasRTVs() || !fence)
		return KFE_INVALID_INDEX;

	const UINT64 completed = fence->GetCompletedValue();

	for (std::uint32_t i = 0; i < m_nBufferCount; ++i)
	{
		if (completed >= m_backBufferFenceValues[i])
			return i;
	}

	if (forceWaitIfNone)
	{
		HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (!hEvent)
			return KFE_INVALID_INDEX;

		fence->SetEventOnCompletion(fenceValue, hEvent);
		WaitForSingleObject(hEvent, INFINITE);
		CloseHandle(hEvent);

		const UINT64 completedAfterWait = fence->GetCompletedValue();

		for (std::uint32_t i = 0; i < m_nBufferCount; ++i)
		{
			if (completedAfterWait >= m_backBufferFenceValues[i])
				return i;
		}
	}

	return KFE_INVALID_INDEX;
}

void kfe::KFESwapChain::Impl::UpdateAspectRatio() noexcept
{
}

_Use_decl_annotations_
bool kfe::KFESwapChain::Impl::HasRTVs() const noexcept
{
	return m_bRTVsBuilt && m_pRTVHeap != nullptr && !m_backBufferRTVIndices.empty();
}

_Use_decl_annotations_
kfe::KFERTVHeap* kfe::KFESwapChain::Impl::GetRTVHeap() const noexcept
{
	return m_pRTVHeap;
}

_Use_decl_annotations_
std::uint32_t kfe::KFESwapChain::Impl::GetBackBufferRTVIndex(std::uint32_t bufferIndex) const noexcept
{
	if (bufferIndex >= m_backBufferRTVIndices.size())
	{
		LOG_ERROR("KFESwapChain::Impl::GetBackBufferRTVIndex: bufferIndex {} out of range.", bufferIndex);
		return KFE_INVALID_INDEX;
	}

	const std::uint32_t index = m_backBufferRTVIndices[bufferIndex];

	if (index == KFE_INVALID_INDEX)
	{
		LOG_WARNING("KFESwapChain::Impl::GetBackBufferRTVIndex: No RTV index stored for buffer {}", bufferIndex);
	}

	return index;
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE
kfe::KFESwapChain::Impl::GetBackBufferRTVHandle(std::uint32_t bufferIndex) const noexcept
{
	return GetRTVHandleInternal(bufferIndex);
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE
kfe::KFESwapChain::Impl::GetCurrentBackBufferRTVHandle() const noexcept
{
	return GetRTVHandleInternal(m_nCurrentBackBufferIdx);
}

#pragma endregion
