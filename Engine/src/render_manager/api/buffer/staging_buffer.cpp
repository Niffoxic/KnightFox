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

#include "engine/render_manager/api/buffer/staging_buffer.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/utils/logger.h"

#include <d3d12.h>
#include <cstring>
#include <utility>

#pragma region Impl_Declaration

class kfe::KFEStagingBuffer::Impl
{
public:
    Impl() = default;
    ~Impl()
    {
        if (!Destroy())
        {
            LOG_ERROR("KFEStagingBuffer::Impl::~Impl: Failed to destroy staging buffer cleanly.");
        }
    }

    NODISCARD bool Initialize(_In_ const KFE_STAGING_BUFFER_CREATE_DESC& desc);

    NODISCARD bool Destroy()       noexcept;
    NODISCARD bool IsInitialized() const noexcept;

    NODISCARD KFEBuffer* GetUploadBuffer()       noexcept;
    NODISCARD const KFEBuffer* GetUploadBuffer() const noexcept;

    NODISCARD KFEBuffer* GetDefaultBuffer()       noexcept;
    NODISCARD const KFEBuffer* GetDefaultBuffer() const noexcept;

    NODISCARD std::uint64_t GetSizeInBytes() const noexcept;
    NODISCARD void* GetMappedData()       noexcept;
    NODISCARD const void* GetMappedData() const noexcept;

    NODISCARD bool WriteBytes(
        _In_reads_bytes_(numBytes) const void* data,
        std::uint64_t                          numBytes,
        std::uint64_t                          dstOffsetBytes) noexcept;

    NODISCARD bool RecordUploadToDefault(
        _In_ ID3D12GraphicsCommandList* cmdList,
        std::uint64_t                   numBytes,
        std::uint64_t                   srcOffsetBytes,
        std::uint64_t                   dstOffsetBytes) const noexcept;

private:
    KFEDevice* m_pDevice{ nullptr };

    KFEBuffer     m_uploadBuffer;   // UPLOAD heap
    KFEBuffer     m_defaultBuffer;  // DEFAULT heap

    void* m_mappedUpload{ nullptr };
    std::uint64_t m_sizeInBytes{ 0u };

    bool          m_bInitialized{ false };
};

#pragma endregion

#pragma region Class_Implementation

kfe::KFEStagingBuffer::KFEStagingBuffer() noexcept
    : m_impl(std::make_unique<kfe::KFEStagingBuffer::Impl>())
{
}

kfe::KFEStagingBuffer::~KFEStagingBuffer() noexcept = default;

kfe::KFEStagingBuffer::KFEStagingBuffer(KFEStagingBuffer&& other) noexcept = default;
kfe::KFEStagingBuffer& kfe::KFEStagingBuffer::operator=(KFEStagingBuffer&& other) noexcept = default;

_Use_decl_annotations_
std::string kfe::KFEStagingBuffer::GetName() const noexcept
{
    return "KFEStagingBuffer";
}

_Use_decl_annotations_
std::string kfe::KFEStagingBuffer::GetDescription() const noexcept
{
    return "Staging buffer: owns an UPLOAD + DEFAULT KFEBuffer pair and records CopyBufferRegion commands.";
}

_Use_decl_annotations_
bool kfe::KFEStagingBuffer::Initialize(const KFE_STAGING_BUFFER_CREATE_DESC& desc)
{
    return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFEStagingBuffer::Destroy() noexcept
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFEStagingBuffer::IsInitialized() const noexcept
{
    return m_impl->IsInitialized();
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEStagingBuffer::GetUploadBuffer() noexcept
{
    return m_impl->GetUploadBuffer();
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEStagingBuffer::GetUploadBuffer() const noexcept
{
    return m_impl->GetUploadBuffer();
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEStagingBuffer::GetDefaultBuffer() noexcept
{
    return m_impl->GetDefaultBuffer();
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEStagingBuffer::GetDefaultBuffer() const noexcept
{
    return m_impl->GetDefaultBuffer();
}

_Use_decl_annotations_
std::uint64_t kfe::KFEStagingBuffer::GetSizeInBytes() const noexcept
{
    return m_impl->GetSizeInBytes();
}

_Use_decl_annotations_
void* kfe::KFEStagingBuffer::GetMappedData() noexcept
{
    return m_impl->GetMappedData();
}

_Use_decl_annotations_
const void* kfe::KFEStagingBuffer::GetMappedData() const noexcept
{
    return m_impl->GetMappedData();
}

_Use_decl_annotations_
bool kfe::KFEStagingBuffer::WriteBytes(
    _In_reads_bytes_(numBytes) const void* data,
    std::uint64_t                          numBytes,
    std::uint64_t                          dstOffsetBytes) noexcept
{
    return m_impl->WriteBytes(data, numBytes, dstOffsetBytes);
}

_Use_decl_annotations_
bool kfe::KFEStagingBuffer::RecordUploadToDefault(
    _In_ ID3D12GraphicsCommandList* cmdList,
    std::uint64_t                   numBytes,
    std::uint64_t                   srcOffsetBytes,
    std::uint64_t                   dstOffsetBytes) const noexcept
{
    return m_impl->RecordUploadToDefault(cmdList, numBytes, srcOffsetBytes, dstOffsetBytes);
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFEStagingBuffer::Impl::Initialize(const KFE_STAGING_BUFFER_CREATE_DESC& desc)
{
    // Validate device
    if (!desc.Device || !desc.Device->GetNative())
    {
        LOG_ERROR("KFEStagingBuffer::Impl::Initialize: Device or native device is null.");
        return false;
    }

    if (desc.SizeInBytes == 0u)
    {
        LOG_ERROR("KFEStagingBuffer::Impl::Initialize: SizeInBytes must be > 0.");
        return false;
    }

    // If already initialized, destroy and recreate
    if (IsInitialized())
    {
        LOG_WARNING("KFEStagingBuffer::Impl::Initialize: Already initialized. Destroying and recreating.");
        if (!Destroy())
        {
            LOG_ERROR("KFEStagingBuffer::Impl::Initialize: Destroy failed; cannot reinitialize.");
            return false;
        }
    }

    m_pDevice      = desc.Device;
    m_sizeInBytes  = desc.SizeInBytes;
    m_mappedUpload = nullptr;

    // Create the UPLOAD heap buffer
    KFE_CREATE_BUFFER_DESC uploadDesc{};
    uploadDesc.Device        = m_pDevice;
    uploadDesc.SizeInBytes   = m_sizeInBytes;
    uploadDesc.HeapType      = D3D12_HEAP_TYPE_UPLOAD;
    uploadDesc.ResourceFlags = static_cast<D3D12_RESOURCE_FLAGS>(0);
    uploadDesc.InitialState  = D3D12_RESOURCE_STATE_GENERIC_READ;
    uploadDesc.DebugName     = "KFEStagingBuffer_Upload";

    if (!m_uploadBuffer.Initialize(uploadDesc))
    {
        LOG_ERROR("KFEStagingBuffer::Impl::Initialize: Failed to create UPLOAD buffer.");
        m_pDevice     = nullptr;
        m_sizeInBytes = 0u;
        return false;
    }

    // Expect KFEBuffer to map UPLOAD heaps and expose a persistent pointer
    m_mappedUpload = m_uploadBuffer.GetMappedData();
    if (!m_mappedUpload)
    {
        LOG_ERROR(
            "KFEStagingBuffer::Impl::Initialize: UPLOAD buffer did not provide mapped data pointer.");
        
        if (!m_uploadBuffer.Destroy())
        {
            LOG_ERROR("Failed to destroy upload buffer!");
        }
        m_pDevice     = nullptr;
        m_sizeInBytes = 0u;
        return false;
    }

    // Create the DEFAULT heap buffer
    KFE_CREATE_BUFFER_DESC defaultDesc{};
    defaultDesc.Device        = m_pDevice;
    defaultDesc.SizeInBytes   = m_sizeInBytes;
    defaultDesc.HeapType      = D3D12_HEAP_TYPE_DEFAULT;
    defaultDesc.ResourceFlags = static_cast<D3D12_RESOURCE_FLAGS>(0);
    defaultDesc.InitialState  = D3D12_RESOURCE_STATE_COPY_DEST;
    defaultDesc.DebugName     = "KFEStagingBuffer_Default";

    if (!m_defaultBuffer.Initialize(defaultDesc))
    {
        LOG_ERROR("KFEStagingBuffer::Impl::Initialize: Failed to create DEFAULT buffer.");

        if (!m_uploadBuffer.Destroy())
        {
            LOG_ERROR("Failed to destroy upload buffer!");
        }

        m_mappedUpload = nullptr;
        m_pDevice      = nullptr;
        m_sizeInBytes  = 0u;

        return false;
    }

    LOG_SUCCESS(
        "KFEStagingBuffer::Impl::Initialize: Initialized staging buffer. SizeInBytes={}.",
        m_sizeInBytes);

    m_bInitialized = true;
    return true;
}

_Use_decl_annotations_
bool kfe::KFEStagingBuffer::Impl::Destroy() noexcept
{
    if (m_uploadBuffer.IsInitialized())
    {
        if (!m_uploadBuffer.Destroy())
        {
            LOG_ERROR("KFEStagingBuffer::Impl::Destroy: Failed to destroy UPLOAD buffer.");
        }
    }

    if (m_defaultBuffer.IsInitialized())
    {
        if (!m_defaultBuffer.Destroy())
        {
            LOG_ERROR("KFEStagingBuffer::Impl::Destroy: Failed to destroy DEFAULT buffer.");
        }
    }

    m_pDevice       = nullptr;
    m_mappedUpload  = nullptr;
    m_sizeInBytes   = 0u;
    m_bInitialized  = false;

    return true;
}

_Use_decl_annotations_
bool kfe::KFEStagingBuffer::Impl::IsInitialized() const noexcept
{
    return m_bInitialized;
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEStagingBuffer::Impl::GetUploadBuffer() noexcept
{
    if (!m_bInitialized)
    {
        return nullptr;
    }
    return &m_uploadBuffer;
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEStagingBuffer::Impl::GetUploadBuffer() const noexcept
{
    if (!m_bInitialized)
    {
        return nullptr;
    }
    return &m_uploadBuffer;
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEStagingBuffer::Impl::GetDefaultBuffer() noexcept
{
    if (!m_bInitialized)
    {
        return nullptr;
    }
    return &m_defaultBuffer;
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEStagingBuffer::Impl::GetDefaultBuffer() const noexcept
{
    if (!m_bInitialized)
    {
        return nullptr;
    }
    return &m_defaultBuffer;
}

_Use_decl_annotations_
std::uint64_t kfe::KFEStagingBuffer::Impl::GetSizeInBytes() const noexcept
{
    return m_sizeInBytes;
}

_Use_decl_annotations_
void* kfe::KFEStagingBuffer::Impl::GetMappedData() noexcept
{
    return m_mappedUpload;
}

_Use_decl_annotations_
const void* kfe::KFEStagingBuffer::Impl::GetMappedData() const noexcept
{
    return m_mappedUpload;
}

_Use_decl_annotations_
bool kfe::KFEStagingBuffer::Impl::WriteBytes(
    _In_reads_bytes_(numBytes) const void* data,
    std::uint64_t                          numBytes,
    std::uint64_t                          dstOffsetBytes) noexcept
{
    if (!m_bInitialized)
    {
        LOG_ERROR("KFEStagingBuffer::Impl::WriteBytes: Staging buffer not initialized.");
        return false;
    }

    if (!data)
    {
        LOG_ERROR("KFEStagingBuffer::Impl::WriteBytes: Source data pointer is null.");
        return false;
    }

    if (!m_mappedUpload)
    {
        LOG_ERROR("KFEStagingBuffer::Impl::WriteBytes: Upload buffer mapped pointer is null.");
        return false;
    }

    if (dstOffsetBytes > m_sizeInBytes)
    {
        LOG_ERROR(
            "KFEStagingBuffer::Impl::WriteBytes: dstOffsetBytes ({}) is > buffer size ({}).",
            dstOffsetBytes, m_sizeInBytes);
        return false;
    }

    if (dstOffsetBytes + numBytes > m_sizeInBytes)
    {
        LOG_ERROR(
            "KFEStagingBuffer::Impl::WriteBytes: Range [offset={}, offset+size={}] exceeds buffer size ({}).",
            dstOffsetBytes,
            dstOffsetBytes + numBytes,
            m_sizeInBytes);
        return false;
    }

    auto* dst = static_cast<std::uint8_t*>(m_mappedUpload) + dstOffsetBytes;
    std::memcpy(dst, data, static_cast<std::size_t>(numBytes));

    return true;
}

_Use_decl_annotations_
bool kfe::KFEStagingBuffer::Impl::RecordUploadToDefault(
    _In_ ID3D12GraphicsCommandList* cmdList,
    std::uint64_t                   numBytes,
    std::uint64_t                   srcOffsetBytes,
    std::uint64_t                   dstOffsetBytes) const noexcept
{
    if (!m_bInitialized)
    {
        LOG_ERROR("KFEStagingBuffer::Impl::RecordUploadToDefault: Staging buffer not initialized.");
        return false;
    }

    if (!cmdList)
    {
        LOG_ERROR("KFEStagingBuffer::Impl::RecordUploadToDefault: cmdList is null.");
        return false;
    }

    if (numBytes == 0u)
    {
        LOG_WARNING("KFEStagingBuffer::Impl::RecordUploadToDefault: numBytes == 0, nothing to copy.");
        return true;
    }

    if (srcOffsetBytes > m_sizeInBytes || dstOffsetBytes > m_sizeInBytes)
    {
        LOG_ERROR(
            "KFEStagingBuffer::Impl::RecordUploadToDefault: srcOffsetBytes={} or dstOffsetBytes={} "
            "is > buffer size ({}).",
            srcOffsetBytes, dstOffsetBytes, m_sizeInBytes);
        return false;
    }

    if (srcOffsetBytes + numBytes > m_sizeInBytes ||
        dstOffsetBytes + numBytes > m_sizeInBytes)
    {
        LOG_ERROR(
            "KFEStagingBuffer::Impl::RecordUploadToDefault: Copy range exceeds buffer size ({}). "
            "SrcRange=[{}, {}], DstRange=[{}, {}], NumBytes={}.",
            m_sizeInBytes,
            srcOffsetBytes,
            srcOffsetBytes + numBytes,
            dstOffsetBytes,
            dstOffsetBytes + numBytes,
            numBytes);
        return false;
    }

    ID3D12Resource* uploadResource = m_uploadBuffer.GetNative();
    ID3D12Resource* defaultResource = m_defaultBuffer.GetNative();

    if (!uploadResource || !defaultResource)
    {
        LOG_ERROR("KFEStagingBuffer::Impl::RecordUploadToDefault: Underlying resources are null.");
        return false;
    }

    cmdList->CopyBufferRegion(
        defaultResource,
        static_cast<UINT64>(dstOffsetBytes),
        uploadResource,
        static_cast<UINT64>(srcOffsetBytes),
        static_cast<UINT64>(numBytes)
    );

    return true;
}

#pragma endregion
