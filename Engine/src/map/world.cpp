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
#include "engine/system/registry/registry_light.h"
#include "engine/render_manager/components/render_queue.h"
#include "engine/system/exception/base_exception.h"

#include <algorithm>

#include "engine/utils/logger.h"

namespace kfe
{
    KFEWorld::KFEWorld() = default;

    bool KFEWorld::Initialize()
    {
        return true;
    }

    bool KFEWorld::Destroy()
    {
        m_sceneObjects   .clear();
        m_sceneObjectView.clear();
        m_sceneViewDirty = true;
        return true;
    }

    void KFEWorld::Update(float deltaTime)
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

            scene->SetObjectName(name);
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
            objNode["ID"]   = idStr;
            objNode["Type"] = scene->GetTypeName  ();
            objNode["Data"] = scene->GetJsonData  ();
            objNode["Name"] = scene->GetObjectName();
        }

        return root;
    }

    void KFEWorld::AddLight(std::unique_ptr<IKFELight> light)
    {
        if (!light)
            return;

        const KID id = light->GetAssignedKey();

        if (m_lights.contains(id))
        {
            THROW_MSG("Huge error!, Light ID Already Exists!");
            return;
        }

        KFERenderQueue::Instance().AddLight(light.get());

        std::string type = light->GetLightTypeName();
        m_lights[id] = std::move(light);
        m_lightViewDirty = true;
        LOG_INFO("Added Light: {}, type=", id, type);
    }

    void KFEWorld::AddLight(const std::string& lightName)
    {
        auto light = RegistryLights::Create(lightName);
        if (!light)
            return;

        AddLight(std::move(light));
    }

    std::unique_ptr<IKFELight> KFEWorld::RemoveLight(IKFELight* light)
    {
        if (!light)
            return nullptr;

        return RemoveLight(light->GetAssignedKey());
    }

    std::unique_ptr<IKFELight> KFEWorld::RemoveLight(const KID id)
    {
        if (!m_lights.contains(id))
            return nullptr;

        KFERenderQueue::Instance().RemoveLight(id);

        std::unique_ptr<IKFELight> removed = std::move(m_lights[id]);
        m_lights.erase(id);
        m_lightViewDirty = true;
        return removed;
    }

    const std::vector<IKFELight*>& KFEWorld::GetAllLights()
    {
        if (m_lightViewDirty)
        {
            m_lightView.clear();
            m_lightView.reserve(m_lights.size());

            for (auto& [id, uptr] : m_lights)
            {
                if (uptr)
                    m_lightView.push_back(uptr.get());
            }

            m_lightViewDirty = false;
        }

        return m_lightView;
    }

    void kfe::KFEWorld::LoadLightData(const JsonLoader& loader)
    {
        m_lights.clear();
        m_lightView.clear();
        m_lightViewDirty = true;

        for (const auto& [idKey, node] : loader)
        {
            if (!node.Contains("Type") || !node.Contains("Data"))
                continue;

            const std::string& typeName = node["Type"].GetValue();
            const std::string& name = node["Name"].GetValue();
            const JsonLoader& dataNode = node["Data"];

            auto light = RegistryLights::Create(typeName);
            if (!light)
                continue;

            light->SetLightName(name);
            light->LoadFromJson(dataNode);
            AddLight(std::move(light));

            LOG_SUCCESS("Added Light {}", typeName);
        }
    }

    JsonLoader kfe::KFEWorld::GetLightData() const
    {
        JsonLoader root{};

        for (const auto& [id, light] : m_lights)
        {
            if (!light)
                continue;

            const std::string idStr = std::to_string(id);

            JsonLoader& objNode = root[idStr];
            objNode["ID"]   = idStr;
            objNode["Type"] = light->GetLightTypeName();
            objNode["Data"] = light->GetJsonData();
            objNode["Name"] = light->GetLightName();
        }

        return root;
    }

} // namespace kfe
