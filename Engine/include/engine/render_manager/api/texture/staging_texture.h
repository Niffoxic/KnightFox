// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */

#pragma once
#include "EngineAPI.h"
#include "engine/core.h"

#include <cstdint>
#include <memory>
#include <string>
#include <dxgiformat.h>

struct ID3D12GraphicsCommandList;

namespace kfe
{
    class KFEDevice;
    class KFEBuffer;
    class KFETexture;

    typedef struct _KFE_STAGING_TEXTURE_CREATE_DESC
    {
        KFEDevice* Device = nullptr;

        std::uint32_t Width = 0u;
        std::uint32_t Height = 0u;
        DXGI_FORMAT   Format = static_cast<DXGI_FORMAT>(0);

        std::uint32_t MipLevels = 1u;
        std::uint32_t ArraySize = 1u;

    } KFE_STAGING_TEXTURE_CREATE_DESC;

    /// <summary>
    /// Staging texture: owns an UPLOAD buffer
    /// DEFAULT KFETexture and records CopyTextureRegion.
    /// </summary>
    class KFE_API KFEStagingTexture final : public IKFEObject
    {
    public:
        KFEStagingTexture() noexcept;
        ~KFEStagingTexture() noexcept;

        KFEStagingTexture(const KFEStagingTexture&) = delete;
        KFEStagingTexture& operator=(const KFEStagingTexture&) = delete;

        KFEStagingTexture(KFEStagingTexture&&) noexcept;
        KFEStagingTexture& operator=(KFEStagingTexture&&) noexcept;

        // IKFEObject
        NODISCARD std::string GetName()        const noexcept override;
        NODISCARD std::string GetDescription() const noexcept override;

        NODISCARD bool Initialize(_In_ const KFE_STAGING_TEXTURE_CREATE_DESC& desc);
        NODISCARD bool Destroy() noexcept;
        NODISCARD bool IsInitialized() const noexcept;

        // Accessors
        NODISCARD KFEBuffer* GetUploadBuffer()  noexcept;
        NODISCARD const KFEBuffer* GetUploadBuffer()  const noexcept;
        NODISCARD KFETexture* GetTexture()       noexcept;
        NODISCARD const KFETexture* GetTexture()       const noexcept;

        NODISCARD std::uint32_t GetWidth() const noexcept;
        NODISCARD std::uint32_t GetHeight() const noexcept;
        NODISCARD DXGI_FORMAT   GetFormat() const noexcept;

        NODISCARD bool WritePixels(
            _In_ const void* data,
            std::uint32_t    srcRowPitchBytes) noexcept;

        NODISCARD bool RecordUploadToTexture(
            _In_ ID3D12GraphicsCommandList* cmdList) const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace kfe
