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
    class KFEResourceHeap;

    typedef struct _KFE_CONSTANT_BUFFER_CREATE_DESC
    {
        KFEDevice*       Device         = nullptr;
        KFEBuffer*       ResourceBuffer = nullptr;
        KFEResourceHeap* ResourceHeap   = nullptr;
        std::uint32_t    SizeInBytes    = 0u;
        std::uint64_t    OffsetInBytes  = 0u;
    } KFE_CONSTANT_BUFFER_CREATE_DESC;

    class KFE_API KFEConstantBuffer final : public IKFEObject
    {
    public:
         KFEConstantBuffer() noexcept;
        ~KFEConstantBuffer() noexcept;

        KFEConstantBuffer           (const KFEConstantBuffer&) = delete;
        KFEConstantBuffer& operator=(const KFEConstantBuffer&) = delete;

        KFEConstantBuffer           (KFEConstantBuffer&& other) noexcept;
        KFEConstantBuffer& operator=(KFEConstantBuffer&& other) noexcept;

        NODISCARD std::string GetName       () const noexcept override;
        NODISCARD std::string GetDescription() const noexcept override;

        NODISCARD bool Initialize(_In_ const KFE_CONSTANT_BUFFER_CREATE_DESC& desc);
        
        NODISCARD bool Destroy      () noexcept;
        NODISCARD bool IsInitialized() const noexcept;

        NODISCARD       KFEBuffer* GetBuffer()       noexcept;
        NODISCARD const KFEBuffer* GetBuffer() const noexcept;

        NODISCARD       KFEResourceHeap* GetResourceHeap()       noexcept;
        NODISCARD const KFEResourceHeap* GetResourceHeap() const noexcept;

        NODISCARD std::uint32_t GetSizeInBytes       () const noexcept;
        NODISCARD std::uint32_t GetAlignedSizeInBytes() const noexcept;
        NODISCARD std::uint64_t GetOffsetInBytes     () const noexcept;

        NODISCARD std::uint32_t GetCBVDescriptorIndex() const noexcept;
        NODISCARD std::uint64_t GetGPUVirtualAddress () const noexcept;

        NODISCARD       void* GetMappedData()       noexcept;
        NODISCARD const void* GetMappedData() const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace kfe
