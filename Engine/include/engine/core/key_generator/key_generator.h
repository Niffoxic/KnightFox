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

        [[nodiscard]] static KID Next() noexcept
        {
            return s_counter.fetch_add(1u, std::memory_order_relaxed) + 1u;
        }

        static void Reset(KID start = 10u) noexcept
        {
            s_counter.store(start, std::memory_order_relaxed);
        }

        [[nodiscard]] static bool IsValid(KID id) noexcept
        {
            return id > 0u;
        }

    private:
        static std::atomic<KID> s_counter;
    };
}
