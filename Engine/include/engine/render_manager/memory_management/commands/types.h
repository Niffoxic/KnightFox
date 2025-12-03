#pragma once

#include <cstdint>

struct ID3D12PipelineState;
struct ID3D12Fence;
namespace kfe
{
    typedef struct _KFE_RESET_COMMAND_LIST
    {
        ID3D12PipelineState* PSO;
        ID3D12Fence*         Fence;
        std::uint64_t        FenceValue;
    } KFE_RESET_COMMAND_LIST;
}
