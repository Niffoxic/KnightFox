// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */

#include "pch.h"
#include "engine/render_manager/graph/compiled_graph.h"
#include "engine/render_manager/graph/context.h"
#include "engine/render_manager/graph/frame_context.h"

#include <d3d12.h>

namespace kfe::rg
{
	RGCompiled::RGCompiled () = default;
	RGCompiled::~RGCompiled() = default;

	RGCompiled::RGCompiled(RGCompiled&& other) noexcept = default;
	RGCompiled& RGCompiled::operator=(RGCompiled&& other) noexcept = default;

    void RGCompiled::Execute(const FrameContext& frameCtx)
    {
        RGExecutionContext execCtx{};

        execCtx.CommandList = frameCtx.GetCommandList();

        execCtx.Textures = m_TextureResources;
        execCtx.Buffers  = m_BufferResources;

        execCtx.RTVs = m_RTVs;
        execCtx.DSVs = m_DSVs;
        execCtx.SRVs = m_SRVs;
        execCtx.UAVs = m_UAVs;

        KFEGraphicsCommandList* cmd = execCtx.GetCommandList();
        ID3D12GraphicsCommandList* nativeCmd = nullptr;

        if (cmd != nullptr)
        {
            nativeCmd = cmd->GetNative();
        }

        for (auto& compiledPass : m_ExecutionOrder)
        {
            if (!compiledPass.IsValid())
            {
                continue;
            }

            if (nativeCmd != nullptr && !compiledPass.BarriersBefore.empty())
            {
                nativeCmd->ResourceBarrier(
                    static_cast<UINT>(compiledPass.BarriersBefore.size()),
                    compiledPass.BarriersBefore.data());
            }

            compiledPass.Pass->Execute(execCtx);
        }
    }

} // namespace kfe::rg
