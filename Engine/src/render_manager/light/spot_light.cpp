#include "pch.h"
#include "engine/render_manager/light/spot_light.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "imgui/imgui.h"

using namespace DirectX;

namespace
{
    static inline float KFE_Clamp01(const float v) noexcept
    {
        return std::clamp(v, 0.0f, 1.0f);
    }

    static inline float KFE_ClampCos(const float v) noexcept
    {
        return std::clamp(v, -1.0f, 1.0f);
    }

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

kfe::KFESpotLight::KFESpotLight()
{
    ResetToDefaults();
}

kfe::KFESpotLight::~KFESpotLight() = default;

std::string kfe::KFESpotLight::GetName() const noexcept
{
    return "KFESpotLight";
}

std::string kfe::KFESpotLight::GetDescription() const noexcept
{
    return "Cone light defined by position + direction + inner/outer angle. Distance-culled on CPU.";
}

bool kfe::KFESpotLight::CanCullByDistance() const
{
    return true;
}

void kfe::KFESpotLight::ResetToDefaults()
{
    SetLightName("SpotLight");

    SetPositionWS({ 0.0f, 2.0f, 0.0f });
    SetDirectionWS({ 0.0f, -1.0f, 0.0f });

    SetColor({ 1.0f, 1.0f, 1.0f });
    SetIntensity(6.0f);

    SetRange(18.0f);
    SetAttenuation(1.0f);

    // Typical spotlight cone
    SetSpotInnerAngle(0.55f);
    SetSpotOuterAngle(0.80f);
    SetSpotSoftness(0.2f);

    SetCullRadius(40.0f);
}

std::string kfe::KFESpotLight::GetLightType() const
{
    return "KFESpotLight";
}

void kfe::KFESpotLight::UpdateLight(const KFECamera*)
{
    // Sanitize core params
    SetIntensity(std::max(0.0f, GetIntensity()));
    SetAttenuation(std::max(0.0f, GetAttenuation()));
    SetRange(std::max(0.0f, GetRange()));

    // Direction normalized
    {
        const XMFLOAT3 d = GetDirectionWS();
        XMVECTOR v = XMVectorSet(d.x, d.y, d.z, 0.0f);
        v = NormalizeSafe(v);
        XMFLOAT3 out{};
        XMStoreFloat3(&out, v);
        SetDirectionWS(out);
    }

    // Clamp spot softness
    SetSpotSoftness(KFE_Clamp01(GetSpotSoftness()));

    const float inner = KFE_ClampCos(std::cos(std::max(0.0f, GetSpotInnerAngle())));
    const float outer = KFE_ClampCos(std::cos(std::max(0.0f, GetSpotOuterAngle())));

    float innerCos = inner;
    float outerCos = outer;

    if (innerCos < outerCos)
        std::swap(innerCos, outerCos);

    SetSpotInnerAngle(std::acos(KFE_ClampCos(innerCos)));
    SetSpotOuterAngle(std::acos(KFE_ClampCos(outerCos)));
}

void kfe::KFESpotLight::ImguiView(float)
{
    if (!ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen))
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

    // Direction
    {
        XMFLOAT3 dir = GetDirectionWS();
        float d[3]{ dir.x, dir.y, dir.z };
        if (ImGui::DragFloat3("Direction", d, 0.01f, -1.0f, 1.0f))
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

    // Angles
    {
        float inner = GetSpotInnerAngle();
        float outer = GetSpotOuterAngle();

        if (ImGui::DragFloat("Inner Angle (rad)", &inner, 0.001f, 0.0f, 3.13f))
            SetSpotInnerAngle(inner);

        if (ImGui::DragFloat("Outer Angle (rad)", &outer, 0.001f, 0.0f, 3.13f))
            SetSpotOuterAngle(outer);

        float soft = GetSpotSoftness();
        if (ImGui::SliderFloat("Softness", &soft, 0.0f, 1.0f))
            SetSpotSoftness(soft);

        if (outer < inner)
            ImGui::TextDisabled("Note: Outer angle should be >= Inner angle (will be fixed on update).");
    }

    if (ImGui::Button("Reset"))
        ResetToDefaults();
}

void kfe::KFESpotLight::LoadFromJson(const JsonLoader& loader)
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

    if (loader.Has("SpotInnerAngle")) SetSpotInnerAngle(loader["SpotInnerAngle"].AsFloat());
    if (loader.Has("SpotOuterAngle")) SetSpotOuterAngle(loader["SpotOuterAngle"].AsFloat());
    if (loader.Has("SpotSoftness"))   SetSpotSoftness(loader["SpotSoftness"].AsFloat());

    if (loader.Has("CullRadius"))
        SetCullRadius(loader["CullRadius"].AsFloat());

    // Enforce spot invariants
    SetSpotSoftness(KFE_Clamp01(GetSpotSoftness()));
}

JsonLoader kfe::KFESpotLight::GetJsonData() const
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

    j["SpotInnerAngle"] = GetSpotInnerAngle();
    j["SpotOuterAngle"] = GetSpotOuterAngle();
    j["SpotSoftness"] = GetSpotSoftness();

    j["CullRadius"] = GetCullRadius();

    return j;
}
