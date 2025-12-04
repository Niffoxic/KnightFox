#pragma once

#include "EngineAPI.h"

#include <memory>

#include "engine/core/core.h"

struct ID3D12Device;

namespace kfe
{
	class KFEAdapter;
	class KFEFactory;
	class KFEMonitor;

	enum class EDeviceFeatureLevel : uint8_t
	{
		FL_11_0,
		FL_11_1,
		FL_12_0,
		FL_12_1,
	};

	enum class EDeviceCreateFlags : uint32_t
	{
		None					 = 0,
		EnableDebugLayer		 = 1u << 0,
		EnableGPUBasedValidation = 1u << 1,
		EnableStablePowerState	 = 1u << 2,
	};

	inline EDeviceCreateFlags operator|(EDeviceCreateFlags a,
										EDeviceCreateFlags b)
	{
		return static_cast<EDeviceCreateFlags>(	static_cast<uint32_t>(a) |
												static_cast<uint32_t>(b));
	}

	inline EDeviceCreateFlags& operator|=(EDeviceCreateFlags& a,
										  EDeviceCreateFlags b)
	{
		a = a | b;
		return a;
	}

	inline EDeviceCreateFlags operator&(EDeviceCreateFlags a, EDeviceCreateFlags b)
	{
		return static_cast<EDeviceCreateFlags>(	static_cast<uint32_t>(a) &
												static_cast<uint32_t>(b));
	}

	inline EDeviceCreateFlags& operator&=(EDeviceCreateFlags& a, EDeviceCreateFlags b)
	{
		a = a & b;
		return a;
	}

	inline bool operator==(EDeviceCreateFlags a, uint32_t b)
	{
		return static_cast<uint32_t>(a) == b;
	}

	inline bool operator!=(EDeviceCreateFlags a, uint32_t b)
	{
		return static_cast<uint32_t>(a) != b;
	}

	typedef struct _KFE_DEVICE_CREATE_DESC
	{
		const KFEAdapter* Adapter;
		const KFEFactory* Factory;
		const KFEMonitor* Monitor;

		EDeviceFeatureLevel MinimumFeatureLevel = EDeviceFeatureLevel::FL_12_0;
		EDeviceCreateFlags  Flags				= EDeviceCreateFlags ::None;

		const char* debugName = nullptr;
	} KFE_DEVICE_CREATE_DESC;

	class KFE_API KFEDevice
	{
	public:
		 KFEDevice();
		~KFEDevice();

		NODISCARD bool Initialize(_In_ const KFE_DEVICE_CREATE_DESC& desc);
		NODISCARD bool Release   ();

		NODISCARD ID3D12Device*		  GetNative			 () const;
		NODISCARD EDeviceFeatureLevel GetFeatureLevel	 () const noexcept;
		NODISCARD bool                IsDebugLayerEnabled() const noexcept;
		NODISCARD bool                IsValid			 () const noexcept;

		NODISCARD UINT GetRTVDescriptorSize() const noexcept;
		NODISCARD UINT GetDSVDescriptorSize() const noexcept;
		NODISCARD UINT GetCRUDescriptorSize() const noexcept;

		//~ for debugging
		void LogDevice() const noexcept;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
} // namespace kfe
