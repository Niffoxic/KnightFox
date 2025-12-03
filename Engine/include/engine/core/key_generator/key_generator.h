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

        static void Reset(KID start = 0u) noexcept
        {
            s_counter.store(start, std::memory_order_relaxed);
        }

    private:
        static std::atomic<KID> s_counter;
    };
}

#define KEYGEN_CLASS()                                          \
private:                                                        \
    ::kfe::KID m_AssignedKey_{ 0u };                            \
public:                                                         \
    void AssignKey() {                                          \
        if (!m_AssignedKey_) {                                  \
            m_AssignedKey_ = ::kfe::KeyGenerator::Next();       \
        }                                                       \
    }                                                           \
    [[nodiscard]] ::kfe::KID GetAssignedKey()                   \
        const noexcept {                                        \
        return m_AssignedKey_;                                  \
    }                                                           \
    bool HasAssignedKey() const noexcept                        \
    { return m_AssignedKey_ != 0; }
