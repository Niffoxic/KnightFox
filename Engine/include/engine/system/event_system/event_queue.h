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

#include "engine/core.h"

#include <typeindex>
#include <functional>
#include <cstdint>
#include <atomic>

#include <unordered_map>
#include <vector>

namespace kfe
{
    template<typename EventT>
    struct Channel
    {
        using Callback = std::function<void(_In_ const EventT&)>;

        inline static std::vector<Callback> Subscribers{};
        inline static std::vector<EventT>   Queue{};
    };

    using EventToken = std::uint64_t;

    class KFE_API EventQueue
    {
        struct TypeOps
        {
            void (*dispatch)();                
            void (*clear)();                   
            bool (*unsubscribe)(_In_ std::size_t);
        };

        struct SubscriptionInfo
        {
            std::type_index type;
            std::size_t     index;
        };

    public:
        // Subscribe to a specific EventT
        template<typename EventT>
        _Ret_valid_
        static EventToken Subscribe(_In_ typename Channel<EventT>::Callback cb)
        {
            RegisterIfNeeded<EventT>();

            auto& subs = Channel<EventT>::Subscribers;
            subs.push_back(std::move(cb));

            const std::size_t index = subs.size() - 1;

            auto& nextToken = GetNextToken();
            const EventToken tok = ++nextToken;

            GetSubscriptions().emplace(
                tok,
                SubscriptionInfo{ std::type_index(typeid(EventT)), index }
            );

            return tok;
        }

        // Queue an event for later dispatch
        template<typename EventT>
        static void Post(_In_ const EventT& event)
        {
            RegisterIfNeeded<EventT>();
            Channel<EventT>::Queue.push_back(event);
        }

        // Dispatch every queued event of every registered type
        static void DispatchAll()
        {
            auto& reg = GetTypeRegistry();
            for (const auto& kv : reg)
            {
                kv.second.dispatch();
            }
        }

        // Clear all pending events of every registered type
        static void ClearAll()
        {
            auto& reg = GetTypeRegistry();
            for (const auto& kv : reg)
            {
                kv.second.clear();
            }
        }

        // Unsubscribe using token
        static void Unsubscribe(_In_ EventToken token)
        {
            auto& subs = GetSubscriptions();
            auto  it = subs.find(token);
            if (it == subs.end())
                return;

            const SubscriptionInfo info = it->second;
            subs.erase(it);

            auto& reg = GetTypeRegistry();
            auto  regIt = reg.find(info.type);
            if (regIt != reg.end())
            {
                regIt->second.unsubscribe(info.index);
            }
        }

        // Dispatch only one known event type
        template<typename EventT>
        static void DispatchType()
        {
            auto& queue = Channel<EventT>::Queue;
            auto& subscribers = Channel<EventT>::Subscribers;

            for (std::size_t i = 0; i < queue.size(); ++i)
            {
                const EventT& event = queue[i];
                for (std::size_t s = 0; s < subscribers.size(); ++s)
                {
                    auto& cb = subscribers[s];
                    if (cb)
                        cb(event);
                }
            }

            queue.clear();
        }

    private:
        template<typename EventT>
        static void DispatchThunk()
        {
            DispatchType<EventT>();
        }

        template<typename EventT>
        static void ClearThunk()
        {
            Channel<EventT>::Queue.clear();
        }

        template<typename EventT>
        _Success_(return)
            static bool UnsubThunk(_In_ std::size_t idx)
        {
            auto& subs = Channel<EventT>::Subscribers;
            if (idx >= subs.size())
                return false;

            subs[idx] = nullptr;
            return true;
        }

        template<typename EventT>
        static void RegisterIfNeeded()
        {
            const std::type_index key(typeid(EventT));

            auto& reg = GetTypeRegistry();
            if (reg.contains(key))
                return;

            TypeOps ops
            {
                &DispatchThunk<EventT>,
                &ClearThunk<EventT>,
                &UnsubThunk<EventT>
            };

            reg.emplace(key, ops);
        }

    private:
        static std::unordered_map<std::type_index, TypeOps>&     GetTypeRegistry ();
        static std::unordered_map<EventToken, SubscriptionInfo>& GetSubscriptions();
        static std::atomic<EventToken>&                          GetNextToken    ();
    };

} // namespace kfe
