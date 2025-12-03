#pragma once
#include "EngineAPI.h"

#include <cstdint>
#include <memory>

#include "engine/core/common_types.h"
#include "engine/core/core.h"

enum D3D12_COMMAND_LIST_TYPE;

namespace kfe
{
	class KFEDevice;
	class KFECommandAllocator;

	typedef struct _KFE_CA_POOL_CREATE_DESC
	{
		KFEDevice*				Device;
		D3D12_COMMAND_LIST_TYPE	CmdListType;
		uint64_t				BlockMaxTime { 5u };
		uint64_t				InitialCounts{ 3u };
		uint64_t				MaxCounts    { 10u };
	} KFE_CA_POOL_CREATE_DESC;

	/// <summary>
		/// Command Allocator Pool
		/// </summary>
	class KFE_API KFECommandAllocatorPool
	{
	public:
		 KFECommandAllocatorPool() noexcept;
		~KFECommandAllocatorPool() noexcept;

		KFECommandAllocatorPool(const KFECommandAllocatorPool&) = delete;
		KFECommandAllocatorPool(KFECommandAllocatorPool&&) noexcept;

		KFECommandAllocatorPool& operator=(const KFECommandAllocatorPool&) = delete;
		KFECommandAllocatorPool& operator=(KFECommandAllocatorPool&&) noexcept;

		NODISCARD bool Initialize(_In_ const KFE_CA_POOL_CREATE_DESC& desc);
		
		// Must be called per frame
		void UpdateAllocators();

		// Force cpu to wait if all the allocators are busy
		NODISCARD _Ret_maybenull_ KFECommandAllocator* GetCommandAllocatorWait  ();

		// Create another allocator iff all the allocators are busy 
		NODISCARD _Ret_maybenull_ KFECommandAllocator* GetCommandAllocatorCreate();

		NODISCARD bool IsAllBusy		() const noexcept;
		NODISCARD bool IsFreeAll		() const noexcept;
		NODISCARD bool IsFreeAny		() const noexcept;
		NODISCARD bool DestroyAll		();
		NODISCARD bool DestroyAllForce	();
		NODISCARD bool WaitAll			();
		NODISCARD bool WaitAny			();

		NODISCARD bool IsBusy		(_In_ KID id) const noexcept;
		NODISCARD bool IsFree		(_In_ KID id) const noexcept;
		NODISCARD bool Destroy		(_In_ KID id);
		NODISCARD bool DestroyForce	(_In_ KID id);
		NODISCARD bool Wait			(_In_ KID id);

		// Helpers
		NODISCARD bool IsInitialized() const noexcept;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
} // namespace kfe
