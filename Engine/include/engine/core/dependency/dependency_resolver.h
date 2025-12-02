// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : PixelFox (WMG Warwick - Module 2)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */

#pragma once

#include "EngineAPI.h"

#include "engine/core/interface/interface_manager.h"

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <sal.h>

namespace kfe
{
	/// <summary>
	/// Resolves Dependency for initailization and run process
	/// Initializes in provided relation but run loops from reverse order
	/// </summary>
	class KFE_API DependencyResolver
	{
	public:
		 DependencyResolver();
		~DependencyResolver() = default;

		//~ no copy or move
		DependencyResolver(_In_ const DependencyResolver&) = delete;
		DependencyResolver(_Inout_ DependencyResolver&&)   = delete;

		DependencyResolver& operator=(_In_ const DependencyResolver&) = delete;
		DependencyResolver& operator=(_Inout_ DependencyResolver&&)   = delete;

		//~ Features
		void Register(_In_opt_ IManager* instance);
		void Clear();

		_NODISCARD _Check_return_ _Success_(return != false)
		bool Init();
		bool UpdateLoopStart(_In_ float deltaTime) const;
		bool UpdateLoopEnd() const;
		bool Shutdown();

		void AddDependency(_In_ IManager* kid, _In_ IManager* parent);

	private:
		class Impl;
		std::shared_ptr<Impl>		GetImpl		()		 { return m_impl; }
		std::shared_ptr<const Impl> GetConstImpl() const { return m_impl; }

		std::shared_ptr<Impl> m_impl{ nullptr };
	};
} // namespace kfe
