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

namespace kfe
{
    class KFEDevice;
    class KFEBuffer;

    typedef struct _KFE_UPLOAD_BUFFER_CREATE_DESC
    {
        KFEDevice*    Device = nullptr;
        std::uint64_t SizeInBytes = 0u;
    } KFE_UPLOAD_BUFFER_CREATE_DESC;

    class KFE_API KFEUploadBuffer final : public IKFEObject
    {
    public:
        KFEUploadBuffer () noexcept;
        ~KFEUploadBuffer() noexcept;

        KFEUploadBuffer           (const KFEUploadBuffer&) = delete;
        KFEUploadBuffer& operator=(const KFEUploadBuffer&) = delete;

        KFEUploadBuffer           (KFEUploadBuffer&& other) noexcept;
        KFEUploadBuffer& operator=(KFEUploadBuffer&& other) noexcept;

        NODISCARD std::string GetName       () const noexcept override;
        NODISCARD std::string GetDescription() const noexcept override;

        /// Creates an upload heap KFEBuffer and maps it.
        NODISCARD bool Initialize(_In_ const KFE_UPLOAD_BUFFER_CREATE_DESC& desc);
        
        NODISCARD bool Destroy      () noexcept;
        NODISCARD bool IsInitialized() const noexcept;

        /// Returns underlying buffer (UPLOAD heap).
        NODISCARD       KFEBuffer* GetBuffer()       noexcept;
        NODISCARD const KFEBuffer* GetBuffer() const noexcept;

        /// Returns persistent mapped pointer.
        NODISCARD       void* GetMappedData()       noexcept;
        NODISCARD const void* GetMappedData() const noexcept;

        /// Raw byte write.
        NODISCARD bool WriteBytes(
            _In_reads_bytes_(numBytes) const void* data,
            std::uint64_t numBytes,
            std::uint64_t offsetBytes = 0u) noexcept;

        /// Typed write (e.g., Write<Matrix>(myMatrix)).
        template<typename T>
        NODISCARD bool Write(const T& value, std::uint64_t offsetBytes = 0u) noexcept
        {
            return WriteBytes(&value, sizeof(T), offsetBytes);
        }

        NODISCARD std::uint64_t GetSizeInBytes() const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace kfe
