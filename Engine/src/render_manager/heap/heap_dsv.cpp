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
#include "engine/render_manager/heap/heap_dsv.h"
#include "engine/render_manager/components/device.h"

#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"

#pragma region Impl_Declaration

class kfe::KFEDSVHeap::Impl
{
public:
	static constexpr std::uint32_t InvalidIndex = 0xFFFFFFFFu;

	Impl() = default;
	~Impl()
	{
		if (!Destroy())
		{
			LOG_ERROR("Failed to Destroy kfe::KFEDSVHeap::Impl Successfully!");
		}
	}

	bool Initialize(const KFE_DSV_HEAP_CREATE_DESC& desc);

	bool		  Destroy			()		 noexcept;
	bool          IsInitialized		() const noexcept;
	std::uint32_t GetNumDescriptors	() const noexcept;
	std::uint32_t GetAllocatedCount	() const noexcept;
	std::uint32_t GetRemaining		() const noexcept;
	std::uint32_t GetHandleSize		() const noexcept;
	std::uint32_t Allocate			()		 noexcept;
	bool		  Reset				()		 noexcept;

	KFE_CPU_DESCRIPTOR_HANDLE GetStartHandle()					  const noexcept;
	KFE_CPU_DESCRIPTOR_HANDLE GetHandle		(std::uint32_t index) const noexcept;
	bool					  Free			(std::uint32_t index)		noexcept;
	bool					  IsValidIndex	(std::uint32_t idx	) const noexcept;

	ID3D12DescriptorHeap* GetNative	  ()					   const noexcept;
	void				  SetDebugName(_In_ const std::string& name) noexcept;

private:
	KFE_CPU_DESCRIPTOR_HANDLE ComputeHandle(std::uint32_t index) const noexcept;

	void ClearState() noexcept;
	bool HasHeap() const noexcept;

private:
	inline static constexpr D3D12_DESCRIPTOR_HEAP_TYPE m_type{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV };

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap{ nullptr };
	KFEDevice* m_pDevice{ nullptr };

	//~ configurations
	std::uint32_t m_nCapacity  { 0u };
	std::uint32_t m_nAllocated { 0u };
	std::uint32_t m_nHandleSize{ 0u };

	KFE_CPU_DESCRIPTOR_HANDLE  m_handleStart{};
	std::vector<EWorkState>    m_workStates {};

	bool m_bInitialized{ false };
	
	//~ debugs
	std::string m_szDebugName{};
};

#pragma endregion

#pragma region DSV_Heap_Implementation

kfe::KFEDSVHeap::KFEDSVHeap() noexcept
	: m_impl(std::make_unique<kfe::KFEDSVHeap::Impl>())
{
}

kfe::KFEDSVHeap::~KFEDSVHeap()							  noexcept = default;
kfe::KFEDSVHeap::KFEDSVHeap(KFEDSVHeap&&)				  noexcept = default;

kfe::KFEDSVHeap& kfe::KFEDSVHeap::operator=(KFEDSVHeap&&) noexcept = default;

std::string kfe::KFEDSVHeap::GetName() const noexcept
{
	return "KFEDSVHeap";
}

std::string kfe::KFEDSVHeap::GetDescription() const noexcept
{
	if (!m_impl || !m_impl->IsInitialized())
		return "DSV Heap: Not Initialized";

	return std::format(
		"DSV Heap | Capacity: {} | Allocated: {} | Remaining: {} | HandleSize: {} bytes",
		m_impl->GetNumDescriptors(),
		m_impl->GetAllocatedCount(),
		m_impl->GetRemaining(),
		m_impl->GetHandleSize()
	);
}

bool kfe::KFEDSVHeap::Initialize(const KFE_DSV_HEAP_CREATE_DESC& desc)
{
	return m_impl->Initialize(desc);
}

bool kfe::KFEDSVHeap::Destroy() noexcept
{
	return m_impl->Destroy();
}

bool kfe::KFEDSVHeap::IsInitialized() const noexcept
{
	return m_impl->IsInitialized();
}

std::uint32_t kfe::KFEDSVHeap::GetNumDescriptors() const noexcept
{
	return m_impl->GetNumDescriptors();
}

std::uint32_t kfe::KFEDSVHeap::GetAllocatedCount() const noexcept
{
	return m_impl->GetAllocatedCount();
}

std::uint32_t kfe::KFEDSVHeap::GetRemaining() const noexcept
{
	return m_impl->GetRemaining();
}

std::uint32_t kfe::KFEDSVHeap::GetHandleSize() const noexcept
{
	return m_impl->GetHandleSize();
}

KFE_CPU_DESCRIPTOR_HANDLE kfe::KFEDSVHeap::GetStartHandle() const noexcept
{
	return m_impl->GetStartHandle();
}

KFE_CPU_DESCRIPTOR_HANDLE kfe::KFEDSVHeap::GetHandle(std::uint32_t index) const noexcept
{
	return m_impl->GetHandle(index);
}

std::uint32_t kfe::KFEDSVHeap::Allocate() noexcept
{
	return m_impl->Allocate();
}

bool kfe::KFEDSVHeap::Free(std::uint32_t index) noexcept
{
	return m_impl->Free(index);
}

bool kfe::KFEDSVHeap::Reset() noexcept
{
	return m_impl->Reset();
}

bool kfe::KFEDSVHeap::IsValidIndex(std::uint32_t idx) const noexcept
{
	return m_impl->IsValidIndex(idx);
}

ID3D12DescriptorHeap* kfe::KFEDSVHeap::GetNative() const noexcept
{
	return m_impl->GetNative();
}

void kfe::KFEDSVHeap::SetDebugName(const std::string& name) noexcept
{
	return m_impl->SetDebugName(name);
}

#pragma endregion

#pragma region Impl_Implementation

bool kfe::KFEDSVHeap::Impl::Initialize(const KFE_DSV_HEAP_CREATE_DESC& desc)
{
	//~ Validate inputs
	if (desc.Device == nullptr)
	{
		LOG_ERROR("KFEDSVHeap::Impl::Initialize: Invalid argument. Device is nullptr.");
		return false;
	}

	if (desc.DescriptorCounts == 0u)
	{
		LOG_ERROR("KFEDSVHeap::Impl::Initialize: DescriptorCounts must be greater than 0.");
		return false;
	}

	if (IsInitialized())
	{
		LOG_WARNING("KFEDSVHeap::Impl::Initialize: Already initialized. Destroying existing heap.");
		Destroy();
	}

	ClearState();
	m_pDevice = desc.Device;

	auto* nativeDevice = m_pDevice->GetNative();

	if (nativeDevice == nullptr)
	{
		LOG_ERROR("KFEDSVHeap::Impl::Initialize: Native D3D12 device is nullptr.");
		m_pDevice = nullptr;
		return false;
	}

	// Describe DSV descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = m_type;
	heapDesc.NumDescriptors = desc.DescriptorCounts;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0u;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
	const HRESULT hr = nativeDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));

	if (FAILED(hr))
	{
		LOG_ERROR(
			"KFEDSVHeap::Impl::Initialize: Failed to create DSV descriptor heap. HRESULT = 0x{:08X}",
			static_cast<unsigned int>(hr)
		);
		m_pDevice = nullptr;
		return false;
	}

	m_pDescriptorHeap = std::move(heap);
	m_nCapacity = desc.DescriptorCounts;
	m_nHandleSize = nativeDevice->GetDescriptorHandleIncrementSize(m_type);

	const auto startHandle = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_handleStart.ptr = startHandle.ptr;

	//~ reset states
	m_workStates.clear();
	m_workStates.resize(m_nCapacity, EWorkState::Free);

	m_nAllocated = 0u;

	if (desc.DebugName != nullptr)
	{
		m_szDebugName = desc.DebugName;
		SetDebugName(m_szDebugName);
	}

	m_bInitialized = true;

	LOG_SUCCESS(
		"KFEDSVHeap::Impl::Initialize: DSV heap created. Capacity = {}, HandleSize = {} bytes.",
		m_nCapacity,
		m_nHandleSize
	);

	return true;
}

bool kfe::KFEDSVHeap::Impl::Destroy() noexcept
{
	if (!HasHeap())
	{
		ClearState();
		return true;
	}

	LOG_INFO("KFEDSVHeap::Impl::Destroy: Releasing DSV descriptor heap.");

	m_pDescriptorHeap.Reset();
	ClearState();

	return true;
}

bool kfe::KFEDSVHeap::Impl::IsInitialized() const noexcept
{
	return HasHeap();
}

std::uint32_t kfe::KFEDSVHeap::Impl::GetNumDescriptors() const noexcept
{
	return m_nCapacity;
}

std::uint32_t kfe::KFEDSVHeap::Impl::GetAllocatedCount() const noexcept
{
	return m_nAllocated;
}

std::uint32_t kfe::KFEDSVHeap::Impl::GetRemaining() const noexcept
{
	if (m_nCapacity <= m_nAllocated)
	{
		return 0u;
	}
	return m_nCapacity - m_nAllocated;
}

std::uint32_t kfe::KFEDSVHeap::Impl::GetHandleSize() const noexcept
{
	return m_nHandleSize;
}

KFE_CPU_DESCRIPTOR_HANDLE kfe::KFEDSVHeap::Impl::GetStartHandle() const noexcept
{
	return m_handleStart;
}

KFE_CPU_DESCRIPTOR_HANDLE kfe::KFEDSVHeap::Impl::GetHandle(std::uint32_t index) const noexcept
{
	if (!IsValidIndex(index))
	{
		LOG_ERROR(
			"KFEDSVHeap::Impl::GetHandle: Invalid index {}. Capacity = {}.",
			index,
			m_nCapacity
		);
		return KFE_CPU_DESCRIPTOR_HANDLE{};
	}

	return ComputeHandle(index);
}

std::uint32_t kfe::KFEDSVHeap::Impl::Allocate() noexcept
{
	if (!IsInitialized())
	{
		LOG_ERROR("KFEDSVHeap::Impl::Allocate: Heap is not initialized.");
		return InvalidIndex;
	}

	if (m_nAllocated >= m_nCapacity)
	{
		LOG_WARNING(
			"KFEDSVHeap::Impl::Allocate: No more descriptors available. Capacity = {}.",
			m_nCapacity
		);
		return InvalidIndex;
	}

	//~ search for a free slot
	for (std::uint32_t i = 0; i < m_nCapacity; ++i)
	{
		if (m_workStates[i] == EWorkState::Free)
		{
			m_workStates[i] = EWorkState::Working;
			++m_nAllocated;

			LOG_INFO(
				"KFEDSVHeap::Impl::Allocate: Allocated descriptor index {}. Allocated = {}, Remaining = {}.",
				i,
				m_nAllocated,
				GetRemaining()
			);

			return i;
		}
	}

	LOG_ERROR(
		"KFEDSVHeap::Impl::Allocate: Failed to find free descriptor despite remaining count > 0. "
		"Capacity = {}, Allocated = {}.",
		m_nCapacity,
		m_nAllocated
	);

	return InvalidIndex;
}

bool kfe::KFEDSVHeap::Impl::Free(std::uint32_t index) noexcept
{
	if (!IsInitialized())
	{
		LOG_ERROR("KFEDSVHeap::Impl::Free: Heap is not initialized.");
		return false;
	}

	if (!IsValidIndex(index))
	{
		LOG_ERROR(
			"KFEDSVHeap::Impl::Free: Invalid index {}. Capacity = {}.",
			index,
			m_nCapacity
		);
		return false;
	}

	if (m_workStates[index] == EWorkState::Free)
	{
		LOG_WARNING(
			"KFEDSVHeap::Impl::Free: Descriptor index {} is already free.",
			index
		);
		return false;
	}

	m_workStates[index] = EWorkState::Free;

	if (m_nAllocated > 0u)
	{
		--m_nAllocated;
	}

	LOG_INFO(
		"KFEDSVHeap::Impl::Free: Freed descriptor index {}. Allocated = {}, Remaining = {}.",
		index,
		m_nAllocated,
		GetRemaining()
	);

	return true;
}

bool kfe::KFEDSVHeap::Impl::Reset() noexcept
{
	if (!IsInitialized())
	{
		LOG_WARNING("KFEDSVHeap::Impl::Reset: Heap is not initialized. Nothing to reset.");
		return false;
	}

	for (auto& state : m_workStates)
	{
		state = EWorkState::Free;
	}

	m_nAllocated = 0u;

	LOG_INFO(
		"KFEDSVHeap::Impl::Reset: All descriptor slots marked free. Capacity = {}.",
		m_nCapacity
	);

	return true;
}

bool kfe::KFEDSVHeap::Impl::IsValidIndex(std::uint32_t idx) const noexcept
{
	return idx < m_nCapacity;
}

ID3D12DescriptorHeap* kfe::KFEDSVHeap::Impl::GetNative() const noexcept
{
	return m_pDescriptorHeap.Get();
}

void kfe::KFEDSVHeap::Impl::SetDebugName(const std::string& name) noexcept
{
	m_szDebugName = name;

	if (!HasHeap())
	{
		return;
	}

	if (m_szDebugName.empty())
	{
		return;
	}

	auto wideName = kfe_helpers::AnsiToWide(m_szDebugName);
	m_pDescriptorHeap->SetName(wideName.c_str());
}

KFE_CPU_DESCRIPTOR_HANDLE kfe::KFEDSVHeap::Impl::ComputeHandle(std::uint32_t index) const noexcept
{
	KFE_CPU_DESCRIPTOR_HANDLE handle = m_handleStart;
	handle.ptr += static_cast<std::size_t>(index) * static_cast<std::size_t>(m_nHandleSize);
	return handle;
}

void kfe::KFEDSVHeap::Impl::ClearState() noexcept
{
	m_pDevice		= nullptr;
	m_nCapacity		= 0u;
	m_nAllocated	= 0u;
	m_nHandleSize	= 0u;
	m_handleStart	= KFE_CPU_DESCRIPTOR_HANDLE{};
	
	m_workStates .clear();
	m_szDebugName.clear();
}

bool kfe::KFEDSVHeap::Impl::HasHeap() const noexcept
{
	return (m_pDescriptorHeap != nullptr);
}

#pragma endregion
