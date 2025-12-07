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

#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"

#include <d3d12.h>
#include <cstring>

#pragma region Impl_Declaration

namespace
{
    inline const char* HeapTypeToString(D3D12_HEAP_TYPE type) noexcept
    {
        switch (type)
        {
        case D3D12_HEAP_TYPE_DEFAULT:  return "DEFAULT";
        case D3D12_HEAP_TYPE_UPLOAD:   return "UPLOAD";
        case D3D12_HEAP_TYPE_READBACK: return "READBACK";
        default:                       return "UNKNOWN";
        }
    }

    inline const char* ResourceStateToString(D3D12_RESOURCE_STATES state) noexcept
    {
        switch (state)
        {
        case D3D12_RESOURCE_STATE_COMMON:                        return "COMMON";
        case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:    return "VB/CB";
        case D3D12_RESOURCE_STATE_INDEX_BUFFER:                  return "IB";
        case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:              return "UAV";
        case D3D12_RESOURCE_STATE_COPY_SOURCE:                   return "COPY_SRC";
        case D3D12_RESOURCE_STATE_COPY_DEST:                     return "COPY_DST";
        case D3D12_RESOURCE_STATE_GENERIC_READ:                  return "GENERIC_READ";
        case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:     return "NPS_RESOURCE";
        case D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT:             return "INDIRECT_ARGS";
        default:                                                 return "UNKNOWN";
        }
    }

    inline std::string ResourceFlagsToString(D3D12_RESOURCE_FLAGS flags) noexcept
    {
        if (flags == D3D12_RESOURCE_FLAG_NONE)
            return "NONE";

        std::string s{};
        if (flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)    s += "RTV ";
        if (flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)    s += "DSV ";
        if (flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) s += "UAV ";
        if (flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)   s += "NO_SRV ";
        if (flags & D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER)    s += "X_ADAPTER ";

        if (s.empty()) return "UNKNOWN";
        return s;
    }
} // namespace


class kfe::KFEBuffer::Impl
{
public:
    Impl() = default;
    ~Impl()
    {
        if (!Destroy())
        {
            LOG_ERROR("KFEBuffer::Impl: Failed to destroy buffer successfully!");
        }
    }

    /// Initialize the GPU buffer with the given description
    NODISCARD bool Initialize(_In_ const KFE_CREATE_BUFFER_DESC& desc);

    NODISCARD bool                  Destroy         ()       noexcept;
    NODISCARD bool                  IsInitialized   () const noexcept;
    NODISCARD std::uint64_t         GetSizeInBytes  () const noexcept;
    NODISCARD D3D12_HEAP_TYPE       GetHeapType     () const noexcept;
    NODISCARD D3D12_RESOURCE_STATES GetInitialState () const noexcept;
    NODISCARD ID3D12Resource*       GetNative       () const noexcept;

    NODISCARD bool IsUploadHeap  () const noexcept;
    NODISCARD bool IsReadbackHeap() const noexcept;

    _Ret_maybenull_ NODISCARD void*       GetMappedData()       noexcept;
    _Ret_maybenull_ NODISCARD const void* GetMappedData() const noexcept;

    NODISCARD bool CopyCPUToGPU(
        _In_reads_bytes_(sizeInBytes) const void* sourceData,
        std::uint64_t                             sizeInBytes,
        std::uint64_t                             dstOffsetBytes = 0u) noexcept;

    NODISCARD bool CopyGPUToCPU(
        _Out_writes_bytes_(sizeInBytes) void* destination,
        std::uint64_t                         sizeInBytes,
        std::uint64_t                         srcOffsetBytes = 0u) const noexcept;

    void SetDebugName(_In_ const std::string& name) noexcept;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource{ nullptr };

    // Cached description
    std::uint64_t         m_sizeInBytes   = 0u;
    D3D12_HEAP_TYPE       m_heapType      = static_cast<D3D12_HEAP_TYPE>(0);
    D3D12_RESOURCE_FLAGS  m_resourceFlags = static_cast<D3D12_RESOURCE_FLAGS>(0);
    D3D12_RESOURCE_STATES m_initialState  = static_cast<D3D12_RESOURCE_STATES>(0);

    void* m_pMappedData  = nullptr;
    bool  m_bMapped      = false;
    bool  m_bInitialized = false;

    std::string m_szDebugName{};
};

#pragma endregion

#pragma region Buffer_Implementation

kfe::KFEBuffer::KFEBuffer()
    : m_impl(std::make_unique<kfe::KFEBuffer::Impl>())
{
}

kfe::KFEBuffer::~KFEBuffer() = default;

kfe::KFEBuffer::KFEBuffer(KFEBuffer&&) noexcept = default;
kfe::KFEBuffer& kfe::KFEBuffer::operator=(KFEBuffer&&) noexcept = default;

_Use_decl_annotations_
bool kfe::KFEBuffer::Initialize(const KFE_CREATE_BUFFER_DESC& desc)
{
    return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFEBuffer::Destroy() noexcept
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFEBuffer::IsInitialized() const noexcept
{
    return m_impl->IsInitialized();
}

_Use_decl_annotations_
std::uint64_t kfe::KFEBuffer::GetSizeInBytes() const noexcept
{
    return m_impl->GetSizeInBytes();
}

_Use_decl_annotations_
D3D12_HEAP_TYPE kfe::KFEBuffer::GetHeapType() const noexcept
{
    return m_impl->GetHeapType();
}

_Use_decl_annotations_
D3D12_RESOURCE_STATES kfe::KFEBuffer::GetInitialState() const noexcept
{
    return m_impl->GetInitialState();
}

_Use_decl_annotations_
ID3D12Resource* kfe::KFEBuffer::GetNative() const noexcept
{
    return m_impl->GetNative();
}

_Use_decl_annotations_
bool kfe::KFEBuffer::IsUploadHeap() const noexcept
{
    return m_impl->IsUploadHeap();
}

_Use_decl_annotations_
bool kfe::KFEBuffer::IsReadbackHeap() const noexcept
{
    return m_impl->IsReadbackHeap();
}

_Use_decl_annotations_
void* kfe::KFEBuffer::GetMappedData() noexcept
{
    return m_impl->GetMappedData();
}

_Use_decl_annotations_
const void* kfe::KFEBuffer::GetMappedData() const noexcept
{
    return m_impl->GetMappedData();
}

_Use_decl_annotations_
bool kfe::KFEBuffer::CopyCPUToGPU(const void* sourceData, std::uint64_t sizeInBytes, std::uint64_t dstOffsetBytes) noexcept
{
    return m_impl->CopyCPUToGPU(sourceData, sizeInBytes, dstOffsetBytes);
}

_Use_decl_annotations_
bool kfe::KFEBuffer::CopyGPUToCPU(void* destination, std::uint64_t sizeInBytes, std::uint64_t srcOffsetBytes) const noexcept
{
    return m_impl->CopyGPUToCPU(destination, sizeInBytes, srcOffsetBytes);
}

_Use_decl_annotations_
void kfe::KFEBuffer::SetDebugName(const std::string& name) noexcept
{
    m_impl->SetDebugName(name);
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFEBuffer::Impl::Initialize(const KFE_CREATE_BUFFER_DESC& desc)
{
    if (m_bInitialized)
    {
        LOG_WARNING("KFEBuffer::Impl::Initialize: Buffer already initialized. Skipping re-initialization.");
        return true;
    }

    if (desc.Device == nullptr || desc.Device->GetNative() == nullptr)
    {
        LOG_ERROR("KFEBuffer::Impl::Initialize: Failed to create buffer. Device or Native Device is null.");
        return false;
    }

    if (desc.SizeInBytes == 0u)
    {
        LOG_ERROR("KFEBuffer::Impl::Initialize: SizeInBytes is 0. Cannot create zero-sized buffer.");
        return false;
    }

    auto* native = desc.Device->GetNative();

    D3D12_HEAP_PROPERTIES properties{};
    properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    properties.Type                 = desc.HeapType;
    properties.VisibleNodeMask      = 1u;
    properties.CreationNodeMask     = 1u;

    D3D12_RESOURCE_DESC resource{};
    resource.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource.Alignment          = 0u;
    resource.Width              = desc.SizeInBytes;
    resource.Height             = 1u;
    resource.DepthOrArraySize   = 1u;
    resource.MipLevels          = 1u;
    resource.Format             = DXGI_FORMAT_UNKNOWN;
    resource.SampleDesc.Count   = 1u;
    resource.SampleDesc.Quality = 0u;
    resource.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource.Flags              = desc.ResourceFlags;

    const HRESULT hr = native->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &resource,
        desc.InitialState,
        nullptr,
        IID_PPV_ARGS(&m_pResource)
    );

    if (FAILED(hr))
    {
        LOG_ERROR("KFEBuffer::Impl::Initialize: Failed to create buffer resource. HRESULT={:#x}", static_cast<unsigned int>(hr));
        return false;
    }

    // Cache
    m_sizeInBytes   = desc.SizeInBytes;
    m_heapType      = desc.HeapType;
    m_resourceFlags = desc.ResourceFlags;
    m_initialState  = desc.InitialState;
    m_szDebugName   = desc.DebugName;

    // Map upload or readback heaps
    if (IsUploadHeap() || IsReadbackHeap())
    {
        HRESULT mapHr = m_pResource->Map(0, nullptr, &m_pMappedData);
        if (FAILED(mapHr))
        {
            m_pMappedData = nullptr;
            m_bMapped     = false;
            LOG_ERROR("KFEBuffer::Impl::Initialize: Failed to map buffer for CPU access. HRESULT={:#x}", static_cast<unsigned int>(mapHr));
            return false;
        }
        m_bMapped = true;
    }

    if (!m_szDebugName.empty())
    {
        SetDebugName(m_szDebugName);
    }

#if defined(DEBUG) || defined(_DEBUG)
    LOG_INFO(
        "KFEBuffer::Initialize Success!\n"
        "  Name          : {}\n"
        "  Size (bytes)  : {}\n"
        "  Heap Type     : {}\n"
        "  Initial State : {}\n"
        "  Flags         : {}\n"
        "  CPU Mapped    : {}\n"
        "  Native Ptr    : {:#x}\n",
        m_szDebugName.empty() ? "<unnamed>" : m_szDebugName,
        m_sizeInBytes,
        HeapTypeToString     (m_heapType),
        ResourceStateToString(m_initialState),
        ResourceFlagsToString(m_resourceFlags),
        m_bMapped ? "YES" : "NO",
        reinterpret_cast<std::uintptr_t>(m_pResource.Get())
    );
#endif

    m_bInitialized = true;
    return true;
}

_Use_decl_annotations_
bool kfe::KFEBuffer::Impl::Destroy() noexcept
{
    if (!m_bInitialized)
    {
        return true;
    }

    if (m_bMapped && m_pResource)
    {
        m_pResource->Unmap(0, nullptr);
    }

    m_pResource.Reset();

    m_pMappedData   = nullptr;
    m_bMapped       = false;
    m_bInitialized  = false;

    m_sizeInBytes   = 0u;
    m_heapType      = static_cast<D3D12_HEAP_TYPE>      (0);
    m_resourceFlags = static_cast<D3D12_RESOURCE_FLAGS> (0);
    m_initialState  = static_cast<D3D12_RESOURCE_STATES>(0);
    m_szDebugName.clear();

    return true;
}

_Use_decl_annotations_
bool kfe::KFEBuffer::Impl::IsInitialized() const noexcept
{
    return m_bInitialized;
}

_Use_decl_annotations_
std::uint64_t kfe::KFEBuffer::Impl::GetSizeInBytes() const noexcept
{
    return m_sizeInBytes;
}

_Use_decl_annotations_
D3D12_HEAP_TYPE kfe::KFEBuffer::Impl::GetHeapType() const noexcept
{
    return m_heapType;
}

_Use_decl_annotations_
D3D12_RESOURCE_STATES kfe::KFEBuffer::Impl::GetInitialState() const noexcept
{
    return m_initialState;
}

_Use_decl_annotations_
ID3D12Resource* kfe::KFEBuffer::Impl::GetNative() const noexcept
{
    return m_pResource.Get();
}

_Use_decl_annotations_
bool kfe::KFEBuffer::Impl::IsUploadHeap() const noexcept
{
    return m_heapType == D3D12_HEAP_TYPE_UPLOAD;
}

_Use_decl_annotations_
bool kfe::KFEBuffer::Impl::IsReadbackHeap() const noexcept
{
    return m_heapType == D3D12_HEAP_TYPE_READBACK;
}

_Use_decl_annotations_
void* kfe::KFEBuffer::Impl::GetMappedData() noexcept
{
    return m_pMappedData;
}

_Use_decl_annotations_
const void* kfe::KFEBuffer::Impl::GetMappedData() const noexcept
{
    return m_pMappedData;
}

_Use_decl_annotations_
bool kfe::KFEBuffer::Impl::CopyCPUToGPU(const void* sourceData, std::uint64_t sizeInBytes, std::uint64_t dstOffsetBytes) noexcept
{
    if (!m_bInitialized)
    {
        LOG_ERROR("KFEBuffer::Impl::CopyCPUToGPU: Buffer not initialized.");
        return false;
    }

    if (!sourceData)
    {
        LOG_ERROR("KFEBuffer::Impl::CopyCPUToGPU: sourceData is null.");
        return false;
    }

    if (dstOffsetBytes + sizeInBytes > m_sizeInBytes)
    {
        LOG_ERROR(
            "KFEBuffer::Impl::CopyCPUToGPU: Out of bounds copy. Size={} Offset={} BufferSize={}",
            sizeInBytes, dstOffsetBytes, m_sizeInBytes);
        return false;
    }

    if (!m_bMapped || m_pMappedData == nullptr)
    {
        LOG_ERROR("KFEBuffer::Impl::CopyCPUToGPU: Buffer is not mapped. Only mapped upload or readback heaps are supported.");
        return false;
    }

    std::byte* dst = static_cast<std::byte*>(m_pMappedData) + dstOffsetBytes;
    std::memcpy(dst, sourceData, static_cast<std::size_t>(sizeInBytes));
    return true;
}

_Use_decl_annotations_
bool kfe::KFEBuffer::Impl::CopyGPUToCPU(void* destination, std::uint64_t sizeInBytes, std::uint64_t srcOffsetBytes) const noexcept
{
    if (!m_bInitialized)
    {
        LOG_ERROR("KFEBuffer::Impl::CopyGPUToCPU: Buffer not initialized.");
        return false;
    }

    if (!destination)
    {
        LOG_ERROR("KFEBuffer::Impl::CopyGPUToCPU: destination pointer is null.");
        return false;
    }

    if (srcOffsetBytes + sizeInBytes > m_sizeInBytes)
    {
        LOG_ERROR(
            "KFEBuffer::Impl::CopyGPUToCPU: Out of bounds copy. Size={} Offset={} BufferSize={}",
            sizeInBytes, srcOffsetBytes, m_sizeInBytes);
        return false;
    }

    if (!m_bMapped || m_pMappedData == nullptr)
    {
        LOG_ERROR("KFEBuffer::Impl::CopyGPUToCPU: Buffer is not mapped, Only mapped READBACK or UPLOAD heaps are supported");
        return false;
    }

    const std::byte* src = static_cast<const std::byte*>(m_pMappedData) + srcOffsetBytes;
    std::memcpy(destination, src, static_cast<std::size_t>(sizeInBytes));
    return true;
}

_Use_decl_annotations_
void kfe::KFEBuffer::Impl::SetDebugName(const std::string& name) noexcept
{
    m_szDebugName = name;

    if (!m_pResource)
    {
        return;
    }

    if (m_szDebugName.empty())
    {
        return;
    }

    std::wstring wname = kfe_helpers::AnsiToWide(name);
    m_pResource->SetName(wname.c_str());
}

#pragma endregion
