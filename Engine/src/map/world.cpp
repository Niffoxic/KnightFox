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
#include "engine/map/world.h"

#include "engine/system/registry/registry_scene.h"
#include "engine/render_manager/components/render_queue.h"
#include "engine/system/exception/base_exception.h"

#include <algorithm>

#include "engine/utils/logger.h"

namespace kfe
{
    bool KFEWorld::Initialize()
    {
        return true;
    }

    bool KFEWorld::Destroy()
    {
        m_sceneObjects.clear();
        m_sceneObjectView.clear();
        m_sceneViewDirty = true;
        return true;
    }

    void KFEWorld::Update(float /*deltaTime*/)
    {

    }

    void KFEWorld::AddSceneObject(std::unique_ptr<IKFESceneObject> scene)
    {
        if (!scene)
            return;

        const KID id = scene->GetAssignedKey();

        if (m_sceneObjects.contains(id))
        {
            THROW_MSG("Huge error!, ID Already Exists!");
            return;
        }

        // Add to render queue
        KFERenderQueue::Instance().AddSceneObject(scene.get());

        // Store object
        m_sceneObjects[id] = std::move(scene);
        m_sceneViewDirty = true;
    }

    void KFEWorld::AddSceneObject(const std::string& sceneName)
    {
        auto scene = RegistrySceneObject::Create(sceneName);
        if (!scene)
            return;

        AddSceneObject(std::move(scene));
    }

    std::unique_ptr<IKFESceneObject>
        KFEWorld::RemoveSceneObject(IKFESceneObject* scene)
    {
        if (!scene)
            return nullptr;

        return RemoveSceneObject(scene->GetAssignedKey());
    }

    std::unique_ptr<IKFESceneObject>
        KFEWorld::RemoveSceneObject(const KID id)
    {
        if (!m_sceneObjects.contains(id))
            return nullptr;

        KFERenderQueue::Instance().RemoveSceneObject(id);

        std::unique_ptr<IKFESceneObject> removed = std::move(m_sceneObjects[id]);
        m_sceneObjects.erase(id);
        m_sceneViewDirty = true;
        return removed;
    }

    const std::vector<IKFESceneObject*>& KFEWorld::GetAllSceneObjects()
    {
        if (m_sceneViewDirty)
        {
            m_sceneObjectView.clear();
            m_sceneObjectView.reserve(m_sceneObjects.size());

            for (auto& [id, uptr] : m_sceneObjects)
            {
                if (uptr)
                    m_sceneObjectView.push_back(uptr.get());
            }

            m_sceneViewDirty = false;
        }

        return m_sceneObjectView;
    }

    void KFEWorld::LoadSceneData(const JsonLoader& loader)
    {
        m_sceneObjects.clear();
        m_sceneObjectView.clear();
        m_sceneViewDirty = true;

        for (const auto& [idKey, node] : loader)
        {
            if (!node.Contains("Type") || !node.Contains("Data"))
                continue;

            const std::string& typeName = node["Type"].GetValue();
            const std::string& name = node["Name"].GetValue();
            const JsonLoader& dataNode = node["Data"];

            auto scene = RegistrySceneObject::Create(typeName);
            if (!scene)
                continue;

            scene->SetOjbjectName(name);
            scene->LoadFromJson(dataNode);
            AddSceneObject(std::move(scene));

            LOG_SUCCESS("Added {}", typeName);
        }
    }

    JsonLoader KFEWorld::GetSceneData() const
    {
        JsonLoader root{};

        for (const auto& [id, scene] : m_sceneObjects)
        {
            if (!scene)
                continue;

            const std::string idStr = std::to_string(id);

            JsonLoader& objNode = root[idStr];
            objNode["ID"] = idStr;
            objNode["Type"] = scene->GetTypeName();
            objNode["Data"] = scene->GetJsonData();
            objNode["Name"] = scene->GetName    ();
        }

        return root;
    }

} // namespace kfe
