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
#include "engine/render_manager/graph/render_graph.h"
#include "engine/render_manager/graph/compiled_graph.h"

_Use_decl_annotations_
kfe::rg::RGCompiled kfe::rg::RenderGraph::Compile() const
{
	RGCompiled compiled{};

	const std::uint32_t textureCount = m_resources.GetTextureCount();
	const std::uint32_t bufferCount = m_resources.GetBufferCount();

	compiled.m_TextureResources.resize(textureCount, nullptr);
	compiled.m_BufferResources.resize(bufferCount, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE nullCpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE nullGpu{};
	nullGpu.ptr = 0;

	compiled.m_RTVs.resize(textureCount, nullCpu);
	compiled.m_DSVs.resize(textureCount, nullCpu);
	compiled.m_SRVs.resize(textureCount, nullGpu);
	compiled.m_UAVs.resize(textureCount, nullGpu);

	compiled.m_ExecutionOrder.clear();
	compiled.m_ExecutionOrder.reserve(m_passes.size());

	for (std::size_t i = 0; i < m_passes.size(); ++i)
	{
		const RenderPass& srcPass = m_passes[i];

		CompiledPass compiledPass{};
		compiledPass.Pass = const_cast<RenderPass*>(&srcPass);
		compiled.m_ExecutionOrder.emplace_back(std::move(compiledPass));
	}

	return compiled;
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
