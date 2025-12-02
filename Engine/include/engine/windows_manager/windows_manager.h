#pragma once

// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com


/*
 *  -----------------------------------------------------------------------------
 *  Project   : PixelFox (WMG Warwick - Module 2)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */

#pragma once

#include "EngineAPI.h"

#include "engine/core/interface/interface_frame.h"
#include "engine/core/common_types.h"

#include <windows.h>
#include <sal.h>
#include <memory>

namespace kfe
{
	typedef struct _KFE_WINDOW_CREATE_DESC
	{
		_In_					 std::string  WindowTitle{ "KnightEngine" };
		_Field_range_(100, 1920) UINT		  Width		 { 800u };
		_Field_range_(100, 1080) UINT		  Height	 { 800u };
		_Field_range_(0, 200)    UINT		  IconId	 { 0u };
		_In_					 EScreenState ScreenState{ EScreenState::Windowed };
	} KFE_WINDOW_CREATE_DESC;

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
		std::shared_ptr<Impl>		GetImpl		()		 { return m_impl; }
		std::shared_ptr<const Impl> GetConstImpl() const { return m_impl; }
		
		std::shared_ptr<Impl> m_impl{ nullptr };
	};
} // namespace kfe
