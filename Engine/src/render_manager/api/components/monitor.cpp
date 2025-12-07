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
#include "engine/render_manager/api/components/monitor.h"
#include "engine/render_manager/api/components/factory.h"
#include "engine/render_manager/api/components/adapter.h"

#include "engine/utils/helpers.h"
#include "engine/utils/logger.h"

#include <shellscalingapi.h>

//~ Impl Declarations
class kfe::KFEMonitor::Impl
{
public:
     Impl() = default;
    ~Impl() = default;

    [[nodiscard]] bool Initialize(const KFEAdapter* adapter);

    void LogOutputs() const noexcept;

    std::vector<MonitorInfo>         Monitors           {};
    MonitorInfo                      PrimaryMonitor     {};
    std::unordered_map<UINT, size_t> OutputToMonitorMap {};

    UINT PrimaryOutputIndex { UINT_MAX };
    UINT SelectedOutputIndex{ UINT_MAX };

private:
    //~ Helpers
    float ComputeRefreshHz         (const DXGI_RATIONAL r) const noexcept;
    void ResetState                ();
    bool EnumerateOutputsForAdapter(IDXGIAdapter4* nativeAdapter);
    bool ProcessSingleOutput       (IDXGIOutput* output, UINT outputIndex);
    
    void FillBasicInfo(
        MonitorInfo&             info,
        bool                     hasDesc1,
        const DXGI_OUTPUT_DESC1& desc1,
        const DXGI_OUTPUT_DESC&  desc,
        UINT                     outputIndex);
   
    void FillDisplayModes(
        MonitorInfo& info,
        IDXGIOutput* output);
    
    void FillDpiInfo(MonitorInfo& info);
    
    void FillHdrInfo(
        MonitorInfo&             info,
        IDXGIOutput6*            output6,
        bool                     hasDesc1,
        const DXGI_OUTPUT_DESC1& desc1);
    
    void FinalizeSelection();
};

kfe::KFEMonitor::KFEMonitor()
    : m_impl(std::make_unique<kfe::KFEMonitor::Impl>())
{}

kfe::KFEMonitor::~KFEMonitor() = default;

_Use_decl_annotations_
bool kfe::KFEMonitor::Initialize(const KFEAdapter* adapter)
{
	return m_impl->Initialize(adapter);
}

_Use_decl_annotations_
const std::vector<kfe::MonitorInfo>& kfe::KFEMonitor::GetMonitors() const noexcept
{
    return m_impl->Monitors;
}

_Use_decl_annotations_
const kfe::MonitorInfo* kfe::KFEMonitor::GetPrimaryMonitor() const noexcept
{
    if (m_impl->PrimaryOutputIndex == UINT_MAX) return nullptr;
	return &m_impl->Monitors.at(m_impl->PrimaryOutputIndex);
}

_Use_decl_annotations_
const kfe::MonitorInfo* kfe::KFEMonitor::GetMonitorByIndex(uint32_t outputIndex) const noexcept
{
    if (m_impl->OutputToMonitorMap.contains(outputIndex))
    {
        auto index = m_impl->OutputToMonitorMap.at(outputIndex);
        return &m_impl->Monitors.at(index);
    }
    return nullptr;
}

void kfe::KFEMonitor::LogOutputs() const noexcept
{
    m_impl->LogOutputs();
}

//~ imp implementation
bool kfe::KFEMonitor::Impl::Initialize(const KFEAdapter* adapter)
{
    if (!adapter) return false;

    ResetState();

    if (EnumerateOutputsForAdapter(adapter->GetNative()))
    {
        FinalizeSelection();
        return true;
    }

    const auto  allNative     = adapter->GetAllNative();
    const size_t adapterCount = allNative.size();

    for (size_t i = 0; i < adapterCount; ++i)
    {
        AdapterInfo info = adapter->GetAdapterInfo(static_cast<UINT>(i));
        if (info.AdapterType == EAdapterType::Integrated)
        {
            if (EnumerateOutputsForAdapter(allNative[i]))
            {
                FinalizeSelection();
                return true;
            }
        }
    }

    FinalizeSelection();
    return !Monitors.empty();
}

void kfe::KFEMonitor::Impl::LogOutputs() const noexcept
{
    if (Monitors.empty())
    {
        LOG_WARNING("[KFEMonitor] No outputs (monitors) found for this adapter.");
        return;
    }

    LOG_INFO("DXGI OUTPUTS (MONITORS)");
    LOG_INFO("Total Outputs: {}", static_cast<uint32_t>(Monitors.size()));

    for (const auto& info : Monitors)
    {
        const bool isPrimary = (PrimaryOutputIndex != UINT_MAX && info.OutputIndex == PrimaryOutputIndex) ||
            (PrimaryOutputIndex == UINT_MAX && info.IsPrimary);
        const bool isSelected = (SelectedOutputIndex != UINT_MAX && info.OutputIndex == SelectedOutputIndex);

        // Rotation string
        const char* rotationStr = "Identity";
        switch (info.Rotation)
        {
        case DXGI_MODE_ROTATION_IDENTITY:  rotationStr = "Identity";    break;
        case DXGI_MODE_ROTATION_ROTATE90:  rotationStr = "Rotate 90";   break;
        case DXGI_MODE_ROTATION_ROTATE180: rotationStr = "Rotate 180";  break;
        case DXGI_MODE_ROTATION_ROTATE270: rotationStr = "Rotate 270";  break;
        default:                           rotationStr = "Unknown";     break;
        }

        // Color space string
        const char* colorSpaceStr = "Unknown";
        switch (info.ColorSpace)
        {
        case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:         colorSpaceStr = "sRGB (Rec.709)";          break;
        case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:      colorSpaceStr = "HDR10 (PQ, Rec.2020)";    break;
        case DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709:       colorSpaceStr = "Studio sRGB";             break;
        case DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020:    colorSpaceStr = "Studio HDR10";            break;
        case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:         colorSpaceStr = "scRGB (linear)";          break;
        default:                                              colorSpaceStr = "Other/Unknown";           break;
        }

        LOG_INFO("--------------------------------------------");
        LOG_INFO("Output Index   : {}", info.OutputIndex);
        LOG_INFO("Name           : {}", info.DeviceName);
        LOG_INFO("Desktop Rect   : L={} T={} R={} B={}",
            info.DesktopRect.left, info.DesktopRect.top,
            info.DesktopRect.right, info.DesktopRect.bottom);
        LOG_INFO("Primary        : {}", isPrimary ? "Yes" : "No");
        LOG_INFO("Selected       : {}", isSelected ? "Yes" : "No");

        LOG_INFO("Current Mode   : {}x{} @ {:.2f} Hz (Format={})",
            info.CurrentWidth,
            info.CurrentHeight,
            info.CurrentRefreshHz,
            static_cast<uint32_t>(info.CurrentFormat));

        LOG_INFO("Rotation       : {}", rotationStr);

        LOG_INFO("DPI            : {} x {}  (Scale: {:.2f} x {:.2f})",
            info.DpiX, info.DpiY, info.ScaleX, info.ScaleY);

        LOG_INFO("HDR Capable    : {}", info.IsHDRCapable ? "Yes" : "No");
        LOG_INFO("Color Space    : {} (raw={} , BitsPerColor={})",
            colorSpaceStr,
            static_cast<uint32_t>(info.ColorSpace),
            info.BitsPerColor);

        LOG_INFO("Supported Modes: {}", static_cast<uint32_t>(info.SupportedModes.size()));

        // only top fews
        const size_t maxModesToLog = 5;
        size_t count = 0;
        for (const auto& mode : info.SupportedModes)
        {
            if (count++ >= maxModesToLog)
            {
                LOG_INFO("    ... ({} more modes omitted)",
                    static_cast<uint32_t>(info.SupportedModes.size() - maxModesToLog));
                break;
            }

            LOG_INFO("    Mode {:2}: {}x{} @ {:.2f} Hz (Format={})",
                static_cast<uint32_t>(count),
                mode.Width,
                mode.Height,
                mode.RefreshHz,
                static_cast<uint32_t>(mode.Format));
        }
    }
}

float kfe::KFEMonitor::Impl::ComputeRefreshHz(const DXGI_RATIONAL r) const noexcept
{
    if (r.Denominator == 0)
        return 0.0f;

    return  static_cast<float>(r.Numerator) /
            static_cast<float>(r.Denominator);
}

void kfe::KFEMonitor::Impl::ResetState()
{
    Monitors          .clear();
    OutputToMonitorMap.clear();
    PrimaryOutputIndex  = UINT_MAX;
    SelectedOutputIndex = UINT_MAX;
}

bool kfe::KFEMonitor::Impl::EnumerateOutputsForAdapter(
    IDXGIAdapter4* nativeAdapter)
{
    if (!nativeAdapter)
        return false;

    UINT outputIndex = 0;
    for (;; ++outputIndex)
    {
        Microsoft::WRL::ComPtr<IDXGIOutput> output;
        HRESULT hr = nativeAdapter->EnumOutputs(outputIndex, &output);

        if (hr == DXGI_ERROR_NOT_FOUND)
            break;

        if (FAILED(hr) || !output)
            return false;

        if (!ProcessSingleOutput(output.Get(), outputIndex))
            return false;
    }

    return !Monitors.empty();
}

bool kfe::KFEMonitor::Impl::ProcessSingleOutput(
    IDXGIOutput* output,
    UINT outputIndex)
{
    Microsoft::WRL::ComPtr<IDXGIOutput6> output6;
    output->QueryInterface(IID_PPV_ARGS(&output6));

    DXGI_OUTPUT_DESC1 desc1{};
    DXGI_OUTPUT_DESC  desc {};
    bool hasDesc1 = false;

    if (output6 && SUCCEEDED(output6->GetDesc1(&desc1)))
    {
        hasDesc1 = true;
    }
    else
    {
        if (FAILED(output->GetDesc(&desc)))
            return false;
    }

    MonitorInfo info{};
    FillBasicInfo   (info, hasDesc1, desc1, desc, outputIndex);
    FillDisplayModes(info, output);
    FillDpiInfo     (info);
    FillHdrInfo     (info, output6.Get(), hasDesc1, desc1);

    const size_t monitorIdx = Monitors.size();
    OutputToMonitorMap[info.OutputIndex] = monitorIdx;
    Monitors.push_back(info);

    if (info.IsPrimary && PrimaryOutputIndex == UINT_MAX)
    {
        PrimaryOutputIndex = info.OutputIndex;
    }

    return true;
}

void kfe::KFEMonitor::Impl::FillBasicInfo(
    MonitorInfo& info,
    bool hasDesc1,
    const DXGI_OUTPUT_DESC1& desc1,
    const DXGI_OUTPUT_DESC& desc,
    UINT outputIndex)
{
    std::string device     = kfe_helpers::WideToAnsi(desc1.DeviceName);
    std::string deviceBase = kfe_helpers::WideToAnsi(desc.DeviceName);
    info.OutputIndex       = outputIndex;
    info.MonitorHandle     = hasDesc1 ? desc1.Monitor : desc.Monitor;
    info.DeviceName        = hasDesc1 ? device : deviceBase;
    info.DesktopRect       = hasDesc1 ? desc1.DesktopCoordinates : desc.DesktopCoordinates;
    info.Rotation          = hasDesc1 ? desc1.Rotation : DXGI_MODE_ROTATION_IDENTITY;

    info.IsPrimary = (info.DesktopRect.left == 0 && info.DesktopRect.top == 0);
}

void kfe::KFEMonitor::Impl::FillDisplayModes(
    MonitorInfo& info,
    IDXGIOutput* output)
{
    UINT        modeCount = 0;
    DXGI_FORMAT preferredFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    HRESULT hr = output->GetDisplayModeList(
        preferredFormat,
        DXGI_ENUM_MODES_INTERLACED | DXGI_ENUM_MODES_SCALING,
        &modeCount, nullptr);

    if (FAILED(hr) || modeCount == 0)
        return;

    std::vector<DXGI_MODE_DESC> modeList(modeCount);

    hr = output->GetDisplayModeList(
        preferredFormat,
        DXGI_ENUM_MODES_INTERLACED | DXGI_ENUM_MODES_SCALING,
        &modeCount, modeList.data());

    if (FAILED(hr))
        return;

    info.SupportedModes.reserve(modeCount);

    for (const auto& m : modeList)
    {
        MonitorInfo::DisplayMode dm{};
        dm.Width     = m.Width;
        dm.Height    = m.Height;
        dm.Format    = m.Format;
        dm.RefreshHz = ComputeRefreshHz(m.RefreshRate);

        info.SupportedModes.push_back(dm);
    }

    if (!info.SupportedModes.empty())
    {
        const auto& last      = info.SupportedModes.back();
        info.CurrentWidth     = last.Width;
        info.CurrentHeight    = last.Height;
        info.CurrentFormat    = last.Format;
        info.CurrentRefreshHz = last.RefreshHz;
    }
}

void kfe::KFEMonitor::Impl::FillDpiInfo(MonitorInfo& info)
{
    UINT dpiX = 96, dpiY = 96;

    if (info.MonitorHandle)
    {
        GetDpiForMonitor(info.MonitorHandle, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
    }

    info.DpiX   = dpiX;
    info.DpiY   = dpiY;
    info.ScaleX = dpiX / 96.0f;
    info.ScaleY = dpiY / 96.0f;
}

void kfe::KFEMonitor::Impl::FillHdrInfo(
    MonitorInfo& info,
    IDXGIOutput6* output6,
    bool hasDesc1,
    const DXGI_OUTPUT_DESC1& desc1)
{
    if (!output6 || !hasDesc1)
        return;

    info.BitsPerColor = desc1.BitsPerColor;
    info.ColorSpace = desc1.ColorSpace;

    info.IsHDRCapable =
        (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) ||
        (desc1.BitsPerColor >= 10);
}

void kfe::KFEMonitor::Impl::FinalizeSelection()
{
    if (!Monitors.empty() && PrimaryOutputIndex == UINT_MAX)
    {
        PrimaryOutputIndex = Monitors.front().OutputIndex;
    }

    if (!Monitors.empty() && SelectedOutputIndex == UINT_MAX)
    {
        SelectedOutputIndex = PrimaryOutputIndex;
    }
}
