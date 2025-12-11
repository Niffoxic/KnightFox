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

#include "engine/system/interface/interface_scene.h"

#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <string>

namespace kfe
{
    class KFE_API RegistrySceneObject final
    {
    public:
        using CreateFunctor = std::function<std::unique_ptr<IKFESceneObject>()>;

        static void Register(const std::string& name, CreateFunctor fn);

        NODISCARD static std::unique_ptr<IKFESceneObject>
        Create(const std::string& name);

        NODISCARD static const std::vector<std::string>&
        GetRegisteredNames();
    };

} // namespace kfe

#define KFE_REGISTER_SCENE_OBJECT(CLASS_NAME)                                  \
    namespace {                                                                \
        struct CLASS_NAME##SceneRegistrar                                      \
        {                                                                      \
            CLASS_NAME##SceneRegistrar()                                       \
            {                                                                  \
                kfe::RegistrySceneObject::Register(                            \
                    #CLASS_NAME,                                               \
                    []() -> std::unique_ptr<IKFESceneObject>                   \
                    {                                                          \
                        auto obj = std::make_unique<CLASS_NAME>();            \
                        obj->SetTypeName(#CLASS_NAME);                         \
                        return obj;                                            \
                    });                                                        \
            }                                                                  \
        };                                                                     \
        static CLASS_NAME##SceneRegistrar CLASS_NAME##_scene_registrar_instance; \
    }
