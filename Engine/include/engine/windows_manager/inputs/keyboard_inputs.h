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

#include <Windows.h>
#include <initializer_list>
#include <cstdint>
#include <algorithm>


namespace kfe
{
	static constexpr unsigned int MAX_KEYBOARD_INPUTS = 256u;
#define _FOX_VK_VALID _In_range_(0, MAX_KEYBOARD_INPUTS - 1) _Valid_

	enum KFE_API KFEKeyboardMode : uint8_t
	{
		None	= 0,
		Ctrl	= 1,
		Shift	= 1 << 1,
		Alt		= 1 << 2,
		Super	= 1 << 3
	};

	class KFE_API KFEKeyboardInput
	{
	public:
		 KFEKeyboardInput() noexcept;
		~KFEKeyboardInput() noexcept = default;

		//~ No Copy or Move
		KFEKeyboardInput(_In_ const KFEKeyboardInput&) noexcept = delete;
		KFEKeyboardInput(_Inout_ KFEKeyboardInput&&)   noexcept = delete;

		KFEKeyboardInput& operator=(_In_ const KFEKeyboardInput&) noexcept = delete;
		KFEKeyboardInput& operator=(_Inout_ KFEKeyboardInput&&)   noexcept = delete;

		//~ Inherited via IInputHandler
		NODISCARD std::string GetObjectName() const;

		NODISCARD bool Initialize();
		NODISCARD bool Release   ();

		NODISCARD bool ProcessMessage(
			_In_ UINT message,
			_In_ WPARAM wParam,
			_In_ LPARAM lParam) noexcept;

		void OnFrameBegin(_In_ float deltaTime) noexcept;
		void OnFrameEnd()				   noexcept;

		//~ Queries
		NODISCARD bool IsKeyPressed  (_FOX_VK_VALID int virtualKey) const noexcept;
		NODISCARD bool WasKeyPressed (_FOX_VK_VALID int virtualKey) const noexcept;
		NODISCARD bool WasKeyReleased(_FOX_VK_VALID int virtualKey) const noexcept;

		NODISCARD bool WasChordPressed(_FOX_VK_VALID int key,
			_In_ const kfe::KFEKeyboardMode& mode = kfe::KFEKeyboardMode::None)
			const noexcept;

		NODISCARD bool WasMultipleKeyPressed(
			_In_reads_(keys.size()) std::initializer_list<int> keys) const noexcept;

	private:

		//~ Internal Helpers
		void ClearAll() noexcept;
		NODISCARD bool IsSetAutoRepeat(_In_ LPARAM lParam) noexcept;

		NODISCARD bool IsCtrlPressed () const noexcept;
		NODISCARD bool IsShiftPressed() const noexcept;
		NODISCARD bool IsAltPressed  () const noexcept;
		NODISCARD bool IsSuperPressed() const noexcept;

		NODISCARD bool IsInside(_FOX_VK_VALID int virtualKey) const noexcept { return virtualKey >= 0 && virtualKey < MAX_KEYBOARD_INPUTS; }

	private:
		bool m_keyDown[MAX_KEYBOARD_INPUTS];
		bool m_keyPressed[MAX_KEYBOARD_INPUTS];
		bool m_keyReleased[MAX_KEYBOARD_INPUTS];
	};
}
