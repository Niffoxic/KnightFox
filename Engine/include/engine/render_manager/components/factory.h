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
#include "EngineAPI.h"
#include "engine/core.h"

#include <memory>

struct IDXGIFactory7;

namespace kfe
{
	/// <summary>
	/// Creates DXGI Factory and Tests Feature Supports
	/// - Currently Tests Feature For
	///		1. Tearing
	/// </summary>
	class KFE_API KFEFactory
	{
	public:
		 KFEFactory();
		~KFEFactory();

		KFEFactory			 (const KFEFactory&) = delete;
		KFEFactory& operator=(const KFEFactory&) = delete;

		NODISCARD bool			 Initialize		   ();
		NODISCARD IDXGIFactory7* GetNative		   () const noexcept;
		NODISCARD bool			 IsTearingSupported() const noexcept;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
} // namespace kfe
