#pragma once
#include "EngineAPI.h"

#include <memory>

#include "engine/core/core.h"

enum   D3D12_COMMAND_LIST_TYPE;
struct ID3D12GraphicsCommandList;
struct ID3D12PipelineState;
struct ID3D12Fence;

namespace kfe
{
    class KFEDevice;

    typedef struct _KFE_RESET_COMMAND_LIST
    {
        ID3D12PipelineState* PSO;
        ID3D12Fence*         Fence;
        UINT64               FenceValue;
    } KFE_RESET_COMMAND_LIST;

    // Graphics command list creation descriptor
    typedef struct _KFE_GFX_COMMAND_LIST_CREATE_DESC
    {
        KFEDevice*   Device;
        uint64_t     BlockMaxTime { 5u };
        uint64_t     InitialCounts{ 3u };
        uint64_t     MaxCounts    { 10u };
    } KFE_GFX_COMMAND_LIST_CREATE_DESC;

    /// <summary>
    /// DirectX 12 Graphics Command List
    /// </summary>
    class KFE_API KFEGraphicsCommandList
    {
    public:
         KFEGraphicsCommandList() noexcept;
        ~KFEGraphicsCommandList() noexcept;

        KFEGraphicsCommandList(const KFEGraphicsCommandList&)            = delete;
        KFEGraphicsCommandList& operator=(const KFEGraphicsCommandList&) = delete;

        KFEGraphicsCommandList(KFEGraphicsCommandList&&)            noexcept;
        KFEGraphicsCommandList& operator=(KFEGraphicsCommandList&&) noexcept;

        // Initialize the graphics command list
        NODISCARD bool Initialize(_In_ const KFE_GFX_COMMAND_LIST_CREATE_DESC& desc);

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
