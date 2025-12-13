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

#include "engine/render_manager/light/directional_light.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace kfe
{
    class KFE_API KFEWorld
    {
    public:
        KFEWorld();
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
        void AddSceneObject(const std::string& sceneName);
        std::unique_ptr<IKFESceneObject> RemoveSceneObject(IKFESceneObject* scene);
        std::unique_ptr<IKFESceneObject> RemoveSceneObject(const KID id);
        const std::vector<IKFESceneObject*>& GetAllSceneObjects();

        void       LoadSceneData(const JsonLoader& loader);
        JsonLoader GetSceneData() const;

        //~ Get Light
        KFEDirectionalLight* GetDirectionalLight() const { return m_directionaLight.get(); }

    private:
        //~ Scene Object Infos
        std::unordered_map<KID, std::unique_ptr<IKFESceneObject>> m_sceneObjects;
        std::vector<IKFESceneObject*>                             m_sceneObjectView;
        bool                                                      m_sceneViewDirty{ true };
    
        //~ Lights
        std::unique_ptr<KFEDirectionalLight> m_directionaLight;
    };
} // namespace kfe
