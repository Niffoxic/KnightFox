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
#include <vector>
#include <cstdint>
#include <string>
#include <dxgi1_6.h>

namespace kfe
{
	class KFEAdapter;

    struct MonitorInfo
    {
        UINT         OutputIndex{ 0u };
        std::string  DeviceName {};
        RECT         DesktopRect{};
        bool         IsPrimary  { false };

        // what Windows is using right now
        UINT        CurrentWidth    { 0u };
        UINT        CurrentHeight   { 0u };
        DXGI_FORMAT CurrentFormat   { DXGI_FORMAT_UNKNOWN };
        float       CurrentRefreshHz{ 0.0f };

        // All supported modes
        struct DisplayMode
        {
            UINT        Width    { 0u };
            UINT        Height   { 0u };
            float       RefreshHz{ 0.0f };
            DXGI_FORMAT Format   { DXGI_FORMAT_UNKNOWN };
        };

        std::vector<DisplayMode> SupportedModes{};

        DXGI_MODE_ROTATION    Rotation    { DXGI_MODE_ROTATION_IDENTITY };
        bool                  IsHDRCapable{ false };
        DXGI_COLOR_SPACE_TYPE ColorSpace  { DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709 };
        UINT                  BitsPerColor{ 8u };

        // DPI
        HMONITOR MonitorHandle{ nullptr };
        UINT     DpiX         { 96u };
        UINT     DpiY         { 96u };
        float    ScaleX       { 1.0f };
        float    ScaleY       { 1.0f };
    };

	class KFE_API KFEMonitor
	{
	public:
		 KFEMonitor();
		~KFEMonitor();

		KFEMonitor			 (const KFEMonitor&) = delete;
		KFEMonitor& operator=(const KFEMonitor&) = delete;

        NODISCARD bool Initialize(_In_ const KFEAdapter* adapter);

		// Accessors
		NODISCARD const std::vector<MonitorInfo>& GetMonitors	   ()					  const noexcept;
        NODISCARD const MonitorInfo*			  GetPrimaryMonitor()					  const noexcept;
        NODISCARD const MonitorInfo*			  GetMonitorByIndex(uint32_t outputIndex) const noexcept;

		//~ for debugging
		void LogOutputs() const noexcept;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
