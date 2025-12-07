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

#include <memory>
#include <cstdint>

#include "engine/system/common_types.h"
#include "engine/core.h"

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct IDXGISwapChain4;
struct ID3D12Fence;
enum   DXGI_FORMAT;
enum   DXGI_SWAP_EFFECT;
struct ID3D12Resource;

namespace kfe
{
	class KFEFactory;
	class KFEGraphicsCmdQ;
	class KFEWindows;
	class KFEMonitor;
	class KFERTVHeap;
	class KFEDevice;

	typedef struct _KFE_SWAP_CHAIN_CREATE_DESC
	{
		KFEMonitor*		 Monitor;
		KFEFactory*		 Factory;
		KFEGraphicsCmdQ* GraphicsQueue;
		KFEWindows*		 Windows;
		std::uint16_t	 BufferCount;

		DXGI_FORMAT      BackBufferFormat;
		DXGI_FORMAT      DepthStencilFormat;

		KFE_WinSizeU     WindowSize;

		DXGI_SWAP_EFFECT SwapEffect;

		std::uint32_t    BufferUsage;
		std::uint32_t    SwapChainFlags;

		bool             EnableVSync;
		bool             AllowTearing;
		EScreenState     WindowState;

		KFEDevice*  Device;
		KFERTVHeap* RtvHeap;
	} KFE_SWAP_CHAIN_CREATE_DESC;

	typedef struct _KFE_SWAP_CHAIN_DATA 
	{
		D3D12_CPU_DESCRIPTOR_HANDLE BufferHandle;
		std::uint32_t				BufferIndex;
		ID3D12Resource*				BufferResource;
	} KFE_SWAP_CHAIN_DATA;

	class KFE_API KFESwapChain final: public IKFEObject
	{
	public:
		KFESwapChain () noexcept;
		~KFESwapChain() noexcept override;

		KFESwapChain(const KFESwapChain&)	  = delete;
		KFESwapChain(KFESwapChain&&) noexcept = delete;

		KFESwapChain& operator=(const KFESwapChain&)	 = delete;
		KFESwapChain& operator=(KFESwapChain&&) noexcept = delete;

		//~ Object Interface implementation
		std::string GetName		  () const noexcept override;
		std::string GetDescription() const noexcept override;

		NODISCARD bool Initialize(_In_ const KFE_SWAP_CHAIN_CREATE_DESC& desc);

		NODISCARD bool Destroy () noexcept;
		NODISCARD bool Recreate();
		NODISCARD bool Present ();
		NODISCARD bool Present(std::uint32_t syncInterval, std::uint32_t flags);

		//~ Buffers Life Cycle
		NODISCARD KFE_SWAP_CHAIN_DATA GetAndMarkBackBufferData(
			_In_ ID3D12Fence*  fence,
			_In_ std::uint64_t fenceValue
		) noexcept;

		NODISCARD bool			 HasRTVs   () const noexcept;
		NODISCARD KFERTVHeap*	 GetRTVHeap() const noexcept;

		NODISCARD std::uint32_t				  GetBackBufferRTVIndex (_In_ std::uint32_t bufferIndex) const noexcept;
		NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRTVHandle(_In_ std::uint32_t bufferIndex) const noexcept;
		NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTVHandle() const noexcept;

		//~ Setters
		NODISCARD bool SetScreenState(_In_ const EScreenState	state  );
		NODISCARD bool SetResolution (_In_ const KFE_WinSizeU&	winSize);
		NODISCARD bool SetTearing	 (_In_ const bool			tearing);

		void AttachFactory		(_In_opt_ KFEFactory*	   factory		) noexcept;
		void AttachMonitor		(_In_opt_ KFEMonitor*	   monitor		) noexcept;
		void AttachGraphicsQueue(_In_opt_ KFEGraphicsCmdQ* graphicsQueue) noexcept;
		void AttachWindows		(_In_opt_ KFEWindows*	   windows		) noexcept;

		// Buffer / format configuration setters (call recreate afterward)
		void SetBufferCount			(_In_ std::uint16_t		bufferCount     ) noexcept;
		void SetBackBufferFormat	(_In_ DXGI_FORMAT		backBufferFormat) noexcept;
		void SetDepthStencilFormat	(_In_ DXGI_FORMAT		depthFormat     ) noexcept;
		void SetSwapEffect			(_In_ DXGI_SWAP_EFFECT	swapEffect      ) noexcept;
		void SetBufferUsage			(_In_ std::uint32_t		bufferUsage     ) noexcept;
		void SetSwapChainFlags		(_In_ std::uint32_t		flags           ) noexcept;
		void SetVSyncEnabled		(_In_ bool				enableVSync     ) noexcept;
		void SetWindowState			(_In_ EScreenState		state           ) noexcept;

		//~ Getters
		NODISCARD bool                IsInitialize			 () const noexcept;
		NODISCARD std::uint16_t       GetBufferCount		 () const noexcept;
		NODISCARD DXGI_FORMAT         GetBufferFormat		 () const noexcept;
		NODISCARD DXGI_FORMAT         GetDepthStencilFormat	 () const noexcept;
		NODISCARD const KFE_WinSizeU& GetResolution			 () const noexcept;
		NODISCARD float               GetAspectRatio		 () const noexcept;
		NODISCARD bool                IsVSyncEnabled		 () const noexcept;
		NODISCARD bool                IsTearingOn			 () const noexcept;
		NODISCARD std::uint32_t       GetCurrentBackBufferIdx() const noexcept;
		NODISCARD IDXGISwapChain4*	  GetNative				 () const noexcept;
		NODISCARD bool				  IsReadyToCreate		 () const noexcept;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
} // namespace kfe
