#include "pch.h"
#include "key_generator.h"

namespace kfe
{
    std::atomic<KID> KeyGenerator::s_counter{ 0u };
}
