#pragma once

#include "EngineAPI.h"

#include <atomic>
#include <cstdint>

namespace kfe
{
    class KFE_API KeyGenerator
    {
    public:
        using key_type = std::uint64_t;

        [[nodiscard]] static key_type Next() noexcept
        {
            return s_counter.fetch_add(1u, std::memory_order_relaxed) + 1u;
        }

        static void Reset(key_type start = 0u) noexcept
        {
            s_counter.store(start, std::memory_order_relaxed);
        }

    private:
        static std::atomic<key_type> s_counter;
    };
}

#define KEYGEN_CLASS()                                          \
private:                                                        \
    ::kfe::KeyGenerator::key_type m_AssignedKey_{ 0u };         \
public:                                                         \
    void AssignKey() {                                          \
        if (!m_AssignedKey_) {                                  \
            m_AssignedKey_ = ::kfe::KeyGenerator::Next();       \
        }                                                       \
    }                                                           \
    [[nodiscard]] std::uint64_t GetAssignedKey()                \
        const noexcept {                                        \
        return static_cast<std::uint64_t>(m_AssignedKey_);      \
    }                                                           \
    bool HasAssignedKey() const noexcept                        \
    { return m_AssignedKey_ != 0; }
