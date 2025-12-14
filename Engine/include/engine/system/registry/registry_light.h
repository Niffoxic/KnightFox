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

#include "engine/system/interface/interface_light.h"

#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <string>

namespace kfe
{
    class KFE_API RegistryLights final
    {
    public:
        using CreateFunctor = std::function<std::unique_ptr<IKFELight>()>;

        static void Register(const std::string& name, CreateFunctor fn);

        NODISCARD static std::unique_ptr<IKFELight>
            Create(const std::string& name);

        NODISCARD static const std::vector<std::string>&
            GetRegisteredNames();
    };

} // namespace kfe

#define KFE_REGISTER_LIGHT(CLASS_NAME)                                        \
    namespace {                                                                \
        struct CLASS_NAME##LightRegistrar                                      \
        {                                                                      \
            CLASS_NAME##LightRegistrar()                                       \
            {                                                                  \
                kfe::RegistryLights::Register(                                  \
                    #CLASS_NAME,                                               \
                    []() -> std::unique_ptr<IKFELight>                          \
                    {                                                          \
                        auto obj = std::make_unique<CLASS_NAME>();            \
                        return obj;                                            \
                    });                                                        \
            }                                                                  \
        };                                                                     \
        static CLASS_NAME##LightRegistrar CLASS_NAME##_light_registrar_instance; \
    }
