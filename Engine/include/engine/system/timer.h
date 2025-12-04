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

#include <chrono>
#include <atomic>

namespace kfe
{
	class KFE_API KFETimer
	{
	public:
		using TimerClock	 = std::chrono::steady_clock;
		using TimeClockPoint = TimerClock::time_point;

		KFETimer();

		KFETimer(_In_ const KFETimer&) = delete;
		KFETimer(_Inout_ KFETimer&&)   = delete;

		KFETimer& operator=(_In_ const KFETimer&) = delete;
		KFETimer& operator=(_Inout_ KFETimer&&)	  = delete;

		//~ KFETimer features
		void ResetTime();

		// returns delta time in seconds
		NODISCARD float Tick	   ();
		NODISCARD float TimeElapsed() const;
		NODISCARD float DeltaTime  () const;
	private:
		std::atomic<TimeClockPoint> m_timeStart;
		std::atomic<TimeClockPoint> m_timeLastTick;
	};
} // namespace kfe
