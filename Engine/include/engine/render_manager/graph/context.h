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

#include "engine/core.h"
#include "types.h"

#include <memory>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

namespace kfe
{
	class KFEGraphicsCommandList;
	class KFETexture;
	class KFEBuffer;
} // namespace kfe

namespace kfe::rg
{
	class RGCompiled;

	/// <summary>
	/// Execution time view
	/// </summary>
	class RGExecutionContext
	{
	public:
		 RGExecutionContext();
		~RGExecutionContext();

		RGExecutionContext			 (const RGExecutionContext&) = delete;
		RGExecutionContext& operator=(const RGExecutionContext&) = delete;

		RGExecutionContext			 (RGExecutionContext&&) noexcept;
		RGExecutionContext& operator=(RGExecutionContext&&) noexcept;

		NODISCARD KFEGraphicsCommandList* GetCommandList() const noexcept;

		NODISCARD KFETexture* GetTexture(RGTextureHandle handle) const noexcept;
		NODISCARD KFEBuffer*  GetBuffer (RGBufferHandle handle) const noexcept;

		//~ queries
		NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(RGTextureHandle handle) const noexcept;
		NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetDSV(RGTextureHandle handle) const noexcept;
		NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE GetSRV(RGTextureHandle handle) const noexcept;
		NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE GetUAV(RGTextureHandle handle) const noexcept;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;

		friend class CompiledRenderGraph;
	};
} // namespace kfe::rg
