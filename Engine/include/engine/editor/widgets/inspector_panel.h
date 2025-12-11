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

#include "imgui/imgui.h"

#include <string>

namespace kfe
{
    class KFEWorld;
    class IKFESceneObject;

    class KFE_API KFEInspectorPanel
    {
    public:
        KFEInspectorPanel() = default;
        ~KFEInspectorPanel() = default;

        KFEInspectorPanel(const KFEInspectorPanel&) = delete;
        KFEInspectorPanel& operator=(const KFEInspectorPanel&) = delete;

        KFEInspectorPanel(KFEInspectorPanel&&) = default;
        KFEInspectorPanel& operator=(KFEInspectorPanel&&) = default;

        void SetPanelName(const char* name) noexcept { m_title = name ? name : "Inspector"; }
        void SetVisible(bool v) noexcept { m_visible = v; }
        void SetWorld(KFEWorld* world) noexcept { m_pWorld = world; }

        [[nodiscard]] bool Initialize() { return true; }
        [[nodiscard]] bool Release()
        {
            m_pWorld = nullptr;
            m_pSelected = nullptr;
            return true;
        }

        void Draw(float deltaTime);

        void SetSelectedObject(IKFESceneObject* obj) noexcept { m_pSelected = obj; }
        IKFESceneObject* GetSelectedObject() const noexcept { return m_pSelected; }

    private:
        void SyncSelection(const std::vector<IKFESceneObject*>& objects);
        void DrawScenesSection(float deltaTime, const std::vector<IKFESceneObject*>& objects);
        void DrawScenesObjectList(const std::vector<IKFESceneObject*>& objects);
        void DrawSelectedObjectInspector(float deltaTime);
        void DrawLightsSection();
        void DrawPostProcessingSection();

        static std::string BuildObjectLabel(IKFESceneObject* obj, std::size_t index);

    private:
        const char*      m_title    { "Inspector" };
        bool             m_visible  { false };
        KFEWorld*        m_pWorld   { nullptr };
        IKFESceneObject* m_pSelected{ nullptr };
    };
} // namespace kfe
