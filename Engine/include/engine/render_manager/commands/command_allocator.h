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

struct ID3D12CommandAllocator;
struct ID3D12Fence;
enum   D3D12_COMMAND_LIST_TYPE;

namespace kfe 
{
	class KFEDevice;

	typedef struct _KFE_CA_CREATE_DESC
	{
		KFEDevice*				Device;
		D3D12_COMMAND_LIST_TYPE	CmdListType;
		uint64_t				BlockMaxTime{ 5u };
	} KFE_CA_CREATE_DESC;

	typedef struct _KFE_CA_ATTACH_FENCE
	{
		ID3D12Fence* Fence;
		uint64_t	 FenceWaitValue;
	} KFE_CA_ATTACH_FENCE;

	/// <summary>
	/// DirectX Command allocator Wrapper
	/// </summary>
	class KFE_API KFECommandAllocator final: public IKFEObject
	{
	public:
		 KFECommandAllocator();
		 ~KFECommandAllocator() noexcept override;

		KFECommandAllocator(const KFECommandAllocator&) = delete;
		KFECommandAllocator(KFECommandAllocator&&) noexcept;

		KFECommandAllocator& operator=(const KFECommandAllocator&) = delete;
		KFECommandAllocator& operator=(KFECommandAllocator&&) noexcept;

		//~ Inherited IKObjects
		std::string GetName		  () const noexcept override;
		std::string GetDescription() const noexcept override;

		// Initialize allocator
		NODISCARD bool Initialize(_In_ const KFE_CA_CREATE_DESC& desc);

		// If reset is possible, resets. Otherwise returns false
		NODISCARD bool Reset	 ();

		// If Reset is not possible, force CPU wait + reset
		NODISCARD bool ForceReset();

		// Hard blocking: waits until GPU finishes (needs valid fence)
		NODISCARD bool ForceWait ();

		// Non-blocking: true if fence has passed
		NODISCARD bool IsFree	 () const noexcept;

		// Attach fence for safe recycling
		NODISCARD bool AttachFence(_In_ const KFE_CA_ATTACH_FENCE& fence);

		// Hard destroy: always blocks
		NODISCARD bool ForceDestroy();

		// Safe destroy: destroys only if GPU completed
		NODISCARD bool Destroy	   ();

		// Returns the native DX12 allocator
		NODISCARD _Ret_maybenull_ ID3D12CommandAllocator* GetNative() const noexcept;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
} // namespace kfe
