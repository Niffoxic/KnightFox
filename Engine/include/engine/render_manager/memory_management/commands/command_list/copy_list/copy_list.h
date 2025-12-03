#pragma once
#include "EngineAPI.h"

#include <memory>

#include "engine/render_manager/memory_management/commands/types.h"
#include "engine/core/core.h"

enum   D3D12_COMMAND_LIST_TYPE;
struct ID3D12GraphicsCommandList;

namespace kfe
{
    class KFEDevice;

    // Copy command list creation descriptor
    typedef struct _KFE_COPY_COMMAND_LIST_CREATE_DESC
    {
        KFEDevice* Device;
        uint64_t   BlockMaxTime{ 5u };
        uint64_t   InitialCounts{ 3u };
        uint64_t   MaxCounts{ 10u };
    } KFE_COPY_COMMAND_LIST_CREATE_DESC;

    /// <summary>
    /// DirectX 12 Copy Command List
    /// </summary>
    class KFE_API KFECopyCommandList
    {
    public:
        KFECopyCommandList() noexcept;
        ~KFECopyCommandList() noexcept;

        KFECopyCommandList(const KFECopyCommandList&) = delete;
        KFECopyCommandList& operator=(const KFECopyCommandList&) = delete;

        KFECopyCommandList(KFECopyCommandList&&)            noexcept;
        KFECopyCommandList& operator=(KFECopyCommandList&&) noexcept;

        // Initialize the copy command list
        NODISCARD bool Initialize(_In_ const KFE_COPY_COMMAND_LIST_CREATE_DESC& desc);

        // Reset the command list with a fresh allocator from the pool.
        // PSO not needed.
        NODISCARD bool Reset(_In_ const KFE_RESET_COMMAND_LIST& reset);

        // Close the command list for execution.
        NODISCARD bool Close  () noexcept;
        NODISCARD bool Destroy() noexcept;

        NODISCARD ID3D12GraphicsCommandList* GetNative    () const noexcept;
        NODISCARD bool                       IsInitialized() const noexcept;

        // Updates allocator states; should be called every frame.
        void Update() noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace kfe
