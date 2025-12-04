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
