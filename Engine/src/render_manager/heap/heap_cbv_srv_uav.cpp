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
#include "engine/render_manager/heap/heap_cbv_srv_uav.h"
#include "engine/render_manager/components/device.h"

#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"

#pragma region Impl_Declaration

class kfe::KFEResourceHeap::Impl 
{
public:
	 Impl() = default;
	~Impl()
	{
		if (!Destroy()) 
		{
			LOG_ERROR("KFEResourceHeap::Impl Failed to Destroy Object without an exception!");
		}
	}

    NODISCARD bool Initialize(const KFE_RESOURCE_HEAP_CREATE_DESC& desc);

    NODISCARD bool          Destroy          ()       noexcept;
    NODISCARD bool          IsInitialized    () const noexcept;
    NODISCARD std::uint32_t GetNumDescriptors() const noexcept;
    NODISCARD std::uint32_t GetAllocatedCount() const noexcept;
    NODISCARD std::uint32_t GetRemaining     () const noexcept;
    NODISCARD std::uint32_t GetHandleSize    () const noexcept;
    NODISCARD std::uint32_t Allocate         ()       noexcept;
    NODISCARD bool          Reset            ()       noexcept;

    NODISCARD KFE_CPU_DESCRIPTOR_HANDLE GetStartHandle   () const noexcept;
    NODISCARD KFE_GPU_DESCRIPTOR_HANDLE GetGPUStartHandle() const noexcept;

    NODISCARD KFE_CPU_DESCRIPTOR_HANDLE GetHandle   (_In_ std::uint32_t index) const noexcept;
    NODISCARD KFE_GPU_DESCRIPTOR_HANDLE GetGPUHandle(_In_ std::uint32_t index) const noexcept;

    NODISCARD bool Free        (_In_ std::uint32_t index) noexcept;
    NODISCARD bool IsValidIndex(std::uint32_t idx)  const noexcept;

    _Maybenull_ NODISCARD
    ID3D12DescriptorHeap* GetNative() const noexcept;
    void SetDebugName(_In_ const std::string& name) noexcept;

private:
    KFE_CPU_DESCRIPTOR_HANDLE ComputeCPUHandle(_In_ std::uint32_t index) const noexcept;
    KFE_GPU_DESCRIPTOR_HANDLE ComputeGPUHandle(_In_ std::uint32_t index) const noexcept;

    void ClearState()       noexcept;
    bool HasHeap   () const noexcept;

private:
    inline static constexpr std::uint32_t               InvalidIndex{ 0xFFFFFFFFu };
    inline static constexpr D3D12_DESCRIPTOR_HEAP_TYPE  m_type      { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV	};
    inline static constexpr D3D12_DESCRIPTOR_HEAP_FLAGS m_flag      { D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap{ nullptr };
    KFEDevice*                                   m_pDevice        { nullptr };

    //~ configurations
    std::uint32_t m_nCapacity       { 0u };
    std::uint32_t m_nAllocated      { 0u };
    std::uint32_t m_nHandleSize     { 0u };
    std::uint32_t m_nNextSearchIndex{ 0u };

    KFE_CPU_DESCRIPTOR_HANDLE m_cpuHandleStart{};
    KFE_GPU_DESCRIPTOR_HANDLE m_gpuHandleStart{};

    std::vector<EWorkState> m_workStates{};

    //~ debugs
    std::string m_szDebugName{};
};

#pragma endregion

#pragma region ResourceHeap_Implementation

kfe::KFEResourceHeap::KFEResourceHeap() noexcept
	: m_impl(std::make_unique<kfe::KFEResourceHeap::Impl>())
{}

kfe::KFEResourceHeap::~KFEResourceHeap				 ()					 noexcept = default;
kfe::KFEResourceHeap::KFEResourceHeap				 (KFEResourceHeap&&) noexcept = default;
kfe::KFEResourceHeap& kfe::KFEResourceHeap::operator=(KFEResourceHeap&&) noexcept = default;

std::string kfe::KFEResourceHeap::GetName() const noexcept
{
    return "KFEResourceHeap";
}

std::string kfe::KFEResourceHeap::GetDescription() const noexcept
{
    if (!m_impl || !m_impl->IsInitialized())
        return "CBV/SRV/UAV Heap: Not Initialized";

    return std::format(
        "CBV/SRV/UAV Heap | Capacity: {} | Allocated: {} | Remaining: {} | HandleSize: {} bytes",
        m_impl->GetNumDescriptors(),
        m_impl->GetAllocatedCount(),
        m_impl->GetRemaining     (),
        m_impl->GetHandleSize    ()
    );
}

_Use_decl_annotations_
bool kfe::KFEResourceHeap::Initialize(const KFE_RESOURCE_HEAP_CREATE_DESC& desc)
{
	return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFEResourceHeap::Destroy() noexcept
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFEResourceHeap::IsInitialized() const noexcept
{
    return m_impl->IsInitialized();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEResourceHeap::GetNumDescriptors() const noexcept
{
    return m_impl->GetNumDescriptors();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEResourceHeap::GetAllocatedCount() const noexcept 
{
    return m_impl->GetAllocatedCount();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEResourceHeap::GetRemaining() const noexcept
{
    return m_impl->GetRemaining();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEResourceHeap::GetHandleSize() const noexcept
{
    return m_impl->GetHandleSize();
}

_Use_decl_annotations_
KFE_CPU_DESCRIPTOR_HANDLE kfe::KFEResourceHeap::GetStartHandle() const noexcept
{
    return m_impl->GetStartHandle();
}

_Use_decl_annotations_
KFE_GPU_DESCRIPTOR_HANDLE kfe::KFEResourceHeap::GetGPUStartHandle() const noexcept
{
	return m_impl->GetGPUStartHandle();
}

_Use_decl_annotations_
KFE_CPU_DESCRIPTOR_HANDLE kfe::KFEResourceHeap::GetHandle(std::uint32_t index) const noexcept
{
	return m_impl->GetHandle(index);
}

_Use_decl_annotations_
KFE_GPU_DESCRIPTOR_HANDLE kfe::KFEResourceHeap::GetGPUHandle(std::uint32_t index) const noexcept
{
	return m_impl->GetGPUHandle(index);
}

_Use_decl_annotations_
std::uint32_t kfe::KFEResourceHeap::Allocate() noexcept
{
	return m_impl->Allocate();
}

_Use_decl_annotations_
bool kfe::KFEResourceHeap::Free(_In_ std::uint32_t index) noexcept
{
	return m_impl->Free(index);
}

_Use_decl_annotations_
bool kfe::KFEResourceHeap::Reset() noexcept
{
	return m_impl->Reset();
}

_Use_decl_annotations_
bool kfe::KFEResourceHeap::IsValidIndex(std::uint32_t idx) const noexcept 
{
	return m_impl->IsValidIndex(idx);
}

_Use_decl_annotations_
ID3D12DescriptorHeap* kfe::KFEResourceHeap::GetNative() const noexcept
{
	return m_impl->GetNative();
}

_Use_decl_annotations_
void kfe::KFEResourceHeap::SetDebugName(const std::string& name) noexcept
{
	m_impl->SetDebugName(name);
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFEResourceHeap::Impl::Initialize(const KFE_RESOURCE_HEAP_CREATE_DESC& desc)
{
	//~ Validate inputs
	if (desc.Device == nullptr)
	{
		LOG_ERROR("KFEResourceHeap::Impl::Initialize: Invalid argument. Device is nullptr.");
		return false;
	}

	if (desc.DescriptorCounts == 0u)
	{
		LOG_ERROR("KFEResourceHeap::Impl::Initialize: DescriptorCounts must be greater than 0.");
		return false;
	}

	if (IsInitialized())
	{
		LOG_WARNING("KFEResourceHeap::Impl::Initialize: Already initialized. Destroying existing heap.");

		if (!Destroy())
		{
			LOG_ERROR("KFEResourceHeap::Impl::Initialize: Failed to Release Resources!");
			return false;
		}
	}

	ClearState();
	m_pDevice = desc.Device;

	auto* nativeDevice = m_pDevice->GetNative();

	if (nativeDevice == nullptr)
	{
		LOG_ERROR("KFEResourceHeap::Impl::Initialize: Native D3D12 device is nullptr.");
		m_pDevice = nullptr;
		return false;
	}

	// Describe CBV/SRV/UAV descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type			= m_type;
	heapDesc.NumDescriptors = desc.DescriptorCounts;
	heapDesc.Flags			= m_flag;
	heapDesc.NodeMask		= 0u;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
	const HRESULT hr = nativeDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));

	if (FAILED(hr))
	{
		LOG_ERROR(
			"KFEResourceHeap::Impl::Initialize: Failed to create CBV/SRV/UAV descriptor heap. HRESULT = 0x{:08X}",
			static_cast<unsigned int>(hr)
		);
		m_pDevice = nullptr;
		return false;
	}

	m_pDescriptorHeap	= std::move(heap);
	m_nCapacity			= desc.DescriptorCounts;
	m_nHandleSize		= nativeDevice->GetDescriptorHandleIncrementSize(m_type);

	const auto startHandle	= m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_cpuHandleStart.ptr	= startHandle.ptr;

	const auto gpuStartHandle	= m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	m_gpuHandleStart.ptr		= gpuStartHandle.ptr;

	//~ reset states
	m_workStates.clear();
	m_workStates.resize(m_nCapacity, EWorkState::Free);

	m_nAllocated		= 0u;
	m_nNextSearchIndex	= 0u;

	if (desc.DebugName != nullptr)
	{
		m_szDebugName = desc.DebugName;
		SetDebugName(m_szDebugName);
	}

	LOG_SUCCESS(
		"KFEResourceHeap::Impl::Initialize: Resource heap created. Capacity = {}, HandleSize = {} bytes.",
		m_nCapacity,
		m_nHandleSize
	);

	return true;
}

_Use_decl_annotations_
bool kfe::KFEResourceHeap::Impl::Destroy() noexcept
{
	if (!HasHeap())
	{
		// Already destroyed or never initialized
		ClearState();
		return true;
	}

	LOG_INFO("KFEResourceHeap::Impl::Destroy: Releasing CBV/SRV/UAV descriptor heap.");

	m_pDescriptorHeap.Reset();
	ClearState();

	return true;
}

_Use_decl_annotations_
bool kfe::KFEResourceHeap::Impl::IsInitialized() const noexcept
{
	return HasHeap();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEResourceHeap::Impl::GetNumDescriptors() const noexcept
{
	return m_nCapacity;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEResourceHeap::Impl::GetAllocatedCount() const noexcept
{
	return m_nAllocated;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEResourceHeap::Impl::GetRemaining() const noexcept
{
	if (m_nCapacity <= m_nAllocated)
	{
		return 0u;
	}
	return m_nCapacity - m_nAllocated;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEResourceHeap::Impl::GetHandleSize() const noexcept
{
	return m_nHandleSize;
}

_Use_decl_annotations_
KFE_CPU_DESCRIPTOR_HANDLE kfe::KFEResourceHeap::Impl::GetStartHandle() const noexcept
{
	return m_cpuHandleStart;
}

_Use_decl_annotations_
KFE_GPU_DESCRIPTOR_HANDLE kfe::KFEResourceHeap::Impl::GetGPUStartHandle() const noexcept
{
	return m_gpuHandleStart;
}

_Use_decl_annotations_
KFE_GPU_DESCRIPTOR_HANDLE kfe::KFEResourceHeap::Impl::GetGPUHandle(std::uint32_t index) const noexcept
{
	if (!IsValidIndex(index))
	{
		LOG_ERROR(
			"KFEResourceHeap::Impl::GetGPUHandle: Invalid index {}. Capacity = {}.",
			index,
			m_nCapacity
		);
		return KFE_GPU_DESCRIPTOR_HANDLE{};
	}

	return ComputeGPUHandle(index);
}

_Use_decl_annotations_
KFE_CPU_DESCRIPTOR_HANDLE kfe::KFEResourceHeap::Impl::ComputeCPUHandle(std::uint32_t index) const noexcept
{
	KFE_CPU_DESCRIPTOR_HANDLE handle = m_cpuHandleStart;
	handle.ptr += static_cast<std::size_t>(index) * static_cast<std::size_t>(m_nHandleSize);
	return handle;
}

_Use_decl_annotations_
KFE_GPU_DESCRIPTOR_HANDLE kfe::KFEResourceHeap::Impl::ComputeGPUHandle(std::uint32_t index) const noexcept
{
	KFE_GPU_DESCRIPTOR_HANDLE handle = m_gpuHandleStart;
	handle.ptr += static_cast<std::size_t>(index) * static_cast<std::size_t>(m_nHandleSize);
	return handle;
}

void kfe::KFEResourceHeap::Impl::ClearState() noexcept
{
	m_pDevice			= nullptr;
	m_nCapacity			= 0u;
	m_nAllocated		= 0u;
	m_nHandleSize		= 0u;
	m_nNextSearchIndex	= 0u;

	m_cpuHandleStart = KFE_CPU_DESCRIPTOR_HANDLE{};
	m_gpuHandleStart = KFE_GPU_DESCRIPTOR_HANDLE{};

	m_workStates .clear();
	m_szDebugName.clear();
}

bool kfe::KFEResourceHeap::Impl::HasHeap() const noexcept
{
	return (m_pDescriptorHeap != nullptr);
}
_Use_decl_annotations_
KFE_CPU_DESCRIPTOR_HANDLE kfe::KFEResourceHeap::Impl::GetHandle(std::uint32_t index) const noexcept
{
	if (!IsValidIndex(index))
	{
		LOG_ERROR(
			"KFEResourceHeap::Impl::GetHandle: Invalid index {}. Capacity = {}.",
			index,
			m_nCapacity
		);
		return KFE_CPU_DESCRIPTOR_HANDLE{};
	}

	return ComputeCPUHandle(index);
}

_Use_decl_annotations_
std::uint32_t kfe::KFEResourceHeap::Impl::Allocate() noexcept
{
	if (!IsInitialized())
	{
		LOG_ERROR("KFEResourceHeap::Impl::Allocate: Heap is not initialized.");
		return InvalidIndex;
	}

	if (m_nAllocated >= m_nCapacity)
	{
		LOG_WARNING(
			"KFEResourceHeap::Impl::Allocate: No more descriptors available. Capacity = {}.",
			m_nCapacity
		);
		return InvalidIndex;
	}

	std::uint32_t startIndex = m_nNextSearchIndex;
	std::uint32_t index = startIndex;

	do
	{
		if (m_workStates[index] == EWorkState::Free)
		{
			m_workStates[index] = EWorkState::Working;
			++m_nAllocated;

			m_nNextSearchIndex = (index + 1u) < m_nCapacity ? (index + 1u) : 0u;

			LOG_INFO(
				"KFEResourceHeap::Impl::Allocate: Allocated descriptor index {}. Allocated = {}, Remaining = {}.",
				index,
				m_nAllocated,
				GetRemaining()
			);

			return index;
		}

		index = (index + 1u) < m_nCapacity ? (index + 1u) : 0u;
	} while (index != startIndex);

	LOG_ERROR(
		"KFEResourceHeap::Impl::Allocate: Failed to find free descriptor despite remaining count > 0. "
		"Capacity = {}, Allocated = {}.",
		m_nCapacity,
		m_nAllocated
	);

	return InvalidIndex;
}

_Use_decl_annotations_
bool kfe::KFEResourceHeap::Impl::Free(std::uint32_t index) noexcept
{
	if (!IsInitialized())
	{
		LOG_ERROR("KFEResourceHeap::Impl::Free: Heap is not initialized.");
		return false;
	}

	if (!IsValidIndex(index))
	{
		LOG_ERROR(
			"KFEResourceHeap::Impl::Free: Invalid index {}. Capacity = {}.",
			index,
			m_nCapacity
		);
		return false;
	}

	if (m_workStates[index] == EWorkState::Free)
	{
		LOG_WARNING(
			"KFEResourceHeap::Impl::Free: Descriptor index {} is already free.",
			index
		);
		return false;
	}

	m_workStates[index] = EWorkState::Free;

	if (m_nAllocated > 0u)
	{
		--m_nAllocated;
	}

	if (index < m_nNextSearchIndex)
	{
		m_nNextSearchIndex = index;
	}

	LOG_INFO(
		"KFEResourceHeap::Impl::Free: Freed descriptor index {}. Allocated = {}, Remaining = {}.",
		index,
		m_nAllocated,
		GetRemaining()
	);

	return true;
}

_Use_decl_annotations_
bool kfe::KFEResourceHeap::Impl::Reset() noexcept
{
	if (!IsInitialized())
	{
		LOG_WARNING("KFEResourceHeap::Impl::Reset: Heap is not initialized. Nothing to reset.");
		return false;
	}

	for (auto& state : m_workStates)
	{
		state = EWorkState::Free;
	}

	m_nAllocated = 0u;
	m_nNextSearchIndex = 0u;

	LOG_INFO(
		"KFEResourceHeap::Impl::Reset: All descriptor slots marked free. Capacity = {}.",
		m_nCapacity
	);

	return true;
}

_Use_decl_annotations_
bool kfe::KFEResourceHeap::Impl::IsValidIndex(std::uint32_t idx) const noexcept
{
	return idx < m_nCapacity;
}

_Use_decl_annotations_
ID3D12DescriptorHeap* kfe::KFEResourceHeap::Impl::GetNative() const noexcept
{
	return m_pDescriptorHeap.Get();
}

_Use_decl_annotations_
void kfe::KFEResourceHeap::Impl::SetDebugName(const std::string& name) noexcept
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

	const std::wstring converted = kfe_helpers::AnsiToWide(m_szDebugName);
	m_pDescriptorHeap->SetName(converted.c_str());
}

#pragma endregion
