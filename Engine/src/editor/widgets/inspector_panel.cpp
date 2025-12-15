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
#include "engine/editor/widgets/inspector_panel.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "engine/map/world.h"
#include "engine/system/interface/interface_scene.h"
#include "engine/system/interface/interface_light.h"

using namespace kfe;


void KFEInspectorPanel::Draw(float deltaTime)
{
    if (!m_visible || !m_pWorld)
    {
        return;
    }

    const auto& objects = m_pWorld->GetAllSceneObjects();
    const auto& lights = m_pWorld->GetAllLights();

    SyncSelection(objects);
    SyncLightSelection(lights);

    if (ImGui::Begin(m_title))
    {
        DrawScenesSection(deltaTime, objects);
        DrawLightsSection(deltaTime, lights);
        DrawPostProcessingSection();
    }
    ImGui::End();
}


void KFEInspectorPanel::SyncSelection(const std::vector<IKFESceneObject*>& objects)
{
    if (!m_pSelected)
    {
        if (!objects.empty())
        {
            m_pSelected = objects.front();
        }
        return;
    }

    bool stillThere = false;
    for (auto* obj : objects)
    {
        if (obj == m_pSelected)
        {
            stillThere = true;
            break;
        }
    }

    if (!stillThere)
    {
        m_pSelected = objects.empty() ? nullptr : objects.front();
    }
}

void KFEInspectorPanel::SyncLightSelection(
    const std::vector<IKFELight*>& lights)
{
    if (!m_pSelectedLight)
    {
        if (!lights.empty())
            m_pSelectedLight = lights.front();
        return;
    }

    bool stillThere = false;
    for (auto* l : lights)
    {
        if (l == m_pSelectedLight)
        {
            stillThere = true;
            break;
        }
    }

    if (!stillThere)
        m_pSelectedLight = lights.empty() ? nullptr : lights.front();
}

void KFEInspectorPanel::DrawScenesSection(
    float deltaTime,
    const std::vector<IKFESceneObject*>& objects)
{
    if (!ImGui::CollapsingHeader("Scenes", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    if (objects.empty())
    {
        ImGui::TextUnformatted("No scene objects in world.");
        return;
    }

    if (ImGui::TreeNodeEx("Scene Objects",
        ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_SpanAvailWidth))
    {
        for (std::size_t i = 0; i < objects.size(); ++i)
        {
            IKFESceneObject* obj = objects[i];
            if (!obj)
            {
                continue;
            }

            const auto key = obj->GetAssignedKey();
            std::string label = BuildObjectLabel(obj, i);

            ImGui::PushID(static_cast<int>(key));

            ImGuiTreeNodeFlags nodeFlags =
                ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                ImGuiTreeNodeFlags_AllowOverlap;

            bool open = ImGui::TreeNodeEx("##object_node", nodeFlags, "%s", label.c_str());

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Type: %s", obj->GetTypeName().c_str());
                ImGui::Text("Key : %llu",
                    static_cast<unsigned long long>(key));
                ImGui::EndTooltip();
            }

            if (open)
            {
                ImGui::Indent();
                obj->ImguiView(deltaTime);
                ImGui::Unindent();

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        ImGui::TreePop();
    }
}

void KFEInspectorPanel::DrawScenesObjectList(
    const std::vector<IKFESceneObject*>& objects)
{
    (void)objects;
}

void KFEInspectorPanel::DrawSelectedObjectInspector(float deltaTime)
{
    (void)deltaTime;
}

void KFEInspectorPanel::DrawLightsSection(float deltaTime, const std::vector<IKFELight*>& lights)
{
    if (!ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    if (lights.empty())
    {
        ImGui::TextDisabled("No lights in world.");
        return;
    }

    if (ImGui::TreeNodeEx("Light Stack",
        ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_SpanAvailWidth))
    {
        for (std::size_t i = 0; i < lights.size(); ++i)
        {
            IKFELight* light = lights[i];
            if (!light)
                continue;

            const auto key = light->GetAssignedKey();
            std::string label = BuildLightLabel(light, i);

            ImGui::PushID(static_cast<int>(key));

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                ImGuiTreeNodeFlags_AllowOverlap;

            bool open = ImGui::TreeNodeEx(
                "##light_node",
                flags,
                "%s",
                label.c_str());

            if (ImGui::IsItemClicked())
            {
                m_pSelectedLight = light;
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Type: %s", light->GetName().c_str());
                ImGui::Text("Key : %llu",
                    static_cast<unsigned long long>(key));
                ImGui::EndTooltip();
            }

            if (open)
            {
                ImGui::Indent();
                light->ImguiView(deltaTime);
                ImGui::Unindent();
                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        ImGui::TreePop();
    }
}

void KFEInspectorPanel::DrawPostProcessingSection()
{
    if (!ImGui::CollapsingHeader("Post Processing"))
    {
        return;
    }

    ImGui::TextDisabled("No post-processing stack yet.");
    ImGui::TextDisabled("In future: expose bloom/tonemap/AA/etc.");
}

std::string kfe::KFEInspectorPanel::BuildLightLabel(IKFELight* light, std::size_t index)
{
    if (!light)
        return "Null Light";

    std::string name = light->GetName();
    const auto key = light->GetAssignedKey();

    if (name.empty())
        name = "Light #" + std::to_string(index);

    return name + " [" + std::to_string(key) + "]";
}

std::string KFEInspectorPanel::BuildObjectLabel(IKFESceneObject* obj, std::size_t index)
{
    if (!obj)
    {
        return "Null Object";
    }

    std::string name = obj->GetObjectName();
    const auto  key = obj->GetAssignedKey();

    if (name.empty())
    {
        name = "Object #" + std::to_string(index);
    }

    return name + " [" + std::to_string(key) + "]";
}
