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

    typedef struct _KFE_READBACK_BUFFER_CREATE_DESC
    {
        KFEDevice*    Device      = nullptr;
        std::uint64_t SizeInBytes = 0u;
    } KFE_READBACK_BUFFER_CREATE_DESC;

    class KFE_API KFEReadbackBuffer final : public IKFEObject
    {
    public:
         KFEReadbackBuffer() noexcept;
        ~KFEReadbackBuffer() noexcept;

        KFEReadbackBuffer           (const KFEReadbackBuffer&) = delete;
        KFEReadbackBuffer& operator=(const KFEReadbackBuffer&) = delete;

        KFEReadbackBuffer           (KFEReadbackBuffer&& other) noexcept;
        KFEReadbackBuffer& operator=(KFEReadbackBuffer&& other) noexcept;

        NODISCARD std::string GetName       () const noexcept override;
        NODISCARD std::string GetDescription() const noexcept override;

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
            std::uint64_t                     srcOffsetBytes = 0u) const noexcept;

        template<typename T>
        NODISCARD bool Read(_Out_ T& value, std::uint64_t srcOffsetBytes = 0u) const noexcept
        {
            return ReadBytes(&value, sizeof(T), srcOffsetBytes);
        }

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace kfe
