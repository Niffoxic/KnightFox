#include "pch.h"
#include "engine/render_manager/light/directional_light.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "imgui/imgui.h"

using namespace DirectX;

namespace
{
    static inline void WriteFloat3(JsonLoader& j, const char* key, const XMFLOAT3& v) noexcept
    {
        j[key]["x"] = v.x;
        j[key]["y"] = v.y;
        j[key]["z"] = v.z;
    }

    static inline XMFLOAT3 ReadFloat3(const JsonLoader& j, const char* key, const XMFLOAT3& fallback = { 0,0,0 }) noexcept
    {
        if (!j.Has(key))
            return fallback;

        XMFLOAT3 out{};
        out.x = j[key]["x"].AsFloat();
        out.y = j[key]["y"].AsFloat();
        out.z = j[key]["z"].AsFloat();
        return out;
    }
}

kfe::KFEDirectionalLight::KFEDirectionalLight()
{
    ResetToDefaults();
}

kfe::KFEDirectionalLight::~KFEDirectionalLight() = default;

std::string kfe::KFEDirectionalLight::GetName() const noexcept
{
    return "KFEDirectionalLight";
}

std::string kfe::KFEDirectionalLight::GetDescription() const noexcept
{
    return "Infinite light source with a single direction. Used for sun/moon and large scale lighting.";
}

bool kfe::KFEDirectionalLight::CanCullByDistance() const
{
    return false;
}

void kfe::KFEDirectionalLight::UpdateLight(const KFECamera* camera)
{
    (void)camera;

    const XMFLOAT3 raw = GetDirectionWS();
    XMVECTOR d = XMVectorSet(raw.x, raw.y, raw.z, 0.0f);
    d = NormalizeSafe(d);

    XMFLOAT3 dn{};
    XMStoreFloat3(&dn, d);
    SetDirectionWS(dn);
}

std::string kfe::KFEDirectionalLight::GetLightType() const
{
    return "KFEDirectionalLight";
}

void kfe::KFEDirectionalLight::ImguiView(float dt)
{
    (void)dt;

    if (!ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
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

    {
        float cull = GetCullRadius();
        ImGui::BeginDisabled(true);
        ImGui::DragFloat("Cull Radius", &cull, 0.1f, 0.0f, 100000.0f);
        ImGui::EndDisabled();
        ImGui::TextDisabled("Directional lights are not distance-culled.");
    }

    if (ImGui::Button("Reset"))
        ResetToDefaults();
}

void kfe::KFEDirectionalLight::LoadFromJson(const JsonLoader& loader)
{
    if (loader.Has("LightName"))
        SetLightName(loader["LightName"].GetValue());

    if (loader.Has("Enabled"))
    {
        const bool en = loader["Enabled"].AsBool();
        if (en) Enable();
        else Disable();
    }

    if (loader.Has("DirectionWS"))
        SetDirectionWS(ReadFloat3(loader, "DirectionWS", { 0.0f, -1.0f, 0.0f }));
    else if (loader.Has("DirectionWSNormalized"))
        SetDirectionWS(ReadFloat3(loader, "DirectionWSNormalized", { 0.0f, -1.0f, 0.0f }));

    if (loader.Has("Color"))
        SetColor(ReadFloat3(loader, "Color", { 1.0f, 1.0f, 1.0f }));

    if (loader.Has("Intensity"))
        SetIntensity(loader["Intensity"].AsFloat());

    SetPositionWS({ 0.0f, 0.0f, 0.0f });
    SetRange(0.0f);
    SetAttenuation(1.0f);

    SetSpotInnerAngle(0.0f);
    SetSpotOuterAngle(0.0f);
    SetSpotSoftness(0.0f);

    SetCullRadius(0.0f);
}

JsonLoader kfe::KFEDirectionalLight::GetJsonData() const
{
    JsonLoader j{};

    j["Type"] = GetLightType();

    j["LightName"] = GetLightName();
    j["Enabled"] = IsEnable();

    WriteFloat3(j, "DirectionWS", GetDirectionWSNormalized());
    WriteFloat3(j, "Color", GetColor());

    j["Intensity"] = GetIntensity();

    return j;
}

void kfe::KFEDirectionalLight::ResetToDefaults()
{
    SetLightName("Directional");

    SetDirectionWS({ 0.0f, -1.0f, 0.0f });
    SetColor({ 1.0f, 1.0f, 1.0f });
    SetIntensity(1.0f);

    // Directional invariants
    SetPositionWS({ 0.0f, 0.0f, 0.0f });
    SetRange(0.0f);
    SetAttenuation(1.0f);

    // Spot params unused
    SetSpotInnerAngle(0.0f);
    SetSpotOuterAngle(0.0f);
    SetSpotSoftness(0.0f);

    SetCullRadius(0.0f);
}
