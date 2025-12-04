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

#include "engine/system/interface/interface_manager.h"
#include "engine/system/common_types.h"

#include <windows.h>
#include <sal.h>
#include <memory>

namespace kfe
{
	enum class EProcessedMessage: bool
	{
		Alive       = 0,
		QuitInvoked = 1,
	};

	class KFE_API KFEWindows final : public IManager
	{
	public:
		 KFEWindows();
		 KFEWindows(_In_ const KFE_WINDOW_CREATE_DESC& desc);
		~KFEWindows() override;

		KFEWindows(_In_ const KFEWindows&) = delete;
		KFEWindows(_Inout_ KFEWindows&&)   = delete;

		KFEWindows& operator=(_In_ const KFEWindows&) = delete;
		KFEWindows& operator=(_Inout_ KFEWindows&&)   = delete;

		_NODISCARD _Check_return_ 
		EProcessedMessage ProcessMessage() const noexcept;

		//~ IManager Interface Implementation
		_NODISCARD _Check_return_ bool Initialize() override;
		_NODISCARD _Check_return_ bool Release   () override;

		void OnFrameBegin(_In_ float deltaTime) override;
		void OnFrameEnd  () override;

		std::string GetName() const noexcept override { return "WindowsManager"; }

		//~ Queries
		_NODISCARD _Ret_maybenull_ HWND		 GetWindowsHandle  () const;
		_NODISCARD _Ret_maybenull_ HINSTANCE GetWindowsInstance() const;

		_NODISCARD _Check_return_ __forceinline
		bool IsFullScreen() const;

		void SetScreenState(_In_ EScreenState state);

		_NODISCARD _Check_return_ KFE_WinSizeU GetWinSize() const;

		void SetWindowTitle			(_In_ const std::string& title);
		void SetWindowMessageOnTitle(_In_ const std::string& message) const;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl{ nullptr };
	};
} // namespace kfe
