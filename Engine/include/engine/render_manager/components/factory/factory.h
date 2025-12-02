#pragma once

#include "EngineAPI.h"

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

		[[nodiscard]] bool			 Initialize		   ();
		[[nodiscard]] IDXGIFactory7* GetNative		   () const noexcept;
		[[nodiscard]] bool			 IsTearingSupported() const noexcept;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
} // namespace kfe
