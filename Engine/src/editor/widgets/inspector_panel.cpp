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
#include "engine/render_manager/light/directional_light.h"

using namespace kfe;


void KFEInspectorPanel::Draw(float deltaTime)
{
    if (!m_visible || !m_pWorld)
    {
        return;
    }

    const auto& objects = m_pWorld->GetAllSceneObjects();

    SyncSelection(objects);

    if (ImGui::Begin(m_title))
    {
        DrawScenesSection(deltaTime, objects);
        DrawLightsSection();
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

void KFEInspectorPanel::DrawLightsSection()
{
    KFEDirectionalLight* light = m_pWorld->GetDirectionalLight();
    if (!light)
        return;

    if (!ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::PushID("DirectionalLight");

    ImGui::SeparatorText("Directional Light");

    //~ Direction
    {
        DirectX::XMFLOAT3 dir = light->GetDirectionWS();
        if (ImGui::DragFloat3("Direction", &dir.x, 0.01f, -1.0f, 1.0f))
        {
            DirectX::XMVECTOR v = DirectX::XMLoadFloat3(&dir);
            v = DirectX::XMVector3Normalize(v);
            DirectX::XMStoreFloat3(&dir, v);
            light->SetDirectionWS(dir);
        }
    }

    //~ Color
    {
        DirectX::XMFLOAT3 col = light->GetColor();
        if (ImGui::ColorEdit3("Color", &col.x))
        {
            light->SetColor(col);
        }
    }

    //~ Intensity
    {
        float intensity = light->GetIntensity();
        if (ImGui::DragFloat("Intensity", &intensity, 0.05f, 0.0f, 50.0f))
        {
            light->SetIntensity(intensity);
        }
    }

    ImGui::SeparatorText("Shadow (prep)");

    //~ Shadow strength
    {
        float v = light->GetShadowStrength();
        if (ImGui::SliderFloat("Shadow Strength", &v, 0.0f, 1.0f))
        {
            light->SetShadowStrength(v);
        }
    }

    //~ Shadow bias
    {
        float bias = light->GetShadowBias();
        if (ImGui::DragFloat("Shadow Bias", &bias, 0.0001f, 0.0f, 0.05f, "%.5f"))
        {
            light->SetShadowBias(bias);
        }
    }

    //~ Normal bias
    {
        float nb = light->GetNormalBias();
        if (ImGui::DragFloat("Normal Bias", &nb, 0.0001f, 0.0f, 0.05f, "%.5f"))
        {
            light->SetNormalBias(nb);
        }
    }

    //~ Shadow distance
    {
        float d = light->GetShadowDistance();
        if (ImGui::DragFloat("Shadow Distance", &d, 1.0f, 1.0f, 500.0f))
        {
            light->SetShadowDistance(d);
        }
    }

    //~ Ortho size
    {
        float o = light->GetOrthoSize();
        if (ImGui::DragFloat("Ortho Size", &o, 1.0f, 1.0f, 500.0f))
        {
            light->SetOrthoSize(o);
        }
    }

    ImGui::PopID();
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
