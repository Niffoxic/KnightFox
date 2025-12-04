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
#include "engine/render_manager/components/device.h"
#include "engine/render_manager/components/adapter.h"
#include "engine/render_manager/components/factory.h"
#include "engine/render_manager/components/monitor.h"

#include "engine/system/common_types.h"

#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"
#include "engine/system/exception/dx_exception.h"

using Microsoft::WRL::ComPtr;

namespace
{
    inline D3D_FEATURE_LEVEL ToD3DFeatureLevel(kfe::EDeviceFeatureLevel fl)
    {
        switch (fl)
        {
        case kfe::EDeviceFeatureLevel::FL_11_0: return D3D_FEATURE_LEVEL_11_0;
        case kfe::EDeviceFeatureLevel::FL_11_1: return D3D_FEATURE_LEVEL_11_1;
        case kfe::EDeviceFeatureLevel::FL_12_0: return D3D_FEATURE_LEVEL_12_0;
        case kfe::EDeviceFeatureLevel::FL_12_1: return D3D_FEATURE_LEVEL_12_1;
        default:                                return D3D_FEATURE_LEVEL_11_0;
        }
    }

    inline kfe::EDeviceFeatureLevel ToFeatureLevel(D3D_FEATURE_LEVEL fl)
    {
        switch (fl)
        {
        case D3D_FEATURE_LEVEL_12_1: return kfe::EDeviceFeatureLevel::FL_12_1; break;
        case D3D_FEATURE_LEVEL_12_0: return kfe::EDeviceFeatureLevel::FL_12_0; break;
        case D3D_FEATURE_LEVEL_11_1: return kfe::EDeviceFeatureLevel::FL_11_1; break;
        case D3D_FEATURE_LEVEL_11_0: return kfe::EDeviceFeatureLevel::FL_11_0; break;
        default:                     return kfe::EDeviceFeatureLevel::FL_11_0; break;
        }
    }

    inline std::string ToString(kfe::EDeviceFeatureLevel fl)
    {
        switch (fl)
        {
        case kfe::EDeviceFeatureLevel::FL_11_0: return "D3D_FEATURE_LEVEL_11_0";
        case kfe::EDeviceFeatureLevel::FL_11_1: return "D3D_FEATURE_LEVEL_11_1";
        case kfe::EDeviceFeatureLevel::FL_12_0: return "D3D_FEATURE_LEVEL_12_0";
        case kfe::EDeviceFeatureLevel::FL_12_1: return "D3D_FEATURE_LEVEL_12_1";
        default:                                return "D3D_FEATURE_LEVEL_11_0";
        }
    }

    inline const D3D_FEATURE_LEVEL* GetFallbackFeatureLevels(UINT& count)
    {
        static const D3D_FEATURE_LEVEL levels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };
        count = (UINT)std::size(levels);
        return levels;
    }

    // Debug layer loader
    inline void EnableDebugLayer(bool gpuValidation)
    {
        ComPtr<ID3D12Debug> debugController;

        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            LOG_SUCCESS("Debugger Enabled!");
            if (gpuValidation)
            {
                ComPtr<ID3D12Debug1> debug1;
                if (SUCCEEDED(debugController.As(&debug1)))
                {
                    debug1->SetEnableGPUBasedValidation(TRUE);
                    LOG_WARNING("GPU Based Validation: Enabled!");
                }
                else LOG_WARNING("GPU Based Validation: Failed!");
            }
        }else LOG_WARNING("GPU Debugger: Failed!");
    }

    inline void Log(const char* msg)
    {
        OutputDebugStringA(msg);
        OutputDebugStringA("\n");
    }
}

class kfe::KFEDevice::Impl
{
public:
	 Impl() = default;
	~Impl() = default;

	bool Initialize(const KFE_DEVICE_CREATE_DESC& desc);
	bool Release();

	ID3D12Device*		GetNative		   () const;
	EDeviceFeatureLevel GetFeatureLevel	   () const noexcept;
	bool                IsDebugLayerEnabled() const noexcept;
	bool                IsValid			   () const noexcept;

    void LogDevice() const noexcept;

private:
    //~ helpers
    bool CreateDebugLayerIfRequested    (const KFE_DEVICE_CREATE_DESC& desc);
    bool CreateDevice                   (const KFE_DEVICE_CREATE_DESC& desc);
    bool TryCreateDeviceWithFeatureLevel(IDXGIAdapter4* adapter,
                                         D3D_FEATURE_LEVEL fl);
private:
    ComPtr<ID3D12Device> m_pDevice{ nullptr };
    EDeviceFeatureLevel  m_eFeatureLevel{};

#if defined(DEBUG) || defined(_DEBUG)
    bool m_bDebuggerEnabled{ true };
#else
    bool m_bDebuggerEnabled{ false };
#endif
};

kfe::KFEDevice::KFEDevice()
	: m_impl(std::make_unique<kfe::KFEDevice::Impl>())
{}

kfe::KFEDevice::~KFEDevice() = default;

_Use_decl_annotations_
bool kfe::KFEDevice::Initialize(const KFE_DEVICE_CREATE_DESC& desc)
{
	return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFEDevice::Release()
{
	return m_impl->Release();
}

_Use_decl_annotations_
ID3D12Device* kfe::KFEDevice::GetNative() const
{
	return m_impl->GetNative();
}

_Use_decl_annotations_
kfe::EDeviceFeatureLevel kfe::KFEDevice::GetFeatureLevel() const noexcept
{
    return m_impl->GetFeatureLevel();
}

_Use_decl_annotations_
bool kfe::KFEDevice::IsDebugLayerEnabled() const noexcept
{
    return m_impl->IsDebugLayerEnabled();
}

_Use_decl_annotations_
bool kfe::KFEDevice::IsValid() const noexcept
{
    return m_impl->IsValid();
}

_Use_decl_annotations_
UINT kfe::KFEDevice::GetRTVDescriptorSize() const noexcept
{
    return GetNative()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

_Use_decl_annotations_
UINT kfe::KFEDevice::GetDSVDescriptorSize() const noexcept
{
    return GetNative()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

_Use_decl_annotations_
UINT kfe::KFEDevice::GetCRUDescriptorSize() const noexcept
{
    return GetNative()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void kfe::KFEDevice::LogDevice() const noexcept
{
    m_impl->LogDevice();
}

//~ Impl Implementation
bool kfe::KFEDevice::Impl::Initialize(const KFE_DEVICE_CREATE_DESC& desc)
{
    if (!Release())
    {
        LOG_ERROR("Failed to Release DirectX::Device before recreating it");
    }

    if (!CreateDebugLayerIfRequested(desc))
    {
        LOG_ERROR("Failed to create d3d12 debugger!");
        return false;
    }

    if (!CreateDevice(desc))
    {
        LOG_ERROR("Failed to create d3d12 device!");
        return false;
    }

    if (desc.debugName && m_pDevice) 
    {
        auto wname = kfe_helpers::AnsiToWide(desc.debugName);
        m_pDevice->SetName(wname.c_str());
    }

    LOG_SUCCESS("D3D12 Device Created Sucessfully!");

    return true;
}

bool kfe::KFEDevice::Impl::Release()
{
    SAFE_RELEASE(m_pDevice);
    m_bDebuggerEnabled = false;
    return true;
}

ID3D12Device* kfe::KFEDevice::Impl::GetNative() const
{
    return m_pDevice.Get();
}

kfe::EDeviceFeatureLevel kfe::KFEDevice::Impl::GetFeatureLevel() const noexcept
{
    return m_eFeatureLevel;
}

bool kfe::KFEDevice::Impl::IsDebugLayerEnabled() const noexcept
{
    return m_bDebuggerEnabled;
}

bool kfe::KFEDevice::Impl::IsValid() const noexcept
{
    return m_pDevice.Get() != nullptr;
}

bool kfe::KFEDevice::Impl::CreateDebugLayerIfRequested(
    const KFE_DEVICE_CREATE_DESC& desc)
{
    bool enableLayer =
        (desc.Flags & EDeviceCreateFlags::EnableDebugLayer) == EDeviceCreateFlags::EnableDebugLayer;

    bool enableGPUValidation =
        (desc.Flags & EDeviceCreateFlags::EnableGPUBasedValidation) == EDeviceCreateFlags::EnableGPUBasedValidation;

    if (!enableLayer)
    {
        return true;
    }

    EnableDebugLayer(enableGPUValidation);
    m_bDebuggerEnabled = true;

    Log("[KFEDevice] Debug layer enabled.");

    return true;
}

bool kfe::KFEDevice::Impl::CreateDevice(
    const KFE_DEVICE_CREATE_DESC& desc)
{
    IDXGIAdapter4* adapter = desc.Adapter->GetNative();
    if (!adapter)
    {
        LOG_ERROR("KFEDevice::Impl::CreateDevice No Valid Adapter found!");
    }

    bool enableStablePower =
        (desc.Flags & EDeviceCreateFlags::EnableStablePowerState) == EDeviceCreateFlags::EnableStablePowerState;

    const D3D_FEATURE_LEVEL fl = ToD3DFeatureLevel(desc.MinimumFeatureLevel);

    if (TryCreateDeviceWithFeatureLevel(adapter, fl))
    {
        m_eFeatureLevel = desc.MinimumFeatureLevel;
        LOG_SUCCESS("Created D3D12 Device with requested configuration: {}", ToString(m_eFeatureLevel));
        
        if (enableStablePower)
        {
            LOG_WARNING("GPU Stable Power State: Enabled!");
            m_pDevice->SetStablePowerState(TRUE);
        }
        
        return true;
    }

    //~ incase failed: fallback
    LOG_WARNING("Failed to Create D3D12 Device: Fallback on lower feature level");
    UINT count                            = 0u;
    const D3D_FEATURE_LEVEL* fallbackList = GetFallbackFeatureLevels(count);

    for (UINT i = 0u; i < count; i++)
    {
        if (fallbackList[i] == fl) continue;

        if (TryCreateDeviceWithFeatureLevel(adapter, fallbackList[i]))
        {
            m_eFeatureLevel = ToFeatureLevel(fallbackList[i]);
            LOG_SUCCESS("Created D3D12 Device with fallback configuration: {}", ToString(m_eFeatureLevel));
            
            if (enableStablePower)
            {
                LOG_WARNING("GPU Stable Power State: Enabled!");
                m_pDevice->SetStablePowerState(TRUE);
            }
            return true;
        }
        else
        {
            LOG_WARNING("Failed With: {} Fallback applied even lower", ToString(m_eFeatureLevel));
        }
    }

    LOG_ERROR("KFEDevice::Impl::CreateDevice All fallback failed, Failed to create device!");
    return false;
}

bool kfe::KFEDevice::Impl::TryCreateDeviceWithFeatureLevel(
    IDXGIAdapter4* adapter,
    D3D_FEATURE_LEVEL fl)
{
    const HRESULT hr = D3D12CreateDevice(adapter, fl, IID_PPV_ARGS(&m_pDevice));

    if (SUCCEEDED(hr)) return true;
    
    return false;
}

void kfe::KFEDevice::Impl::LogDevice() const noexcept
{
    if (!m_pDevice)
    {
        LOG_WARNING("KFEDevice::LogDevice called but device is NULL.");
        return;
    }

    LOG_INFO("KFEDevice Information");
    LOG_INFO("Feature Level      : {}", ToString(m_eFeatureLevel));
    LOG_INFO("Debug Layer        : {}", m_bDebuggerEnabled ? "Enabled" : "Disabled");

    UINT nodeCount = m_pDevice->GetNodeCount();
    LOG_INFO("Node Count         : {}", nodeCount);

    // Architecture
    D3D12_FEATURE_DATA_ARCHITECTURE1 arch = {};
    if (SUCCEEDED(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE1, &arch, sizeof(arch))))
    {
        LOG_INFO("UMA                : {}", arch.UMA ? "Yes" : "No");
        LOG_INFO("Cache Coherent UMA : {}", arch.CacheCoherentUMA ? "Yes" : "No");
    }
    else
    {
        LOG_WARNING("Unable to query architecture information.");
    }

    // Shader Model
    D3D12_FEATURE_DATA_SHADER_MODEL sm = { D3D_SHADER_MODEL_6_6 };
    if (SUCCEEDED(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm))))
    {
        LOG_INFO("Shader Model       : SM {}", static_cast<int>(sm.HighestShaderModel));
    }
    else
    {
        LOG_WARNING("Unable to query shader model support.");
    }
}
