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
#include "engine/render_manager/api/heap/heap_sampler.h"
#include "engine/render_manager/api/components/device.h"

#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"

#pragma region Impl_Declaration

class kfe::KFESamplerHeap::Impl
{
public:
	 Impl() = default;
	~Impl()
	{
		if (!Destroy())
		{
			LOG_ERROR("KFESamplerHeap::Impl Failed to Destroy Object without an exception!");
		}
	}

	NODISCARD bool Initialize(const KFE_DESCRIPTOR_HEAP_CREATE_DESC& desc);

	NODISCARD bool          Destroy			 ()       noexcept;
	NODISCARD bool          IsInitialized	 () const noexcept;
	NODISCARD std::uint32_t GetNumDescriptors() const noexcept;
	NODISCARD std::uint32_t GetAllocatedCount() const noexcept;
	NODISCARD std::uint32_t GetRemaining	 () const noexcept;
	NODISCARD std::uint32_t GetHandleSize	 () const noexcept;
	NODISCARD std::uint32_t Allocate		 ()       noexcept;
	NODISCARD bool          Reset			 ()       noexcept;

	NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetStartHandle   () const noexcept;
	NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE GetGPUStartHandle() const noexcept;

	NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetHandle	(_In_ std::uint32_t index) const noexcept;
	NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(_In_ std::uint32_t index) const noexcept;

	NODISCARD bool Free		   (_In_ std::uint32_t index) noexcept;
	NODISCARD bool IsValidIndex(std::uint32_t idx)  const noexcept;

	_Maybenull_ NODISCARD
	ID3D12DescriptorHeap* GetNative()		  const noexcept;
	void SetDebugName(_In_ const std::string& name) noexcept;

private:
    D3D12_CPU_DESCRIPTOR_HANDLE ComputeCPUHandle(_In_ std::uint32_t index) const noexcept;
    D3D12_GPU_DESCRIPTOR_HANDLE ComputeGPUHandle(_In_ std::uint32_t index) const noexcept;

	void ClearState()       noexcept;
	bool HasHeap   () const noexcept;

private:
	inline static constexpr std::uint32_t               InvalidIndex{ 0xFFFFFFFFu };
	inline static constexpr D3D12_DESCRIPTOR_HEAP_TYPE  m_type{ D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER };
	inline static constexpr D3D12_DESCRIPTOR_HEAP_FLAGS m_flag{ D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap{ nullptr };
	KFEDevice* m_pDevice{ nullptr };

	//~ configurations
	std::uint32_t m_nCapacity		{ 0u };
	std::uint32_t m_nAllocated		{ 0u };
	std::uint32_t m_nHandleSize		{ 0u };
	std::uint32_t m_nNextSearchIndex{ 0u };

    D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandleStart{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandleStart{};

	std::vector<EWorkState> m_workStates{};

	//~ debugs
	std::string m_szDebugName{};
};

#pragma endregion

#pragma region SamplerHeap_Implementation

kfe::KFESamplerHeap::KFESamplerHeap() noexcept
    : m_impl(std::make_unique<kfe::KFESamplerHeap::Impl>())
{
}

kfe::KFESamplerHeap::~KFESamplerHeap()                      noexcept = default;
kfe::KFESamplerHeap::KFESamplerHeap(KFESamplerHeap&&)       noexcept = default;
kfe::KFESamplerHeap& kfe::KFESamplerHeap::operator=(KFESamplerHeap&&) noexcept = default;

std::string kfe::KFESamplerHeap::GetName() const noexcept
{
    return "KFESamplerHeap";
}

std::string kfe::KFESamplerHeap::GetDescription() const noexcept
{
    if (!m_impl || !m_impl->IsInitialized())
        return "Sampler Heap: Not Initialized";

    return std::format(
        "Sampler Heap | Capacity: {} | Allocated: {} | Remaining: {} | HandleSize: {} bytes",
        m_impl->GetNumDescriptors(),
        m_impl->GetAllocatedCount(),
        m_impl->GetRemaining     (),
        m_impl->GetHandleSize    ()
    );
}

_Use_decl_annotations_
bool kfe::KFESamplerHeap::Initialize(const KFE_DESCRIPTOR_HEAP_CREATE_DESC& desc)
{
    return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFESamplerHeap::Destroy() noexcept
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFESamplerHeap::IsInitialized() const noexcept
{
    return m_impl->IsInitialized();
}

_Use_decl_annotations_
std::uint32_t kfe::KFESamplerHeap::GetNumDescriptors() const noexcept
{
    return m_impl->GetNumDescriptors();
}

_Use_decl_annotations_
std::uint32_t kfe::KFESamplerHeap::GetAllocatedCount() const noexcept
{
    return m_impl->GetAllocatedCount();
}

_Use_decl_annotations_
std::uint32_t kfe::KFESamplerHeap::GetRemaining() const noexcept
{
    return m_impl->GetRemaining();
}

_Use_decl_annotations_
std::uint32_t kfe::KFESamplerHeap::GetHandleSize() const noexcept
{
    return m_impl->GetHandleSize();
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFESamplerHeap::GetStartHandle() const noexcept
{
    return m_impl->GetStartHandle();
}

_Use_decl_annotations_
D3D12_GPU_DESCRIPTOR_HANDLE kfe::KFESamplerHeap::GetGPUStartHandle() const noexcept
{
    return m_impl->GetGPUStartHandle();
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFESamplerHeap::GetHandle(std::uint32_t index) const noexcept
{
    return m_impl->GetHandle(index);
}

_Use_decl_annotations_
D3D12_GPU_DESCRIPTOR_HANDLE kfe::KFESamplerHeap::GetGPUHandle(std::uint32_t index) const noexcept
{
    return m_impl->GetGPUHandle(index);
}

_Use_decl_annotations_
std::uint32_t kfe::KFESamplerHeap::Allocate() noexcept
{
    return m_impl->Allocate();
}

_Use_decl_annotations_
bool kfe::KFESamplerHeap::Free(_In_ std::uint32_t index) noexcept
{
    return m_impl->Free(index);
}

_Use_decl_annotations_
bool kfe::KFESamplerHeap::Reset() noexcept
{
    return m_impl->Reset();
}

_Use_decl_annotations_
bool kfe::KFESamplerHeap::IsValidIndex(std::uint32_t idx) const noexcept
{
    return m_impl->IsValidIndex(idx);
}

_Use_decl_annotations_
ID3D12DescriptorHeap* kfe::KFESamplerHeap::GetNative() const noexcept
{
    return m_impl->GetNative();
}

_Use_decl_annotations_
void kfe::KFESamplerHeap::SetDebugName(const std::string& name) noexcept
{
    m_impl->SetDebugName(name);
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFESamplerHeap::Impl::Initialize(const KFE_DESCRIPTOR_HEAP_CREATE_DESC& desc)
{
    //~ Validate inputs
    if (desc.Device == nullptr)
    {
        LOG_ERROR("KFESamplerHeap::Impl::Initialize: Invalid argument. Device is nullptr.");
        return false;
    }

    if (desc.DescriptorCounts == 0u)
    {
        LOG_ERROR("KFESamplerHeap::Impl::Initialize: DescriptorCounts must be greater than 0.");
        return false;
    }

    if (IsInitialized())
    {
        LOG_WARNING("KFESamplerHeap::Impl::Initialize: Already initialized. Destroying existing heap.");

        if (!Destroy())
        {
            LOG_ERROR("KFESamplerHeap::Impl::Initialize: Failed to Release Resources!");
            return false;
        }
    }

    ClearState();
    m_pDevice = desc.Device;

    auto* nativeDevice = m_pDevice->GetNative();

    if (nativeDevice == nullptr)
    {
        LOG_ERROR("KFESamplerHeap::Impl::Initialize: Native D3D12 device is nullptr.");
        m_pDevice = nullptr;
        return false;
    }

    // Describe Sampler descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.Type           = m_type;
    heapDesc.NumDescriptors = desc.DescriptorCounts;
    heapDesc.Flags          = m_flag;
    heapDesc.NodeMask       = 0u;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
    const HRESULT hr = nativeDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));

    if (FAILED(hr))
    {
        LOG_ERROR(
            "KFESamplerHeap::Impl::Initialize: Failed to create Sampler descriptor heap. HRESULT = 0x{:08X}",
            static_cast<unsigned int>(hr)
        );
        m_pDevice = nullptr;
        return false;
    }

    m_pDescriptorHeap = std::move(heap);
    m_nCapacity   = desc.DescriptorCounts;
    m_nHandleSize = nativeDevice->GetDescriptorHandleIncrementSize(m_type);

    const auto startHandle = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    m_cpuHandleStart.ptr   = startHandle.ptr;

    const auto gpuStartHandle = m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    m_gpuHandleStart.ptr      = gpuStartHandle.ptr;

    //~ reset states
    m_workStates.clear();
    m_workStates.resize(m_nCapacity, EWorkState::Free);

    m_nAllocated       = 0u;
    m_nNextSearchIndex = 0u;

    if (desc.DebugName != nullptr)
    {
        m_szDebugName = desc.DebugName;
        SetDebugName(m_szDebugName);
    }

    LOG_SUCCESS(
        "KFESamplerHeap::Impl::Initialize: Sampler heap created. Capacity = {}, HandleSize = {} bytes.",
        m_nCapacity,
        m_nHandleSize
    );

    return true;
}

_Use_decl_annotations_
bool kfe::KFESamplerHeap::Impl::Destroy() noexcept
{
    if (!HasHeap())
    {
        //~ Already destroyed or never initialized
        ClearState();
        return true;
    }

    LOG_INFO("KFESamplerHeap::Impl::Destroy: Releasing Sampler descriptor heap.");

    m_pDescriptorHeap.Reset();
    ClearState();

    return true;
}

_Use_decl_annotations_
bool kfe::KFESamplerHeap::Impl::IsInitialized() const noexcept
{
    return HasHeap();
}

_Use_decl_annotations_
std::uint32_t kfe::KFESamplerHeap::Impl::GetNumDescriptors() const noexcept
{
    return m_nCapacity;
}

_Use_decl_annotations_
std::uint32_t kfe::KFESamplerHeap::Impl::GetAllocatedCount() const noexcept
{
    return m_nAllocated;
}

_Use_decl_annotations_
std::uint32_t kfe::KFESamplerHeap::Impl::GetRemaining() const noexcept
{
    if (m_nCapacity <= m_nAllocated)
    {
        return 0u;
    }
    return m_nCapacity - m_nAllocated;
}

_Use_decl_annotations_
std::uint32_t kfe::KFESamplerHeap::Impl::GetHandleSize() const noexcept
{
    return m_nHandleSize;
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFESamplerHeap::Impl::GetStartHandle() const noexcept
{
    return m_cpuHandleStart;
}

_Use_decl_annotations_
D3D12_GPU_DESCRIPTOR_HANDLE kfe::KFESamplerHeap::Impl::GetGPUStartHandle() const noexcept
{
    return m_gpuHandleStart;
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFESamplerHeap::Impl::GetHandle(std::uint32_t index) const noexcept
{
    if (!IsValidIndex(index))
    {
        LOG_ERROR(
            "KFESamplerHeap::Impl::GetHandle: Invalid index {}. Capacity = {}.",
            index,
            m_nCapacity
        );
        return D3D12_CPU_DESCRIPTOR_HANDLE{};
    }

    return ComputeCPUHandle(index);
}

_Use_decl_annotations_
D3D12_GPU_DESCRIPTOR_HANDLE kfe::KFESamplerHeap::Impl::GetGPUHandle(std::uint32_t index) const noexcept
{
    if (!IsValidIndex(index))
    {
        LOG_ERROR(
            "KFESamplerHeap::Impl::GetGPUHandle: Invalid index {}. Capacity = {}.",
            index,
            m_nCapacity
        );
        return D3D12_GPU_DESCRIPTOR_HANDLE{};
    }

    return ComputeGPUHandle(index);
}

_Use_decl_annotations_
std::uint32_t kfe::KFESamplerHeap::Impl::Allocate() noexcept
{
    if (!IsInitialized())
    {
        LOG_ERROR("KFESamplerHeap::Impl::Allocate: Heap is not initialized.");
        return InvalidIndex;
    }

    if (m_nAllocated >= m_nCapacity)
    {
        LOG_WARNING(
            "KFESamplerHeap::Impl::Allocate: No more descriptors available. Capacity = {}.",
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
                "KFESamplerHeap::Impl::Allocate: Allocated descriptor index {}. Allocated = {}, Remaining = {}.",
                index,
                m_nAllocated,
                GetRemaining()
            );

            return index;
        }

        index = (index + 1u) < m_nCapacity ? (index + 1u) : 0u;
    } while (index != startIndex);

    LOG_ERROR(
        "KFESamplerHeap::Impl::Allocate: Failed to find free descriptor despite remaining count > 0. "
        "Capacity = {}, Allocated = {}.",
        m_nCapacity,
        m_nAllocated
    );

    return InvalidIndex;
}

_Use_decl_annotations_
bool kfe::KFESamplerHeap::Impl::Free(std::uint32_t index) noexcept
{
    if (!IsInitialized())
    {
        LOG_ERROR("KFESamplerHeap::Impl::Free: Heap is not initialized.");
        return false;
    }

    if (!IsValidIndex(index))
    {
        LOG_ERROR(
            "KFESamplerHeap::Impl::Free: Invalid index {}. Capacity = {}.",
            index,
            m_nCapacity
        );
        return false;
    }

    if (m_workStates[index] == EWorkState::Free)
    {
        LOG_WARNING(
            "KFESamplerHeap::Impl::Free: Descriptor index {} is already free.",
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
        "KFESamplerHeap::Impl::Free: Freed descriptor index {}. Allocated = {}, Remaining = {}.",
        index,
        m_nAllocated,
        GetRemaining()
    );

    return true;
}

_Use_decl_annotations_
bool kfe::KFESamplerHeap::Impl::Reset() noexcept
{
    if (!IsInitialized())
    {
        LOG_WARNING("KFESamplerHeap::Impl::Reset: Heap is not initialized. Nothing to reset.");
        return false;
    }

    for (auto& state : m_workStates)
    {
        state = EWorkState::Free;
    }

    m_nAllocated = 0u;
    m_nNextSearchIndex = 0u;

    LOG_INFO(
        "KFESamplerHeap::Impl::Reset: All descriptor slots marked free. Capacity = {}.",
        m_nCapacity
    );

    return true;
}

_Use_decl_annotations_
bool kfe::KFESamplerHeap::Impl::IsValidIndex(std::uint32_t idx) const noexcept
{
    return idx < m_nCapacity;
}

_Use_decl_annotations_
ID3D12DescriptorHeap* kfe::KFESamplerHeap::Impl::GetNative() const noexcept
{
    return m_pDescriptorHeap.Get();
}

_Use_decl_annotations_
void kfe::KFESamplerHeap::Impl::SetDebugName(const std::string& name) noexcept
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

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFESamplerHeap::Impl::ComputeCPUHandle(std::uint32_t index) const noexcept
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_cpuHandleStart;
    handle.ptr += static_cast<std::size_t>(index) * static_cast<std::size_t>(m_nHandleSize);
    return handle;
}

_Use_decl_annotations_
D3D12_GPU_DESCRIPTOR_HANDLE kfe::KFESamplerHeap::Impl::ComputeGPUHandle(std::uint32_t index) const noexcept
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = m_gpuHandleStart;
    handle.ptr += static_cast<std::size_t>(index) * static_cast<std::size_t>(m_nHandleSize);
    return handle;
}

void kfe::KFESamplerHeap::Impl::ClearState() noexcept
{
    m_pDevice           = nullptr;
    m_nCapacity         = 0u;
    m_nAllocated        = 0u;
    m_nHandleSize       = 0u;
    m_nNextSearchIndex  = 0u;

    m_cpuHandleStart = D3D12_CPU_DESCRIPTOR_HANDLE{};
    m_gpuHandleStart = D3D12_GPU_DESCRIPTOR_HANDLE{};

    m_workStates .clear();
    m_szDebugName.clear();
}

bool kfe::KFESamplerHeap::Impl::HasHeap() const noexcept
{
    return (m_pDescriptorHeap != nullptr);
}

#pragma endregion
