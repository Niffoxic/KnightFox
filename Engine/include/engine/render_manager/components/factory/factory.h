#pragma once

#include <memory>

class IDXGIFactory7;

namespace kfe
{
	class KFEFactory
	{
	public:
		 KFEFactory();
		~KFEFactory() = default;

		[[nodiscard]] bool Initialize();
		IDXGIFactory7* GetFactory() const noexcept;

		[[nodiscard]] bool IsTearingSupported() const noexcept;

	private:
		class Impl;
		std::shared_ptr<Impl>		GetImpl		()		 { return m_impl; }
		std::shared_ptr<const Impl> GetConstImpl() const { return m_impl; }

		std::shared_ptr<Impl> m_impl{ nullptr };
	};
} // namespace kfe
