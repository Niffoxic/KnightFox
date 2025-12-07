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
#include "engine/render_manager/render_manager.h"

#include "engine/windows_manager/windows_manager.h"
#include "engine/utils/logger.h"

//~ Components
#include "engine/render_manager/api/components/factory.h"
#include "engine/render_manager/api/components/adapter.h"
#include "engine/render_manager/api/components/monitor.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/components/swap_chain.h"

//~ Memory Managements
#include "engine/render_manager/api/queue/graphics_queue.h"
#include "engine/render_manager/api/queue/compute_queue.h"
#include "engine/render_manager/api/queue/copy_queue.h"

//~ Tests
#include "engine/render_manager/api/commands/command_allocator.h"
#include "engine/render_manager/api/commands/graphics_list.h"
#include "engine/render_manager/api/commands/copy_list.h"
#include "engine/render_manager/api/commands/compute_list.h"
#include "engine/render_manager/api/pool/allocator_pool.h"

//~ Test Heaps
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"
#include "engine/render_manager/api/heap/heap_dsv.h"
#include "engine/render_manager/api/heap/heap_rtv.h"
#include "engine/render_manager/api/heap/heap_sampler.h"

//~ Test buffers
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/buffer/constant_buffer.h"
#include "engine/render_manager/api/buffer/index_buffer.h"
#include "engine/render_manager/api/buffer/raw_buffer.h"
#include "engine/render_manager/api/buffer/readback_buffer.h"
#include "engine/render_manager/api/buffer/staging_buffer.h"
#include "engine/render_manager/api/buffer/structured_buffer.h"
#include "engine/render_manager/api/buffer/upload_buffer.h"
#include "engine/render_manager/api/buffer/vertex_buffer.h"

//~ Test Textures
#include "engine/render_manager/api/texture/texture.h"
#include "engine/render_manager/api/texture/texture_dsv.h"

#include "engine/system/exception/base_exception.h"
#include "engine/system/exception/dx_exception.h"
#include "engine/system/common_types.h"

#pragma region IMPL

class kfe::KFERenderManager::Impl
{
public:
	Impl(KFEWindows* windows);

	bool Initialize();
	bool Release   ();

	void FrameBegin(float dt);
	void FrameEnd  ();

private:
	bool InitializeComponents();
	bool InitializeQueues	 ();
	bool InitializeCommands  ();
	bool InitializeHeaps	 ();
	bool InitializeTextures  ();

	void CreateViewport();

private:
	KFEWindows* m_pWindows{ nullptr };

	//~ Components
	std::unique_ptr<KFEFactory>    m_pFactory  { nullptr };
	std::unique_ptr<KFEAdapter>    m_pAdapter  { nullptr };
	std::unique_ptr<KFEMonitor>    m_pMonitor  { nullptr };
	std::unique_ptr<KFEDevice>     m_pDevice   { nullptr };
	std::unique_ptr<KFESwapChain>  m_pSwapChain{ nullptr };

	//~ Queues
	std::unique_ptr<KFEGraphicsCmdQ> m_pGraphicsQueue{ nullptr };
	std::unique_ptr<KFEComputeCmdQ>  m_pComputeQueue { nullptr };
	std::unique_ptr<KFECopyCmdQ>	 m_pCopyQueue	 { nullptr };

	std::unique_ptr<KFEGraphicsCommandList>  m_pGfxList		 { nullptr };
	std::unique_ptr<KFEComputeCommandList>   m_pComputeList  { nullptr };
	std::unique_ptr<KFECopyCommandList>		 m_pCopyList	 { nullptr };

	//~ Test Heaps
	std::unique_ptr<KFERTVHeap>		 m_pRTVHeap		{ nullptr };
	std::unique_ptr<KFEDSVHeap>		 m_pDSVHeap		{ nullptr };
	std::unique_ptr<KFEResourceHeap> m_pResourceHeap{ nullptr };
	std::unique_ptr<KFESamplerHeap>  m_pSamplerHeap { nullptr };

	//~ Test textures
	std::unique_ptr<KFETexture> m_pTestTexture1D;

	//~ Test DSV
	std::unique_ptr<KFETextureDSV>		m_pDSV{ nullptr };
	Microsoft::WRL::ComPtr<ID3D12Fence>	m_pFence{ nullptr };
	std::uint64_t						m_nFenceValue{ 0u };
	bool m_bInitialized{ false };

	//~ test render
	D3D12_VIEWPORT m_viewport{};
	D3D12_RECT     m_scissorRect{};
};

#pragma endregion

kfe::KFERenderManager::KFERenderManager(KFEWindows* windows)
	: m_impl(std::make_unique<kfe::KFERenderManager::Impl>(windows))
{}

kfe::KFERenderManager::~KFERenderManager()
{
	if (m_impl) 
	{
		m_impl->Release();
	}
}

_Use_decl_annotations_
bool kfe::KFERenderManager::Initialize()
{
	return m_impl->Initialize();
}

_Use_decl_annotations_
bool kfe::KFERenderManager::Release()
{
	return m_impl->Release();
}

void kfe::KFERenderManager::OnFrameBegin(float deltaTime)
{
	m_impl->FrameBegin(deltaTime);
}

void kfe::KFERenderManager::OnFrameEnd()
{
	m_impl->FrameEnd();
}

std::string kfe::KFERenderManager::GetName() const noexcept
{
	return "RenderManager";
}

//~ Impl 
kfe::KFERenderManager::Impl::Impl(KFEWindows* windows)
	: m_pWindows(windows)
{
	//~ Create Components
	m_pFactory   = std::make_unique<KFEFactory>  ();
	m_pAdapter   = std::make_unique<KFEAdapter>  ();
	m_pMonitor   = std::make_unique<KFEMonitor>  ();
	m_pDevice    = std::make_unique<KFEDevice>   ();
	m_pSwapChain = std::make_unique<KFESwapChain>();

	//~ Create Dx Queues
	m_pGraphicsQueue = std::make_unique<KFEGraphicsCmdQ>();
	m_pComputeQueue  = std::make_unique<KFEComputeCmdQ> ();
	m_pCopyQueue	 = std::make_unique<KFECopyCmdQ>	();

	//~ tests
	m_pGfxList		 = std::make_unique<KFEGraphicsCommandList>	();
	m_pComputeList	 = std::make_unique<KFEComputeCommandList>	();
	m_pCopyList		 = std::make_unique<KFECopyCommandList>		();

	//~ Test Heaps
	m_pRTVHeap		= std::make_unique<KFERTVHeap>	   ();
	m_pDSVHeap		= std::make_unique<KFEDSVHeap>	   ();
	m_pResourceHeap = std::make_unique<KFEResourceHeap>();
	m_pSamplerHeap  = std::make_unique<KFESamplerHeap> ();

	//~ Test Textures
	m_pTestTexture1D = std::make_unique<KFETexture>();

	//~ views
	m_pDSV = std::make_unique<KFETextureDSV>();
}

bool kfe::KFERenderManager::Impl::Initialize()
{
	if (!InitializeComponents())
	{
		return false;
	}

	if (!InitializeQueues())
	{
		return false;
	}

	if (!InitializeCommands())
	{
		return false;
	}

	if (!InitializeHeaps()) 
	{
		return false;
	}

	KFE_SWAP_CHAIN_CREATE_DESC swap{};
	swap.Monitor		= m_pMonitor.get();
	swap.Factory		= m_pFactory.get();
	swap.GraphicsQueue	= m_pGraphicsQueue.get();
	swap.Windows		= m_pWindows;

	swap.BufferCount		= 4u;
	swap.BackBufferFormat	= DXGI_FORMAT_R8G8B8A8_UNORM;
	swap.DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;

	swap.WindowSize		= m_pWindows->GetWinSize();
	swap.SwapEffect		= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap.BufferUsage	= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap.SwapChainFlags = 0u;

	swap.EnableVSync  = false;
	swap.AllowTearing = true;
	swap.WindowState  = EScreenState::Windowed;
	swap.Device  = m_pDevice.get();
	swap.RtvHeap = m_pRTVHeap.get();

	if (!m_pSwapChain->Initialize(swap))
	{
		LOG_ERROR("Failed to Create Swapchain!");
		return false;
	}

	if (!InitializeTextures()) 
	{
		return false;
	}

	//~ Depth stencil view
	KFE_DSV_CREATE_DESC dsv{};
	dsv.Device	= m_pDevice.get();
	dsv.Heap	= m_pDSVHeap.get();
	dsv.Texture = m_pTestTexture1D.get();
	dsv.Format  = m_pTestTexture1D->GetFormat();

	dsv.ViewDimension	= D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv.MipSlice	    = 0u;
	dsv.FirstArraySlice = 0u;
	dsv.ArraySize		= 1u;
	dsv.Flags			= D3D12_DSV_FLAG_NONE;

	dsv.DescriptorIndex = KFE_INVALID_INDEX;

	if (!m_pDSV->Initialize(dsv))
	{
		LOG_ERROR("Failed to initialize depth-stencil view.");
		return false;
	}

	const HRESULT hr = m_pDevice->GetNative()->CreateFence(10u, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&m_pFence));

	THROW_DX_IF_FAILS(hr);

	CreateViewport();
	m_bInitialized = true;

	return true;
}

bool kfe::KFERenderManager::Impl::Release()
{
	return true;
}

void kfe::KFERenderManager::Impl::FrameBegin(float dt)
{
	static float totalTime = 0.0f;
	totalTime += dt;

	if (!m_pFence) 
	{
		THROW_MSG("Fence is null!");
	}
	++m_nFenceValue;
	KFE_RESET_COMMAND_LIST resetter{};
	resetter.Fence		= m_pFence.Get();
	resetter.FenceValue = m_nFenceValue;
	resetter.PSO		= nullptr;

	LOG_INFO("Fence Value: {}", m_pFence->GetCompletedValue());
	if (!m_pGfxList->Reset(resetter))
	{
		THROW_MSG("Failed to Render!");
	}
	auto* cmdList = m_pGfxList->GetNative();

	auto swapData = m_pSwapChain->GetAndMarkBackBufferData(m_pFence.Get(), m_nFenceValue);
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags				   = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource   = swapData.BufferResource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

	cmdList->ResourceBarrier(1u, &barrier);
	cmdList->RSSetViewports(1u, &m_viewport);
	cmdList->RSSetScissorRects(1u, &m_scissorRect);

	const float color[4]
	{
		std::sinf(totalTime),
		std::cosf(totalTime),
		std::sinf(std::sinf(totalTime) + std::cosf(totalTime)),
		std::sinf(totalTime)
	};

	cmdList->ClearRenderTargetView(swapData.BufferHandle, color, 0u, nullptr);
	
	auto dsvHandle = m_pDSV->GetCPUHandle();
	cmdList->ClearDepthStencilView(dsvHandle,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.f, 0u, 0u, nullptr);
	
	cmdList->OMSetRenderTargets(1u, &swapData.BufferHandle,
		TRUE, &dsvHandle);

	barrier = {};
	barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource	= swapData.BufferResource;
	barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore	= D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter	= D3D12_RESOURCE_STATE_PRESENT;
	cmdList->ResourceBarrier(1u, &barrier);

	cmdList->Close();
	auto* queue = m_pGraphicsQueue->GetNative();
	ID3D12CommandList* cmdLists[] = { cmdList };
	queue->ExecuteCommandLists(1u, cmdLists);

	m_pSwapChain->Present();

	queue->Signal(m_pFence.Get(), m_nFenceValue);
}

void kfe::KFERenderManager::Impl::FrameEnd()
{

}

bool kfe::KFERenderManager::Impl::InitializeComponents()
{
	if (!m_pFactory->Initialize())
	{
		LOG_ERROR("Failed to Initialize Factory");
		return false;
	}

	auto strategy = std::make_unique<AdapterStrategyBestVram>();
	if (!m_pAdapter->Initialize(m_pFactory.get(), strategy.get()))
	{
		LOG_ERROR("Failed to Initialize Factory");
		return false;
	}

	if (!m_pMonitor->Initialize(m_pAdapter.get()))
	{
		LOG_ERROR("Failed to Initialize Monitor");
		return false;
	}

	KFE_DEVICE_CREATE_DESC deviceDesc{};
	deviceDesc.Adapter   = m_pAdapter.get();
	deviceDesc.debugName = "KnightDxDebugger";
	deviceDesc.Factory   = m_pFactory.get();
	deviceDesc.Monitor   = m_pMonitor.get();

#if defined(_DEBUG) || defined(DEBUG)
	deviceDesc.Flags = EDeviceCreateFlags::EnableDebugLayer |
			   EDeviceCreateFlags::EnableGPUBasedValidation |
			EDeviceCreateFlags::EnableStablePowerState;
#endif

	if (!m_pDevice->Initialize(deviceDesc))
	{
		LOG_ERROR("Failed to Initialize Device");
		return false;
	}

#if defined(_DEBUG) || defined(DEBUG)
	m_pAdapter->LogAdapters();
	m_pMonitor->LogOutputs ();
	m_pDevice-> LogDevice  ();
#endif

	LOG_SUCCESS("RenderManager: All Components initialized!");
	return true;
}

bool kfe::KFERenderManager::Impl::InitializeQueues()
{
	if (!m_pGraphicsQueue->Initialize(m_pDevice.get()))
	{
		LOG_ERROR("Failed To Initialize Graphics Queue");
		return false;
	}

	if (!m_pComputeQueue->Initialize(m_pDevice.get()))
	{
		LOG_ERROR("Failed To Initialize Compute Queue");
		return false;
	}

	if (!m_pCopyQueue->Initialize(m_pDevice.get()))
	{
		LOG_ERROR("Failed To Initialize Copy Queue");
		return false;
	}

	LOG_SUCCESS("RenderManager: All Queues initialized!");
	return true;
}

bool kfe::KFERenderManager::Impl::InitializeCommands()
{
	KFE_GFX_COMMAND_LIST_CREATE_DESC graphics{};
	graphics.BlockMaxTime	= 5u;
	graphics.Device			= m_pDevice.get();
	graphics.InitialCounts	= 7u;
	graphics.MaxCounts		= 10u;
	if (!m_pGfxList->Initialize(graphics))
	{
		LOG_ERROR("Failed To Initialize Test Graphics Command List");
		return false;
	}

	KFE_COMPUTE_COMMAND_LIST_CREATE_DESC compute{};
	compute.BlockMaxTime	= 5u;
	compute.Device			= m_pDevice.get();
	compute.InitialCounts	= 3u;
	compute.MaxCounts		= 10u;

	if (!m_pComputeList->Initialize(compute))
	{
		LOG_ERROR("Failed To Initialize Test Compute Command List");
		return false;
	}

	KFE_COPY_COMMAND_LIST_CREATE_DESC copy{};
	copy.BlockMaxTime  = 5u;
	copy.Device		   = m_pDevice.get();
	copy.InitialCounts = 3u;
	copy.MaxCounts	   = 10u;

	if (!m_pCopyList->Initialize(copy))
	{
		LOG_ERROR("Failed To Initialize Test Copy Command List");
		return false;
	}

	LOG_SUCCESS("RenderManager: All Commands initialized!");
	return true;
}

bool kfe::KFERenderManager::Impl::InitializeHeaps()
{
	if (!m_pDevice)
	{
		LOG_ERROR("KFERenderManager::Impl::InitializeHeaps: Device is nullptr.");
		return false;
	}

	//~ RTV Heap
	KFE_DESCRIPTOR_HEAP_CREATE_DESC rtv{};
	rtv.Device			 = m_pDevice.get();
	rtv.DescriptorCounts = 16u;
	rtv.DebugName		 = "KnightFox Render Target Heap Descriptor";

	if (!m_pRTVHeap || !m_pRTVHeap->Initialize(rtv))
	{
		LOG_ERROR("KFERenderManager::Impl::InitializeHeaps: Failed to initialize RTV heap.");
		return false;
	}

	//~ DSV Heap
	KFE_DESCRIPTOR_HEAP_CREATE_DESC dsv{};
	dsv.Device			 = m_pDevice.get();
	dsv.DescriptorCounts = 16u;
	dsv.DebugName		 = "KnightFox Depth-Stencil Heap Descriptor";

	if (!m_pDSVHeap || !m_pDSVHeap->Initialize(dsv))
	{
		LOG_ERROR("KFERenderManager::Impl::InitializeHeaps: Failed to initialize DSV heap.");
		return false;
	}

	//~ CBV/SRV/UAV Heap
	KFE_DESCRIPTOR_HEAP_CREATE_DESC resource{};
	resource.Device			  = m_pDevice.get();
	resource.DescriptorCounts = 1024u;
	resource.DebugName		  = "KnightFox CBV/SRV/UAV Heap Descriptor";

	if (!m_pResourceHeap || !m_pResourceHeap->Initialize(resource))
	{
		LOG_ERROR("KFERenderManager::Impl::InitializeHeaps: Failed to initialize CBV/SRV/UAV heap.");
		return false;
	}

	//~ Sampler Heap
	KFE_DESCRIPTOR_HEAP_CREATE_DESC sampler{};
	sampler.Device			 = m_pDevice.get();
	sampler.DescriptorCounts = 32u;
	sampler.DebugName		 = "KnightFox Sampler Heap Descriptor";

	if (!m_pSamplerHeap || !m_pSamplerHeap->Initialize(sampler))
	{
		LOG_ERROR("KFERenderManager::Impl::InitializeHeaps: Failed to initialize Sampler heap.");
		return false;
	}

	LOG_SUCCESS("RenderManager: All descriptor heaps initialized!");
	return true;
}

bool kfe::KFERenderManager::Impl::InitializeTextures()
{
	if (!m_pDevice)
	{
		LOG_ERROR("KFERenderManager::Impl::InitializeTextures: Device is nullptr.");
		return false;
	}

	if (!m_pTestTexture1D) m_pTestTexture1D = std::make_unique<KFETexture>();

	const DXGI_FORMAT testFormat			 = DXGI_FORMAT_D24_UNORM_S8_UINT;
	const D3D12_HEAP_TYPE heapType			 = D3D12_HEAP_TYPE_DEFAULT;
	const D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
	const D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// 1D Texture Test
	KFE_TEXTURE_CREATE_DESC tex1D{};
	tex1D.Device = m_pDevice.get();
	tex1D.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	tex1D.Width = 256u;
	tex1D.Height = 1u;
	tex1D.DepthOrArraySize = 1u;
	tex1D.MipLevels = 1u;
	tex1D.Format = testFormat;
	tex1D.SampleDesc.Count = 1u;
	tex1D.SampleDesc.Quality = 0u;
	tex1D.ResourceFlags = resourceFlags;
	tex1D.HeapType = heapType;
	tex1D.InitialState = initialState;
	tex1D.ClearValue = nullptr;

	if (!m_pTestTexture1D->Initialize(tex1D))
	{
		LOG_ERROR("Failed to initialize Test Texture 1D.");
		return false;
	}

	LOG_INFO("TestTexture1D: dim={}, {}x{}, mip={}, fmt={}",
		static_cast<int>(m_pTestTexture1D->GetDimension()),
		m_pTestTexture1D->GetWidth(),
		m_pTestTexture1D->GetHeight(),
		m_pTestTexture1D->GetMipLevels(),
		static_cast<int>(m_pTestTexture1D->GetFormat()));

	LOG_SUCCESS("All test textures initialized successfully.");
	return true;
}

void kfe::KFERenderManager::Impl::CreateViewport()
{
	auto winSize = m_pWindows->GetWinSize();
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.Width = static_cast<float>(winSize.Width);
	m_viewport.Height = static_cast<float>(winSize.Height);
	m_viewport.MaxDepth = 1.0f;
	m_viewport.MinDepth = 0.0f;

	m_scissorRect = { 0, 0,
		static_cast<long>(winSize.Width),
		static_cast<long>(winSize.Height) };
}
