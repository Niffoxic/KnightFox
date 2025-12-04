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

#include <memory>

#include "types.h"
#include "engine/core.h"

enum   D3D12_COMMAND_LIST_TYPE;
struct ID3D12GraphicsCommandList;

namespace kfe
{
    class KFEDevice;

    // Compute command list creation descriptor
    typedef struct _KFE_COMPUTE_COMMAND_LIST_CREATE_DESC
    {
        KFEDevice*   Device;
        uint64_t     BlockMaxTime{ 5u };
        uint64_t     InitialCounts{ 3u };
        uint64_t     MaxCounts{ 10u };
    } KFE_COMPUTE_COMMAND_LIST_CREATE_DESC;

    /// <summary>
    /// DirectX 12 Compute Command List
    /// </summary>
    class KFE_API KFEComputeCommandList final : public IKFEObject
    {
    public:
         KFEComputeCommandList() noexcept;
        ~KFEComputeCommandList() noexcept override;

        KFEComputeCommandList(const KFEComputeCommandList&) = delete;
        KFEComputeCommandList& operator=(const KFEComputeCommandList&) = delete;

        KFEComputeCommandList(KFEComputeCommandList&&)            noexcept;
        KFEComputeCommandList& operator=(KFEComputeCommandList&&) noexcept;

        // Inherited via IKFEObject
        std::string GetName       () const noexcept override;
        std::string GetDescription() const noexcept override;

        // Initialize the graphics command list
        NODISCARD bool Initialize(_In_ const KFE_COMPUTE_COMMAND_LIST_CREATE_DESC& desc);

        // Reset the command list with a fresh allocator from the pool.
        NODISCARD bool Reset(_In_ const KFE_RESET_COMMAND_LIST& reset);

        // Close the command list for execution.
        NODISCARD bool Close  () noexcept;
        NODISCARD bool Destroy() noexcept;

        NODISCARD ID3D12GraphicsCommandList* GetNative    () const noexcept;
        NODISCARD bool                       IsInitialized() const noexcept;

        // Updates States: Must be called very frame
        void Update() noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace kfe
