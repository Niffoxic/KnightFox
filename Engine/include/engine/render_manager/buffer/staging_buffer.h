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

struct ID3D12GraphicsCommandList;

namespace kfe
{
    class KFEDevice;
    class KFEBuffer;

    typedef struct _KFE_STAGING_BUFFER_CREATE_DESC
    {
        KFEDevice*    Device      = nullptr;
        std::uint64_t SizeInBytes = 0u;
    } KFE_STAGING_BUFFER_CREATE_DESC;

    class KFE_API KFEStagingBuffer final : public IKFEObject
    {
    public:
         KFEStagingBuffer() noexcept;
        ~KFEStagingBuffer() noexcept;

        KFEStagingBuffer            (const KFEStagingBuffer&) = delete;
        KFEStagingBuffer& operator= (const KFEStagingBuffer&) = delete;

        KFEStagingBuffer            (KFEStagingBuffer&& other) noexcept;
        KFEStagingBuffer& operator= (KFEStagingBuffer&& other) noexcept;

        NODISCARD std::string GetName       () const noexcept override;
        NODISCARD std::string GetDescription() const noexcept override;

        NODISCARD bool Initialize(_In_ const KFE_STAGING_BUFFER_CREATE_DESC& desc);
        
        NODISCARD bool Destroy      ()       noexcept;
        NODISCARD bool IsInitialized() const noexcept;

        NODISCARD       KFEBuffer* GetUploadBuffer()       noexcept;
        NODISCARD const KFEBuffer* GetUploadBuffer() const noexcept;

        NODISCARD       KFEBuffer* GetDefaultBuffer()       noexcept;
        NODISCARD const KFEBuffer* GetDefaultBuffer() const noexcept;

        NODISCARD std::uint64_t GetSizeInBytes() const noexcept;
        NODISCARD void*         GetMappedData ()       noexcept;
        NODISCARD const void*   GetMappedData () const noexcept;

        NODISCARD bool WriteBytes(
            _In_reads_bytes_(numBytes) const void* data,
            std::uint64_t                          numBytes,
            std::uint64_t                          dstOffsetBytes = 0u) noexcept;

        template<typename T>
        NODISCARD bool Write(const T& value, std::uint64_t dstOffsetBytes = 0u) noexcept
        {
            return WriteBytes(&value, sizeof(T), dstOffsetBytes);
        }

        // Records a CopyBufferRegion from upload to default
        NODISCARD bool RecordUploadToDefault(
            _In_ ID3D12GraphicsCommandList* cmdList,
            std::uint64_t                   numBytes,
            std::uint64_t                   srcOffsetBytes = 0u,
            std::uint64_t                   dstOffsetBytes = 0u) const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace kfe
