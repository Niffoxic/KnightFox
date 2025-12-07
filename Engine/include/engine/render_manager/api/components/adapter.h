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

#include <string>
#include <optional>
#include <vector>
#include <memory>
#include <cstdint>

struct IDXGIAdapter4;

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
		std::uint32_t Index{ 0u };
		std::string   Description{};
		std::uint32_t VendorId{};
		std::uint32_t DeviceId{};
		std::uint32_t SubSysId{};
		std::uint32_t Revision{};
		std::uint64_t DedicatedVideoMemory{};
		std::uint64_t DedicatedSystemMemory{};
		std::uint64_t SharedSystemMemory{};
		std::uint32_t Flags{};
		EAdapterType AdapterType{ EAdapterType::Unknown };
	};

	class KFE_API IAdapterSelectionStrategy
	{
	public:
		virtual ~IAdapterSelectionStrategy() = default;
		
		[[nodiscard]]
		virtual std::optional<std::uint32_t> SelectAdapterIndex(
			const std::vector<AdapterInfo>& adapters) const = 0;
	};

	class KFE_API KFEAdapter
	{
	public:
		 KFEAdapter();
		~KFEAdapter();
		
		KFEAdapter			 (const KFEAdapter&) = delete;
		KFEAdapter& operator=(const KFEAdapter&) = delete;

		NODISCARD bool Initialize(const KFEFactory*				   factory,
								  const IAdapterSelectionStrategy* strategy);
		
		NODISCARD std::string				  GetName			   () const noexcept;
		NODISCARD EAdapterType				  GetAdapterType	   () const noexcept;
		NODISCARD std::uint64_t				  GetDedicatedVRam	   () const noexcept;
		NODISCARD std::uint64_t				  GetSharedSystemMemory() const noexcept;
		NODISCARD IDXGIAdapter4*			  GetNative			   () const noexcept;
		NODISCARD std::vector<IDXGIAdapter4*> GetAllNative		   () const noexcept;
		NODISCARD AdapterInfo				  GetAdapterInfo	   (std::uint32_t index) const noexcept;

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

		NODISCARD
		std::optional<std::uint32_t> SelectAdapterIndex(
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

		NODISCARD
		std::optional<std::uint32_t> SelectAdapterIndex(
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

		NODISCARD
		std::optional<std::uint32_t> SelectAdapterIndex(
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

		NODISCARD
		std::optional<std::uint32_t> SelectAdapterIndex(
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

		NODISCARD
		std::optional<std::uint32_t> SelectAdapterIndex(
			const std::vector<AdapterInfo>& adapters) const override;
	};

	/// <summary>
	/// Pickup adapter based on the index
	/// </summary>
	class AdapterStrategyIndex final : public IAdapterSelectionStrategy
	{
	public:
		explicit AdapterStrategyIndex(std::uint32_t index)
			: m_nIndex(index)
		{}

		~AdapterStrategyIndex() override = default;

		NODISCARD
		std::optional<std::uint32_t> SelectAdapterIndex(
			const std::vector<AdapterInfo>& adapters) const override;

	private:
		std::uint32_t m_nIndex{ 0u };
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

		NODISCARD
		std::optional<std::uint32_t> SelectAdapterIndex(
			const std::vector<AdapterInfo>& adapters) const override;

	private:
		std::string m_szSubstring;
	};
} // namespace kfe
