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
#include "engine/system/event_system/event_queue.h"

namespace kfe
{
    std::unordered_map<std::type_index, EventQueue::TypeOps>& EventQueue::GetTypeRegistry()
    {
        static std::unordered_map<std::type_index, TypeOps> s_TypeRegistry;
        return s_TypeRegistry;
    }

    std::unordered_map<EventToken, EventQueue::SubscriptionInfo>& EventQueue::GetSubscriptions()
    {
        static std::unordered_map<EventToken, SubscriptionInfo> s_Subscriptions;
        return s_Subscriptions;
    }

    std::atomic<EventToken>& EventQueue::GetNextToken()
    {
        static std::atomic<EventToken> s_NextToken{ 0 };
        return s_NextToken;
    }

} // namespace kfe
