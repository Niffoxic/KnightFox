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
#include "engine/render_manager/api/pool/allocator_pool.h"

#include "engine/utils/logger.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/commands/command_allocator.h"
#include "engine/system/exception/base_exception.h"

#pragma region Impl_Declaration

class kfe::KFECommandAllocatorPool::Impl
{
	using AllocatorPool	= std::unordered_map<KID, std::unique_ptr<KFECommandAllocator>>;
	
	enum class EAllocState: uint8_t
	{
		Free,
		Working,
	};
	using PoolTrack	= std::unordered_map<KID, EAllocState>;
public:
	 Impl() = default;
	~Impl()
	{
		if (!DestroyAllForce())
		{
			LOG_ERROR("Failed to Destroy Allocator Pool successfully!");
		}
	}

	NODISCARD bool Initialize(const KFE_CA_POOL_CREATE_DESC& desc);
	
	NODISCARD KFE_RET_MAYBE_NULL KFECommandAllocator* GetCommandAllocatorWait  ();
	NODISCARD KFE_RET_MAYBE_NULL KFECommandAllocator* GetCommandAllocatorCreate();
	
	NODISCARD bool IsAllBusy		() const noexcept;
	NODISCARD bool IsFreeAll		() const noexcept;
	NODISCARD bool IsFreeAny		() const noexcept;
	NODISCARD bool DestroyAll		();
	NODISCARD bool DestroyAllForce	();
	NODISCARD bool WaitAll			();
	NODISCARD bool WaitAny			();

	NODISCARD bool IsBusy		(const KID id) const noexcept;
	NODISCARD bool IsFree		(const KID id) const noexcept;
	NODISCARD bool Destroy		(const KID id);
	NODISCARD bool DestroyForce	(const KID id);
	NODISCARD bool Wait			(const KID id);

	void UpdateAllocators();

	// ~ Helpers
	NODISCARD bool IsInitialized() const noexcept;

private:
	NODISCARD
	bool CreateAllocator();
	void TrackStates    ();
	void DestroyPendings();
	void RemoveAllocator(KID id);

	//~ tracking
	KID GetFreePool() const;

private:
	D3D12_COMMAND_LIST_TYPE m_type	 { D3D12_COMMAND_LIST_TYPE_DIRECT };
	KFEDevice*				m_pDevice{ nullptr };
	AllocatorPool			m_mapAllocators	   {};
	PoolTrack			    m_mapAllocatorState{};
	std::unordered_set<KID> m_pendingDestroy   {};

	UINT m_nWaitTime   { 5u };
	UINT m_nMaxCounts  { 10u };
	bool m_bInitialized{ false };
};

#pragma endregion

#pragma region CA_Pool_Implementation

kfe::KFECommandAllocatorPool::KFECommandAllocatorPool() noexcept
	: m_impl(std::make_unique<kfe::KFECommandAllocatorPool::Impl>())
{}

kfe::KFECommandAllocatorPool::~KFECommandAllocatorPool() noexcept = default;

kfe::KFECommandAllocatorPool::KFECommandAllocatorPool(KFECommandAllocatorPool&&)				 noexcept = default;
kfe::KFECommandAllocatorPool& kfe::KFECommandAllocatorPool::operator=(KFECommandAllocatorPool&&) noexcept = default;

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Initialize(const KFE_CA_POOL_CREATE_DESC& desc)
{
	return m_impl->Initialize(desc);
}

void kfe::KFECommandAllocatorPool::UpdateAllocators()
{
	m_impl->UpdateAllocators();
}

_Use_decl_annotations_
kfe::KFECommandAllocator* kfe::KFECommandAllocatorPool::GetCommandAllocatorWait()
{
	return m_impl->GetCommandAllocatorWait();
}

_Use_decl_annotations_
kfe::KFECommandAllocator* kfe::KFECommandAllocatorPool::GetCommandAllocatorCreate()
{
	return m_impl->GetCommandAllocatorCreate();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::IsAllBusy() const noexcept
{
	return m_impl->IsAllBusy();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::IsFreeAll() const noexcept
{
	return m_impl->IsFreeAll();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::IsFreeAny() const noexcept
{
	return m_impl->IsFreeAny();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::DestroyAll()
{
	return m_impl->DestroyAll();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::DestroyAllForce()
{
	return m_impl->DestroyAllForce();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::WaitAll()
{
	return m_impl->WaitAll();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::WaitAny()
{
	return m_impl->WaitAny();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::IsBusy(KID id) const noexcept
{
	return m_impl->IsBusy(id);
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::IsFree(KID id) const noexcept
{
	return m_impl->IsFree(id);
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Destroy(KID id)
{
	return m_impl->Destroy(id);
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::DestroyForce(KID id)
{
	return m_impl->DestroyForce(id);
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Wait(KID id)
{
	return m_impl->Wait(id);
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::IsInitialized() const noexcept
{
	return m_impl->IsInitialized();
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Impl::Initialize(const KFE_CA_POOL_CREATE_DESC& desc)
{
	if (m_bInitialized) return true;
	
	m_pDevice	 = desc.Device;
	m_nWaitTime  = desc.BlockMaxTime;
	m_type		 = desc.CmdListType;
	m_nMaxCounts = desc.MaxCounts;

	int succesCounts = 0u;
	for (int i = 0; i < desc.InitialCounts; i++)
	{
		if (CreateAllocator()) ++succesCounts;
	}
	
	LOG_WARNING("Allocator Pool Creation Complete: {}/{}",
		succesCounts, desc.InitialCounts);

	TrackStates();

	m_bInitialized = true;
	return true;
}

_Use_decl_annotations_
kfe::KFECommandAllocator* kfe::KFECommandAllocatorPool::Impl::GetCommandAllocatorWait()
{
	if (!WaitAny())
	{
		if (!WaitAll()) return nullptr;
	}

	KID id = GetFreePool();
	if (id == 0u) return nullptr;

	LOG_WARNING("I'm giving {}, Allocator!", id);

	m_mapAllocatorState   [id] = EAllocState::Working;
	return m_mapAllocators[id].get();
}

_Use_decl_annotations_
kfe::KFECommandAllocator* kfe::KFECommandAllocatorPool::Impl::GetCommandAllocatorCreate()
{
	if (!WaitAny())
	{
		if (!CreateAllocator()) return nullptr;
	}

	KID id = GetFreePool();
	if (id == 0u) return nullptr;

	m_mapAllocatorState[id] = EAllocState::Working;
	return m_mapAllocators[id].get();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Impl::IsAllBusy() const noexcept
{
	for (auto& [id, state] : m_mapAllocatorState)
	{
		if (state != EAllocState::Working) return false;
	}
	return true;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Impl::IsFreeAll() const noexcept
{
	for (auto& [id, state] : m_mapAllocatorState)
	{
		if (state != EAllocState::Free) return false;
	}
	return true;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Impl::IsFreeAny() const noexcept
{
	return GetFreePool() > 0u;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Impl::DestroyAll()
{
	const int count = static_cast<int>(m_mapAllocators.size());
	int destroyed = 0;

	std::vector<KID> ids;
	ids.reserve(count);
	for (auto& [id, _] : m_mapAllocators)
		ids.push_back(id);

	for (KID id : ids)
	{
		if (Destroy(id))
			++destroyed;
	}

	return destroyed == count;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Impl::DestroyAllForce()
{
	if (m_mapAllocators.empty())
	{
		LOG_WARNING("No allocators in pool. Nothing to delete.");
		return true;
	}
	UINT poolSize = m_mapAllocators.size();

	if (!WaitAll   ()) return false;
	if (!DestroyAll()) return false;

	UINT newPoolSize = m_mapAllocators.size();
	LOG_SUCCESS("Destroyed {}/{} Allocators from the pool!",
		poolSize - newPoolSize ,
		poolSize);
	return true;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Impl::WaitAll()
{
	if (m_mapAllocators.empty())
	{
		LOG_WARNING("There's no Allocator to wait for in The pool");
		return false;
	}

	bool allOk = true;

	for (auto& [id, alloc] : m_mapAllocators)
	{
		if (alloc->ForceWait())
		{
			m_mapAllocatorState[id] = EAllocState::Free;
		}
		else
		{
			allOk = false;
		}
	}

	return allOk;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Impl::WaitAny()
{
	if (m_mapAllocators.empty()) return false;

	if (GetFreePool() != 0u) // already tracked a free one
		return true;

	DWORD remainingMs = m_nWaitTime * 1000u;

	while (remainingMs > 0)
	{
		bool found = false;
		for (auto& [id, alloc] : m_mapAllocators)
		{
			if (alloc->IsFree())
			{
				m_mapAllocatorState[id] = EAllocState::Free;
				found = true;
			}
		}

		if (found) return true;

		Sleep(1);
		--remainingMs;
	}

	return false;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Impl::IsBusy(KID id) const noexcept
{
	if (!m_mapAllocators.contains(id)) return false;
	return !m_mapAllocators.at(id)->IsFree();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Impl::IsFree(KID id) const noexcept
{
	if (!m_mapAllocators.contains(id)) return false;
	return m_mapAllocators.at(id)->IsFree();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Impl::Destroy(KID id)
{
	if (!m_mapAllocators.contains(id))
	{
		LOG_ERROR("Failed to Destroy for {}, No Such Allocator Found!", id);
		return false;
	}

	auto* alloc = m_mapAllocators[id].get();

	if (alloc->IsFree())
	{
		RemoveAllocator(id);
		return true;
	}

	m_pendingDestroy.insert(id);
	return true;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Impl::DestroyForce(KID id)
{
	if (!m_mapAllocators.contains(id))
		return false;

	bool ok = m_mapAllocators[id]->ForceDestroy();
	RemoveAllocator(id);
	return ok;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Impl::Wait(KID id)
{
	if (!m_mapAllocators.contains(id))
	{
		LOG_ERROR("Failed to Wait for {}, No Such Allocator Found!", id);
		return false;
	}

	bool ok = m_mapAllocators[id]->ForceWait();
	if (ok)
	{
		m_mapAllocatorState[id] = EAllocState::Free;
	}
	return ok;
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Impl::IsInitialized() const noexcept
{
	return m_bInitialized;
}

void kfe::KFECommandAllocatorPool::Impl::UpdateAllocators()
{
	TrackStates	   ();
	DestroyPendings();
}

_Use_decl_annotations_
bool kfe::KFECommandAllocatorPool::Impl::CreateAllocator()
{
	if (m_mapAllocators.size() >= m_nMaxCounts)
	{
		LOG_WARNING("Allocator pool at MaxCounts ({}). Cannot create more.", m_nMaxCounts);
		return false;
	}

	if (!m_pDevice || !m_pDevice->GetNative())
	{
		THROW_MSG("KFECommandAllocatorPool::Impl::CreateAllocator: Device is null or not initialized!");
		return false;
	}

	auto alloc = std::make_unique<KFECommandAllocator>();

	KFE_CA_CREATE_DESC desc{};
	desc.BlockMaxTime = m_nWaitTime;
	desc.CmdListType  = m_type;
	desc.Device		  = m_pDevice;

	if (!alloc->Initialize(desc))
	{
		LOG_ERROR("Failed To Create Allocator (KFECommandAllocatorPool)!");
		return false;
	}

	LOG_SUCCESS("Created Allocator (KFECommandAllocatorPool)!");

	KID id = alloc->GetAssignedKey();
	m_mapAllocatorState[id] = EAllocState::Free;
	m_mapAllocators    [id]	= std::move(alloc);

	return true;
}

void kfe::KFECommandAllocatorPool::Impl::TrackStates()
{
	for (auto& [id, state] : m_mapAllocatorState) 
	{
		if (IsFree(id))
		{
			state = EAllocState::Free;
		}
		else state = EAllocState::Working;
	}
}

void kfe::KFECommandAllocatorPool::Impl::DestroyPendings()
{
	for (auto it = m_pendingDestroy.begin(); it != m_pendingDestroy.end(); )
	{
		KID id = *it;

		if (IsFree(id))
		{
			RemoveAllocator(id);
			LOG_INFO("Remove Pending Allocator: {}", id);
			it = m_pendingDestroy.erase(it);
		}
		else ++it;
	}
}

void kfe::KFECommandAllocatorPool::Impl::RemoveAllocator(KID id)
{
	if (!m_mapAllocators.contains(id)) return;
	m_mapAllocators		.erase(id);
	m_mapAllocatorState .erase(id);
}

kfe::KID kfe::KFECommandAllocatorPool::Impl::GetFreePool() const
{
	for (auto& [id, state] : m_mapAllocatorState)
	{
		if (state == EAllocState::Free) return id;
	}
	return 0u;
}

#pragma endregion
