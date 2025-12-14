#include "pch.h"
#include "engine/render_manager/light/spot_light.h"

#include <algorithm>
#include <cmath>

#include "imgui/imgui.h"

using namespace DirectX;

kfe::KFESpotLight::KFESpotLight()
{
    SetLightType(KFE_LIGHT_SPOT);
    ResetToDefaults();
}

kfe::KFESpotLight::~KFESpotLight() = default;

std::string kfe::KFESpotLight::GetName() const noexcept
{
    return "KFESpotLight";
}

std::string kfe::KFESpotLight::GetDescription() const noexcept
{
    return "Cone light with position and direction. Supports perspective shadow mapping.";
}

bool kfe::KFESpotLight::CanCullByDistance() const
{
    return true;
}

void kfe::KFESpotLight::ResetToDefaults()
{
    SetLightType(KFE_LIGHT_SPOT);

    SetLightName("SpotLight");

    SetPositionWS({ 0.0f, 3.0f, 0.0f });
    SetDirectionWS({ 0.0f, -1.0f, 0.0f });

    SetColor({ 1.0f, 1.0f, 1.0f });
    SetIntensity(2.0f);

    SetRange(15.0f);
    SetAttenuation(1.0f);

    SetSpotInnerAngle(0.35f);
    SetSpotOuterAngle(0.60f);

    SetShadowStrength(1.0f);
    SetShadowBias(0.0005f);
    SetNormalBias(0.01f);

    SetShadowDistance(0.0f);
    SetOrthoSize(0.0f);

    SetShadowMapSize(1024u, 1024u);

    SetLightView(XMMatrixIdentity());
    SetLightProj(XMMatrixIdentity());
    SetLightViewProj(XMMatrixIdentity());

    SetCullRadius(40.0f);
}

void kfe::KFESpotLight::UpdateLight(const KFECamera*)
{
    XMVECTOR pos = XMVectorSet(
        m_lightData.PositionWS.x,
        m_lightData.PositionWS.y,
        m_lightData.PositionWS.z,
        1.0f);

    XMVECTOR dir = XMVectorSet(
        m_lightData.DirectionWS.x,
        m_lightData.DirectionWS.y,
        m_lightData.DirectionWS.z,
        0.0f);

    dir = NormalizeSafe(dir);

    const float range = std::max(0.1f, m_lightData.Range);
    float outer = std::max(0.001f, m_lightData.SpotOuterAngle);
    float inner = std::max(0.0f, m_lightData.SpotInnerAngle);

    if (inner > outer)
        inner = outer;

    m_lightData.SpotInnerAngle = inner;
    m_lightData.SpotOuterAngle = outer;

    const XMVECTOR target = XMVectorAdd(pos, dir);
    const XMVECTOR up = ChooseUpVector(dir);

    const XMMATRIX V = XMMatrixLookAtLH(pos, target, up);

    const float fovY = std::min(outer * 2.0f, XM_PI - 0.01f);
    const float zn = 0.1f;
    const float zf = range;

    const XMMATRIX P = XMMatrixPerspectiveFovLH(fovY, 1.0f, zn, zf);

    m_lightData.LightView = V;
    m_lightData.LightProj = P;
    m_lightData.LightViewProj = XMMatrixMultiply(V, P);

    if (m_lightData.Intensity < 0.0f)       m_lightData.Intensity = 0.0f;
    if (m_lightData.ShadowStrength < 0.0f)  m_lightData.ShadowStrength = 0.0f;
    if (m_lightData.ShadowStrength > 1.0f)  m_lightData.ShadowStrength = 1.0f;
    if (m_lightData.ShadowBias < 0.0f)      m_lightData.ShadowBias = 0.0f;
    if (m_lightData.NormalBias < 0.0f)      m_lightData.NormalBias = 0.0f;
    if (m_lightData.Attenuation < 0.0f)     m_lightData.Attenuation = 0.0f;
}

void kfe::KFESpotLight::ImguiView(float)
{
    if (!ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    {
        char nameBuf[128]{};
        const std::string cur = GetLightName();
        strncpy_s(nameBuf, cur.c_str(), _TRUNCATE);
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
            SetLightName(std::string{ nameBuf });
    }

    {
        DirectX::XMFLOAT3 pos = GetPositionWS();
        float p[3]{ pos.x, pos.y, pos.z };
        if (ImGui::DragFloat3("PositionWS", p, 0.05f))
            SetPositionWS({ p[0], p[1], p[2] });
    }

    {
        DirectX::XMFLOAT3 dir = GetDirectionWS();
        float d[3]{ dir.x, dir.y, dir.z };
        if (ImGui::DragFloat3("DirectionWS", d, 0.01f, -1.0f, 1.0f))
            SetDirectionWS({ d[0], d[1], d[2] });

        ImGui::SameLine();
        if (ImGui::Button("Normalize"))
        {
            XMVECTOR v = XMVectorSet(d[0], d[1], d[2], 0.0f);
            v = NormalizeSafe(v);
            XMFLOAT3 out{};
            XMStoreFloat3(&out, v);
            SetDirectionWS(out);
        }
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
        float inner = GetSpotInnerAngle();
        float outer = GetSpotOuterAngle();

        if (ImGui::DragFloat("InnerAngle(rad)", &inner, 0.001f, 0.0f, 3.12f))
            SetSpotInnerAngle(inner);

        if (ImGui::DragFloat("OuterAngle(rad)", &outer, 0.001f, 0.001f, 3.12f))
            SetSpotOuterAngle(outer);

        if (inner > outer)
        {
            ImGui::Text("InnerAngle > OuterAngle (will clamp)");
        }
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
        int w = static_cast<int>(GetShadowMapWidth());
        int h = static_cast<int>(GetShadowMapHeight());
        if (w <= 0) w = 1024;
        if (h <= 0) h = 1024;

        if (ImGui::DragInt("ShadowMapWidth", &w, 1.0f, 1, 16384))
            SetShadowMapSize(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h));

        if (ImGui::DragInt("ShadowMapHeight", &h, 1.0f, 1, 16384))
            SetShadowMapSize(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h));
    }

    {
        if (ImGui::Button("Reset"))
            ResetToDefaults();
    }
}

void kfe::KFESpotLight::LoadFromJson(const JsonLoader& loader)
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
    if (loader.Has("Range"))          SetRange(loader["Range"].AsFloat());
    if (loader.Has("Attenuation"))    SetAttenuation(loader["Attenuation"].AsFloat());

    if (loader.Has("SpotInnerAngle")) SetSpotInnerAngle(loader["SpotInnerAngle"].AsFloat());
    if (loader.Has("SpotOuterAngle")) SetSpotOuterAngle(loader["SpotOuterAngle"].AsFloat());

    if (loader.Has("ShadowStrength")) SetShadowStrength(loader["ShadowStrength"].AsFloat());
    if (loader.Has("ShadowBias"))     SetShadowBias(loader["ShadowBias"].AsFloat());
    if (loader.Has("NormalBias"))     SetNormalBias(loader["NormalBias"].AsFloat());

    if (loader.Has("CullRadius"))     SetCullRadius(loader["CullRadius"].AsFloat());

    if (loader.Has("ShadowMapSize"))
    {
        const std::uint32_t w = loader["ShadowMapSize"]["w"].AsUInt();
        const std::uint32_t h = loader["ShadowMapSize"]["h"].AsUInt();
        SetShadowMapSize(w, h);
    }

    SetLightType(KFE_LIGHT_SPOT);
}

JsonLoader kfe::KFESpotLight::GetJsonData() const
{
    JsonLoader j{};

    j["Type"] = ToString(KFE_LIGHT_SPOT);
    j["Name"] = GetLightName();

    j["PositionWS"]["x"] = m_lightData.PositionWS.x;
    j["PositionWS"]["y"] = m_lightData.PositionWS.y;
    j["PositionWS"]["z"] = m_lightData.PositionWS.z;

    j["DirectionWS"]["x"] = m_lightData.DirectionWS.x;
    j["DirectionWS"]["y"] = m_lightData.DirectionWS.y;
    j["DirectionWS"]["z"] = m_lightData.DirectionWS.z;

    j["Color"]["x"] = m_lightData.Color.x;
    j["Color"]["y"] = m_lightData.Color.y;
    j["Color"]["z"] = m_lightData.Color.z;

    j["Intensity"] = m_lightData.Intensity;
    j["Range"] = m_lightData.Range;
    j["Attenuation"] = m_lightData.Attenuation;

    j["SpotInnerAngle"] = m_lightData.SpotInnerAngle;
    j["SpotOuterAngle"] = m_lightData.SpotOuterAngle;

    j["ShadowStrength"] = m_lightData.ShadowStrength;
    j["ShadowBias"] = m_lightData.ShadowBias;
    j["NormalBias"] = m_lightData.NormalBias;

    j["CullRadius"] = m_cullRadius;

    j["ShadowMapSize"]["w"] = std::to_string(GetShadowMapWidth());
    j["ShadowMapSize"]["h"] = std::to_string(GetShadowMapHeight());

    return j;
}
