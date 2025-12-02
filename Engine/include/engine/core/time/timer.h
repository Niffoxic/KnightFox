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
#include <windows.h> 
#include <chrono>
#include <atomic>
#include <sal.h>

namespace kfe
{
	class KFE_API KFETimer
	{
	public:
		using TimerClock	 = std::chrono::steady_clock;
		using TimeClockPoint = TimerClock::time_point;

		KFETimer();

		KFETimer(_In_ const KFETimer&) = delete;
		KFETimer(_Inout_ KFETimer&&)	 = delete;

		KFETimer& operator=(_In_ const KFETimer&) = delete;
		KFETimer& operator=(_Inout_ KFETimer&&)	= delete;

		//~ KFETimer features
		void ResetTime();

		// returns delta time in seconds
		_NODISCARD _Check_return_ float Tick	   ();
		_NODISCARD _Check_return_ float TimeElapsed() const;
		_NODISCARD _Check_return_ float DeltaTime  () const;
	private:
		std::atomic<TimeClockPoint> m_timeStart;
		std::atomic<TimeClockPoint> m_timeLastTick;
	};
} // namespace kfe
