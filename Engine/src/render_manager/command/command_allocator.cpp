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
#include "engine/render_manager/commands/command_allocator.h"

#include "engine/render_manager/components/device.h"
#include "engine/utils/logger.h"

#pragma region Impl_Declaration

class kfe::KFECommandAllocator::Impl
{
public:
	Impl () = default;
	~Impl()
	{ 
		if (!ForceDestroy())
		{
			LOG_ERROR("Failed To Destroy Command Allocator!");
		}
	}

	NODISCARD bool Initialize (_In_ const KFE_CA_CREATE_DESC& desc);
	NODISCARD bool AttachFence(_In_ const KFE_CA_ATTACH_FENCE fence);

	NODISCARD bool Reset		();
	NODISCARD bool ForceReset	();
	NODISCARD bool ForceWait	();
	NODISCARD bool IsFree		() const noexcept;
	NODISCARD bool ForceDestroy	();
	NODISCARD bool Destroy		();
	NODISCARD bool IsInitialized() const noexcept;

	NODISCARD _Ret_maybenull_ ID3D12CommandAllocator* GetNative() const;

private:
	NODISCARD bool CreateAllocator(_In_ const KFE_CA_CREATE_DESC& desc);
	NODISCARD bool CreateEventHandle();
	NODISCARD bool BlockThread		() const;

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator{ nullptr };
	ID3D12Fence* m_pAttachedFence{ nullptr };
	uint64_t     m_nWaitUntil    {    0u   };
	uint64_t     m_nBlockMaxTime {    5u   };
	HANDLE		 m_waitHandle	 { nullptr };
	bool		 m_bInitialized  {  false  };

};

#pragma endregion

#pragma region CommandAllocator_Implementation
kfe::KFECommandAllocator::KFECommandAllocator()
	: m_impl(std::make_unique<kfe::KFECommandAllocator::Impl>())
{}

kfe::KFECommandAllocator::~KFECommandAllocator() noexcept = default;

kfe::KFECommandAllocator::KFECommandAllocator(KFECommandAllocator&&) noexcept = default;
kfe::KFECommandAllocator& kfe::KFECommandAllocator::operator=(KFECommandAllocator&&) noexcept = default;

_Use_decl_annotations_
bool kfe::KFECommandAllocator::Initialize(const KFE_CA_CREATE_DESC& desc)
{
	return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::Reset()
{
	return m_impl->Reset();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::ForceReset()
{
	return m_impl->ForceReset();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::ForceWait()
{
	return m_impl->ForceWait();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::IsFree() const noexcept
{
	return m_impl->IsFree();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::AttachFence(const KFE_CA_ATTACH_FENCE fence)
{
	return m_impl->AttachFence(fence);
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::ForceDestroy()
{
	return m_impl->ForceDestroy();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::Destroy()
{
	return m_impl->Destroy();
}

_Use_decl_annotations_
ID3D12CommandAllocator* kfe::KFECommandAllocator::GetNative() const noexcept
{
	return m_impl->GetNative();
}

std::string kfe::KFECommandAllocator::GetName() const noexcept
{
	return "KFECommandAllocator";
}

std::string kfe::KFECommandAllocator::GetDescription() const noexcept
{
	return "DirectX12 Command Allocator";
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFECommandAllocator::Impl::Initialize(const KFE_CA_CREATE_DESC& desc)
{
	if (m_bInitialized) return true;

	if (!CreateAllocator(desc))
	{
		LOG_ERROR("Failed to Initialize Command Allocator!");
		return false;
	}

	if (!CreateEventHandle())
	{
		LOG_ERROR("Failed to Create Wait Event Handle!");
		return false;
	}

	m_bInitialized = true;

	return true;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::Impl::Reset()
{
	if (!IsFree()) return false;
	m_pCommandAllocator->Reset();
	return true;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::Impl::ForceReset()
{
	if (!IsInitialized()) return false;
	if (!ForceWait    ()) return false;
	
	return Reset();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::Impl::ForceWait()
{
	if (!IsInitialized()) return false;

	if (!m_pAttachedFence || m_nWaitUntil == 0u) return true;

	const auto completed = m_pAttachedFence->GetCompletedValue();

	if (completed >= m_nWaitUntil) return true;

	m_pAttachedFence->SetEventOnCompletion(m_nWaitUntil, m_waitHandle);
	
	LOG_WARNING("FORCED TO WAIT!");
	return BlockThread();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::Impl::IsFree() const noexcept
{
	if (!IsInitialized()) return false;
	if (m_nWaitUntil == 0u) return true;

	if (m_pAttachedFence)
	{
		if (m_pAttachedFence->GetCompletedValue() >= m_nWaitUntil)
		{
			return true;
		}
	}
	return false;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::Impl::AttachFence(const KFE_CA_ATTACH_FENCE fence)
{
	if (!fence.Fence) return false;
	if (!IsFree())    return false;

	m_pAttachedFence = fence.Fence;
	m_nWaitUntil     = fence.FenceWaitValue;

	return true;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::Impl::ForceDestroy()
{
	if (!IsInitialized()) return true;
	if (!ForceWait    ()) return false;
	if (!Destroy      ()) return false;
	return true;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::Impl::Destroy()
{
	if (!IsInitialized()) return true;
	if (!IsFree		  ()) return false;

	m_pAttachedFence = nullptr;
	m_nWaitUntil	 = 0u;
	m_pCommandAllocator.Reset();

	if (m_waitHandle)
	{
		CloseHandle(m_waitHandle);
		m_waitHandle = nullptr;
	}

	m_bInitialized = false;
	return true;
}

_Use_decl_annotations_
ID3D12CommandAllocator* kfe::KFECommandAllocator::Impl::GetNative() const
{
	return m_pCommandAllocator.Get();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::Impl::CreateAllocator(const KFE_CA_CREATE_DESC& desc)
{
	if (!desc.Device || !desc.Device->GetNative())
	{
		LOG_ERROR("Failed to Create Command Allocator Null Device Provided!");
		return false;
	}

	if (m_bInitialized)
	{
		LOG_WARNING("Attempting to create again! [Ignored]");
		return true;
	}

	auto native = desc.Device->GetNative();
	const HRESULT hr = native->CreateCommandAllocator(desc.CmdListType,
		IID_PPV_ARGS(&m_pCommandAllocator));

	if (FAILED(hr)) 
	{
		LOG_ERROR("Failed to Create Command Allocator!");
		return false;
	}

	LOG_SUCCESS("Created Command Allocator!");

	return true;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::Impl::CreateEventHandle()
{
	if (m_waitHandle) return true;

	m_waitHandle = CreateEventEx(
		nullptr,
		nullptr,
		0u, 
		EVENT_MODIFY_STATE | SYNCHRONIZE
	);

	if (!m_waitHandle)
	{
		LOG_ERROR("Failed to create Event Handle for Command Allocator! Win32 Error: {}", GetLastError());
		return false;
	}

	LOG_SUCCESS("Created Command Wait Event!");

	return true;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::Impl::BlockThread() const
{
	if (!m_waitHandle)
		return false;

	DWORD timeoutMs = static_cast<DWORD>(m_nBlockMaxTime * 1000ull);
	DWORD result = WaitForSingleObject(m_waitHandle, timeoutMs);

	switch (result)
	{
	case WAIT_OBJECT_0:
		return true;

	case WAIT_TIMEOUT:
		LOG_ERROR("CommandAllocator BlockThread(): Wait timed out after {} ms!",
			m_nBlockMaxTime);
		return false;

	default:
		LOG_ERROR("CommandAllocator BlockThread(): Wait failed! Win32 Error: {}",
			GetLastError());
		return false;
	}
}

_Use_decl_annotations_
bool kfe::KFECommandAllocator::Impl::IsInitialized() const noexcept
{
	return m_pCommandAllocator && m_bInitialized;
}

#pragma endregion
