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

struct ID3D12Resource;
enum   D3D12_RESOURCE_FLAGS;
enum   D3D12_HEAP_TYPE;
enum   D3D12_RESOURCE_STATES;

namespace kfe
{
    class KFEDevice;

    // Creation descriptor for a generic GPU buffer
    typedef struct _KFE_CREATE_BUFFER_DESC
    {
        KFEDevice*    Device      = nullptr;
        std::uint64_t SizeInBytes = 0u;

        D3D12_RESOURCE_FLAGS  ResourceFlags = static_cast<D3D12_RESOURCE_FLAGS> (0);
        D3D12_HEAP_TYPE       HeapType      = static_cast<D3D12_HEAP_TYPE>      (0);
        D3D12_RESOURCE_STATES InitialState  = static_cast<D3D12_RESOURCE_STATES>(0);

        std::string           DebugName{};
    } KFE_CREATE_BUFFER_DESC;

    /// <summary>
    /// DirectX 12 GPU buffer wrapper
    /// </summary>
    class KFE_API KFEBuffer
    {
    public:
         KFEBuffer();
        ~KFEBuffer();

        KFEBuffer(const KFEBuffer&)            = delete;
        KFEBuffer& operator=(const KFEBuffer&) = delete;

        KFEBuffer(KFEBuffer&&) noexcept;
        KFEBuffer& operator=(KFEBuffer&&) noexcept;

        /// Initialize the GPU buffer with the given description
        NODISCARD bool Initialize(_In_ const KFE_CREATE_BUFFER_DESC& desc);

        /// Destroy the resource and reset all state
        NODISCARD bool Destroy() noexcept;

        //~ queries
        NODISCARD bool                  IsInitialized   () const noexcept;
        NODISCARD std::uint64_t         GetSizeInBytes  () const noexcept;
        NODISCARD D3D12_HEAP_TYPE       GetHeapType     () const noexcept;
        NODISCARD D3D12_RESOURCE_STATES GetInitialState () const noexcept;
        NODISCARD ID3D12Resource*       GetNative       () const noexcept;

        NODISCARD bool IsUploadHeap  () const noexcept;
        NODISCARD bool IsReadbackHeap() const noexcept;

        /// Persistent mapping pointer for upload if mapped
        _Ret_maybenull_ NODISCARD void*       GetMappedData()       noexcept;
        
        // Persistent mapping pointer for readback heaps if mapped
        _Ret_maybenull_ NODISCARD const void* GetMappedData() const noexcept;

        /// Copy CPU data into the buffer
        NODISCARD bool CopyCPUToGPU(
            _In_reads_bytes_(sizeInBytes) const void* sourceData,
            std::uint64_t                             sizeInBytes,
            std::uint64_t                             dstOffsetBytes = 0u) noexcept;

        /// Copy data from the buffer into a CPU destination for READBACK heap
        /// Returns false if the heap type or mapping does not support readback
        NODISCARD bool CopyGPUToCPU(
            _Out_writes_bytes_(sizeInBytes) void* destination,
            std::uint64_t                         sizeInBytes,
            std::uint64_t                         srcOffsetBytes = 0u) const noexcept;

        /// Set a debug name on the resource for PIX and RenderDoc
        void SetDebugName(_In_ const std::string& name) noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace kfe
