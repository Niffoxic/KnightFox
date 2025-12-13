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
#include "engine/render_manager/components/render_queue.h"

//~ Heaps
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"
#include "engine/render_manager/api/heap/heap_dsv.h"
#include "engine/render_manager/api/heap/heap_rtv.h"
#include "engine/render_manager/api/heap/heap_sampler.h"

//~ components
#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/components/swap_chain.h"
#include "engine/windows_manager/windows_manager.h"

//~ Command Lists
#include "engine/render_manager/api/commands/command_allocator.h"
#include "engine/render_manager/api/commands/compute_list.h"
#include "engine/render_manager/api/commands/copy_list.h"
#include "engine/render_manager/api/commands/graphics_list.h"

//~ Queues
#include "engine/render_manager/api/queue/compute_queue.h"
#include "engine/render_manager/api/queue/copy_queue.h"
#include "engine/render_manager/api/queue/graphics_queue.h"

//~ Utility
#include "engine/utils/logger.h"
#include <unordered_map>
#include <wrl/client.h>

#pragma region Impl_Definition

class kfe::KFERenderQueue::Impl
{
public:
	 Impl() = default;
	~Impl() = default;

	NODISCARD bool Initialize(const KFE_RENDER_QUEUE_INIT_DESC& desc);
	NODISCARD bool Destroy   () noexcept;

	void Update(float deltaTime);

	//~ Scene Objects
	void AddSceneObject	  (IKFESceneObject* scene) noexcept;
	void RemoveSceneObject(IKFESceneObject* scene) noexcept;
	void RemoveSceneObject(const KID id)		   noexcept;
	void RenderSceneObject(const KFE_RENDER_QUEUE_RENDER_DESC& desc) noexcept;
	void RenderShadowSceneObject(const KFE_RENDER_QUEUE_RENDER_DESC& desc) noexcept;

	//~ Light Objects
	void AddDirectionalLight	(KFEDirectionalLight* light) noexcept;
	void RemoveDirectionalLight	(KFEDirectionalLight* scene)	 noexcept;
	void RenderShadowPass		(const KFE_RENDER_QUEUE_RENDER_DESC& desc) noexcept;

private:
	//~ Updater Scene Objects
	void BuildSceneObjects();
	void UpdateSceneObjects(float deltaTime);

	//~ Update Lights
	void BuildDirectionalLight();
	void UpdateDirectionalLight(float deltaTime);

private:
	//~ Cache Builder Informations
	KFECamera*				m_pCamera;
	KFEResourceHeap*		m_pResourceHeap;
	KFERTVHeap*				m_pRTVHeap;
	KFEDevice*				m_pDevice;
	KFESamplerHeap*			m_pSamplerHeap;
	KFEGraphicsCommandList* m_pGraphicsCommandList;
	KFEWindows*				m_pWindows;
	KFEGraphicsCmdQ*		m_pGraphicsCommandQ;
	KFESwapChain*			m_pSwapChain;

	//~ Owned Resources
	std::unique_ptr<KFEGraphicsCommandList> m_pCopyCommandList;
	std::unique_ptr<KFEGraphicsCmdQ>		m_pCopyCommandQ;
	Microsoft::WRL::ComPtr<ID3D12Fence>		m_pFence;
	std::uint64_t							m_nCopyFenceValue{ 1u };

	//~ Renderables objects
	std::unordered_map<KID, IKFESceneObject*> m_sceneObjects{};
	std::vector<KID> m_sceneObjectToBuild{};

	//~ Lights
	std::unordered_map<KFEDirectionalLight*, KFEDirectionalLight*> m_directionalLights{};
};
#pragma endregion

#pragma region RenderQ_Body

kfe::KFERenderQueue::KFERenderQueue()
	: m_impl(std::make_unique<kfe::KFERenderQueue::Impl>())
{}

kfe::KFERenderQueue::~KFERenderQueue() = default;

_Use_decl_annotations_
bool kfe::KFERenderQueue::Initialize(const KFE_RENDER_QUEUE_INIT_DESC& desc)
{
	return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFERenderQueue::Destroy() noexcept
{
	return m_impl->Destroy();
}

void kfe::KFERenderQueue::Update(float deltaTime)
{
	m_impl->Update(deltaTime);
}

void kfe::KFERenderQueue::AddSceneObject(IKFESceneObject* scene) noexcept
{
	m_impl->AddSceneObject(scene);

}

void kfe::KFERenderQueue::RemoveSceneObject(IKFESceneObject* scene) noexcept
{
	m_impl->RemoveSceneObject(scene);
}

void kfe::KFERenderQueue::RemoveSceneObject(const KID id) noexcept
{
	m_impl->RemoveSceneObject(id);
}

void kfe::KFERenderQueue::RenderSceneObject(const KFE_RENDER_QUEUE_RENDER_DESC& desc) noexcept
{
	m_impl->RenderSceneObject(desc);
}

void kfe::KFERenderQueue::RenderShadowSceneObject(const KFE_RENDER_QUEUE_RENDER_DESC& desc) noexcept
{
	m_impl->RenderShadowSceneObject(desc);
}

void kfe::KFERenderQueue::AddDirectionalLight(KFEDirectionalLight* light) noexcept
{
	m_impl->AddDirectionalLight(light);
}

void kfe::KFERenderQueue::RemoveDirectionalLight(KFEDirectionalLight* scene) noexcept
{
	m_impl->RemoveDirectionalLight(scene);
}

void kfe::KFERenderQueue::RenderShadowPass(const KFE_RENDER_QUEUE_RENDER_DESC& desc) noexcept
{
	m_impl->RenderShadowPass(desc);
}
#pragma endregion

#pragma region Impl_Body

_Use_decl_annotations_
bool kfe::KFERenderQueue::Impl::Initialize(const KFE_RENDER_QUEUE_INIT_DESC& desc)
{
	m_pCamera				= desc.pCamera;
	m_pResourceHeap			= desc.pResourceHeap;
	m_pRTVHeap				= desc.pRTVHeap;
	m_pDevice				= desc.pDevice;
	m_pSamplerHeap			= desc.pSamplerHeap;
	m_pGraphicsCommandList	= desc.pGraphicsCommandList;
	m_pWindows				= desc.pWindows;
	m_pGraphicsCommandQ		= desc.pGraphicsCommandQ;
	m_pSwapChain			= desc.pSwapChain;

	m_pCopyCommandQ = std::make_unique<KFEGraphicsCmdQ>();
	
	if (!m_pCopyCommandQ->Initialize(m_pDevice)) 
	{
		LOG_ERROR("Failed to initialize Copy Command Queue!");
		return false;
	}

	m_pCopyCommandList = std::make_unique < KFEGraphicsCommandList>();

	KFE_GFX_COMMAND_LIST_CREATE_DESC gfx{};
	gfx.BlockMaxTime  = 5.f;
	gfx.Device		  = m_pDevice;
	gfx.InitialCounts = 5u;
	gfx.MaxCounts	  = 15u;
	if (!m_pCopyCommandList->Initialize(gfx))
	{
		LOG_ERROR("Failed to initialize Copy Command List!");
		return false;
	}

	auto* device = m_pDevice->GetNative();
	device->CreateFence(0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));

	return true;
}

_Use_decl_annotations_
bool kfe::KFERenderQueue::Impl::Destroy() noexcept
{
	//~ destroy objects
	for (auto& [id, scene] : m_sceneObjects)
	{
		if (!scene) continue;
		if (!scene->Destroy()) 
		{
			LOG_ERROR("Failed to destroy {} Scene Object", id);
		}
	}
	return true;
}

void kfe::KFERenderQueue::Impl::Update(float deltaTime)
{
	UpdateSceneObjects(deltaTime);
	UpdateDirectionalLight(deltaTime);
}

void kfe::KFERenderQueue::Impl::AddSceneObject(IKFESceneObject* scene) noexcept
{
	if (!scene) return;
	KID id = scene->GetAssignedKey();

	if (!m_sceneObjects.contains(id))
	{
		if (!scene->IsInitialized())
		{
			m_sceneObjectToBuild.push_back(id);
		}
		m_sceneObjects[id] = scene;
	}
}

void kfe::KFERenderQueue::Impl::RemoveSceneObject(IKFESceneObject* scene) noexcept
{
	if (!scene) return;
	KID id = scene->GetAssignedKey();
	RemoveSceneObject(id);
}

void kfe::KFERenderQueue::Impl::RemoveSceneObject(const KID id) noexcept
{
	if (m_sceneObjects.contains(id))
	{
		m_sceneObjects.erase(id);
	}
}

void kfe::KFERenderQueue::Impl::BuildSceneObjects()
{
	if (m_sceneObjectToBuild.empty()) return;

	auto* copyQueue = m_pCopyCommandQ->GetNative();

	++m_nCopyFenceValue;
	KFE_RESET_COMMAND_LIST resetter{};
	resetter.Fence		= m_pFence.Get();
	resetter.FenceValue = m_nCopyFenceValue;
	resetter.PSO		= nullptr;

	if (!m_pCopyCommandList->Reset(resetter))
	{
		LOG_ERROR("Failed to reset graphics command list for upload.");
	}

	ID3D12GraphicsCommandList* cmdList = m_pCopyCommandList->GetNative();
	if (!cmdList)
	{
		LOG_ERROR("KCommand list is null.");
	}

	//~ Build
	for (auto id : m_sceneObjectToBuild)
	{
		auto* obj = m_sceneObjects[id];
		if (!obj) continue;

		KFE_BUILD_OBJECT_DESC builder{};
		builder.ComandQueue  = m_pCopyCommandQ.get();
		builder.CommandList  = m_pCopyCommandList.get();
		builder.Device		 = m_pDevice;
		builder.Fence		 = m_pFence.Get();
		builder.FenceValue   = m_nCopyFenceValue;
		builder.ResourceHeap = m_pResourceHeap;
		builder.SamplerHeap  = m_pSamplerHeap;

		if (!obj->Build(builder))
		{
			LOG_ERROR("Failed to build {}", obj->GetName());
		}
	}

	cmdList->Close();
	ID3D12CommandList* cmdLists[] = { cmdList };
	copyQueue->ExecuteCommandLists(1u, cmdLists);

	HRESULT hr = copyQueue->Signal(m_pFence.Get(), m_nCopyFenceValue);

	m_pCopyCommandList->Wait();
	m_sceneObjectToBuild.clear();
}

void kfe::KFERenderQueue::Impl::UpdateSceneObjects(float deltaTime)
{
	BuildSceneObjects();

	//~ Update
	KFE_UPDATE_OBJECT_DESC updatter{};
	updatter.CameraPosition		= m_pCamera->GetPosition();
	updatter.ZFar				= m_pCamera->GetFarZ();
	updatter.ZNear				= m_pCamera->GetNearZ();
	updatter.OrthographicMatrix = m_pCamera->GetOrthographicMatrix();
	updatter.PerpectiveMatrix	= m_pCamera->GetPerspectiveMatrix();
	updatter.ViewMatrix			= m_pCamera->GetViewMatrix();

	updatter.deltaTime = deltaTime;

	int x = 0, y = 0;
	m_pWindows->Mouse.GetMousePosition(x, y);
	updatter.MousePosition = { static_cast<float>(x), static_cast<float>(y) };

	auto winSize			= m_pWindows->GetWinSize().As<float>();
	updatter.Resolution		= { winSize.Width, winSize.Height };
	updatter.PlayerPosition = { 0.f, 0.f, 0.f };

	for (auto& [id, scene] : m_sceneObjects)
	{
		if (!scene || !scene->IsInitialized()) continue;

		for (auto& [id, light] : m_directionalLights)
		{
			if (!light) continue;
			auto info = light->GetCBDesc();
			scene->UpdateDirectionalLight(info);
		}
		scene->Update(updatter);
	}
}

void kfe::KFERenderQueue::Impl::RenderSceneObject(const KFE_RENDER_QUEUE_RENDER_DESC& desc) noexcept
{
	KFE_RENDER_OBJECT_DESC renderInfo{};
	renderInfo.CommandList	= desc.GraphicsCommandList;
	renderInfo.Fence		= desc.pFence;
	renderInfo.FenceValue	= desc.FenceValue;
	renderInfo.ShadowMap	= desc.ShadowMap;

	for (auto& [id, scene] : m_sceneObjects)
	{
		if (!scene || !scene->IsInitialized()) continue;
		scene->Render(renderInfo);
	}
}

void kfe::KFERenderQueue::Impl::RenderShadowSceneObject(const KFE_RENDER_QUEUE_RENDER_DESC& desc) noexcept
{
	KFE_RENDER_OBJECT_DESC renderInfo{};
	renderInfo.CommandList	= desc.GraphicsCommandList;
	renderInfo.Fence		= desc.pFence;
	renderInfo.FenceValue	= desc.FenceValue;

	for (auto& [id, scene] : m_sceneObjects)
	{
		if (!scene || !scene->IsInitialized()) continue;
		scene->ShadowPass(renderInfo);
	}
}

void kfe::KFERenderQueue::Impl::BuildDirectionalLight()
{
}

void kfe::KFERenderQueue::Impl::UpdateDirectionalLight(float deltaTime)
{
	BuildDirectionalLight();

	const DirectX::XMVECTOR camPos = DirectX::XMLoadFloat3(&m_pCamera->GetPosition());
	const DirectX::XMVECTOR camForward = m_pCamera->GetForwardVector();

	for (auto& [id, light] : m_directionalLights)
	{
		light->UpdateMatrices(camPos, camForward);
	}
}

void kfe::KFERenderQueue::Impl::AddDirectionalLight(KFEDirectionalLight * light) noexcept
{
	if (!light) return;
	LOG_SUCCESS("ADDED LIGHT");
	if (m_directionalLights.contains(light)) return;
	m_directionalLights[light] = light;
}

void kfe::KFERenderQueue::Impl::RemoveDirectionalLight(KFEDirectionalLight* light) noexcept
{
	if (!light) return;
	if (m_directionalLights.contains(light)) m_directionalLights.erase(light);
}


void kfe::KFERenderQueue::Impl::RenderShadowPass(const KFE_RENDER_QUEUE_RENDER_DESC& desc) noexcept
{
}

#pragma endregion
