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
#include <string>
#include <vector>
#include <utility>

#include "resources.h"
#include "pass.h"
#include "types.h"
#include "builder.h"

namespace kfe::rg 
{
	class RGCompiled;

	/// <summary>
	/// Central container for the logical render graph.
	///  - Owns all logical resources
	///  - Owns all passes
	///  - Compiles into a runtime graph.
	/// </summary>
	class RenderGraph
	{
	public:
		RenderGraph () = default;
		~RenderGraph() = default;

		RenderGraph			  (const RenderGraph&) = delete;
		RenderGraph& operator=(const RenderGraph&) = delete;

		RenderGraph			  (RenderGraph&&) noexcept = default;
		RenderGraph& operator=(RenderGraph&&) noexcept = default;

		template<typename BuildFunc, typename ExecuteFunc>
		void AddPass(
			_In_ std::string&  name,
			_In_ BuildFunc&&   fnBuild,
			_In_ ExecuteFunc&& fnExecute);

		NODISCARD RGCompiled Compile() const;

		//~ Getters
		NODISCARD		RGResources& GetResources()		  noexcept;
		NODISCARD const RGResources& GetResources() const noexcept;

		NODISCARD std::uint32_t GetPassCounts() const noexcept;

	private:
		RGResources				m_resources{};
		std::vector<RenderPass> m_passes{};
	};

	template<typename BuildFunc, typename ExecuteFunc>
	inline void RenderGraph::AddPass(std::string& name,
									 BuildFunc	&& fnBuild,
									 ExecuteFunc&& fnExecute)
	{
		RenderPass pass{};
		pass.Desc.Name = name;

		RGBuilder builder{ m_resources, pass.Desc };
		fnBuild(builder);
		pass.Execute = std::forward<ExecuteFunc>(fnExecute);

		m_passes.emplace_back(std::move(pass));
	}
} // namespac kfe::rg
