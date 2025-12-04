// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */
#include "pch.h"
#include "engine/render_manager/components/adapter.h"

#include <wrl/client.h>
#include <unordered_map>
#include <dxgi1_6.h>

#include "engine/render_manager/components/factory.h"
#include "engine/system/exception/base_exception.h"
#include "engine/utils/helpers.h"
#include "engine/utils/logger.h"

//~ Impl Declaration
class kfe::KFEAdapter::Impl 
{
public:
	 Impl() = default;
	~Impl() = default;

	bool Initialize(const KFEFactory* factory,
		const IAdapterSelectionStrategy* strategy);
	
	NODISCARD std::string  GetName			    () const noexcept;
	NODISCARD EAdapterType GetAdapterType		() const noexcept;
	NODISCARD UINT64	   GetDedicatedVRam	    () const noexcept;
	NODISCARD UINT64	   GetSharedSystemMemory() const noexcept;

	NODISCARD IDXGIAdapter4*			  GetNative	    ()			 const noexcept;
	NODISCARD std::vector<IDXGIAdapter4*> GetAllNative  ()			 const noexcept;
	NODISCARD AdapterInfo				  GetAdapterInfo(UINT index) const noexcept;

	//~ for debugging
	void LogAdapters() const noexcept;

private:
	std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter4>> m_ppAdapters{};
	std::vector<IDXGIAdapter4*>						   m_ppCached{};
	std::unordered_map<UINT, AdapterInfo>			   m_mapAdapterInfos{};

	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_pAdapter{ nullptr };
	AdapterInfo							  m_adapterInfo{};
	std::string							  m_szAnsiName{};
	kfe::EAdapterType					  m_eAdapterType{ EAdapterType::Discrete };
};

kfe::KFEAdapter::KFEAdapter()
	: m_impl(std::make_unique<kfe::KFEAdapter::Impl>())
{}

kfe::KFEAdapter::~KFEAdapter() = default;

bool kfe::KFEAdapter::Initialize(const KFEFactory* factory,
	const IAdapterSelectionStrategy* strategy)
{
	return m_impl->Initialize(factory, strategy);
}

kfe::EAdapterType kfe::KFEAdapter::GetAdapterType() const noexcept
{
	return m_impl->GetAdapterType();
}

UINT64 kfe::KFEAdapter::GetDedicatedVRam() const noexcept
{
	return m_impl->GetDedicatedVRam();
}

UINT64 kfe::KFEAdapter::GetSharedSystemMemory() const noexcept
{
	return m_impl->GetSharedSystemMemory();
}

IDXGIAdapter4* kfe::KFEAdapter::GetNative() const noexcept
{
	return m_impl->GetNative();
}

std::vector<IDXGIAdapter4*> kfe::KFEAdapter::GetAllNative() const noexcept
{
	return m_impl->GetAllNative();
}

kfe::AdapterInfo kfe::KFEAdapter::GetAdapterInfo(UINT index) const noexcept
{
	return m_impl->GetAdapterInfo(index);
}

void kfe::KFEAdapter::LogAdapters() const noexcept
{
	m_impl->LogAdapters();
}

//~ Impl Implemetation
bool kfe::KFEAdapter::Impl::Initialize(const kfe::KFEFactory* factory,
	const IAdapterSelectionStrategy* strategy)
{
	if (!factory || !strategy) return false;

	IDXGIFactory7* native = factory->GetNative();

	m_ppAdapters	 .clear();
	m_ppCached		 .clear();

	std::vector<AdapterInfo> adaptersInfo;

	UINT index = 0u;
	for (;; ++index)
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter1;
		HRESULT hr = native->EnumAdapters1(index, &adapter1);
		
		if (hr == DXGI_ERROR_NOT_FOUND) // No more adapters
			break;
		
		if (FAILED(hr) || !adapter1) // unexpected result
			return false;

		// Get description
		DXGI_ADAPTER_DESC1 desc{};
		hr = adapter1->GetDesc1(&desc);
		if (FAILED(hr)) return false;

		// Convert to IDXGIAdapter4
		Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter4;
		hr = adapter1.As(&adapter4);
		if (FAILED(hr) || !adapter4) return false;

		// Classification of adapter type
		EAdapterType type = EAdapterType::Unknown;

		auto vram = desc.DedicatedVideoMemory / (1024ull * 1024ull);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			type = EAdapterType::Software;
		}
		else if (vram > 140)
		{
			type = EAdapterType::Discrete;
		}
		else
		{
			type = EAdapterType::Integrated;
		}

		AdapterInfo info{};
		info.Index					= index;
		info.DedicatedSystemMemory	= desc.DedicatedSystemMemory;
		info.DedicatedVideoMemory	= desc.DedicatedVideoMemory;
		info.Description			= kfe_helpers::WideToAnsi(desc.Description);
		info.DeviceId				= desc.DeviceId;
		info.Flags					= desc.Flags;
		info.Revision				= desc.Revision;
		info.SharedSystemMemory		= desc.SharedSystemMemory;
		info.SubSysId				= desc.SubSysId;
		info.VendorId				= desc.VendorId;
		info.AdapterType			= type;

		adaptersInfo.push_back(info);
		m_ppAdapters.push_back(adapter4);
		m_ppCached  .push_back(adapter4.Get());

		m_mapAdapterInfos[index] = info;
	}

	if (adaptersInfo.empty())
	{
		LOG_ERROR("No Adapter Information Found!");
		return false;
	} 

	// Use strategy to select which adapter index to use
	std::optional<UINT> selectedIndexOpt = strategy->SelectAdapterIndex(adaptersInfo);
	if (!selectedIndexOpt) return false;

	const UINT selectedIndex = *selectedIndexOpt;

	// Find matching entry in the arrays
	size_t selectedPosition = static_cast<size_t>(-1);
	for (size_t i = 0; i < adaptersInfo.size(); ++i)
	{
		if (adaptersInfo[i].Index == selectedIndex)
		{
			selectedPosition = i;
			break;
		}
	}

	if (selectedPosition == static_cast<size_t>(-1))
	{
		THROW_MSG("Adapter Strategy return no valid adapter index");
		return false;
	}

	m_pAdapter     = m_ppAdapters[selectedPosition];
	m_adapterInfo  = adaptersInfo[selectedPosition];
	m_eAdapterType = adaptersInfo[selectedPosition].AdapterType;
	m_szAnsiName   = m_adapterInfo.Description;

	return true;
}

std::string kfe::KFEAdapter::Impl::GetName() const noexcept
{
	return m_szAnsiName;
}

kfe::EAdapterType kfe::KFEAdapter::Impl::GetAdapterType() const noexcept
{
	return m_eAdapterType;
}

UINT64 kfe::KFEAdapter::Impl::GetDedicatedVRam() const noexcept
{
	if (m_pAdapter)
	{
		return m_adapterInfo.DedicatedVideoMemory;
	}
	return 0u;
}

UINT64 kfe::KFEAdapter::Impl::GetSharedSystemMemory() const noexcept
{
	if (m_pAdapter)
	{
		return m_adapterInfo.SharedSystemMemory;
	}
	return 0u;
}

IDXGIAdapter4* kfe::KFEAdapter::Impl::GetNative() const noexcept
{
	return m_pAdapter.Get();
}

std::vector<IDXGIAdapter4*> kfe::KFEAdapter::Impl::GetAllNative() const noexcept
{
	return m_ppCached;
}

kfe::AdapterInfo kfe::KFEAdapter::Impl::GetAdapterInfo(UINT index) const noexcept
{
	return m_mapAdapterInfos.contains(index)? m_mapAdapterInfos.at(index) : AdapterInfo{};
}

void kfe::KFEAdapter::Impl::LogAdapters() const noexcept
{
	if (m_mapAdapterInfos.empty())
	{
		LOG_WARNING("[KFEAdapter] No adapters found.");
		return;
	}

	LOG_INFO("DXGI ADAPTERS");

	for (const auto& [index, info] : m_mapAdapterInfos)
	{
		const bool isSelected = (m_pAdapter && m_pAdapter.Get() == m_ppCached.at(index));

		const char* typeStr = "Unknown";
		switch (info.AdapterType)
		{
		case EAdapterType::Software:   typeStr = "Software (WARP)"; break;
		case EAdapterType::Discrete:   typeStr = "Discrete GPU";     break;
		case EAdapterType::Integrated: typeStr = "Integrated GPU";   break;
		default: break;
		}

		const auto& desc = info;

		std::string ansiName = desc.Description;

		LOG_INFO("-----------------------------------");
		LOG_INFO("Adapter Index : {}", index);
		LOG_INFO("Name          : {}", ansiName);
		LOG_INFO("Type          : {}", typeStr);
		LOG_INFO("Vendor ID     : {}", desc.VendorId);
		LOG_INFO("Device ID     : {}", desc.DeviceId);
		LOG_INFO("Revision      : {}", desc.Revision);
		LOG_INFO("Dedicated VRAM: {} MB", desc.DedicatedVideoMemory / (1024ull * 1024ull));
		LOG_INFO("Shared Memory : {} MB", desc.SharedSystemMemory / (1024ull * 1024ull));
		LOG_INFO("System Memory : {} MB", desc.DedicatedSystemMemory / (1024ull * 1024ull));
		LOG_INFO("Flags         : 0x{:X}", desc.Flags);

		if (isSelected)
		{
			LOG_SUCCESS(">>> SELECTED ADAPTER <<<");
		}
	}
}

//~ Strategies
std::optional<UINT> kfe::AdapterStrategyBestVram::SelectAdapterIndex(const std::vector<AdapterInfo>& adapters) const
{
	if (adapters.empty()) 
	{
		return std::nullopt;
	}

	UINT maxRam		   = 0u;
	UINT index		   = adapters.front().Index;
	bool hardwareFound = false;

	for (const auto& info : adapters) 
	{
		if (info.AdapterType == EAdapterType::Software) continue;

		if (info.DedicatedVideoMemory > maxRam)
		{
			maxRam		  = static_cast<UINT>(info.DedicatedVideoMemory);
			index		  = info.Index;
			hardwareFound = true;
		}
	}

	if (!hardwareFound) return adapters.front().Index;

	return index;
}

std::optional<UINT> kfe::AdapterStrategyFirstHardware::SelectAdapterIndex(const std::vector<AdapterInfo>& adapters) const
{
	if (adapters.empty())
	{
		return std::nullopt;
	}

	for (const auto& info : adapters)
	{
		if (info.AdapterType == EAdapterType::Software)   continue;
		if (info.AdapterType == EAdapterType::Integrated) continue;
		return info.Index;
	}

	return adapters.front().Index;
}

std::optional<UINT> kfe::AdapterStrategyDiscreteGpu::SelectAdapterIndex(const std::vector<AdapterInfo>& adapters) const
{
	if (adapters.empty())
	{
		return std::nullopt;
	}

	for (const auto& info : adapters)
	{
		if (info.AdapterType == EAdapterType::Discrete)
		{
			return info.Index;
		}
	}
	
	return std::nullopt;
}

std::optional<UINT> kfe::AdapterStrategyIntegratedGpu::SelectAdapterIndex(const std::vector<AdapterInfo>& adapters) const
{
	if (adapters.empty())
	{
		return std::nullopt;
	}

	for (const auto& info : adapters)
	{
		if (info.AdapterType == EAdapterType::Integrated)
		{
			return info.Index;
		}
	}

	return std::nullopt;
}

std::optional<UINT> kfe::AdapterStrategySoftware::SelectAdapterIndex(const std::vector<AdapterInfo>& adapters) const
{
	if (adapters.empty())
	{
		return std::nullopt;
	}

	for (const auto& info : adapters)
	{
		if (info.AdapterType == EAdapterType::Software)
		{
			return info.Index;
		}
	}

	return std::nullopt;
}

std::optional<UINT> kfe::AdapterStrategyIndex::SelectAdapterIndex(const std::vector<AdapterInfo>& adapters) const
{
	if (adapters.empty())
	{
		return std::nullopt;
	}

	for (const auto& info : adapters)
	{
		if (info.Index == m_nIndex)
		{
			return info.Index;
		}
	}

	return std::nullopt;
}

std::optional<UINT> kfe::AdapterStrategyNameContains::SelectAdapterIndex(const std::vector<AdapterInfo>& adapters) const
{
	if (adapters	 .empty()) return std::nullopt;
	if (m_szSubstring.empty()) return adapters.front().Index;

	std::string search = kfe_helpers::ToLowerAscii(m_szSubstring);

	for (const auto& info : adapters)
	{
		std::string lower = info.Description;
		if (lower.find(search) != std::string::npos)
		{
			return info.Index;
		}
	}

	return std::nullopt;
}
