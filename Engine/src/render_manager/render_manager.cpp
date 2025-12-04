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

#include <d3d12.h>

#include "engine/windows_manager/windows_manager.h"
#include "engine/utils/logger.h"

//~ Components
#include "engine/render_manager/components/factory.h"
#include "engine/render_manager/components/adapter.h"
#include "engine/render_manager/components/monitor.h"
#include "engine/render_manager/components/device.h"
#include "engine/render_manager/components/swap_chain.h"

//~ Memory Managements
#include "engine/render_manager/queue/graphics_queue.h"
#include "engine/render_manager/queue/compute_queue.h"
#include "engine/render_manager/queue/copy_queue.h"

//~ Tests
#include "engine/render_manager/commands/command_allocator.h"
#include "engine/render_manager/commands/graphics_list.h"
#include "engine/render_manager/commands/copy_list.h"
#include "engine/render_manager/commands/compute_list.h"
#include "engine/render_manager/pool/allocator_pool.h"

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

	//~ Test Commands
	std::unique_ptr<KFECommandAllocator>	 m_pAllocator	 { nullptr };
	std::unique_ptr<KFECommandAllocatorPool> m_pAllocatorPool{ nullptr };
	std::unique_ptr<KFEGraphicsCommandList>  m_pGfxList		 { nullptr };
	std::unique_ptr<KFEComputeCommandList>   m_pComputeList  { nullptr };
	std::unique_ptr<KFECopyCommandList>		 m_pCopyList	 { nullptr };
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

bool kfe::KFERenderManager::Initialize()
{
	return m_impl->Initialize();
}

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
	m_pAllocator	 = std::make_unique<KFECommandAllocator>	();
	m_pAllocatorPool = std::make_unique<KFECommandAllocatorPool>();
	m_pGfxList		 = std::make_unique<KFEGraphicsCommandList>	();
	m_pComputeList	 = std::make_unique<KFEComputeCommandList>	();
	m_pCopyList		 = std::make_unique<KFECopyCommandList>		();
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

	if (!m_pSwapChain->Initialize(swap))
	{
		LOG_ERROR("Failed to Create Swapchain!");
		return false;
	}

	return true;
}

bool kfe::KFERenderManager::Impl::Release()
{
	return true;
}

void kfe::KFERenderManager::Impl::FrameBegin(float dt)
{

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
	deviceDesc.Adapter = m_pAdapter.get();
	deviceDesc.debugName = "KnightDxDebugger";
	deviceDesc.Factory = m_pFactory.get();
	deviceDesc.Monitor = m_pMonitor.get();

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
	KFE_CA_CREATE_DESC allocatorDesc{};
	allocatorDesc.BlockMaxTime = 5u;
	allocatorDesc.CmdListType  = D3D12_COMMAND_LIST_TYPE_DIRECT;
	allocatorDesc.Device	   = m_pDevice.get();

	if (!m_pAllocator->Initialize(allocatorDesc))
	{
		LOG_ERROR("Failed To Initialize Test Command Allocator");
		return false;
	}

	KFE_CA_POOL_CREATE_DESC pool{};
	pool.BlockMaxTime  = 5u;
	pool.CmdListType   = D3D12_COMMAND_LIST_TYPE_DIRECT;
	pool.Device		   = m_pDevice.get();
	pool.InitialCounts = 100u;
	pool.MaxCounts	   = 1000u;

	if (!m_pAllocatorPool->Initialize(pool))
	{
		LOG_ERROR("Failed To Initialize Test Command Pool");
		return false;
	}

	KFE_GFX_COMMAND_LIST_CREATE_DESC graphics{};
	graphics.BlockMaxTime	= 5u;
	graphics.Device			= m_pDevice.get();
	graphics.InitialCounts	= 3u;
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
