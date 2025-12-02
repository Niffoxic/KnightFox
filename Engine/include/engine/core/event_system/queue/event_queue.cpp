#include "pch.h"
#include "event_queue.h"

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
