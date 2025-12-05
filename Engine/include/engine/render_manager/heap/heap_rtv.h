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

#include "engine/system/interface/interface_descriptor_heap.h"

#include <memory>
#include <cstdint>
#include <string>

struct ID3D12DescriptorHeap;

namespace kfe
{
    class KFEDevice;

    /// <summary>
    /// Wrapper around a D3D12 RTV descriptor heap
    /// </summary>
    class KFE_API KFERTVHeap final : public IKFEDescriptorHeap
    {
    public:
         KFERTVHeap() noexcept;
        ~KFERTVHeap() noexcept override;

        KFERTVHeap(const KFERTVHeap&) = delete;
        KFERTVHeap(KFERTVHeap&&) noexcept;

        KFERTVHeap& operator=(const KFERTVHeap&) = delete;
        KFERTVHeap& operator=(KFERTVHeap&&) noexcept;

        // Inherited via IKFEObject
        std::string GetName       () const noexcept override;
        std::string GetDescription() const noexcept override;

        /// Initializes the RTV descriptor heap.
        /// If already initialized, implementation may destroy and recreate, or fail and return false.
        NODISCARD bool Initialize(const KFE_DESCRIPTOR_HEAP_CREATE_DESC& desc) override;

        /// Destroys the heap and resets all state.
        NODISCARD bool Destroy() noexcept override;

        NODISCARD bool          IsInitialized    () const noexcept override;
        NODISCARD std::uint32_t GetNumDescriptors() const noexcept override;

        /// Number of descriptors currently allocated via Allocate().
        NODISCARD std::uint32_t GetAllocatedCount() const noexcept override;

        /// Number of remaining descriptors that can still be allocated.
        NODISCARD std::uint32_t GetRemaining     () const noexcept override;

        /// Size, in bytes, between adjacent RTV descriptors in this heap.
        NODISCARD std::uint32_t GetHandleSize    () const noexcept override;

        /// Returns the CPU handle to the first descriptor in the heap.
        NODISCARD KFE_CPU_DESCRIPTOR_HANDLE GetStartHandle() const noexcept override;

        /// Returns the CPU handle at the specified index.
        NODISCARD KFE_CPU_DESCRIPTOR_HANDLE GetHandle(_In_ std::uint32_t index) const noexcept override;

        /// Allocates a single descriptor slot and returns its index.
        /// Returns InvalidIndex if no more descriptors are available.
        NODISCARD std::uint32_t Allocate() noexcept override;

        /// Frees an allocated descriptor index.
        NODISCARD bool Free(_In_ std::uint32_t index) noexcept override;

        /// Resets the internal allocation state without destroying the heap.
        NODISCARD bool Reset() noexcept override;

        NODISCARD bool IsValidIndex(std::uint32_t idx) const noexcept override;

        _Maybenull_ NODISCARD
        ID3D12DescriptorHeap* GetNative() const noexcept override;

        /// Sets a debug name for the heap (for PIX or RenderDoc).
        void SetDebugName(_In_ const std::string& name) noexcept override;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace kfe
