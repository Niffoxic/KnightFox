#include "pch.h"
#include "command_allocator.h"

#include "engine/render_manager/components/device/device.h"
#include "engine/utils/logger/logger.h"

#include <d3d12.h>
#include <wrl/client.h>

//~ Impl Declaration
class kfe::KFECommandAllocator::Impl
{
public:
	Impl() = default;
	~Impl()
	{ 
		if (!ForceDestroy())
		{
			LOG_ERROR("Failed To Destroy Command Allocator!");
		}
	}

	[[nodiscard]] bool Initialize(const KFE_CA_CREATE_DESC& desc);
	[[nodiscard]] bool AttachFence(const KFE_CA_ATTACH_FENCE& fence);

	[[nodiscard]] bool Reset	   ();
	[[nodiscard]] bool ForceReset  ();
	[[nodiscard]] bool ForceWait   ();
	[[nodiscard]] bool IsFree	   () const;
	[[nodiscard]] bool ForceDestroy();
	[[nodiscard]] bool Destroy     ();

	[[nodiscard]] ID3D12CommandAllocator* GetNative() const;

private:
	[[nodiscard]] bool CreateAllocator  ();
	[[nodiscard]] bool CreateEventHandle();

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator{ nullptr };
	ID3D12Fence* m_pAttachedFence{ nullptr };
	uint64_t     m_nWaitUntil    {    0u   };
	HANDLE		 m_waitHandle	 { nullptr };
};

//~ KFE Command Allocator Implementation
kfe::KFECommandAllocator::KFECommandAllocator()
{

}

kfe::KFECommandAllocator::~KFECommandAllocator()
{
}

bool kfe::KFECommandAllocator::Initialize(const KFE_CA_CREATE_DESC& desc)
{
	return false;
}

bool kfe::KFECommandAllocator::Reset()
{
	return false;
}

bool kfe::KFECommandAllocator::ForceReset()
{
	return false;
}

bool kfe::KFECommandAllocator::ForceWait()
{
	return false;
}

bool kfe::KFECommandAllocator::IsFree() const
{
	return false;
}

bool kfe::KFECommandAllocator::AttachFence(const KFE_CA_ATTACH_FENCE& fence)
{
	return false;
}

bool kfe::KFECommandAllocator::ForceDestroy()
{
	return false;
}

bool kfe::KFECommandAllocator::Destroy()
{
	return false;
}

ID3D12CommandAllocator* kfe::KFECommandAllocator::GetNative() const
{

}

//~ Impl Implementation
bool kfe::KFECommandAllocator::Impl::Initialize(const KFE_CA_CREATE_DESC& desc)
{
	return false;
}

bool kfe::KFECommandAllocator::Impl::Reset()
{
	return false;
}

bool kfe::KFECommandAllocator::Impl::ForceReset()
{
	return false;
}

bool kfe::KFECommandAllocator::Impl::ForceWait()
{
	return false;
}

bool kfe::KFECommandAllocator::Impl::IsFree() const
{
	return false;
}

bool kfe::KFECommandAllocator::Impl::AttachFence(const KFE_CA_ATTACH_FENCE& fence)
{
	return false;
}

bool kfe::KFECommandAllocator::Impl::ForceDestroy()
{
	return false;
}

bool kfe::KFECommandAllocator::Impl::Destroy()
{
	return false;
}

ID3D12CommandAllocator* kfe::KFECommandAllocator::Impl::GetNative() const
{

}

bool kfe::KFECommandAllocator::Impl::CreateAllocator()
{
	return false;
}

bool kfe::KFECommandAllocator::Impl::CreateEventHandle()
{
	return false;
}
