#include "pch.h"
#include "engine/render_manager/light/point_light.h"

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

    SetFlags(KFE_LIGHT_FLAG_ENABLED);
    SetShadowTech(KFE_SHADOW_NONE);
    SetShadowMapId(0u);

    SetPositionWS({ 0.0f, 2.0f, 0.0f });
    SetDirectionWS({ 0.0f, -1.0f, 0.0f });

    SetColor({ 1.0f, 1.0f, 1.0f });
    SetIntensity(3.0f);

    SetRange(12.0f);
    SetAttenuation(1.0f);

    SetSpotInnerAngle(0.0f);
    SetSpotOuterAngle(0.0f);
    SetSpotSoftness(0.0f);

    SetShadowStrength(0.0f);
    SetShadowBias(0.0005f);
    SetNormalBias(0.01f);

    SetShadowNearZ(0.1f);
    SetShadowFarZ(100.0f);

    SetShadowFilterRadius(1.0f);
    SetShadowTexelSize(0.0f);
    SetShadowFadeStart(0.0f);
    SetShadowFadeEnd(0.0f);

    SetShadowAtlasUV({ 1.0f, 1.0f }, { 0.0f, 0.0f });

    SetShadowDistance(0.0f);
    SetOrthoSize(0.0f);

    SetShadowMapSize(0u, 0u);

    SetLightView(XMMatrixIdentity());
    SetLightProj(XMMatrixIdentity());

    SetCascadeSplits({ 0.0f, 0.0f, 0.0f, 0.0f });

    SetCullRadius(40.0f);
}

void kfe::KFEPointLight::UpdateLight(const KFECamera*)
{
    //~ sanitize core params
    SetIntensity(std::max(0.0f, GetIntensity()));
    SetAttenuation(std::max(0.0f, GetAttenuation()));
    SetRange(std::max(0.0f, GetRange()));

    SetShadowStrength(KFE_Clamp01(GetShadowStrength()));
    SetShadowBias(std::max(0.0f, GetShadowBias()));
    SetNormalBias(std::max(0.0f, GetNormalBias()));

    SetShadowNearZ(std::max(0.0001f, GetShadowNearZ()));
    SetShadowFarZ(std::max(GetShadowNearZ() + 0.001f, GetShadowFarZ()));

    SetShadowFilterRadius(std::max(0.0f, GetShadowFilterRadius()));
    SetShadowTexelSize(std::max(0.0f, GetShadowTexelSize()));
    SetShadowFadeStart(std::max(0.0f, GetShadowFadeStart()));
    SetShadowFadeEnd(std::max(GetShadowFadeStart(), GetShadowFadeEnd()));

    //~ point lights don't use spot params
    SetSpotInnerAngle(0.0f);
    SetSpotOuterAngle(0.0f);
    SetSpotSoftness(0.0f);

    //~ not used for point
    SetShadowDistance(0.0f);
    SetOrthoSize(0.0f);

    //~ keep direction normalized (even if not used heavily)
    {
        const XMFLOAT3 d = GetDirectionWS();
        XMVECTOR v = XMVectorSet(d.x, d.y, d.z, 0.0f);
        v = NormalizeSafe(v);
        XMFLOAT3 out{};
        XMStoreFloat3(&out, v);
        SetDirectionWS(out);
    }

    //~ enforce tech for point
    {
        const KFE_SHADOW_TECH tech = GetShadowTech();
        if (tech != KFE_SHADOW_NONE && tech != KFE_SHADOW_POINT_CUBE)
            SetShadowTech(KFE_SHADOW_POINT_CUBE);
    }

    //~ point shadow matrices are per-face; keep CPU view/proj identity here
    SetLightView(XMMatrixIdentity());
    SetLightProj(XMMatrixIdentity());
}

void kfe::KFEPointLight::ImguiView(float)
{
    if (!ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    {
        char nameBuf[128]{};
        const std::string cur = GetLightName();
        strncpy_s(nameBuf, cur.c_str(), _TRUNCATE);
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
            SetLightName(std::string{ nameBuf });
    }

    {
        bool enabled = IsEnable();
        if (ImGui::Checkbox("Enabled", &enabled))
        {
            if (enabled) Enable();
            else Disable();
        }
    }

    {
        std::uint32_t flags = GetFlags();
        bool castShadow = (flags & KFE_LIGHT_FLAG_CAST_SHADOW) != 0u;

        if (ImGui::Checkbox("Cast Shadow", &castShadow))
        {
            if (castShadow) AddFlags(KFE_LIGHT_FLAG_CAST_SHADOW);
            else RemoveFlags(KFE_LIGHT_FLAG_CAST_SHADOW);
        }

        bool volumetric = (GetFlags() & KFE_LIGHT_FLAG_VOLUMETRIC) != 0u;
        if (ImGui::Checkbox("Volumetric", &volumetric))
        {
            if (volumetric) AddFlags(KFE_LIGHT_FLAG_VOLUMETRIC);
            else RemoveFlags(KFE_LIGHT_FLAG_VOLUMETRIC);
        }
    }

    {
        int tech = static_cast<int>(GetShadowTech());
        const char* items[] = { "None", "Dir CSM (invalid)", "Spot 2D (invalid)", "Point Cube" };

        if (tech < 0) tech = 0;
        if (tech > 3) tech = 3;

        if (ImGui::Combo("Shadow Tech", &tech, items, IM_ARRAYSIZE(items)))
        {
            if (tech != static_cast<int>(KFE_SHADOW_NONE) && tech != static_cast<int>(KFE_SHADOW_POINT_CUBE))
                tech = static_cast<int>(KFE_SHADOW_POINT_CUBE);

            SetShadowTech(static_cast<KFE_SHADOW_TECH>(tech));
        }
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

        float nearZ = GetShadowNearZ();
        if (ImGui::DragFloat("ShadowNearZ", &nearZ, 0.01f, 0.0001f, 100000.0f))
            SetShadowNearZ(nearZ);

        float farZ = GetShadowFarZ();
        if (ImGui::DragFloat("ShadowFarZ", &farZ, 0.1f, 0.0001f, 100000.0f))
            SetShadowFarZ(farZ);

        float filter = GetShadowFilterRadius();
        if (ImGui::DragFloat("FilterRadius", &filter, 0.01f, 0.0f, 50.0f))
            SetShadowFilterRadius(filter);
    }

    {
        int w = static_cast<int>(GetShadowMapWidth());
        int h = static_cast<int>(GetShadowMapHeight());

        if (w <= 0) w = 1024;
        if (h <= 0) h = 1024;

        if (ImGui::DragInt("ShadowMapWidth", &w, 1.0f, 1, 16384) ||
            ImGui::DragInt("ShadowMapHeight", &h, 1.0f, 1, 16384))
        {
            SetShadowMapSize(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h));
        }

        const auto inv = GetLightData().InvShadowMapSize;
        ImGui::Text("InvShadowMapSize: %.6f, %.6f", inv.x, inv.y);
    }

    if (ImGui::Button("Reset"))
        ResetToDefaults();
}

void kfe::KFEPointLight::LoadFromJson(const JsonLoader& loader)
{
    ResetToDefaults();

    if (loader.Has("LightName")) SetLightName(loader["LightName"].GetValue());
    if (loader.Has("Flags"))     SetFlags(loader["Flags"].AsUInt());

    if (loader.Has("ShadowTech"))
    {
        const auto t = static_cast<KFE_SHADOW_TECH>(loader["ShadowTech"].AsUInt());
        if (t == KFE_SHADOW_NONE || t == KFE_SHADOW_POINT_CUBE)
            SetShadowTech(t);
        else
            SetShadowTech(KFE_SHADOW_POINT_CUBE);
    }

    if (loader.Has("ShadowMapId"))
        SetShadowMapId(loader["ShadowMapId"].AsUInt());

    XMFLOAT3 v{};
    if (KFE_ReadFloat3(loader, "PositionWS", v)) SetPositionWS(v);
    if (KFE_ReadFloat3(loader, "DirectionWS", v) || KFE_ReadFloat3(loader, "DirectionWSNormalized", v)) SetDirectionWS(v);
    if (KFE_ReadFloat3(loader, "Color", v)) SetColor(v);

    if (loader.Has("Intensity"))   SetIntensity(loader["Intensity"].AsFloat());
    if (loader.Has("Range"))       SetRange(loader["Range"].AsFloat());
    if (loader.Has("Attenuation")) SetAttenuation(loader["Attenuation"].AsFloat());

    if (loader.Has("ShadowStrength"))     SetShadowStrength(loader["ShadowStrength"].AsFloat());
    if (loader.Has("ShadowBias"))         SetShadowBias(loader["ShadowBias"].AsFloat());
    if (loader.Has("NormalBias"))         SetNormalBias(loader["NormalBias"].AsFloat());
    if (loader.Has("ShadowNearZ"))        SetShadowNearZ(loader["ShadowNearZ"].AsFloat());
    if (loader.Has("ShadowFarZ"))         SetShadowFarZ(loader["ShadowFarZ"].AsFloat());
    if (loader.Has("ShadowFilterRadius")) SetShadowFilterRadius(loader["ShadowFilterRadius"].AsFloat());

    if (loader.Has("ShadowMapSize"))
    {
        const std::uint32_t w = loader["ShadowMapSize"]["w"].AsUInt();
        const std::uint32_t h = loader["ShadowMapSize"]["h"].AsUInt();
        SetShadowMapSize(w, h);
    }

    if (loader.Has("CullRadius"))
        SetCullRadius(loader["CullRadius"].AsFloat());

    SetLightType(KFE_LIGHT_POINT);

    //~ enforce point invariants
    SetSpotInnerAngle(0.0f);
    SetSpotOuterAngle(0.0f);
    SetSpotSoftness(0.0f);

    SetShadowDistance(0.0f);
    SetOrthoSize(0.0f);

    if (GetShadowTech() != KFE_SHADOW_NONE && GetShadowTech() != KFE_SHADOW_POINT_CUBE)
        SetShadowTech(KFE_SHADOW_POINT_CUBE);
}

JsonLoader kfe::KFEPointLight::GetJsonData() const
{
    JsonLoader j{};

    j["Type"] = ToString(KFE_LIGHT_POINT);

    j["LightName"] = GetLightName();
    j["Flags"] = std::to_string(GetFlags());
    j["ShadowTech"] = std::to_string(static_cast<std::uint32_t>(GetShadowTech()));
    j["ShadowMapId"] = std::to_string(GetShadowMapId());

    KFE_WriteFloat3(j, "PositionWS", GetPositionWS());
    KFE_WriteFloat3(j, "DirectionWS", GetDirectionWSNormalized());
    KFE_WriteFloat3(j, "Color", GetColor());

    j["Intensity"] = GetIntensity();
    j["Range"] = GetRange();
    j["Attenuation"] = GetAttenuation();

    j["ShadowStrength"] = GetShadowStrength();
    j["ShadowBias"] = GetShadowBias();
    j["NormalBias"] = GetNormalBias();
    j["ShadowNearZ"] = GetShadowNearZ();
    j["ShadowFarZ"] = GetShadowFarZ();
    j["ShadowFilterRadius"] = GetShadowFilterRadius();

    j["ShadowMapSize"]["w"] = std::to_string(GetShadowMapWidth());
    j["ShadowMapSize"]["h"] = std::to_string(GetShadowMapHeight());

    j["CullRadius"] = GetCullRadius();

    return j;
}
