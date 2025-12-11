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
#include "engine/render_manager/api/commands/graphics_list.h"

#include "engine/utils/logger.h"
#include "engine/system/exception/base_exception.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/commands/command_allocator.h"
#include "engine/render_manager/api/pool/allocator_pool.h"

#pragma region Impl_Declaration

class kfe::KFEGraphicsCommandList::Impl 
{
public:
	 Impl() = default;
	~Impl() = default;

    NODISCARD bool Initialize(_In_ const KFE_GFX_COMMAND_LIST_CREATE_DESC& desc);
    NODISCARD bool Reset	 (_In_ const KFE_RESET_COMMAND_LIST& reset);
   
	NODISCARD bool Close  () noexcept;
	NODISCARD bool Destroy() noexcept;

    NODISCARD ID3D12GraphicsCommandList* GetNative    () const noexcept;
    NODISCARD bool                       IsInitialized() const noexcept;

	void Update() noexcept;
	void Wait  () noexcept;

private:
	bool CreateAllocatorPool(const KFE_GFX_COMMAND_LIST_CREATE_DESC& desc);

private:
	inline static constexpr D3D12_COMMAND_LIST_TYPE   m_type{ D3D12_COMMAND_LIST_TYPE_DIRECT };
	bool											  m_bInitialized{ false };
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_pList{ nullptr };
	std::unique_ptr<KFECommandAllocatorPool>		  m_pPool{ nullptr };
	KFECommandAllocator*							  m_pCurrentAllocator{ nullptr };
};

#pragma endregion

#pragma region GCL_Implementation

kfe::KFEGraphicsCommandList::KFEGraphicsCommandList() noexcept
	: m_impl(std::make_unique<kfe::KFEGraphicsCommandList::Impl>())
{}

kfe::KFEGraphicsCommandList::~KFEGraphicsCommandList() = default;

kfe::KFEGraphicsCommandList::KFEGraphicsCommandList(KFEGraphicsCommandList&&)				  noexcept = default;
kfe::KFEGraphicsCommandList& kfe::KFEGraphicsCommandList::operator=(KFEGraphicsCommandList&&) noexcept = default;

std::string kfe::KFEGraphicsCommandList::GetName() const noexcept
{
	return "KFEGraphicsCommandList";
}

std::string kfe::KFEGraphicsCommandList::GetDescription() const noexcept
{
	return "DirectX 12: Graphics Command List";
}

_Use_decl_annotations_
bool kfe::KFEGraphicsCommandList::Initialize(const KFE_GFX_COMMAND_LIST_CREATE_DESC& desc)
{
	return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFEGraphicsCommandList::Reset(const KFE_RESET_COMMAND_LIST& reset)
{
	return m_impl->Reset(reset);
}

_Use_decl_annotations_
bool kfe::KFEGraphicsCommandList::Close() noexcept
{
	return m_impl->Close();
}

_Use_decl_annotations_
bool kfe::KFEGraphicsCommandList::Destroy() noexcept
{
	return m_impl->Destroy();
}

_Use_decl_annotations_
ID3D12GraphicsCommandList* kfe::KFEGraphicsCommandList::GetNative() const noexcept
{
	return m_impl->GetNative();
}

_Use_decl_annotations_
bool kfe::KFEGraphicsCommandList::IsInitialized() const noexcept
{
	return m_impl->IsInitialized();
}

void kfe::KFEGraphicsCommandList::Update() noexcept
{
	m_impl->Update();
}

void kfe::KFEGraphicsCommandList::Wait() noexcept
{
	m_impl->Wait();
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFEGraphicsCommandList::Impl::Initialize(const KFE_GFX_COMMAND_LIST_CREATE_DESC& desc)
{
	if (m_bInitialized)
		return true;

	if (!desc.Device || !desc.Device->GetNative())
	{
		LOG_ERROR("Failed to initialize Graphics list: device is null!");
		return false;
	}

	if (!CreateAllocatorPool(desc))
	{
		LOG_ERROR("Failed to initialize Graphics List: Alloc Pool Failed!");
		return false;
	}

	auto* allocObj = m_pPool->GetCommandAllocatorWait();
	if (!allocObj)
	{
		LOG_ERROR("Allocator pool returned null allocator during GraphicsList initialization!");
		return false;
	}

	auto* alloc = allocObj->GetNative();
	auto* native = desc.Device->GetNative();

	const HRESULT hr = native->CreateCommandList(
					   0u,
					   m_type,
					   alloc,
					   nullptr,
					   IID_PPV_ARGS(&m_pList));

	if (FAILED(hr))
	{
		LOG_ERROR("Failed to Create Command List!");
		return false;
	}

	m_pList->Close();

	m_bInitialized = true;
	return true;
}

_Use_decl_annotations_
bool kfe::KFEGraphicsCommandList::Impl::Reset(const KFE_RESET_COMMAND_LIST& reset)
{
	if (!m_pPool)
	{
		THROW_MSG("Failed to reset: no allocator pool is present!");
		return false;
	}

	// Try to get a free allocator
	m_pCurrentAllocator = m_pPool->GetCommandAllocatorWait();
	if (!m_pCurrentAllocator)
	{
		LOG_WARNING("No free allocator from pool, trying to create a new one.");
		m_pCurrentAllocator = m_pPool->GetCommandAllocatorCreate();

		if (!m_pCurrentAllocator)
		{
			THROW_MSG("Failed to acquire allocator: no free or new allocators available!");
			return false;
		}
	}

	// Make sure allocator has a native pointer
	if (!m_pCurrentAllocator->GetNative())
	{
		THROW_MSG("Allocator has no native ID3D12CommandAllocator*!");
		return false;
	}

	// Reset allocator before reusing it for the command list
	if (!m_pCurrentAllocator->Reset())
	{
		THROW_MSG("Failed to reset command allocator in KFEGraphicsCommandList::Impl::Reset!");
		return false;
	}

	auto* nativeAlloc = m_pCurrentAllocator->GetNative();

	// Reset the graphics command list with allocator
	HRESULT hr = m_pList->Reset(nativeAlloc, reset.PSO);
	if (FAILED(hr))
	{
		LOG_ERROR("Graphics List Reset FAILED! HRESULT = 0x{:08X}", static_cast<unsigned>(hr));
		return false;
	}

	// Attach fence info so allocator knows when it's safe to reuse
	if (reset.Fence)
	{
		KFE_CA_ATTACH_FENCE fence{};
		fence.Fence = reset.Fence;
		fence.FenceWaitValue = reset.FenceValue;

		if (!m_pCurrentAllocator->AttachFence(fence))
		{
			LOG_WARNING("Failed to attach fence to allocator after graphics list reset.");
		}
	}
	return true;
}

_Use_decl_annotations_
bool kfe::KFEGraphicsCommandList::Impl::Close() noexcept
{
	if (!m_pList)
		return false;

	const HRESULT hr = m_pList->Close();
	
	if (FAILED(hr))
	{
		LOG_ERROR("Failed to Close Command List! 'KFEGraphicsCommandList::Impl::Close()'");
		return false;
	}

	return true;
}

_Use_decl_annotations_
bool kfe::KFEGraphicsCommandList::Impl::Destroy() noexcept
{
	if (!m_bInitialized)
		return true;

	if (m_pPool)
	{
		if (!m_pPool->DestroyAllForce())
		{
			LOG_ERROR("KFEGraphicsCommandList::Impl::Destroy(): DestroyAllForce() failed.");
		}
		m_pPool.reset();
	}

	m_pList.Reset();
	m_bInitialized = false;
	return true;
}

_Use_decl_annotations_
ID3D12GraphicsCommandList* kfe::KFEGraphicsCommandList::Impl::GetNative() const noexcept
{
	return m_pList.Get();
}

_Use_decl_annotations_
bool kfe::KFEGraphicsCommandList::Impl::IsInitialized() const noexcept
{
	return m_bInitialized;
}

bool kfe::KFEGraphicsCommandList::Impl::CreateAllocatorPool(const KFE_GFX_COMMAND_LIST_CREATE_DESC& desc)
{
	if (m_pPool) return true;

	m_pPool = std::make_unique<KFECommandAllocatorPool>();

	KFE_CA_POOL_CREATE_DESC pool{};
	pool.BlockMaxTime	= desc.BlockMaxTime;
	pool.CmdListType	= m_type;
	pool.Device			= desc.Device;
	pool.InitialCounts  = desc.InitialCounts;
	pool.MaxCounts		= desc.MaxCounts;

	return m_pPool->Initialize(pool);
}

void kfe::KFEGraphicsCommandList::Impl::Update() noexcept
{
	if (m_pPool) 
	{
		m_pPool->UpdateAllocators();
	}
}

void kfe::KFEGraphicsCommandList::Impl::Wait() noexcept
{
	if (m_pCurrentAllocator) 
	{
		m_pCurrentAllocator->ForceWait();
	}
}
#pragma endregion
