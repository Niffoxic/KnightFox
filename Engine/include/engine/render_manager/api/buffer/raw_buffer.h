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
#include <vector>

namespace kfe
{
    class KFEDevice;
    class KFEBuffer;
    class KFEResourceHeap;

    typedef struct _KFE_RAW_BUFFER_CREATE_DESC
    {
        KFEDevice*       Device         = nullptr;
        KFEBuffer*       ResourceBuffer = nullptr;
        KFEResourceHeap* ResourceHeap   = nullptr;

        std::uint64_t    OffsetInBytes = 0u;
        std::uint64_t    SizeInBytes   = 0u;
    } KFE_RAW_BUFFER_CREATE_DESC;

    typedef struct _KFE_RAW_SRV_DESC
    {
        std::uint64_t FirstByteOffset = 0u;
        std::uint64_t NumBytes        = 0u;
    } KFE_RAW_SRV_DESC;

    typedef struct _KFE_RAW_UAV_DESC
    {
        std::uint64_t FirstByteOffset       = 0u;
        std::uint64_t NumBytes              = 0u;
        bool          HasCounter            = false;
        std::uint64_t CounterOffsetInBytes  = 0u;
    } KFE_RAW_UAV_DESC;

    class KFE_API KFERawBuffer final : public IKFEObject
    {
    public:
         KFERawBuffer() noexcept;
        ~KFERawBuffer() noexcept;

        KFERawBuffer            (const KFERawBuffer&) = delete;
        KFERawBuffer& operator= (const KFERawBuffer&) = delete;

        KFERawBuffer            (KFERawBuffer&& other) noexcept;
        KFERawBuffer& operator= (KFERawBuffer&& other) noexcept;

        NODISCARD std::string GetName       () const noexcept override;
        NODISCARD std::string GetDescription() const noexcept override;

        NODISCARD bool Initialize(_In_ const KFE_RAW_BUFFER_CREATE_DESC& desc);
        
        NODISCARD bool Destroy      ()       noexcept;
        NODISCARD bool IsInitialized() const noexcept;

        NODISCARD       KFEBuffer* GetBuffer()       noexcept;
        NODISCARD const KFEBuffer* GetBuffer() const noexcept;

        NODISCARD       KFEResourceHeap* GetResourceHeap()       noexcept;
        NODISCARD const KFEResourceHeap* GetResourceHeap() const noexcept;

        NODISCARD std::uint64_t GetOffsetInBytes() const noexcept;
        NODISCARD std::uint64_t GetSizeInBytes  () const noexcept;

        NODISCARD std::uint32_t CreateSRV(_In_ const KFE_RAW_SRV_DESC& desc);
        NODISCARD std::uint32_t CreateUAV(_In_ const KFE_RAW_UAV_DESC& desc);

        NODISCARD std::uint32_t GetNumSRVs() const noexcept;
        NODISCARD std::uint32_t GetNumUAVs() const noexcept;

        NODISCARD std::uint32_t GetSRVDescriptorIndex(std::uint32_t viewIndex) const noexcept;
        NODISCARD std::uint32_t GetUAVDescriptorIndex(std::uint32_t viewIndex) const noexcept;
    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace kfe
