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
#include "engine/system/common_types.h"

#include <memory>
#include <cstdint>
#include <string>

struct ID3D12DescriptorHeap;

namespace kfe
{
    class KFEDevice;

    typedef struct _KFE_DSV_HEAP_CREATE_DESC
    {
        KFEDevice*    Device;
        std::uint32_t DescriptorCounts;
        const char*   DebugName;
    } KFE_DSV_HEAP_CREATE_DESC;

    /// <summary>
    /// Wrapper around a D3D12 DSV descriptor heap
    /// </summary>
    class KFE_API KFEDSVHeap final : public IKFEObject
    {
    public:
         KFEDSVHeap() noexcept;
        ~KFEDSVHeap() noexcept override;

        KFEDSVHeap(const KFEDSVHeap&) = delete;
        KFEDSVHeap(KFEDSVHeap&&) noexcept;

        KFEDSVHeap& operator=(const KFEDSVHeap&) = delete;
        KFEDSVHeap& operator=(KFEDSVHeap&&) noexcept;

        // Inherited via IKFEObject
        std::string GetName() const noexcept override;
        std::string GetDescription() const noexcept override;

        /// Initializes the DSV descriptor heap.
        /// If already initialized, implementation may destroy and recreate, or fail and return false.
        NODISCARD bool Initialize(const KFE_DSV_HEAP_CREATE_DESC& desc);

        /// Destroys the heap and resets all state.
        NODISCARD bool Destroy() noexcept;

        NODISCARD bool          IsInitialized() const noexcept;
        NODISCARD std::uint32_t GetNumDescriptors() const noexcept;

        /// Number of descriptors currently allocated via Allocate().
        NODISCARD std::uint32_t GetAllocatedCount() const noexcept;

        /// Number of remaining descriptors that can still be allocated.
        NODISCARD std::uint32_t GetRemaining() const noexcept;

        /// Size, in bytes, between adjacent DSV descriptors in this heap.
        NODISCARD std::uint32_t GetHandleSize() const noexcept;

        /// Returns the CPU handle to the first descriptor in the heap.
        NODISCARD KFE_CPU_DESCRIPTOR_HANDLE GetStartHandle() const noexcept;

        /// Returns the CPU handle at the specified index.
        NODISCARD KFE_CPU_DESCRIPTOR_HANDLE GetHandle(_In_ std::uint32_t index) const noexcept;

        /// Allocates a single descriptor slot and returns its index.
        /// Returns InvalidIndex if no more descriptors are available.
        NODISCARD std::uint32_t Allocate() noexcept;

        /// Frees an allocated descriptor index.
        NODISCARD bool Free(_In_ std::uint32_t index) noexcept;

        /// Resets the internal allocation state without destroying the heap.
        NODISCARD bool Reset() noexcept;

        NODISCARD bool IsValidIndex(std::uint32_t idx) const noexcept;

        _Maybenull_ NODISCARD
            ID3D12DescriptorHeap* GetNative() const noexcept;

        /// Sets a debug name for the heap (for PIX or RenderDoc).
        void SetDebugName(_In_ const std::string& name) noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace kfe
