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
#include "engine/utils/json_loader.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace kfe
{
    class KFE_API KFEWorld
    {
    public:
        KFEWorld() = default;
        ~KFEWorld() = default;

        KFEWorld(const KFEWorld&) = delete;
        KFEWorld(KFEWorld&&) noexcept = default;

        KFEWorld& operator=(const KFEWorld&) = delete;
        KFEWorld& operator=(KFEWorld&&) noexcept = default;

        // lifecycle
        [[nodiscard]] bool Initialize();
        [[nodiscard]] bool Destroy();

        void Update(float deltaTime);

        // Add an already-created scene object (any IKFESceneObject-derived type)
        void AddSceneObject(std::unique_ptr<IKFESceneObject> scene);

        // Create and add by registered name via RegistrySceneObject
        void AddSceneObject(const std::string& sceneName);

        // Remove by pointer
        std::unique_ptr<IKFESceneObject> RemoveSceneObject(IKFESceneObject* scene);

        // Remove by ID
        std::unique_ptr<IKFESceneObject> RemoveSceneObject(const KID id);

        const std::vector<IKFESceneObject*>& GetAllSceneObjects();

        void       LoadSceneData(const JsonLoader& loader);
        JsonLoader GetSceneData() const;

    private:
        // Scene Object Infos
        std::unordered_map<KID, std::unique_ptr<IKFESceneObject>> m_sceneObjects;
        std::vector<IKFESceneObject*>                             m_sceneObjectView;
        bool                                                      m_sceneViewDirty{ true };
    };
} // namespace kfe
