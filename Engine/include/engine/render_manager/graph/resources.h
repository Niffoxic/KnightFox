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
#include <memory>

namespace kfe::rg
{
	/// <summary>
	/// Render graph Logical Resources inside the render graph.
	/// </summary>
	class RGResources
	{
	public:
		 RGResources();
		~RGResources();

		RGResources			  (const RGResources&) = delete;
		RGResources& operator=(const RGResources&) = delete;

		RGResources			  (RGResources&&) noexcept;
		RGResources& operator=(RGResources&&) noexcept;

		//~ Life Time Managment
		NODISCARD RGTextureHandle CreateTexture(const RGTextureDesc& desc);
		NODISCARD RGBufferHandle  CreateBuffer (const RGBufferDesc& desc);

		//~ Queries
		NODISCARD const RGTextureDesc& GetTextureDesc(RGTextureHandle handle) const;
		NODISCARD const RGBufferDesc&  GetBufferDesc (RGBufferHandle handle) const;

		//~ management
		void Reset() noexcept;

		NODISCARD std::uint32_t GetTextureCount() const noexcept;
		NODISCARD std::uint32_t GetBufferCount () const noexcept;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
