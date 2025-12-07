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

#include "engine/render_manager/api/buffer/upload_buffer.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/utils/logger.h"

#include <d3d12.h>
#include <cstring>
#include <limits>

#pragma region Impl_Declaration

class kfe::KFEUploadBuffer::Impl
{
public:
    Impl() = default;
    ~Impl()
    {
        if (!Destroy())
        {
            LOG_ERROR("KFEUploadBuffer::Impl::~Impl: Failed to destroy upload buffer cleanly.");
        }
    }

    NODISCARD bool Initialize(_In_ const KFE_UPLOAD_BUFFER_CREATE_DESC& desc);

    NODISCARD bool Destroy      ()       noexcept;
    NODISCARD bool IsInitialized() const noexcept;

    NODISCARD       KFEBuffer* GetBuffer()       noexcept;
    NODISCARD const KFEBuffer* GetBuffer() const noexcept;

    NODISCARD       void* GetMappedData()       noexcept;
    NODISCARD const void* GetMappedData() const noexcept;

    NODISCARD bool WriteBytes(
        _In_reads_bytes_(numBytes) const void* data,
        std::uint64_t                         numBytes,
        std::uint64_t                         offsetBytes) noexcept;

    NODISCARD std::uint64_t GetSizeInBytes() const noexcept;

private:
    KFEDevice*    m_pDevice    { nullptr };
    KFEBuffer     m_buffer;
    void*         m_mappedData { nullptr };
    std::uint64_t m_sizeInBytes{ 0u };

    bool m_bInitialized{ false };
};

#pragma endregion

#pragma region Class_Implementation

kfe::KFEUploadBuffer::KFEUploadBuffer() noexcept
    : m_impl(std::make_unique<kfe::KFEUploadBuffer::Impl>())
{}

kfe::KFEUploadBuffer::~KFEUploadBuffer() noexcept = default;

kfe::KFEUploadBuffer::KFEUploadBuffer(KFEUploadBuffer&& other) noexcept = default;
kfe::KFEUploadBuffer& kfe::KFEUploadBuffer::operator=(KFEUploadBuffer&& other) noexcept = default;

_Use_decl_annotations_
std::string kfe::KFEUploadBuffer::GetName() const noexcept
{
    return "KFEUploadBuffer";
}

_Use_decl_annotations_
std::string kfe::KFEUploadBuffer::GetDescription() const noexcept
{
    return "Upload buffer wrapper: owns a KFEBuffer in UPLOAD heap and exposes mapped writes.";
}

_Use_decl_annotations_
bool kfe::KFEUploadBuffer::Initialize(const KFE_UPLOAD_BUFFER_CREATE_DESC& desc)
{
    return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFEUploadBuffer::Destroy() noexcept
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFEUploadBuffer::IsInitialized() const noexcept
{
    return m_impl->IsInitialized();
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEUploadBuffer::GetBuffer() noexcept
{
    return m_impl->GetBuffer();
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEUploadBuffer::GetBuffer() const noexcept
{
    return m_impl->GetBuffer();
}

_Use_decl_annotations_
void* kfe::KFEUploadBuffer::GetMappedData() noexcept
{
    return m_impl->GetMappedData();
}

_Use_decl_annotations_
const void* kfe::KFEUploadBuffer::GetMappedData() const noexcept
{
    return m_impl->GetMappedData();
}

_Use_decl_annotations_
bool kfe::KFEUploadBuffer::WriteBytes(
    _In_reads_bytes_(numBytes) const void* data,
    std::uint64_t                         numBytes,
    std::uint64_t                         offsetBytes) noexcept
{
    return m_impl->WriteBytes(data, numBytes, offsetBytes);
}

_Use_decl_annotations_
std::uint64_t kfe::KFEUploadBuffer::GetSizeInBytes() const noexcept
{
    return m_impl->GetSizeInBytes();
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFEUploadBuffer::Impl::Initialize(const KFE_UPLOAD_BUFFER_CREATE_DESC& desc)
{
    // Validate device
    if (!desc.Device || !desc.Device->GetNative())
    {
        LOG_ERROR("KFEUploadBuffer::Impl::Initialize: Device or native device is null.");
        return false;
    }

    if (desc.SizeInBytes == 0u)
    {
        LOG_ERROR("KFEUploadBuffer::Impl::Initialize: SizeInBytes must be > 0.");
        return false;
    }

    // If already initialized, destroy and REcreate it 
    if (IsInitialized())
    {
        LOG_WARNING("KFEUploadBuffer::Impl::Initialize: Already initialized. Destroying and recreating.");
        if (!Destroy())
        {
            LOG_ERROR("KFEUploadBuffer::Impl::Initialize: Destroy failed; cannot reinitialize.");
            return false;
        }
    }

    m_pDevice     = desc.Device;
    m_sizeInBytes = desc.SizeInBytes;
    m_mappedData  = nullptr;

    // Create an upload heap KFEBuffer
    KFE_CREATE_BUFFER_DESC bufferDesc{};
    bufferDesc.Device        = m_pDevice;
    bufferDesc.SizeInBytes   = m_sizeInBytes;
    bufferDesc.HeapType      = D3D12_HEAP_TYPE_UPLOAD;
    bufferDesc.ResourceFlags = static_cast<D3D12_RESOURCE_FLAGS>(0);
    bufferDesc.InitialState  = D3D12_RESOURCE_STATE_GENERIC_READ;
    bufferDesc.DebugName     = "KFEUploadBuffer";

    if (!m_buffer.Initialize(bufferDesc))
    {
        LOG_ERROR("KFEUploadBuffer::Impl::Initialize: Failed to initialize underlying KFEBuffer.");
        m_pDevice     = nullptr;
        m_sizeInBytes = 0u;
        return false;
    }

    // For an upload heap, KFEBuffer is expected to map and expose a persistent pointer
    m_mappedData = m_buffer.GetMappedData();
    if (!m_mappedData)
    {
        LOG_ERROR(
            "KFEUploadBuffer::Impl::Initialize: Underlying KFEBuffer did not provide a mapped pointer "
            "for an UPLOAD heap. GetMappedData() returned nullptr.");
        if (!m_buffer.Destroy())
        {
            LOG_ERROR("BUFFER IS ALSO NONE!");
        }
        m_pDevice     = nullptr;
        m_sizeInBytes = 0u;
        return false;
    }

    LOG_SUCCESS(
        "KFEUploadBuffer::Impl::Initialize: Initialized upload buffer. SizeInBytes={}.",
        m_sizeInBytes);

    m_bInitialized = true;
    return true;
}

_Use_decl_annotations_
bool kfe::KFEUploadBuffer::Impl::Destroy() noexcept
{
    if (m_buffer.IsInitialized())
    {
        if (!m_buffer.Destroy())
        {
            LOG_ERROR("KFEUploadBuffer::Impl::Destroy: Failed to destroy underlying KFEBuffer.");
        }
    }

    m_pDevice       = nullptr;
    m_mappedData    = nullptr;
    m_sizeInBytes   = 0u;
    m_bInitialized  = false;

    return true;
}

_Use_decl_annotations_
bool kfe::KFEUploadBuffer::Impl::IsInitialized() const noexcept
{
    return m_bInitialized;
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEUploadBuffer::Impl::GetBuffer() noexcept
{
    if (!m_bInitialized)
    {
        return nullptr;
    }
    return &m_buffer;
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEUploadBuffer::Impl::GetBuffer() const noexcept
{
    if (!m_bInitialized)
    {
        return nullptr;
    }
    return &m_buffer;
}

_Use_decl_annotations_
void* kfe::KFEUploadBuffer::Impl::GetMappedData() noexcept
{
    return m_mappedData;
}

_Use_decl_annotations_
const void* kfe::KFEUploadBuffer::Impl::GetMappedData() const noexcept
{
    return m_mappedData;
}

_Use_decl_annotations_
bool kfe::KFEUploadBuffer::Impl::WriteBytes(
    _In_reads_bytes_(numBytes) const void* data,
    std::uint64_t                          numBytes,
    std::uint64_t                          offsetBytes) noexcept
{
    if (!m_bInitialized)
    {
        LOG_ERROR("KFEUploadBuffer::Impl::WriteBytes: Upload buffer not initialized.");
        return false;
    }

    if (!data)
    {
        LOG_ERROR("KFEUploadBuffer::Impl::WriteBytes: Source data pointer is null.");
        return false;
    }

    if (!m_mappedData)
    {
        LOG_ERROR("KFEUploadBuffer::Impl::WriteBytes: Mapped data pointer is null.");
        return false;
    }

    if (offsetBytes > m_sizeInBytes)
    {
        LOG_ERROR(
            "KFEUploadBuffer::Impl::WriteBytes: offsetBytes ({}) is > buffer size ({}).",
            offsetBytes, m_sizeInBytes);
        return false;
    }

    if (offsetBytes + numBytes > m_sizeInBytes)
    {
        LOG_ERROR(
            "KFEUploadBuffer::Impl::WriteBytes: Range [offset={}, offset+size={}] exceeds buffer size ({}).",
            offsetBytes,
            offsetBytes + numBytes,
            m_sizeInBytes);
        return false;
    }

    auto* dst = static_cast<std::uint8_t*>(m_mappedData) + offsetBytes;
    std::memcpy(dst, data, static_cast<std::size_t>(numBytes));

    return true;
}

_Use_decl_annotations_
std::uint64_t kfe::KFEUploadBuffer::Impl::GetSizeInBytes() const noexcept
{
    return m_sizeInBytes;
}

#pragma endregion
