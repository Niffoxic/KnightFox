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
#include "engine/system/registry/registry_scene.h"
#include "engine/render_manager/scene/cube_scene.h"
#include "engine/render_manager/scene/model_scene.h"

namespace
{
    using CreateFunctor = kfe::RegistrySceneObject::CreateFunctor;
    using RegistryMap = std::unordered_map<std::string, CreateFunctor>;
    using NameList = std::vector<std::string>;

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
    void RegistrySceneObject::Register(const std::string& name, CreateFunctor fn)
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
        std::unique_ptr<IKFESceneObject>
        RegistrySceneObject::Create(const std::string& name)
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
        RegistrySceneObject::GetRegisteredNames()
    {
        return ::GetNameList();
    }

    //~ Define Objects (same as before)
    using KEFCubeSceneObject = kfe::KEFCubeSceneObject;
    KFE_REGISTER_SCENE_OBJECT(KEFCubeSceneObject);
    
    using KFEMeshSceneObject = kfe::KFEMeshSceneObject;
    KFE_REGISTER_SCENE_OBJECT(KFEMeshSceneObject);
} // namespace kfe
