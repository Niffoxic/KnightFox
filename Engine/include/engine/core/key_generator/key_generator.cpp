#include "pch.h"
#include "key_generator.h"

namespace kfe
{
    std::atomic<KeyGenerator::key_type> KeyGenerator::s_counter{ 0u };
}
