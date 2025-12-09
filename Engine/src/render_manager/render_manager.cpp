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

#include "engine/render_manager/graph/compiled_graph.h"
#include "engine/render_manager/graph/frame_context.h"
#include "engine/render_manager/graph/render_graph.h"
#include <d3d12.h>

//~ Test Renderable
#include "engine/render_manager/api/pso.h"
#include "engine/render_manager/scene/renderable.h"
#include "engine/render_manager/assets_library/shader_library.h"
#include "engine/utils/file_system.h"

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
	bool InitializeComponents ();
	bool InitializeQueues	  ();
	bool InitializeCommands   ();
	bool InitializeHeaps	  ();
	bool InitializeTextures   ();
	bool InitializeRenderGraph();
	void CreateViewport		  ();

	bool InitializeTestTriangle();

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
	std::unique_ptr<KFETexture> m_pDSVBuffer;

	//~ Test DSV
	std::unique_ptr<KFETextureDSV>		m_pDSV{ nullptr };
	Microsoft::WRL::ComPtr<ID3D12Fence>	m_pFence{ nullptr };
	std::uint64_t						m_nFenceValue{ 0u };
	bool m_bInitialized{ false };

	//~ test render
	D3D12_VIEWPORT m_viewport{};
	D3D12_RECT     m_scissorRect{};

	rg::RenderGraph m_renderGraph{};
	rg::RGCompiled  m_compiledGraph{};
	float m_totalTime{ 0.0f };

	//~ Test Triangle
	std::unique_ptr<KFEStagingBuffer> m_triangleStaging;
	std::unique_ptr<KFEVertexBuffer>  m_vertexView;
	std::unique_ptr<KFEIndexBuffer>   m_indexView;

	KFEGeometryData                   m_triangleGeometry{};
	std::unique_ptr<KFETransformNode> m_pTriangleTransform;
	std::unique_ptr<IKFERenderable>   m_pTriangleRenderable;
	std::vector<IKFERenderable*>      m_renderables;

	std::unique_ptr<KFEPipelineState> m_pos{ nullptr };
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
	m_pDSVBuffer = std::make_unique<KFETexture>();

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

	m_pos = std::make_unique<KFEPipelineState>();

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
	dsv.Texture = m_pDSVBuffer.get();
	dsv.Format  = m_pDSVBuffer->GetFormat();

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


	if (!InitializeRenderGraph())
	{
		return false;
	}

	m_bInitialized = true;

	if (!InitializeTestTriangle()) return false;

	return true;
}

bool kfe::KFERenderManager::Impl::Release()
{
	return true;
}

void kfe::KFERenderManager::Impl::FrameBegin(float dt)
{
	m_totalTime += dt;

	if (!m_pFence)
	{
		THROW_MSG("Fence is null!");
	}

	++m_nFenceValue;

	KFE_RESET_COMMAND_LIST resetter{};
	resetter.Fence = m_pFence.Get();
	resetter.FenceValue = m_nFenceValue;
	resetter.PSO = nullptr;

	if (!m_pGfxList->Reset(resetter))
	{
		THROW_MSG("Failed to Render!");
	}

	kfe::FrameContext frameCtx{};
	frameCtx.CommandList = m_pGfxList.get();
	frameCtx.Device = m_pDevice.get();
	frameCtx.RtvHeap = m_pRTVHeap.get();
	frameCtx.DsvHeap = m_pDSVHeap.get();
	frameCtx.CbvSrvUavHeap = m_pResourceHeap.get();
	frameCtx.FrameConstantBuffer = nullptr;
	frameCtx.FrameIndex = static_cast<std::uint32_t>(m_nFenceValue);

	m_compiledGraph.Execute(frameCtx);

	auto* cmdList = m_pGfxList->GetNative();
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

	if (!m_pDSVBuffer) m_pDSVBuffer = std::make_unique<KFETexture>();

	const DXGI_FORMAT testFormat			 = DXGI_FORMAT_D24_UNORM_S8_UINT;
	const D3D12_HEAP_TYPE heapType			 = D3D12_HEAP_TYPE_DEFAULT;
	const D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
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
	
	D3D12_CLEAR_VALUE depthClear{};
	depthClear.Format = testFormat;
	depthClear.DepthStencil.Depth = 1.0f;
	depthClear.DepthStencil.Stencil = 0;

	tex1D.ClearValue = &depthClear;

	if (!m_pDSVBuffer->Initialize(tex1D))
	{
		LOG_ERROR("Failed to initialize Test Texture 1D.");
		return false;
	}

	LOG_INFO("TestTexture1D: dim={}, {}x{}, mip={}, fmt={}",
		static_cast<int>(m_pDSVBuffer->GetDimension()),
		m_pDSVBuffer->GetWidth(),
		m_pDSVBuffer->GetHeight(),
		m_pDSVBuffer->GetMipLevels(),
		static_cast<int>(m_pDSVBuffer->GetFormat()));

	LOG_SUCCESS("All test textures initialized successfully.");
	return true;
}

bool kfe::KFERenderManager::Impl::InitializeRenderGraph()
{
	using namespace kfe::rg;

	m_renderGraph = RenderGraph{};

	std::string passName = "MainTrianglePass";
	m_renderGraph.AddPass(
		passName,
		// Build
		[&](RGBuilder& builder)
		{
			// No logical RG resources yet; swapchain + depth are externals for now
			(void)builder;
		},
		// Execute
		[this](RGExecutionContext& ctx)
		{
			KFEGraphicsCommandList* gfxList = ctx.GetCommandList();
			if (!gfxList)
				return;

			ID3D12GraphicsCommandList* cmdList = gfxList->GetNative();
			if (!cmdList)
				return;

			// Make sure triangle resources exist
			if (!m_pos || !m_vertexView || !m_indexView)
				return;

			auto swapData = m_pSwapChain->GetAndMarkBackBufferData(
				m_pFence.Get(), m_nFenceValue);

			// Transition PRESENT -> RENDER_TARGET
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = swapData.BufferResource;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			cmdList->ResourceBarrier(1u, &barrier);

			// Depth buffer: make sure it's in DEPTH_WRITE for clear/draw
			ID3D12Resource* depthRes = m_pDSVBuffer->GetNative();
			if (depthRes)
			{
				D3D12_RESOURCE_BARRIER depthBarrier{};
				depthBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				depthBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				depthBarrier.Transition.pResource = depthRes;
				depthBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				depthBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;       // adjust if you start tracking real state
				depthBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
				cmdList->ResourceBarrier(1u, &depthBarrier);
			}

			cmdList->RSSetViewports(1u, &m_viewport);
			cmdList->RSSetScissorRects(1u, &m_scissorRect);

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = swapData.BufferHandle;
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_pDSV->GetCPUHandle();

			cmdList->OMSetRenderTargets(1u, &rtvHandle, FALSE, &dsvHandle);

			const float color[4]
			{
				std::sinf(m_totalTime),
				std::cosf(m_totalTime),
				std::sinf(std::sinf(m_totalTime) + std::cosf(m_totalTime)),
				1.0f
			};

			cmdList->ClearRenderTargetView(
				rtvHandle,
				color,
				0u,
				nullptr);

			cmdList->ClearDepthStencilView(
				dsvHandle,
				D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
				1.f,
				0u,
				0u,
				nullptr);

			cmdList->SetPipelineState(m_pos->GetNative());

			if (auto* rs = m_pos->GetRootSignature())
			{
				cmdList->SetGraphicsRootSignature(rs);
			}

			const D3D12_VERTEX_BUFFER_VIEW vbView = m_vertexView->GetView();
			const D3D12_INDEX_BUFFER_VIEW  ibView = m_indexView->GetView();

			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->IASetVertexBuffers(0u, 1u, &vbView);
			cmdList->IASetIndexBuffer(&ibView);

			// Draw triangle
			cmdList->DrawIndexedInstanced(
				3u,   // IndexCountPerInstance
				1u,   // InstanceCount
				0u,   // StartIndexLocation
				0,    // BaseVertexLocation
				0u); 

			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			cmdList->ResourceBarrier(1u, &barrier);
		});

	m_compiledGraph = m_renderGraph.Compile();
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

bool kfe::KFERenderManager::Impl::InitializeTestTriangle()
{
	if (!m_pDevice || !m_pDevice->GetNative())
	{
		LOG_ERROR("KFERenderManager::Impl::InitializeTestTriangle: Device is null.");
		return false;
	}

	if (!m_pFence)
	{
		LOG_ERROR("KFERenderManager::Impl::InitializeTestTriangle: Fence is null.");
		return false;
	}

	auto* gfxQueue = m_pGraphicsQueue->GetNative();
	if (!gfxQueue)
	{
		LOG_ERROR("KFERenderManager::Impl::InitializeTestTriangle: Graphics queue is null.");
		return false;
	}

	{
		++m_nFenceValue;
		KFE_RESET_COMMAND_LIST resetter{};
		resetter.Fence = m_pFence.Get();  // no wait, fresh use
		resetter.FenceValue = m_nFenceValue;
		resetter.PSO = nullptr;

		if (!m_pGfxList->Reset(resetter))
		{
			LOG_ERROR("KFERenderManager::Impl::InitializeTestTriangle: Failed to reset graphics command list for upload.");
			return false;
		}
	}

	ID3D12GraphicsCommandList* cmdList = m_pGfxList->GetNative();
	if (!cmdList)
	{
		LOG_ERROR("KFERenderManager::Impl::InitializeTestTriangle: Command list is null.");
		return false;
	}

	struct Vertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Color;
	};

	Vertex vertices[3]
	{
		{ DirectX::XMFLOAT3(0.0f,  0.5f, 0.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ DirectX::XMFLOAT3(0.5f, -0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f) }
	};

	std::uint16_t indices[3] = { 0, 1, 2 };

	const std::uint32_t vbSize = static_cast<std::uint32_t>(sizeof(vertices));
	const std::uint32_t ibSize = static_cast<std::uint32_t>(sizeof(indices));
	const std::uint32_t totalSize = vbSize + ibSize;

	const std::uint64_t vbOffset = 0u;
	const std::uint64_t ibOffset = static_cast<std::uint64_t>(vbSize);

	// Create staging buffer (UPLOAD + DEFAULT) for VB + IB
	m_triangleStaging = std::make_unique<KFEStagingBuffer>();

	KFE_STAGING_BUFFER_CREATE_DESC stagingDesc{};
	stagingDesc.Device = m_pDevice.get();
	stagingDesc.SizeInBytes = totalSize;

	if (!m_triangleStaging->Initialize(stagingDesc))
	{
		LOG_ERROR("InitializeTestTriangle: Failed to initialize KFEStagingBuffer.");
		return false;
	}

	// Write CPU vertex and index data into the UPLOAD heap
	if (!m_triangleStaging->WriteBytes(vertices, vbSize, vbOffset))
	{
		LOG_ERROR("InitializeTestTriangle: Failed to write vertex data into staging upload buffer.");
		return false;
	}

	if (!m_triangleStaging->WriteBytes(indices, ibSize, ibOffset))
	{
		LOG_ERROR("InitializeTestTriangle: Failed to write index data into staging upload buffer.");
		return false;
	}

	// Record UPLOAD → DEFAULT copy on the current GFX command list
	if (!m_triangleStaging->RecordUploadToDefault(
		cmdList,
		totalSize,
		0u,
		0u))
	{
		LOG_ERROR("InitializeTestTriangle: Failed to record upload from UPLOAD to DEFAULT buffer.");
		return false;
	}

	// Transition DEFAULT buffer from COPY_DEST → VB|IB
	KFEBuffer* defaultBuffer = m_triangleStaging->GetDefaultBuffer();
	if (!defaultBuffer || !defaultBuffer->GetNative())
	{
		LOG_ERROR("InitializeTestTriangle: Default buffer is null.");
		return false;
	}

	ID3D12Resource* defaultResource = defaultBuffer->GetNative();

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = defaultResource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter =
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER |
		D3D12_RESOURCE_STATE_INDEX_BUFFER;

	cmdList->ResourceBarrier(1u, &barrier);

	HRESULT hr = cmdList->Close();
	THROW_DX_IF_FAILS(hr);

	ID3D12CommandList* cmdLists[] = { cmdList };
	gfxQueue->ExecuteCommandLists(1u, cmdLists);

	hr = gfxQueue->Signal(m_pFence.Get(), m_nFenceValue);
	THROW_DX_IF_FAILS(hr);

	m_pGfxList->Wait();

	m_vertexView = std::make_unique<KFEVertexBuffer>();

	KFE_VERTEX_BUFFER_CREATE_DESC vbDesc{};
	vbDesc.Device = m_pDevice.get();
	vbDesc.ResourceBuffer = defaultBuffer;
	vbDesc.StrideInBytes = sizeof(Vertex);
	vbDesc.OffsetInBytes = vbOffset;
	vbDesc.DebugName = "Triangle Vertex Buffer";

	if (!m_vertexView->Initialize(vbDesc))
	{
		LOG_ERROR("InitializeTestTriangle: Failed to initialize vertex buffer view.");
		return false;
	}

	m_indexView = std::make_unique<KFEIndexBuffer>();

	KFE_INDEX_BUFFER_CREATE_DESC ibDesc{};
	ibDesc.Device = m_pDevice.get();
	ibDesc.ResourceBuffer = defaultBuffer;
	ibDesc.Format = DXGI_FORMAT_R16_UINT;
	ibDesc.OffsetInBytes = ibOffset;

	if (!m_indexView->Initialize(ibDesc))
	{
		LOG_ERROR("InitializeTestTriangle: Failed to initialize index buffer view.");
		return false;
	}

	std::string path = "shaders/test/vertex.hlsl";

	ID3DBlob* vertexBlob = shaders::GetOrCompile(path);
	ID3DBlob* pixelBlob  = shaders::GetOrCompile("shaders/test/pixel.hlsl",
		"main", "ps_5_0");

	m_pos = std::make_unique<KFEPipelineState>();

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputs
	{
		{
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,      
			0,
			0,                            
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			"COLOR",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,      
			0,
			12,                 
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		}
	};
	m_pos->SetInputLayout(inputs.data(),
		static_cast<std::uint32_t>(inputs.size()));

	D3D12_SHADER_BYTECODE vsBC{};
	vsBC.pShaderBytecode = vertexBlob->GetBufferPointer();
	vsBC.BytecodeLength = vertexBlob->GetBufferSize();
	m_pos->SetVS(vsBC);

	D3D12_SHADER_BYTECODE psBC{};
	psBC.pShaderBytecode = pixelBlob->GetBufferPointer();
	psBC.BytecodeLength  = pixelBlob->GetBufferSize();
	m_pos->SetPS(psBC);

	m_pos->SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	if (!m_pos->Build(m_pDevice.get()))
	{
		LOG_ERROR("InitializeTestTriangle: Failed to build pipeline state object.");
		return false;
	}

	LOG_SUCCESS("InitializeTestTriangle: Triangle geometry uploaded, VB/IB views created, and PSO built successfully.");
	return true;
}
