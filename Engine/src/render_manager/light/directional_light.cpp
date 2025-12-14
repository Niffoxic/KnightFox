// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  File      : directional_light.cpp
 *  -----------------------------------------------------------------------------
 */

#include "pch.h"
#include "engine/render_manager/light/directional_light.h"

#include <algorithm>
#include <cmath>

#include "imgui/imgui.h"

using namespace DirectX;

kfe::KFEDirectionalLight::KFEDirectionalLight()
{
	SetLightType(KFE_LIGHT_DIRECTIONAL);
	ResetToDefaults();
}

kfe::KFEDirectionalLight::~KFEDirectionalLight() = default;

std::string kfe::KFEDirectionalLight::GetName() const noexcept
{
	return "KFEDirectionalLight";
}

std::string kfe::KFEDirectionalLight::GetDescription() const noexcept
{
	return "Infinite light source with a single direction. Used for sun moon and large scale lighting.";
}

void kfe::KFEDirectionalLight::UpdateLight(const KFECamera* camera)
{
    if (!camera)
        return;

    XMVECTOR dir = XMVectorSet(
        m_lightData.DirectionWS.x,
        m_lightData.DirectionWS.y,
        m_lightData.DirectionWS.z,
        0.0f);

    dir = NormalizeSafe(dir);

    const XMVECTOR camPos = XMLoadFloat3(&camera->GetPosition());

    XMVECTOR camFwd = camera->GetForwardVector();
    camFwd = NormalizeSafe(camFwd);

    const float shadowDist = std::max(0.1f, m_lightData.ShadowDistance);
    const float orthoSize = std::max(0.1f, m_lightData.OrthoSize);

    const XMVECTOR center = XMVectorAdd(camPos, XMVectorScale(camFwd, shadowDist * 0.5f));
    const XMVECTOR eye = XMVectorSubtract(center, XMVectorScale(dir, shadowDist));

    const XMVECTOR up = ChooseUpVector(dir);

    const XMMATRIX V = XMMatrixLookAtLH(eye, center, up);
    const XMMATRIX P = XMMatrixOrthographicLH(
        orthoSize * 2.0f,
        orthoSize * 2.0f,
        0.1f,
        shadowDist * 2.0f);

    m_lightData.LightView = V;
    m_lightData.LightProj = P;
    m_lightData.LightViewProj = XMMatrixMultiply(V, P);
}

bool kfe::KFEDirectionalLight::CanCullByDistance() const
{
    return false;
}

void kfe::KFEDirectionalLight::ImguiView(float dt)
{
    if (!ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    {
        char nameBuf[128]{};
        const std::string cur = GetLightName();
        strncpy_s(nameBuf, cur.c_str(), _TRUNCATE);
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
            SetLightName(std::string{ nameBuf });
    }

    {
        float intensity = GetIntensity();
        if (ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100000.0f))
            SetIntensity(intensity);
    }

    {
        DirectX::XMFLOAT3 color = GetColor();
        float c[3]{ color.x, color.y, color.z };
        if (ImGui::ColorEdit3("Color", c))
            SetColor({ c[0], c[1], c[2] });
    }

    {
        DirectX::XMFLOAT3 dir = GetDirectionWS();
        float d[3]{ dir.x, dir.y, dir.z };

        if (ImGui::DragFloat3("DirectionWS", d, 0.01f, -1.0f, 1.0f))
            SetDirectionWS({ d[0], d[1], d[2] });

        ImGui::SameLine();
        if (ImGui::Button("Normalize"))
        {
            DirectX::XMVECTOR v = DirectX::XMVectorSet(d[0], d[1], d[2], 0.0f);
            v = NormalizeSafe(v);
            DirectX::XMFLOAT3 out{};
            DirectX::XMStoreFloat3(&out, v);
            SetDirectionWS(out);
        }
    }

    {
        float shadowStrength = GetShadowStrength();
        if (ImGui::SliderFloat("ShadowStrength", &shadowStrength, 0.0f, 1.0f))
            SetShadowStrength(shadowStrength);
    }

    {
        float shadowBias = GetShadowBias();
        if (ImGui::DragFloat("ShadowBias", &shadowBias, 0.00001f, 0.0f, 0.1f, "%.6f"))
            SetShadowBias(shadowBias);

        float normalBias = GetNormalBias();
        if (ImGui::DragFloat("NormalBias", &normalBias, 0.0001f, 0.0f, 10.0f, "%.6f"))
            SetNormalBias(normalBias);
    }

    {
        float shadowDistance = GetShadowDistance();
        if (ImGui::DragFloat("ShadowDistance", &shadowDistance, 0.1f, 0.1f, 100000.0f))
            SetShadowDistance(shadowDistance);

        float orthoSize = GetOrthoSize();
        if (ImGui::DragFloat("OrthoSize", &orthoSize, 0.1f, 0.1f, 100000.0f))
            SetOrthoSize(orthoSize);
    }

    {
        int w = static_cast<int>(GetShadowMapWidth());
        int h = static_cast<int>(GetShadowMapHeight());

        if (w <= 0) w = 2048;
        if (h <= 0) h = 2048;

        if (ImGui::DragInt("ShadowMapWidth", &w, 1.0f, 1, 16384))
            SetShadowMapSize(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h));

        if (ImGui::DragInt("ShadowMapHeight", &h, 1.0f, 1, 16384))
            SetShadowMapSize(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h));

        const auto inv = GetLightData().InvShadowMapSize;
        ImGui::Text("InvShadowMapSize: %.6f, %.6f", inv.x, inv.y);
    }

    {
        float cull = GetCullRadius();
        ImGui::BeginDisabled(true);
        ImGui::DragFloat("CullRadius", &cull, 0.1f, 0.0f, 100000.0f);
        ImGui::EndDisabled();
        ImGui::Text("Directional lights are not distance-culled.");
    }

    {
        if (ImGui::Button("Reset"))
            ResetToDefaults();
    }
}

void kfe::KFEDirectionalLight::LoadFromJson(const JsonLoader& loader)
{
    if (!loader.Has("Type"))
        ResetToDefaults();

    if (loader.Has("LightName")) SetLightName(loader["LightName"].GetValue());

    if (loader.Has("DirectionWS"))
    {
        XMFLOAT3 d{};
        d.x = loader["DirectionWS"]["x"].AsFloat();
        d.y = loader["DirectionWS"]["y"].AsFloat();
        d.z = loader["DirectionWS"]["z"].AsFloat();
        SetDirectionWS(d);
    }

    if (loader.Has("Color"))
    {
        XMFLOAT3 c{};
        c.x = loader["Color"]["x"].AsFloat();
        c.y = loader["Color"]["y"].AsFloat();
        c.z = loader["Color"]["z"].AsFloat();
        SetColor(c);
    }

    if (loader.Has("Intensity"))      SetIntensity(loader["Intensity"].AsFloat());
    if (loader.Has("ShadowStrength")) SetShadowStrength(loader["ShadowStrength"].AsFloat());
    if (loader.Has("ShadowBias"))     SetShadowBias(loader["ShadowBias"].AsFloat());
    if (loader.Has("NormalBias"))     SetNormalBias(loader["NormalBias"].AsFloat());

    if (loader.Has("ShadowDistance")) SetShadowDistance(loader["ShadowDistance"].AsFloat());
    if (loader.Has("OrthoSize"))      SetOrthoSize(loader["OrthoSize"].AsFloat());

    if (loader.Has("ShadowMapSize"))
    {
        const std::uint32_t w = loader["ShadowMapSize"]["w"].AsUInt();
        const std::uint32_t h = loader["ShadowMapSize"]["h"].AsUInt();
        SetShadowMapSize(w, h);
    }

    SetLightType        (KFE_LIGHT_DIRECTIONAL);
    SetRange            (0.0f);
    SetAttenuation      (1.0f);
    SetSpotInnerAngle   (0.0f);
    SetSpotOuterAngle   (0.0f);
    SetCullRadius       (0.0f);
}

JsonLoader kfe::KFEDirectionalLight::GetJsonData() const
{
    JsonLoader j{};

    j["Type"]      = ToString(KFE_LIGHT_DIRECTIONAL);
    j["LightName"] = GetLightName();

    j["DirectionWS"]["x"] = m_lightData.DirectionWS.x;
    j["DirectionWS"]["y"] = m_lightData.DirectionWS.y;
    j["DirectionWS"]["z"] = m_lightData.DirectionWS.z;

    j["Color"]["x"] = m_lightData.Color.x;
    j["Color"]["y"] = m_lightData.Color.y;
    j["Color"]["z"] = m_lightData.Color.z;

    j["Intensity"] = m_lightData.Intensity;
    j["ShadowStrength"] = m_lightData.ShadowStrength;
    j["ShadowBias"] = m_lightData.ShadowBias;
    j["NormalBias"] = m_lightData.NormalBias;

    j["ShadowDistance"] = m_lightData.ShadowDistance;
    j["OrthoSize"] = m_lightData.OrthoSize;

    j["ShadowMapSize"]["w"] = std::to_string(GetShadowMapWidth());
    j["ShadowMapSize"]["h"] = std::to_string(GetShadowMapHeight());

    return j;
}

void kfe::KFEDirectionalLight::ResetToDefaults()
{
    SetLightType(KFE_LIGHT_DIRECTIONAL);

    SetDirectionWS({ 0.0f, -1.0f, 0.0f });
    SetColor({ 1.0f, 1.0f, 1.0f });
    SetIntensity(1.0f);

    SetShadowStrength(1.0f);
    SetShadowBias(0.0005f);
    SetNormalBias(0.01f);

    SetShadowDistance(60.0f);
    SetOrthoSize(25.0f);

    SetShadowMapSize(2048u, 2048u);

    SetPositionWS({ 0.0f, 0.0f, 0.0f });
    SetRange(0.0f);
    SetAttenuation(1.0f);

    SetSpotInnerAngle(0.0f);
    SetSpotOuterAngle(0.0f);

    SetLightView(XMMatrixIdentity());
    SetLightProj(XMMatrixIdentity());
    SetLightViewProj(XMMatrixIdentity());

    SetCullRadius(0.0f);
}
