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

#include "engine/render_manager/api/buffer/readback_buffer.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/utils/logger.h"

#include <d3d12.h>
#include <cstring>
#include <utility>

#pragma region Impl_Declaration

class kfe::KFEReadbackBuffer::Impl
{
public:
    Impl() = default;
    ~Impl()
    {
        if (!Destroy())
        {
            LOG_ERROR("KFEReadbackBuffer::Impl::~Impl: Failed to destroy readback buffer cleanly.");
        }
    }

    NODISCARD bool Initialize(_In_ const KFE_READBACK_BUFFER_CREATE_DESC& desc);

    NODISCARD bool Destroy      () noexcept;
    NODISCARD bool IsInitialized() const noexcept;

    NODISCARD       KFEBuffer* GetBuffer()       noexcept;
    NODISCARD const KFEBuffer* GetBuffer() const noexcept;

    NODISCARD       void* GetMappedData()       noexcept;
    NODISCARD const void* GetMappedData() const noexcept;

    NODISCARD std::uint64_t GetSizeInBytes() const noexcept;

    NODISCARD bool ReadBytes(
        _Out_writes_bytes_(numBytes) void* destination,
        std::uint64_t                     numBytes,
        std::uint64_t                     srcOffsetBytes) const noexcept;

private:
    KFEDevice*    m_pDevice     { nullptr };
    KFEBuffer     m_buffer;
    void*         m_mappedData  { nullptr };
    std::uint64_t m_sizeInBytes { 0u };
    bool          m_bInitialized{ false };
};

#pragma endregion

#pragma region Class_Implementation

kfe::KFEReadbackBuffer::KFEReadbackBuffer() noexcept
    : m_impl(std::make_unique<kfe::KFEReadbackBuffer::Impl>())
{
}

kfe::KFEReadbackBuffer::~KFEReadbackBuffer() noexcept = default;

kfe::KFEReadbackBuffer::KFEReadbackBuffer(KFEReadbackBuffer&& other) noexcept = default;
kfe::KFEReadbackBuffer& kfe::KFEReadbackBuffer::operator=(KFEReadbackBuffer&& other) noexcept = default;

_Use_decl_annotations_
std::string kfe::KFEReadbackBuffer::GetName() const noexcept
{
    return "KFEReadbackBuffer";
}

_Use_decl_annotations_
std::string kfe::KFEReadbackBuffer::GetDescription() const noexcept
{
    return "Readback buffer wrapper: owns a KFEBuffer in READBACK heap and exposes mapped CPU reads.";
}

_Use_decl_annotations_
bool kfe::KFEReadbackBuffer::Initialize(const KFE_READBACK_BUFFER_CREATE_DESC& desc)
{
    return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFEReadbackBuffer::Destroy() noexcept
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFEReadbackBuffer::IsInitialized() const noexcept
{
    return m_impl->IsInitialized();
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEReadbackBuffer::GetBuffer() noexcept
{
    return m_impl->GetBuffer();
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEReadbackBuffer::GetBuffer() const noexcept
{
    return m_impl->GetBuffer();
}

_Use_decl_annotations_
void* kfe::KFEReadbackBuffer::GetMappedData() noexcept
{
    return m_impl->GetMappedData();
}

_Use_decl_annotations_
const void* kfe::KFEReadbackBuffer::GetMappedData() const noexcept
{
    return m_impl->GetMappedData();
}

_Use_decl_annotations_
std::uint64_t kfe::KFEReadbackBuffer::GetSizeInBytes() const noexcept
{
    return m_impl->GetSizeInBytes();
}

_Use_decl_annotations_
bool kfe::KFEReadbackBuffer::ReadBytes(
    _Out_writes_bytes_(numBytes) void* destination,
    std::uint64_t                     numBytes,
    std::uint64_t                     srcOffsetBytes) const noexcept
{
    return m_impl->ReadBytes(destination, numBytes, srcOffsetBytes);
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFEReadbackBuffer::Impl::Initialize(const KFE_READBACK_BUFFER_CREATE_DESC& desc)
{
    // Validate device
    if (!desc.Device || !desc.Device->GetNative())
    {
        LOG_ERROR("KFEReadbackBuffer::Impl::Initialize: Device or native device is null.");
        return false;
    }

    if (desc.SizeInBytes == 0u)
    {
        LOG_ERROR("KFEReadbackBuffer::Impl::Initialize: SizeInBytes must be > 0.");
        return false;
    }

    // If already initialized, destroy and re-init
    if (IsInitialized())
    {
        LOG_WARNING("KFEReadbackBuffer::Impl::Initialize: Already initialized. Destroying and recreating.");
        if (!Destroy())
        {
            LOG_ERROR("KFEReadbackBuffer::Impl::Initialize: Destroy failed; cannot reinitialize.");
            return false;
        }
    }

    m_pDevice       = desc.Device;
    m_sizeInBytes   = desc.SizeInBytes;
    m_mappedData    = nullptr;

    // Create a READBACK heap buffer
    KFE_CREATE_BUFFER_DESC bufferDesc{};
    bufferDesc.Device        = m_pDevice;
    bufferDesc.SizeInBytes   = m_sizeInBytes;
    bufferDesc.HeapType      = D3D12_HEAP_TYPE_READBACK;
    bufferDesc.ResourceFlags = static_cast<D3D12_RESOURCE_FLAGS>(0);
    bufferDesc.InitialState  = D3D12_RESOURCE_STATE_COPY_DEST;

    if (!m_buffer.Initialize(bufferDesc))
    {
        LOG_ERROR("KFEReadbackBuffer::Impl::Initialize: Failed to initialize underlying KFEBuffer.");
        m_pDevice     = nullptr;
        m_sizeInBytes = 0u;
        return false;
    }

    // READBACK heaps Buffer is expected to map and expose a persistent pointer
    m_mappedData = m_buffer.GetMappedData();
    if (!m_mappedData)
    {
        LOG_ERROR(
            "KFEReadbackBuffer::Impl::Initialize: Underlying KFEBuffer did not provide a mapped pointer "
            "for a READBACK heap. GetMappedData() returned nullptr.");
        if (!m_buffer.Destroy())
        {
            LOG_ERROR("Failed to Release Buffer Resources!");
        }
        m_pDevice     = nullptr;
        m_sizeInBytes = 0u;
        return false;
    }

    LOG_SUCCESS(
        "KFEReadbackBuffer::Impl::Initialize: Initialized readback buffer. SizeInBytes={}",
        m_sizeInBytes);

    m_bInitialized = true;
    return true;
}

_Use_decl_annotations_
bool kfe::KFEReadbackBuffer::Impl::Destroy() noexcept
{
    if (m_buffer.IsInitialized())
    {
        if (!m_buffer.Destroy())
        {
            LOG_ERROR("KFEReadbackBuffer::Impl::Destroy: Failed to destroy underlying KFEBuffer.");
        }
    }

    m_pDevice       = nullptr;
    m_mappedData    = nullptr;
    m_sizeInBytes   = 0u;
    m_bInitialized  = false;

    return true;
}

_Use_decl_annotations_
bool kfe::KFEReadbackBuffer::Impl::IsInitialized() const noexcept
{
    return m_bInitialized;
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEReadbackBuffer::Impl::GetBuffer() noexcept
{
    if (!m_bInitialized)
    {
        return nullptr;
    }
    return &m_buffer;
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEReadbackBuffer::Impl::GetBuffer() const noexcept
{
    if (!m_bInitialized)
    {
        return nullptr;
    }
    return &m_buffer;
}

_Use_decl_annotations_
void* kfe::KFEReadbackBuffer::Impl::GetMappedData() noexcept
{
    return m_mappedData;
}

_Use_decl_annotations_
const void* kfe::KFEReadbackBuffer::Impl::GetMappedData() const noexcept
{
    return m_mappedData;
}

_Use_decl_annotations_
std::uint64_t kfe::KFEReadbackBuffer::Impl::GetSizeInBytes() const noexcept
{
    return m_sizeInBytes;
}

_Use_decl_annotations_
bool kfe::KFEReadbackBuffer::Impl::ReadBytes(
    _Out_writes_bytes_(numBytes) void* destination,
    std::uint64_t                      numBytes,
    std::uint64_t                      srcOffsetBytes) const noexcept
{
    if (!m_bInitialized)
    {
        LOG_ERROR("KFEReadbackBuffer::Impl::ReadBytes: Readback buffer not initialized.");
        return false;
    }

    if (!destination)
    {
        LOG_ERROR("KFEReadbackBuffer::Impl::ReadBytes: Destination pointer is null.");
        return false;
    }

    if (!m_mappedData)
    {
        LOG_ERROR("KFEReadbackBuffer::Impl::ReadBytes: Mapped data pointer is null.");
        return false;
    }

    if (srcOffsetBytes > m_sizeInBytes)
    {
        LOG_ERROR(
            "KFEReadbackBuffer::Impl::ReadBytes: srcOffsetBytes ({}) is > buffer size ({}).",
            srcOffsetBytes, m_sizeInBytes);
        return false;
    }

    if (srcOffsetBytes + numBytes > m_sizeInBytes)
    {
        LOG_ERROR(
            "KFEReadbackBuffer::Impl::ReadBytes: Range [offset={}, offset+size={}] exceeds buffer size ({}).",
            srcOffsetBytes,
            srcOffsetBytes + numBytes,
            m_sizeInBytes);
        return false;
    }

    const auto* src = static_cast<const std::uint8_t*>(m_mappedData) + srcOffsetBytes;
    std::memcpy(destination, src, static_cast<std::size_t>(numBytes));

    return true;
}

#pragma endregion
