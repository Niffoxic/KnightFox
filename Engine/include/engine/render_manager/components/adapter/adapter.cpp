#include "pch.h"
#include "adapter.h"

#include <wrl/client.h>
#include <unordered_map>

#include "engine/render_manager/components/factory/factory.h"
#include "engine/utils/helpers.h"
#include "engine/utils/logger/logger.h"
#include "engine/core/exception/base_exception.h"

//~ Impl Declaration
class kfe::KFEAdapter::Impl 
{
public:
	 Impl() = default;
	~Impl() = default;

	bool Initialize(const KFEFactory* factory,
		const IAdapterSelectionStrategy* strategy);
	
	[[nodiscard]] const DXGI_ADAPTER_DESC1& GetDesc () const noexcept;
	[[nodiscard]] std::string  GetName			    () const noexcept;
	[[nodiscard]] EAdapterType GetAdapterType		() const noexcept;
	[[nodiscard]] UINT64	   GetDedicatedVRam	    () const noexcept;
	[[nodiscard]] UINT64	   GetSharedSystemMemory() const noexcept;

	[[nodiscard]] IDXGIAdapter4*			  GetNative	    ()			 const noexcept;
	[[nodiscard]] std::vector<IDXGIAdapter4*> GetAllNative  ()			 const noexcept;
	[[nodiscard]] AdapterInfo				  GetAdapterInfo(UINT index) const noexcept;

	//~ for debugging
	void LogAdapters() const noexcept;

private:
	std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter4>> m_ppAdapters{};
	std::vector<IDXGIAdapter4*>						   m_ppCached{};
	std::unordered_map<UINT, AdapterInfo>			   m_mapAdapterInfos{};

	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_pAdapter{ nullptr };
	DXGI_ADAPTER_DESC1                    m_desc{};
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

const DXGI_ADAPTER_DESC1& kfe::KFEAdapter::GetDesc() const noexcept
{
	return m_impl->GetDesc();
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
		info.Index		 = index;
		info.Desc		 = desc;
		info.AdapterType = type;

		adaptersInfo.push_back(info);
		m_ppAdapters.push_back(adapter4);
		m_ppCached  .push_back(adapter4.Get());

		m_mapAdapterInfos[index] = info;
	}

	if (adaptersInfo.empty()) return false;

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
	m_desc		   = adaptersInfo[selectedPosition].Desc;
	m_eAdapterType = adaptersInfo[selectedPosition].AdapterType;
	m_szAnsiName   = kfe_helpers::WideToAnsi(m_desc.Description);

	return true;
}

const DXGI_ADAPTER_DESC1& kfe::KFEAdapter::Impl::GetDesc() const noexcept
{
	return m_desc;
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
		return m_desc.DedicatedVideoMemory;
	}
	return 0u;
}

UINT64 kfe::KFEAdapter::Impl::GetSharedSystemMemory() const noexcept
{
	if (m_pAdapter)
	{
		return m_desc.SharedSystemMemory;
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

	LOG_INFO("========== DXGI ADAPTERS ==========");

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

		const auto& desc = info.Desc;

		std::string ansiName = kfe_helpers::WideToAnsi(desc.Description);

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

	LOG_INFO("===================================");
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

		if (info.Desc.DedicatedVideoMemory > maxRam)
		{
			maxRam		  = static_cast<UINT>(info.Desc.DedicatedVideoMemory);
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

	std::wstring search = kfe_helpers::AnsiToWide(m_szSubstring);
	search = kfe_helpers::ToLowerAscii(search);

	for (const auto& info : adapters)
	{
		std::wstring lower = kfe_helpers::ToLowerAscii(info.Desc.Description);
		if (lower.find(search) != std::wstring::npos)
		{
			return info.Index;
		}
	}

	return std::nullopt;
}
