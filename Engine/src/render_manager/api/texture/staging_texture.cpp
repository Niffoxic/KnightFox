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

#include "engine/render_manager/api/texture/staging_texture.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/texture/texture.h"
#include "engine/render_manager/api/components/device.h"

#include "engine/utils/logger.h"

#include <d3d12.h>
#include <dxgiformat.h>
#include <cstring>

#pragma region Impl_Declaration

class kfe::KFEStagingTexture::Impl
{
public:
    Impl() = default;
    ~Impl()
    {
        if (!Destroy())
        {
            LOG_ERROR("KFEStagingTexture::Impl::~Impl: Failed to destroy staging texture cleanly.");
        }
    }

    NODISCARD bool Initialize(_In_ const KFE_STAGING_TEXTURE_CREATE_DESC& desc);

    NODISCARD bool Destroy()       noexcept;
    NODISCARD bool IsInitialized() const noexcept;

    NODISCARD KFEBuffer* GetUploadBuffer()       noexcept;
    NODISCARD const KFEBuffer* GetUploadBuffer() const noexcept;

    NODISCARD KFETexture* GetTexture()       noexcept;
    NODISCARD const KFETexture* GetTexture() const noexcept;

    NODISCARD std::uint32_t GetWidth() const noexcept;
    NODISCARD std::uint32_t GetHeight() const noexcept;
    NODISCARD DXGI_FORMAT   GetFormat() const noexcept;

    NODISCARD bool WritePixels(
        _In_ const void* data,
        std::uint32_t    srcRowPitchBytes) noexcept;

    NODISCARD bool RecordUploadToTexture(
        _In_ ID3D12GraphicsCommandList* cmdList) const noexcept;

private:
    KFEDevice* m_pDevice{ nullptr };
    KFEBuffer   m_uploadBuffer{};
    KFETexture  m_texture{};

    void* m_mappedUpload{ nullptr };

    std::uint32_t m_width{ 0u };
    std::uint32_t m_height{ 0u };
    DXGI_FORMAT   m_format{ DXGI_FORMAT_UNKNOWN };
    std::uint32_t m_mipLevels{ 1u };
    std::uint32_t m_arraySize{ 1u };

    // Copyable footprint data for CopyTextureRegion
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT m_footprint{};
    UINT   m_numRows{ 0u };
    UINT64 m_rowSizeInBytes{ 0u };
    UINT64 m_totalBytes{ 0u };

    bool   m_bInitialized{ false };
};

#pragma endregion

#pragma region Class_Implementation

kfe::KFEStagingTexture::KFEStagingTexture() noexcept
    : m_impl(std::make_unique<kfe::KFEStagingTexture::Impl>())
{
}

kfe::KFEStagingTexture::~KFEStagingTexture() noexcept = default;

kfe::KFEStagingTexture::KFEStagingTexture(KFEStagingTexture&& other) noexcept = default;
kfe::KFEStagingTexture& kfe::KFEStagingTexture::operator=(KFEStagingTexture&& other) noexcept = default;

_Use_decl_annotations_
std::string kfe::KFEStagingTexture::GetName() const noexcept
{
    return "KFEStagingTexture";
}

_Use_decl_annotations_
std::string kfe::KFEStagingTexture::GetDescription() const noexcept
{
    return "Staging texture: owns an UPLOAD buffer + DEFAULT texture and records CopyTextureRegion.";
}

_Use_decl_annotations_
bool kfe::KFEStagingTexture::Initialize(const KFE_STAGING_TEXTURE_CREATE_DESC& desc)
{
    return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFEStagingTexture::Destroy() noexcept
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFEStagingTexture::IsInitialized() const noexcept
{
    return m_impl->IsInitialized();
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEStagingTexture::GetUploadBuffer() noexcept
{
    return m_impl->GetUploadBuffer();
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEStagingTexture::GetUploadBuffer() const noexcept
{
    return m_impl->GetUploadBuffer();
}

_Use_decl_annotations_
kfe::KFETexture* kfe::KFEStagingTexture::GetTexture() noexcept
{
    return m_impl->GetTexture();
}

_Use_decl_annotations_
const kfe::KFETexture* kfe::KFEStagingTexture::GetTexture() const noexcept
{
    return m_impl->GetTexture();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStagingTexture::GetWidth() const noexcept
{
    return m_impl->GetWidth();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStagingTexture::GetHeight() const noexcept
{
    return m_impl->GetHeight();
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFEStagingTexture::GetFormat() const noexcept
{
    return m_impl->GetFormat();
}

_Use_decl_annotations_
bool kfe::KFEStagingTexture::WritePixels(
    _In_ const void* data,
    std::uint32_t    srcRowPitchBytes) noexcept
{
    return m_impl->WritePixels(data, srcRowPitchBytes);
}

_Use_decl_annotations_
bool kfe::KFEStagingTexture::RecordUploadToTexture(
    _In_ ID3D12GraphicsCommandList* cmdList) const noexcept
{
    return m_impl->RecordUploadToTexture(cmdList);
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFEStagingTexture::Impl::Initialize(const KFE_STAGING_TEXTURE_CREATE_DESC& desc)
{
    if (!desc.Device || !desc.Device->GetNative())
    {
        LOG_ERROR("KFEStagingTexture::Impl::Initialize: Device or native device is null.");
        return false;
    }

    if (desc.Width == 0u || desc.Height == 0u)
    {
        LOG_ERROR("KFEStagingTexture::Impl::Initialize: Width/Height must be > 0.");
        return false;
    }

    if (desc.Format == DXGI_FORMAT_UNKNOWN)
    {
        LOG_ERROR("KFEStagingTexture::Impl::Initialize: DXGI_FORMAT_UNKNOWN is not allowed.");
        return false;
    }

    if (IsInitialized())
    {
        LOG_WARNING("KFEStagingTexture::Impl::Initialize: Already initialized. Destroying and recreating.");
        if (!Destroy())
        {
            LOG_ERROR("KFEStagingTexture::Impl::Initialize: Destroy failed; cannot reinitialize.");
            return false;
        }
    }

    m_pDevice = desc.Device;
    m_width = desc.Width;
    m_height = desc.Height;
    m_format = desc.Format;
    m_mipLevels = (desc.MipLevels == 0u ? 1u : desc.MipLevels);
    m_arraySize = (desc.ArraySize == 0u ? 1u : desc.ArraySize);
    m_mappedUpload = nullptr;
    m_numRows = 0u;
    m_rowSizeInBytes = 0u;
    m_totalBytes = 0u;

    ID3D12Device* nativeDevice = m_pDevice->GetNative();

    // Prepare a 2D texture description for footprint calculation
    D3D12_RESOURCE_DESC texDesc{};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = m_width;
    texDesc.Height = m_height;
    texDesc.DepthOrArraySize = static_cast<UINT16>(m_arraySize);
    texDesc.MipLevels = static_cast<UINT16>(m_mipLevels);
    texDesc.Format = m_format;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = static_cast<D3D12_RESOURCE_FLAGS>(0);

    UINT   numSubresources = 1u;
    UINT64 totalBytes = 0u;
    UINT   numRows = 0u;
    UINT64 rowSizeInBytes = 0u;

    nativeDevice->GetCopyableFootprints(
        &texDesc,
        0,               // FirstSubresource
        numSubresources, // NumSubresources
        0,               // BaseOffset
        &m_footprint,
        &numRows,
        &rowSizeInBytes,
        &totalBytes
    );

    m_numRows = numRows;
    m_rowSizeInBytes = rowSizeInBytes;
    m_totalBytes = totalBytes;

    // Create upload buffer
    KFE_CREATE_BUFFER_DESC uploadDesc{};
    uploadDesc.Device = m_pDevice;
    uploadDesc.SizeInBytes = m_totalBytes;
    uploadDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    uploadDesc.InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    uploadDesc.ResourceFlags = static_cast<D3D12_RESOURCE_FLAGS>(0);
    uploadDesc.DebugName = "KFEStagingTexture_Upload";

    if (!m_uploadBuffer.Initialize(uploadDesc))
    {
        LOG_ERROR("KFEStagingTexture::Impl::Initialize: Failed to create upload buffer.");
        m_pDevice = nullptr;
        return false;
    }

    m_mappedUpload = m_uploadBuffer.GetMappedData();
    if (!m_mappedUpload)
    {
        LOG_ERROR("KFEStagingTexture::Impl::Initialize: Upload buffer mapping failed / returned null.");
        m_uploadBuffer.Destroy();
        m_pDevice = nullptr;
        return false;
    }

    // Create default texture using your KFETexture + KFE_TEXTURE_CREATE_DESC
    KFE_TEXTURE_CREATE_DESC texCreate{};
    texCreate.Device = m_pDevice;
    texCreate.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texCreate.Width = m_width;
    texCreate.Height = m_height;
    texCreate.DepthOrArraySize = static_cast<std::uint16_t>(m_arraySize);
    texCreate.MipLevels = static_cast<std::uint16_t>(m_mipLevels);
    texCreate.Format = m_format;
    texCreate.SampleDesc.Count = 1;
    texCreate.SampleDesc.Quality = 0;
    texCreate.ResourceFlags = static_cast<D3D12_RESOURCE_FLAGS>(0);
    texCreate.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    texCreate.InitialState = D3D12_RESOURCE_STATE_COPY_DEST;
    texCreate.ClearValue = nullptr;

    if (!m_texture.Initialize(texCreate))
    {
        LOG_ERROR("KFEStagingTexture::Impl::Initialize: Failed to create default texture.");
        m_uploadBuffer.Destroy();
        m_mappedUpload = nullptr;
        m_pDevice = nullptr;
        return false;
    }

    LOG_SUCCESS("KFEStagingTexture::Impl::Initialize: Created staging texture {}x{}, format={}.",
        m_width, m_height, static_cast<int>(m_format));

    m_bInitialized = true;
    return true;
}

_Use_decl_annotations_
bool kfe::KFEStagingTexture::Impl::Destroy() noexcept
{
    if (m_uploadBuffer.IsInitialized())
    {
        if (!m_uploadBuffer.Destroy())
        {
            LOG_ERROR("KFEStagingTexture::Impl::Destroy: Failed to destroy upload buffer.");
        }
    }

    if (m_texture.IsInitialized())
    {
        if (!m_texture.Destroy())
        {
            LOG_ERROR("KFEStagingTexture::Impl::Destroy: Failed to destroy default texture.");
        }
    }

    m_pDevice = nullptr;
    m_mappedUpload = nullptr;
    m_width = 0u;
    m_height = 0u;
    m_format = DXGI_FORMAT_UNKNOWN;
    m_mipLevels = 1u;
    m_arraySize = 1u;
    m_numRows = 0u;
    m_rowSizeInBytes = 0u;
    m_totalBytes = 0u;
    m_bInitialized = false;

    return true;
}

_Use_decl_annotations_
bool kfe::KFEStagingTexture::Impl::IsInitialized() const noexcept
{
    return m_bInitialized;
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEStagingTexture::Impl::GetUploadBuffer() noexcept
{
    if (!m_bInitialized)
        return nullptr;
    return &m_uploadBuffer;
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEStagingTexture::Impl::GetUploadBuffer() const noexcept
{
    if (!m_bInitialized)
        return nullptr;
    return &m_uploadBuffer;
}

_Use_decl_annotations_
kfe::KFETexture* kfe::KFEStagingTexture::Impl::GetTexture() noexcept
{
    if (!m_bInitialized)
        return nullptr;
    return &m_texture;
}

_Use_decl_annotations_
const kfe::KFETexture* kfe::KFEStagingTexture::Impl::GetTexture() const noexcept
{
    if (!m_bInitialized)
        return nullptr;
    return &m_texture;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStagingTexture::Impl::GetWidth() const noexcept
{
    return m_width;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEStagingTexture::Impl::GetHeight() const noexcept
{
    return m_height;
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFEStagingTexture::Impl::GetFormat() const noexcept
{
    return m_format;
}

_Use_decl_annotations_
bool kfe::KFEStagingTexture::Impl::WritePixels(
    _In_ const void* data,
    std::uint32_t    srcRowPitchBytes) noexcept
{
    if (!m_bInitialized)
    {
        LOG_ERROR("KFEStagingTexture::Impl::WritePixels: Staging texture not initialized.");
        return false;
    }

    if (!data)
    {
        LOG_ERROR("KFEStagingTexture::Impl::WritePixels: Source data pointer is null.");
        return false;
    }

    if (!m_mappedUpload)
    {
        LOG_ERROR("KFEStagingTexture::Impl::WritePixels: Upload buffer mapped pointer is null.");
        return false;
    }

    if (m_numRows == 0u || m_totalBytes == 0u)
    {
        LOG_ERROR("KFEStagingTexture::Impl::WritePixels: Invalid footprint info (NumRows/TotalBytes == 0).");
        return false;
    }

    auto* dstBase = static_cast<std::uint8_t*>(m_mappedUpload);
    const auto* srcBase = static_cast<const std::uint8_t*>(data);

    for (UINT row = 0; row < m_numRows; ++row)
    {
        std::uint8_t* dstRow = dstBase + m_footprint.Offset + row * m_footprint.Footprint.RowPitch;
        const std::uint8_t* srcRow = srcBase + static_cast<std::size_t>(row) * srcRowPitchBytes;

        const UINT copySize = static_cast<UINT>(
            srcRowPitchBytes < m_footprint.Footprint.RowPitch
            ? srcRowPitchBytes
            : m_footprint.Footprint.RowPitch);

        std::memcpy(dstRow, srcRow, copySize);
    }

    return true;
}

_Use_decl_annotations_
bool kfe::KFEStagingTexture::Impl::RecordUploadToTexture(
    _In_ ID3D12GraphicsCommandList* cmdList) const noexcept
{
    if (!m_bInitialized)
    {
        LOG_ERROR("KFEStagingTexture::Impl::RecordUploadToTexture: Staging texture not initialized.");
        return false;
    }

    if (!cmdList)
    {
        LOG_ERROR("KFEStagingTexture::Impl::RecordUploadToTexture: cmdList is null.");
        return false;
    }

    ID3D12Resource* uploadRes = m_uploadBuffer.GetNative();
    ID3D12Resource* defaultRes = m_texture.GetNative();

    if (!uploadRes || !defaultRes)
    {
        LOG_ERROR("KFEStagingTexture::Impl::RecordUploadToTexture: Underlying resources are null.");
        return false;
    }

    D3D12_TEXTURE_COPY_LOCATION srcLocation{};
    srcLocation.pResource = uploadRes;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint = m_footprint;

    D3D12_TEXTURE_COPY_LOCATION dstLocation{};
    dstLocation.pResource = defaultRes;
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0u;

    cmdList->CopyTextureRegion(
        &dstLocation,
        0, 0, 0,
        &srcLocation,
        nullptr
    );

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = defaultRes;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    cmdList->ResourceBarrier(1, &barrier);

    return true;
}

#pragma endregion
