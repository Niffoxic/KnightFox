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
#include "engine/system/registry/registry_light.h"
#include "engine/render_manager/light/directional_light.h"
#include "engine/render_manager/light/point_light.h"
#include "engine/render_manager/light/spot_light.h"

namespace
{
    using CreateFunctor = kfe::RegistryLights::CreateFunctor;
    using RegistryMap   = std::unordered_map<std::string, CreateFunctor>;
    using NameList      = std::vector<std::string>;

    RegistryMap& GetRegistry()
    {
        static RegistryMap s_registry;
        return s_registry;
    }

    NameList& GetNameList()
    {
        static NameList s_names;
        return s_names;
    }
} // anonymous namespace

namespace kfe
{
    void RegistryLights::Register(const std::string& name, CreateFunctor fn)
    {
        if (!fn)
        {
            return;
        }

        auto& registry = ::GetRegistry();
        auto& names = ::GetNameList();

        // same behaviour as before: ignore duplicates
        if (registry.find(name) != registry.end())
        {
            return;
        }

        registry.emplace(name, std::move(fn));
        names.push_back(name);
    }

    _Use_decl_annotations_
        std::unique_ptr<IKFELight>
        RegistryLights::Create(const std::string& name)
    {
        auto& registry = ::GetRegistry();

        const auto it = registry.find(name);
        if (it == registry.end())
        {
            return nullptr;
        }

        return it->second ? it->second() : nullptr;
    }

    _Use_decl_annotations_
        const std::vector<std::string>&
        RegistryLights::GetRegisteredNames()
    {
        return ::GetNameList();
    }

    //~ Define Lights
    using KFEDirectionalLight = kfe::KFEDirectionalLight;
    KFE_REGISTER_LIGHT(KFEDirectionalLight);

    using KFESpotLight        = kfe::KFESpotLight;
    KFE_REGISTER_LIGHT(KFESpotLight);

    using KFEPointLight       = kfe::KFEPointLight;
    KFE_REGISTER_LIGHT(KFEPointLight);

} // namespace kfe
