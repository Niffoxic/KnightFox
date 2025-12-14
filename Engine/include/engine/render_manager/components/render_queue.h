// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */

#pragma once
#include "EngineAPI.h"
#include "engine/core.h"
#include "engine/system/interface/interface_singleton.h"
#include "engine/system/interface/interface_scene.h"
#include "engine/system/interface/interface_light.h"
#include "engine/render_manager/components/camera.h"

//~ Test Light

#include <memory>

struct ID3D12Fence;
struct ID3D12GraphicsCommandList;

namespace kfe 
{
	class KFEResourceHeap;
	class KFEDSVHeap;
	class KFERTVHeap;
	class KFEDevice;
	class KFESamplerHeap;
	class KFEGraphicsCommandList;
	class KFEWindows;
	class KFEGraphicsCmdQ;
	class KFESwapChain;

	typedef struct _KFE_RENDER_QUEUE_INIT_DESC
	{
		KFECamera*				pCamera;
		KFEResourceHeap*		pResourceHeap;
		KFERTVHeap*				pRTVHeap;
		KFEDevice*				pDevice;
		KFESamplerHeap*			pSamplerHeap;
		KFEGraphicsCommandList* pGraphicsCommandList;
		KFEWindows*				pWindows;
		KFEGraphicsCmdQ*		pGraphicsCommandQ;
		KFESwapChain*			pSwapChain;
	} KFE_RENDER_QUEUE_INIT_DESC;

	typedef struct _KFE_RENDER_QUEUE_MAIN_PASS_DESC
	{
		ID3D12Fence*				pFence;
		std::uint64_t				FenceValue;
		ID3D12GraphicsCommandList*	GraphicsCommandList;
		KFEShadowMap*				ShadowMap;
	} KFE_RENDER_QUEUE_MAIN_PASS_DESC;

	typedef struct _KFE_RENDER_QUEUE_SHADOW_PASS_DESC
	{
		ID3D12Fence*				pFence;
		std::uint64_t				FenceValue;
		ID3D12GraphicsCommandList*	GraphicsCommandList;
	} KFE_RENDER_QUEUE_SHADOW_PASS_DESC;

	class KFE_API KFERenderQueue final: public ISingleton<KFERenderQueue>
	{
	public:
		 KFERenderQueue();
		~KFERenderQueue();

		KFERenderQueue			 (const KFERenderQueue&) noexcept = delete;
		KFERenderQueue			 (KFERenderQueue&&)		 noexcept = delete;
		KFERenderQueue& operator=(const KFERenderQueue&) noexcept = delete;
		KFERenderQueue& operator=(KFERenderQueue&&)		 noexcept = delete;

		NODISCARD bool Initialize(const KFE_RENDER_QUEUE_INIT_DESC& desc);
		NODISCARD bool Destroy() noexcept;

		void Update(float deltaTime);

		void RenderShadowPass(const KFE_RENDER_QUEUE_SHADOW_PASS_DESC& desc);
		void RenderMainPass	 (const KFE_RENDER_QUEUE_MAIN_PASS_DESC& desc);

		//~ Scene Object
		void AddSceneObject	  (IKFESceneObject* scene) noexcept;
		void RemoveSceneObject(IKFESceneObject* scene) noexcept;
		void RemoveSceneObject(const KID id)		   noexcept;

		//~ Light Objects
		void AddLight	(IKFELight* light) noexcept;
		void RemoveLight(IKFELight* light) noexcept;
		void RemoveLight(const KID id)	   noexcept;

	private:
		friend class ISingleton<KFERenderQueue>;
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
} // namespace kfe
