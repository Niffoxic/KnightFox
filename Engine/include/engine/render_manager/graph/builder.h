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
#include "types.h"

namespace kfe::rg
{
	class RGResources;

	/// <summary>
	/// Builds up RenderPassDesc and allocates logical resources 
	/// using RenderGraphResources
	/// <summary>
	class RGBuilder
	{
	public:
		RGBuilder(RGResources& resources,
				  RenderPassDesc& desc) noexcept;

		//~ Resource creation
		NODISCARD RGTextureHandle CreateTexture(const RGTextureDesc& desc);
		NODISCARD RGBufferHandle  CreateBuffer (const RGBufferDesc& desc);

		//~ Resource usage
		void ReadTexture	 (RGTextureHandle handle);
		void WriteTexture	 (RGTextureHandle handle);
		void ReadWriteTexture(RGTextureHandle handle);

		void ReadBuffer (RGBufferHandle handle);
		void WriteBuffer(RGBufferHandle handle);

		NODISCARD		RenderPassDesc& GetDesc()       noexcept { return *m_pDesc; }
		NODISCARD const RenderPassDesc& GetDesc() const noexcept { return *m_pDesc; }

	private:
		RGResources*	m_pResources{ nullptr };
		RenderPassDesc* m_pDesc		{ nullptr };
	};
} // namespace kfe::rg
