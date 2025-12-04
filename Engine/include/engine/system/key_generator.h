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

#include <atomic>
#include <cstdint>

namespace kfe
{
    using KID = std::uint64_t;

    class KFE_API KeyGenerator
    {
    public:
        static KID  Next   ()               noexcept;
        static bool IsValid(KID id)          noexcept;
        static void Reset  (KID start = 10u) noexcept;

    private:
        static std::atomic<KID> s_counter;
    };
}
