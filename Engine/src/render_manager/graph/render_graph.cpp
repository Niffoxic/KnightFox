// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */

#include "engine/render_manager/graph/render_graph.h"


kfe::rg::RGCompiled kfe::rg::RenderGraph::Compile() const
{
    return nullptr;
}

kfe::rg::RGResources& kfe::rg::RenderGraph::GetResources() noexcept
{
    return m_resources;
}

const kfe::rg::RGResources& kfe::rg::RenderGraph::GetResources() const noexcept
{
    return m_resources;
}

std::uint32_t kfe::rg::RenderGraph::GetPassCounts() const noexcept
{
    return m_passes.size();
}
