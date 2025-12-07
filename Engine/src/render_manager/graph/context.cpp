// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */

#include "engine/render_manager/graph/context.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/texture/texture.h"
#include "engine/render_manager/api/commands/graphics_list.h"

#include <d3d12.h>

#pragma region Impl_Declaration

class kfe::rg::RGExecutionContext::Impl
{
public:
	kfe::KFEGraphicsCommandList * CommandList{nullptr};

	std::vector<kfe::KFETexture*> Textures;
	std::vector<kfe::KFEBuffer*>  Buffers;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTVs;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> DSVs;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> SRVs;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> UAVs;
};

#pragma endregion

#pragma region Context_Implementation

kfe::rg::RGExecutionContext::RGExecutionContext()
	: m_impl(std::make_unique<Impl>())
{}

kfe::rg::RGExecutionContext::~RGExecutionContext() = default;

kfe::rg::RGExecutionContext::RGExecutionContext					   (RGExecutionContext&&) noexcept = default;
kfe::rg::RGExecutionContext& kfe::rg::RGExecutionContext::operator=(RGExecutionContext&&) noexcept = default;

kfe::KFEGraphicsCommandList* kfe::rg::RGExecutionContext::GetCommandList() const noexcept
{
	return m_impl ? m_impl->CommandList : nullptr;
}

kfe::KFETexture* kfe::rg::RGExecutionContext::GetTexture(RGTextureHandle handle) const noexcept
{
	if (!m_impl || !handle.IsValid())
	{
		return nullptr;
	}

	const auto index = handle.Index;
	if (index >= m_impl->Textures.size())
	{
		return nullptr;
	}

	return m_impl->Textures[index];
}

kfe::KFEBuffer* kfe::rg::RGExecutionContext::GetBuffer(RGBufferHandle handle) const noexcept
{
	if (!m_impl || !handle.IsValid())
	{
		return nullptr;
	}

	const auto index = handle.Index;
	if (index >= m_impl->Buffers.size())
	{
		return nullptr;
	}

	return m_impl->Buffers[index];
}

D3D12_CPU_DESCRIPTOR_HANDLE kfe::rg::RGExecutionContext::GetRTV(RGTextureHandle handle) const noexcept
{
	D3D12_CPU_DESCRIPTOR_HANDLE nullHandle{};
	if (!m_impl || !handle.IsValid())
	{
		return nullHandle;
	}

	const auto index = handle.Index;
	if (index >= m_impl->RTVs.size())
	{
		return nullHandle;
	}

	return m_impl->RTVs[index];
}

D3D12_CPU_DESCRIPTOR_HANDLE kfe::rg::RGExecutionContext::GetDSV(RGTextureHandle handle) const noexcept
{
	D3D12_CPU_DESCRIPTOR_HANDLE nullHandle{};
	if (!m_impl || !handle.IsValid())
	{
		return nullHandle;
	}

	const auto index = handle.Index;
	if (index >= m_impl->DSVs.size())
	{
		return nullHandle;
	}

	return m_impl->DSVs[index];
}

D3D12_GPU_DESCRIPTOR_HANDLE kfe::rg::RGExecutionContext::GetSRV(RGTextureHandle handle) const noexcept
{
	D3D12_GPU_DESCRIPTOR_HANDLE nullHandle{};
	nullHandle.ptr = 0;

	if (!m_impl || !handle.IsValid())
	{
		return nullHandle;
	}

	const auto index = handle.Index;
	if (index >= m_impl->SRVs.size())
	{
		return nullHandle;
	}

	return m_impl->SRVs[index];
}

D3D12_GPU_DESCRIPTOR_HANDLE kfe::rg::RGExecutionContext::GetUAV(RGTextureHandle handle) const noexcept
{
	D3D12_GPU_DESCRIPTOR_HANDLE nullHandle{};
	nullHandle.ptr = 0;

	if (!m_impl || !handle.IsValid())
	{
		return nullHandle;
	}

	const auto index = handle.Index;
	if (index >= m_impl->UAVs.size())
	{
		return nullHandle;
	}

	return m_impl->UAVs[index];
}

#pragma endregion
