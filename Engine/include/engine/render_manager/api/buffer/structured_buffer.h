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

    typedef struct _KFE_STRUCTURED_BUFFER_CREATE_DESC
    {
        KFEDevice* Device             = nullptr;
        KFEBuffer* ResourceBuffer     = nullptr;
        KFEResourceHeap* ResourceHeap = nullptr;

        std::uint32_t    ElementStride = 0u; 
        std::uint32_t    ElementCount = 0u; 
        std::uint64_t    OffsetInBytes = 0u;
    } KFE_STRUCTURED_BUFFER_CREATE_DESC;

    typedef struct _KFE_STRUCTURED_SRV_DESC
    {
        std::uint32_t FirstElement = 0u;
        std::uint32_t NumElements = 0u;  
    } KFE_STRUCTURED_SRV_DESC;

    typedef struct _KFE_STRUCTURED_UAV_DESC
    {
        std::uint32_t FirstElement         = 0u;
        std::uint32_t NumElements          = 0u;
        bool          HasCounter           = false;
        std::uint32_t CounterOffsetInBytes = 0u;
    } KFE_STRUCTURED_UAV_DESC;

    class KFE_API KFEStructuredBuffer final : public IKFEObject
    {
    public:
         KFEStructuredBuffer() noexcept;
        ~KFEStructuredBuffer() noexcept;

        KFEStructuredBuffer           (const KFEStructuredBuffer&) = delete;
        KFEStructuredBuffer& operator=(const KFEStructuredBuffer&) = delete;

        KFEStructuredBuffer           (KFEStructuredBuffer&& other) noexcept;
        KFEStructuredBuffer& operator=(KFEStructuredBuffer&& other) noexcept;

        NODISCARD std::string GetName       () const noexcept override;
        NODISCARD std::string GetDescription() const noexcept override;

        NODISCARD bool Initialize(_In_ const KFE_STRUCTURED_BUFFER_CREATE_DESC& desc);
        
        NODISCARD bool Destroy      () noexcept;
        NODISCARD bool IsInitialized() const noexcept;

        NODISCARD       KFEBuffer* GetBuffer()       noexcept;
        NODISCARD const KFEBuffer* GetBuffer() const noexcept;

        NODISCARD       KFEResourceHeap* GetResourceHeap()       noexcept;
        NODISCARD const KFEResourceHeap* GetResourceHeap() const noexcept;

        NODISCARD std::uint32_t GetElementStride() const noexcept;
        NODISCARD std::uint32_t GetElementCount () const noexcept;
        NODISCARD std::uint64_t GetOffsetInBytes() const noexcept;

        // Creates an SRV in the associated CBV_SRV_UAV heap and returns its descriptor index
        NODISCARD std::uint32_t CreateSRV(_In_ const KFE_STRUCTURED_SRV_DESC& desc);

        // Creates a UAV in the associated CBV_SRV_UAV heap and returns its descriptor index
        NODISCARD std::uint32_t CreateUAV(_In_ const KFE_STRUCTURED_UAV_DESC& desc);

        NODISCARD std::uint32_t GetNumSRVs() const noexcept;
        NODISCARD std::uint32_t GetNumUAVs() const noexcept;

        NODISCARD std::uint32_t GetSRVDescriptorIndex(std::uint32_t viewIndex) const noexcept;
        NODISCARD std::uint32_t GetUAVDescriptorIndex(std::uint32_t viewIndex) const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace kfe
