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
#include "engine/system/timer.h"

kfe::KFETimer::KFETimer()
{
    ResetTime();
}

void kfe::KFETimer::ResetTime()
{
    const TimeClockPoint current = TimerClock::now();

    m_timeStart.store(current, std::memory_order_seq_cst);
    m_timeLastTick.store(current, std::memory_order_seq_cst);
}

_Use_decl_annotations_
float kfe::KFETimer::Tick()
{
    const TimeClockPoint current = TimerClock::now();

    const TimeClockPoint last = m_timeLastTick.load(std::memory_order_acquire);
    const std::chrono::duration<float> delta = current - last;

    m_timeLastTick.store(current, std::memory_order_release);

    return delta.count();
}

_Use_decl_annotations_
float kfe::KFETimer::TimeElapsed() const
{
    const TimeClockPoint start = m_timeStart.load(std::memory_order_acquire);
    const TimeClockPoint current = TimerClock::now();

    return std::chrono::duration<float>(current - start).count();
}

_Use_decl_annotations_
float kfe::KFETimer::DeltaTime() const
{
    const TimeClockPoint last = m_timeLastTick.load(std::memory_order_acquire);
    const TimeClockPoint current = TimerClock::now();

    return std::chrono::duration<float>(current - last).count();
}
