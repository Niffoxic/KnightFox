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
#include "engine/system/key_generator.h"

namespace kfe
{
    std::atomic<KID> KeyGenerator::s_counter{ 10u };

    KID KeyGenerator::Next() noexcept
    {
        return s_counter.fetch_add(1u, std::memory_order_relaxed) + 1u;
    }

    void KeyGenerator::Reset(KID start) noexcept
    {
        s_counter.store(start, std::memory_order_relaxed);
    }

    bool KeyGenerator::IsValid(KID id) noexcept
    {
        return id > 0u;
    }
}
