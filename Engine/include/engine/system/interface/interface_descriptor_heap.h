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
#include <string>
//~ clean later
#include <d3d12.h>

//struct ID3D12DescriptorHeap;
//struct D3D12_CPU_DESCRIPTOR_HANDLE;
//struct D3D12_GPU_DESCRIPTOR_HANDLE;

namespace kfe
{
    class KFEDevice;

    typedef struct _KFE_DESCRIPTOR_HEAP_CREATE_DESC
    {
        KFEDevice*    Device;
        std::uint32_t DescriptorCounts;
        const char*   DebugName;
    } KFE_DESCRIPTOR_HEAP_CREATE_DESC;

    /// <summary>
    /// Interface Wrapper around D3D12 descriptor heap.
    /// </summary>
    class KFE_API IKFEDescriptorHeap : public IKFEObject
    {
    public:
        /// Initializes the Sampler descriptor heap
        /// If already initialized, implementation may destroy and recreate, or fail and return false
        NODISCARD virtual bool Initialize(const KFE_DESCRIPTOR_HEAP_CREATE_DESC& desc) = 0;

        /// Destroys the heap and resets all state.
        NODISCARD virtual bool          Destroy          ()       noexcept = 0;
        NODISCARD virtual bool          IsInitialized    () const noexcept = 0;
        NODISCARD virtual std::uint32_t GetNumDescriptors() const noexcept = 0;
        NODISCARD virtual std::uint32_t GetAllocatedCount() const noexcept = 0;
        NODISCARD virtual std::uint32_t GetRemaining     () const noexcept = 0;

        /// Size, in bytes, between adjacent descriptors in this heap
        NODISCARD virtual std::uint32_t GetHandleSize    () const noexcept = 0;

        NODISCARD virtual D3D12_CPU_DESCRIPTOR_HANDLE GetStartHandle   () const noexcept = 0;
        NODISCARD virtual D3D12_GPU_DESCRIPTOR_HANDLE GetGPUStartHandle() const noexcept { return {}; }

        NODISCARD virtual D3D12_CPU_DESCRIPTOR_HANDLE GetHandle   (_In_ std::uint32_t index) const noexcept = 0;
        NODISCARD virtual D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(_In_ std::uint32_t index) const noexcept { return {}; }

        /// Allocates a single descriptor slot and returns its index
        /// Returns InvalidIndex if no more descriptors are available
        NODISCARD virtual std::uint32_t Allocate() noexcept = 0;

        /// Frees an allocated descriptor index.
        NODISCARD virtual bool Free(_In_ std::uint32_t index) noexcept = 0;

        /// Resets the internal allocation state without destroying the heap
        NODISCARD virtual bool Reset() noexcept = 0;

        NODISCARD virtual bool IsValidIndex(std::uint32_t idx) const noexcept = 0;

        _Maybenull_ NODISCARD
        virtual ID3D12DescriptorHeap* GetNative() const noexcept = 0;

        /// Sets a debug name for the heap (for PIX or RenderDoc).
        virtual void SetDebugName(_In_ const std::string& name) noexcept  = 0;
    };
} // namespace kfe
