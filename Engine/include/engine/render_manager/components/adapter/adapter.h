#pragma once

#include "EngineAPI.h"

#include <string>
#include <optional>
#include <vector>
#include <memory>
#include <cstdint>
#include <dxgi1_6.h>

namespace kfe
{
	class KFEFactory;

	enum class EAdapterType : uint8_t
	{
		Software,
		Discrete,
		Integrated,
		Unknown
	};

	//~ define adapter
	struct AdapterInfo
	{
		UINT                    Index{ 0u };
		DXGI_ADAPTER_DESC1      Desc{};
		EAdapterType AdapterType{ EAdapterType::Unknown };
	};

	class KFE_API IAdapterSelectionStrategy
	{
	public:
		virtual ~IAdapterSelectionStrategy() = default;
		
		[[nodiscard]]
		virtual std::optional<UINT> SelectAdapterIndex(
			const std::vector<AdapterInfo>& adapters) const = 0;
	};

	class KFE_API KFEAdapter
	{
	public:
		 KFEAdapter();
		~KFEAdapter();
		
		KFEAdapter			 (const KFEAdapter&) = delete;
		KFEAdapter& operator=(const KFEAdapter&) = delete;

		[[nodiscard]]
		bool Initialize(const KFEFactory*    factory,
			const IAdapterSelectionStrategy* strategy);
		
		[[nodiscard]] const DXGI_ADAPTER_DESC1&   GetDesc			   () const noexcept;
		[[nodiscard]] std::string				  GetName			   () const noexcept;
		[[nodiscard]] EAdapterType				  GetAdapterType	   () const noexcept;
		[[nodiscard]] UINT64					  GetDedicatedVRam	   () const noexcept;
		[[nodiscard]] UINT64					  GetSharedSystemMemory() const noexcept;
		[[nodiscard]] IDXGIAdapter4*			  GetNative			   () const noexcept;
		[[nodiscard]] std::vector<IDXGIAdapter4*> GetAllNative		   () const noexcept;
		[[nodiscard]] AdapterInfo				  GetAdapterInfo	   (UINT index) const noexcept;

		//~ for debuggin
		void LogAdapters() const noexcept;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};

	//~ Adapter pickups strategies

	/// <summary>
	/// Picks adapter with maximum VRAM.
	/// </summary>
	class AdapterStrategyBestVram final : public IAdapterSelectionStrategy
	{
	public:
		 AdapterStrategyBestVram() = default;
		~AdapterStrategyBestVram() override = default;

		[[nodiscard]]
		std::optional<UINT> SelectAdapterIndex(
			const std::vector<AdapterInfo>& adapters) const override;
	};

	/// <summary>
	/// Picks the first adapter that is available but skips Software
	/// </summary>
	class AdapterStrategyFirstHardware final : public IAdapterSelectionStrategy
	{
	public:
		AdapterStrategyFirstHardware() = default;
		~AdapterStrategyFirstHardware() override = default;

		[[nodiscard]]
		std::optional<UINT> SelectAdapterIndex(
			const std::vector<AdapterInfo>& adapters) const override;
	};

	/// <summary>
	/// Pick discrete GPU Only
	/// </summary>
	class AdapterStrategyDiscreteGpu final : public IAdapterSelectionStrategy
	{
	public:
		AdapterStrategyDiscreteGpu() = default;
		~AdapterStrategyDiscreteGpu() override = default;

		[[nodiscard]]
		std::optional<UINT> SelectAdapterIndex(
			const std::vector<AdapterInfo>& adapters) const override;
	};
	
	/// <summary>
	/// Picks Integrated GPU Only
	/// </summary>
	class AdapterStrategyIntegratedGpu final : public IAdapterSelectionStrategy
	{
	public:
		AdapterStrategyIntegratedGpu() = default;
		~AdapterStrategyIntegratedGpu() override = default;

		[[nodiscard]]
		std::optional<UINT> SelectAdapterIndex(
			const std::vector<AdapterInfo>& adapters) const override;
	};

	/// <summary>
	/// Pickup Software Adapter
	/// </summary>
	class AdapterStrategySoftware final : public IAdapterSelectionStrategy
	{
	public:
		AdapterStrategySoftware() = default;
		~AdapterStrategySoftware() override = default;

		[[nodiscard]]
		std::optional<UINT> SelectAdapterIndex(
			const std::vector<AdapterInfo>& adapters) const override;
	};

	/// <summary>
	/// Pickup adapter based on the index
	/// </summary>
	class AdapterStrategyIndex final : public IAdapterSelectionStrategy
	{
	public:
		explicit AdapterStrategyIndex(UINT index)
			: m_nIndex(index)
		{}

		~AdapterStrategyIndex() override = default;

		[[nodiscard]]
		std::optional<UINT> SelectAdapterIndex(
			const std::vector<AdapterInfo>& adapters) const override;

	private:
		UINT m_nIndex{ 0u };
	};

	/// <summary>
	/// Pickup If specified name is there
	/// </summary>
	class AdapterStrategyNameContains final : public IAdapterSelectionStrategy
	{
	public:
		explicit AdapterStrategyNameContains(std::string substring)
			: m_szSubstring(std::move(substring)) {
		}

		~AdapterStrategyNameContains() override = default;

		[[nodiscard]]
		std::optional<UINT> SelectAdapterIndex(
			const std::vector<AdapterInfo>& adapters) const override;

	private:
		std::string m_szSubstring;
	};
} // namespace kfe
