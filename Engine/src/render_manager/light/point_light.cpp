#include "pch.h"
#include "engine/render_manager/light/point_light.h"

#include <algorithm>
#include <cstring>

#include "imgui/imgui.h"

using namespace DirectX;

namespace
{
    static inline void KFE_WriteFloat3(JsonLoader& j, const char* key, const XMFLOAT3& v) noexcept
    {
        j[key]["x"] = v.x;
        j[key]["y"] = v.y;
        j[key]["z"] = v.z;
    }

    static inline bool KFE_ReadFloat3(const JsonLoader& j, const char* key, XMFLOAT3& out) noexcept
    {
        if (!j.Has(key))
            return false;

        out.x = j[key]["x"].AsFloat();
        out.y = j[key]["y"].AsFloat();
        out.z = j[key]["z"].AsFloat();
        return true;
    }
}

kfe::KFEPointLight::KFEPointLight()
{
    ResetToDefaults();
}

kfe::KFEPointLight::~KFEPointLight() = default;

std::string kfe::KFEPointLight::GetName() const noexcept
{
    return "KFEPointLight";
}

std::string kfe::KFEPointLight::GetDescription() const noexcept
{
    return "Omnidirectional light at a position. Distance-culled on CPU.";
}

bool kfe::KFEPointLight::CanCullByDistance() const
{
    return true;
}

void kfe::KFEPointLight::ResetToDefaults()
{
    SetLightName("PointLight");

    SetPositionWS({ 0.0f, 2.0f, 0.0f });

    SetDirectionWS({ 0.0f, -1.0f, 0.0f });

    SetColor({ 1.0f, 1.0f, 1.0f });
    SetIntensity(3.0f);

    SetRange(12.0f);
    SetAttenuation(1.0f);

    SetSpotInnerAngle(0.0f);
    SetSpotOuterAngle(0.0f);
    SetSpotSoftness(0.0f);

    SetCullRadius(40.0f);
}

std::string kfe::KFEPointLight::GetLightType() const
{
    return "KFEPointLight";
}

void kfe::KFEPointLight::UpdateLight(const KFECamera*)
{
    SetIntensity(std::max(0.0f, GetIntensity()));
    SetAttenuation(std::max(0.0f, GetAttenuation()));
    SetRange(std::max(0.0f, GetRange()));

    SetSpotInnerAngle(0.0f);
    SetSpotOuterAngle(0.0f);
    SetSpotSoftness(0.0f);

    {
        const XMFLOAT3 d = GetDirectionWS();
        XMVECTOR v = XMVectorSet(d.x, d.y, d.z, 0.0f);
        v = NormalizeSafe(v);
        XMFLOAT3 out{};
        XMStoreFloat3(&out, v);
        SetDirectionWS(out);
    }
}

void kfe::KFEPointLight::ImguiView(float)
{
    if (!ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    // Name
    {
        char nameBuf[128]{};
        const std::string cur = GetLightName();
        strncpy_s(nameBuf, sizeof(nameBuf), cur.c_str(), _TRUNCATE);

        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
            SetLightName(std::string{ nameBuf });
    }

    // Enabled
    {
        bool enabled = IsEnable();
        if (ImGui::Checkbox("Enabled", &enabled))
        {
            if (enabled) Enable();
            else Disable();
        }
    }

    // Position
    {
        XMFLOAT3 pos = GetPositionWS();
        float p[3]{ pos.x, pos.y, pos.z };
        if (ImGui::DragFloat3("PositionWS", p, 0.05f))
            SetPositionWS({ p[0], p[1], p[2] });
    }

    // Intensity
    {
        float intensity = GetIntensity();
        if (ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100000.0f))
            SetIntensity(intensity);
    }

    // Color
    {
        XMFLOAT3 color = GetColor();
        float c[3]{ color.x, color.y, color.z };
        if (ImGui::ColorEdit3("Color", c))
            SetColor({ c[0], c[1], c[2] });
    }

    // Range / attenuation / cull radius
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

    if (ImGui::Button("Reset"))
        ResetToDefaults();
}

void kfe::KFEPointLight::LoadFromJson(const JsonLoader& loader)
{
    ResetToDefaults();

    if (loader.Has("LightName"))
        SetLightName(loader["LightName"].GetValue());

    if (loader.Has("Enabled"))
    {
        const bool en = loader["Enabled"].AsBool();
        if (en) Enable();
        else Disable();
    }

    XMFLOAT3 v{};
    if (KFE_ReadFloat3(loader, "PositionWS", v)) SetPositionWS(v);
    if (KFE_ReadFloat3(loader, "DirectionWS", v) || KFE_ReadFloat3(loader, "DirectionWSNormalized", v)) SetDirectionWS(v);
    if (KFE_ReadFloat3(loader, "Color", v)) SetColor(v);

    if (loader.Has("Intensity"))   SetIntensity(loader["Intensity"].AsFloat());
    if (loader.Has("Range"))       SetRange(loader["Range"].AsFloat());
    if (loader.Has("Attenuation")) SetAttenuation(loader["Attenuation"].AsFloat());

    if (loader.Has("CullRadius"))
        SetCullRadius(loader["CullRadius"].AsFloat());

    SetSpotInnerAngle(0.0f);
    SetSpotOuterAngle(0.0f);
    SetSpotSoftness(0.0f);
}

JsonLoader kfe::KFEPointLight::GetJsonData() const
{
    JsonLoader j{};

    j["Type"] = GetLightType();

    j["LightName"] = GetLightName();
    j["Enabled"] = IsEnable();

    KFE_WriteFloat3(j, "PositionWS", GetPositionWS());
    KFE_WriteFloat3(j, "DirectionWS", GetDirectionWSNormalized());
    KFE_WriteFloat3(j, "Color", GetColor());

    j["Intensity"] = GetIntensity();
    j["Range"] = GetRange();
    j["Attenuation"] = GetAttenuation();

    j["CullRadius"] = GetCullRadius();

    return j;
}
