#pragma once

#include "EngineAPI.h"

#include <memory>
#include <cstdint>

#include "engine/core/key_generator/key_generator.h"

class ID3D12CommandAllocator;
class ID3D12Fence;
enum  D3D12_COMMAND_LIST_TYPE;

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
	class KFE_API KFECommandAllocator
	{
		KEYGEN_CLASS();
	public:
		 KFECommandAllocator();
		~KFECommandAllocator();

		[[nodiscard]] bool Initialize(const KFE_CA_CREATE_DESC& desc);

		// if Reset is possible then clean data else returns false
		[[nodiscard]] bool Reset();

		// if Reset is not possible force the cpu to wait and then reset.
		[[nodiscard]] bool ForceReset();

		// blocks thread making CPU Idle until GPU is Finished
		// works only if a valid fence and fence value is attached
		[[nodiscard]] bool ForceWait();

		// non blocking completion check: true if FV is already passed else false
		[[nodiscard]] bool IsFree() const;

		// attach a fence desc to indicate when to free and safe handling
		[[nodiscard]] bool AttachFence(const KFE_CA_ATTACH_FENCE& fence);

		// hard block untill all the allocators processed and then released resources
		[[nodiscard]] bool ForceDestroy();

		// CPU block free destroy, only destroys itself if safe
		[[nodiscard]] bool Destroy();

		// returns DirectX Command Allocator
		[[nodiscard]] ID3D12CommandAllocator* GetNative() const;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
} // namespace kfe
