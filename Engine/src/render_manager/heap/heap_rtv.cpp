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
#include "engine/render_manager/heap/heap_rtv.h"
#include "engine/render_manager/components/device.h"

#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"

#pragma region Impl_Declaration

class kfe::KFERTVHeap::Impl
{
public:
    static constexpr std::uint32_t InvalidIndex = 0xFFFFFFFFu;

	Impl() = default;
	~Impl()
	{
		if (!Destroy())
		{
			LOG_ERROR("Failed to Destroy kfe::KFERTVHeap::Impl Successfully!");
		}
	}

    bool Initialize(const KFE_RTV_HEAP_CREATE_DESC& desc);
    
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
    bool					  IsValidIndex	(std::uint32_t idx  ) const noexcept;
    
	ID3D12DescriptorHeap* GetNative() const noexcept;
    void				  SetDebugName(_In_ const std::string& name) noexcept;

private:
	KFE_CPU_DESCRIPTOR_HANDLE ComputeHandle(std::uint32_t index) const noexcept;
	
	void ClearState() noexcept;
	bool HasHeap   () const noexcept;

private:
	inline static constexpr D3D12_DESCRIPTOR_HEAP_TYPE m_type{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV };

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap{ nullptr };
	KFEDevice*									 m_pDevice		  { nullptr };

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

#pragma region RTV_Heap_Implementation

kfe::KFERTVHeap::KFERTVHeap() noexcept
	: m_impl(std::make_unique<kfe::KFERTVHeap::Impl>())
{}

kfe::KFERTVHeap::~KFERTVHeap()							  noexcept = default;
kfe::KFERTVHeap::KFERTVHeap (KFERTVHeap&&)				  noexcept = default;

kfe::KFERTVHeap& kfe::KFERTVHeap::operator=(KFERTVHeap&&) noexcept = default;

std::string kfe::KFERTVHeap::GetName() const noexcept
{
	return "KFERTVHeap";
}

std::string kfe::KFERTVHeap::GetDescription() const noexcept
{
	if (!m_impl || !m_impl->IsInitialized())
		return "RTV Heap: Not Initialized";

	return std::format(
		"RTV Heap | Capacity: {} | Allocated: {} | Remaining: {} | HandleSize: {} bytes",
		m_impl->GetNumDescriptors(),
		m_impl->GetAllocatedCount(),
		m_impl->GetRemaining(),
		m_impl->GetHandleSize()
	);
}

bool kfe::KFERTVHeap::Initialize(const KFE_RTV_HEAP_CREATE_DESC& desc)
{
	return m_impl->Initialize(desc);
}

bool kfe::KFERTVHeap::Destroy() noexcept
{
	return m_impl->Destroy();
}

bool kfe::KFERTVHeap::IsInitialized() const noexcept
{
	return m_impl->IsInitialized();
}

std::uint32_t kfe::KFERTVHeap::GetNumDescriptors() const noexcept
{
	return m_impl->GetNumDescriptors();
}

std::uint32_t kfe::KFERTVHeap::GetAllocatedCount() const noexcept
{
	return m_impl->GetAllocatedCount();
}

std::uint32_t kfe::KFERTVHeap::GetRemaining() const noexcept
{
	return m_impl->GetRemaining();
}

std::uint32_t kfe::KFERTVHeap::GetHandleSize() const noexcept
{
	return m_impl->GetHandleSize();
}

KFE_CPU_DESCRIPTOR_HANDLE kfe::KFERTVHeap::GetStartHandle() const noexcept
{
	return m_impl->GetStartHandle();
}

KFE_CPU_DESCRIPTOR_HANDLE kfe::KFERTVHeap::GetHandle(std::uint32_t index) const noexcept
{
	return m_impl->GetHandle(index);
}

std::uint32_t kfe::KFERTVHeap::Allocate() noexcept
{
	return m_impl->Allocate();
}

bool kfe::KFERTVHeap::Free(std::uint32_t index) noexcept
{
	return m_impl->Free(index);
}

bool kfe::KFERTVHeap::Reset() noexcept
{
	return m_impl->Reset();
}

bool kfe::KFERTVHeap::IsValidIndex(std::uint32_t idx) const noexcept
{
	return m_impl->IsValidIndex(idx);
}

ID3D12DescriptorHeap* kfe::KFERTVHeap::GetNative() const noexcept
{
	return m_impl->GetNative();
}

void kfe::KFERTVHeap::SetDebugName(const std::string& name) noexcept
{
	return m_impl->SetDebugName(name);
}

#pragma endregion

#pragma region Impl_Implementation

bool kfe::KFERTVHeap::Impl::Initialize(const KFE_RTV_HEAP_CREATE_DESC& desc)
{
	//~ Validate inputs
	if (desc.Device == nullptr)
	{
		LOG_ERROR("KFERTVHeap::Impl::Initialize: Invalid argument. Device is nullptr.");
		return false;
	}

	if (desc.DescriptorCounts == 0u)
	{
		LOG_ERROR("KFERTVHeap::Impl::Initialize: DescriptorCounts must be greater than 0.");
		return false;
	}

	if (IsInitialized())
	{
		LOG_WARNING("KFERTVHeap::Impl::Initialize: Already initialized. Destroying existing heap.");
		Destroy();
	}

	ClearState();
	m_pDevice = desc.Device;

	auto* nativeDevice = m_pDevice->GetNative();

	if (nativeDevice == nullptr)
	{
		LOG_ERROR("KFERTVHeap::Impl::Initialize: Native D3D12 device is nullptr.");
		m_pDevice = nullptr;
		return false;
	}

	// Describe RTV descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type			= m_type;
	heapDesc.NumDescriptors = desc.DescriptorCounts;
	heapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask		= 0u;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
	const HRESULT hr = nativeDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));

	if (FAILED(hr))
	{
		LOG_ERROR(
			"KFERTVHeap::Impl::Initialize: Failed to create RTV descriptor heap. HRESULT = 0x{:08X}",
			static_cast<unsigned int>(hr)
		);
		m_pDevice = nullptr;
		return false;
	}

	m_pDescriptorHeap = std::move(heap);
	m_nCapacity		  = desc.DescriptorCounts;
	m_nHandleSize	  = nativeDevice->GetDescriptorHandleIncrementSize(m_type);

	const auto startHandle	= m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_handleStart.ptr		= startHandle.ptr;

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
		"KFERTVHeap::Impl::Initialize: RTV heap created. Capacity = {}, HandleSize = {} bytes.",
		m_nCapacity,
		m_nHandleSize
	);

	return true;
}

bool kfe::KFERTVHeap::Impl::Destroy() noexcept
{
	if (!HasHeap())
	{
		ClearState();
		return true;
	}

	LOG_INFO("KFERTVHeap::Impl::Destroy: Releasing RTV descriptor heap.");

	m_pDescriptorHeap.Reset();
	ClearState();

	return true;
}

bool kfe::KFERTVHeap::Impl::IsInitialized() const noexcept
{
	return HasHeap();
}

std::uint32_t kfe::KFERTVHeap::Impl::GetNumDescriptors() const noexcept
{
	return m_nCapacity;
}

std::uint32_t kfe::KFERTVHeap::Impl::GetAllocatedCount() const noexcept
{
	return m_nAllocated;
}

std::uint32_t kfe::KFERTVHeap::Impl::GetRemaining() const noexcept
{
	if (m_nCapacity <= m_nAllocated)
	{
		return 0u;
	}
	return m_nCapacity - m_nAllocated;
}

std::uint32_t kfe::KFERTVHeap::Impl::GetHandleSize() const noexcept
{
	return m_nHandleSize;
}

KFE_CPU_DESCRIPTOR_HANDLE kfe::KFERTVHeap::Impl::GetStartHandle() const noexcept
{
	return m_handleStart;
}

KFE_CPU_DESCRIPTOR_HANDLE kfe::KFERTVHeap::Impl::GetHandle(std::uint32_t index) const noexcept
{
	if (!IsValidIndex(index))
	{
		LOG_ERROR(
			"KFERTVHeap::Impl::GetHandle: Invalid index {}. Capacity = {}.",
			index,
			m_nCapacity
		);
		return KFE_CPU_DESCRIPTOR_HANDLE{};
	}

	return ComputeHandle(index);
}

std::uint32_t kfe::KFERTVHeap::Impl::Allocate() noexcept
{
	if (!IsInitialized())
	{
		LOG_ERROR("KFERTVHeap::Impl::Allocate: Heap is not initialized.");
		return InvalidIndex;
	}

	if (m_nAllocated >= m_nCapacity)
	{
		LOG_WARNING(
			"KFERTVHeap::Impl::Allocate: No more descriptors available. Capacity = {}.",
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
				"KFERTVHeap::Impl::Allocate: Allocated descriptor index {}. Allocated = {}, Remaining = {}.",
				i,
				m_nAllocated,
				GetRemaining()
			);

			return i;
		}
	}

	LOG_ERROR(
		"KFERTVHeap::Impl::Allocate: Failed to find free descriptor despite remaining count > 0. "
		"Capacity = {}, Allocated = {}.",
		m_nCapacity,
		m_nAllocated
	);

	return InvalidIndex;
}

bool kfe::KFERTVHeap::Impl::Free(std::uint32_t index) noexcept
{
	if (!IsInitialized())
	{
		LOG_ERROR("KFERTVHeap::Impl::Free: Heap is not initialized.");
		return false;
	}

	if (!IsValidIndex(index))
	{
		LOG_ERROR(
			"KFERTVHeap::Impl::Free: Invalid index {}. Capacity = {}.",
			index,
			m_nCapacity
		);
		return false;
	}

	if (m_workStates[index] == EWorkState::Free)
	{
		LOG_WARNING(
			"KFERTVHeap::Impl::Free: Descriptor index {} is already free.",
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
		"KFERTVHeap::Impl::Free: Freed descriptor index {}. Allocated = {}, Remaining = {}.",
		index,
		m_nAllocated,
		GetRemaining()
	);

	return true;
}

bool kfe::KFERTVHeap::Impl::Reset() noexcept
{
	if (!IsInitialized())
	{
		LOG_WARNING("KFERTVHeap::Impl::Reset: Heap is not initialized. Nothing to reset.");
		return false;
	}

	for (auto& state : m_workStates)
	{
		state = EWorkState::Free;
	}

	m_nAllocated = 0u;

	LOG_INFO(
		"KFERTVHeap::Impl::Reset: All descriptor slots marked free. Capacity = {}.",
		m_nCapacity
	);

	return true;
}

bool kfe::KFERTVHeap::Impl::IsValidIndex(std::uint32_t idx) const noexcept
{
	return idx < m_nCapacity;
}

ID3D12DescriptorHeap* kfe::KFERTVHeap::Impl::GetNative() const noexcept
{
	return m_pDescriptorHeap.Get();
}

void kfe::KFERTVHeap::Impl::SetDebugName(const std::string& name) noexcept
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

KFE_CPU_DESCRIPTOR_HANDLE kfe::KFERTVHeap::Impl::ComputeHandle(std::uint32_t index) const noexcept
{
	KFE_CPU_DESCRIPTOR_HANDLE handle = m_handleStart;
	handle.ptr += static_cast<std::size_t>(index) * static_cast<std::size_t>(m_nHandleSize);
	return handle;
}

void kfe::KFERTVHeap::Impl::ClearState() noexcept
{
	m_pDevice	  = nullptr;
	m_nCapacity   = 0u;
	m_nAllocated  = 0u;
	m_nHandleSize = 0u;

	m_handleStart = KFE_CPU_DESCRIPTOR_HANDLE{};
	m_workStates .clear();
	m_szDebugName.clear();
}

bool kfe::KFERTVHeap::Impl::HasHeap() const noexcept
{
	return (m_pDescriptorHeap != nullptr);
}

#pragma endregion
