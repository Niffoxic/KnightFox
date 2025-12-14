#include "pch.h"
#include "engine/render_manager/light/point_light.h"

#include <algorithm>
#include <cmath>

#include "imgui/imgui.h"

using namespace DirectX;

kfe::KFEPointLight::KFEPointLight()
{
    SetLightType(KFE_LIGHT_POINT);
    ResetToDefaults();
}

kfe::KFEPointLight::~KFEPointLight() = default;

std::string kfe::KFEPointLight::GetName() const noexcept
{
    return "KFEPointLight";
}

std::string kfe::KFEPointLight::GetDescription() const noexcept
{
    return "Omnidirectional light at a position. Distance-culled on CPU and typically shadowed using a cubemap.";
}

bool kfe::KFEPointLight::CanCullByDistance() const
{
    return true;
}

void kfe::KFEPointLight::ResetToDefaults()
{
    SetLightType(KFE_LIGHT_POINT);

    SetLightName("PointLight");

    SetPositionWS({ 0.0f, 2.0f, 0.0f });
    SetDirectionWS({ 0.0f, -1.0f, 0.0f });

    SetColor({ 1.0f, 1.0f, 1.0f });
    SetIntensity(3.0f);

    SetRange(12.0f);
    SetAttenuation(1.0f);

    SetSpotInnerAngle(0.0f);
    SetSpotOuterAngle(0.0f);

    SetShadowStrength(0.0f);
    SetShadowBias(0.0005f);
    SetNormalBias(0.01f);

    SetShadowDistance(0.0f);
    SetOrthoSize(0.0f);

    SetShadowMapSize(0u, 0u);

    SetLightView(XMMatrixIdentity());
    SetLightProj(XMMatrixIdentity());
    SetLightViewProj(XMMatrixIdentity());

    SetCullRadius(40.0f);
}

void kfe::KFEPointLight::UpdateLight(const KFECamera*)
{
    if (m_lightData.Intensity < 0.0f) m_lightData.Intensity = 0.0f;
    if (m_lightData.Attenuation < 0.0f) m_lightData.Attenuation = 0.0f;
    if (m_lightData.Range < 0.0f) m_lightData.Range = 0.0f;

    if (m_lightData.ShadowStrength < 0.0f) m_lightData.ShadowStrength = 0.0f;
    if (m_lightData.ShadowStrength > 1.0f) m_lightData.ShadowStrength = 1.0f;
    if (m_lightData.ShadowBias < 0.0f) m_lightData.ShadowBias = 0.0f;
    if (m_lightData.NormalBias < 0.0f) m_lightData.NormalBias = 0.0f;

    m_lightData.LightView = XMMatrixIdentity();
    m_lightData.LightProj = XMMatrixIdentity();
    m_lightData.LightViewProj = XMMatrixIdentity();

    m_lightData.SpotInnerAngle = 0.0f;
    m_lightData.SpotOuterAngle = 0.0f;

    m_lightData.ShadowDistance = 0.0f;
    m_lightData.OrthoSize = 0.0f;
}

void kfe::KFEPointLight::ImguiView(float)
{
    if (!ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    {
        char nameBuf[128]{};
        const std::string cur = GetLightName();
#if defined(_MSC_VER)
        strncpy_s(nameBuf, cur.c_str(), _TRUNCATE);
#else
        std::strncpy(nameBuf, cur.c_str(), sizeof(nameBuf) - 1);
#endif
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
            SetLightName(std::string{ nameBuf });
    }

    {
        XMFLOAT3 pos = GetPositionWS();
        float p[3]{ pos.x, pos.y, pos.z };
        if (ImGui::DragFloat3("PositionWS", p, 0.05f))
            SetPositionWS({ p[0], p[1], p[2] });
    }

    {
        float intensity = GetIntensity();
        if (ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100000.0f))
            SetIntensity(intensity);
    }

    {
        XMFLOAT3 color = GetColor();
        float c[3]{ color.x, color.y, color.z };
        if (ImGui::ColorEdit3("Color", c))
            SetColor({ c[0], c[1], c[2] });
    }

    {
        float range = GetRange();
        if (ImGui::DragFloat("Range", &range, 0.05f, 0.1f, 100000.0f))
            SetRange(range);

        float att = GetAttenuation();
        if (ImGui::DragFloat("Attenuation", &att, 0.01f, 0.0f, 100000.0f))
            SetAttenuation(att);

        float cull = GetCullRadius();
        if (ImGui::DragFloat("CullRadius", &cull, 0.1f, 0.0f, 100000.0f))
            SetCullRadius(cull);
    }

    {
        float ss = GetShadowStrength();
        if (ImGui::SliderFloat("ShadowStrength", &ss, 0.0f, 1.0f))
            SetShadowStrength(ss);

        float sb = GetShadowBias();
        if (ImGui::DragFloat("ShadowBias", &sb, 0.00001f, 0.0f, 0.1f, "%.6f"))
            SetShadowBias(sb);

        float nb = GetNormalBias();
        if (ImGui::DragFloat("NormalBias", &nb, 0.0001f, 0.0f, 10.0f, "%.6f"))
            SetNormalBias(nb);
    }

    {
        if (ImGui::Button("Reset"))
            ResetToDefaults();
    }
}

void kfe::KFEPointLight::LoadFromJson(const JsonLoader& loader)
{
    ResetToDefaults();

    if (loader.Has("Name"))
        SetLightName(loader["Name"].GetValue());

    if (loader.Has("PositionWS"))
    {
        XMFLOAT3 p{};
        p.x = loader["PositionWS"]["x"].AsFloat();
        p.y = loader["PositionWS"]["y"].AsFloat();
        p.z = loader["PositionWS"]["z"].AsFloat();
        SetPositionWS(p);
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
    if (loader.Has("Range"))          SetRange(loader["Range"].AsFloat());
    if (loader.Has("Attenuation"))    SetAttenuation(loader["Attenuation"].AsFloat());

    if (loader.Has("ShadowStrength")) SetShadowStrength(loader["ShadowStrength"].AsFloat());
    if (loader.Has("ShadowBias"))     SetShadowBias(loader["ShadowBias"].AsFloat());
    if (loader.Has("NormalBias"))     SetNormalBias(loader["NormalBias"].AsFloat());

    if (loader.Has("CullRadius"))     SetCullRadius(loader["CullRadius"].AsFloat());

    SetLightType(KFE_LIGHT_POINT);
}

JsonLoader kfe::KFEPointLight::GetJsonData() const
{
    JsonLoader j{};

    j["Type"] = ToString(KFE_LIGHT_POINT);
    j["Name"] = GetLightName();

    j["PositionWS"]["x"] = m_lightData.PositionWS.x;
    j["PositionWS"]["y"] = m_lightData.PositionWS.y;
    j["PositionWS"]["z"] = m_lightData.PositionWS.z;

    j["Color"]["x"] = m_lightData.Color.x;
    j["Color"]["y"] = m_lightData.Color.y;
    j["Color"]["z"] = m_lightData.Color.z;

    j["Intensity"] = m_lightData.Intensity;
    j["Range"] = m_lightData.Range;
    j["Attenuation"] = m_lightData.Attenuation;

    j["ShadowStrength"] = m_lightData.ShadowStrength;
    j["ShadowBias"] = m_lightData.ShadowBias;
    j["NormalBias"] = m_lightData.NormalBias;

    j["CullRadius"] = m_cullRadius;

    return j;
}
