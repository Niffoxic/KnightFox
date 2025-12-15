#include "pch.h"
#include "engine/render_manager/light/directional_light.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "imgui/imgui.h"

using namespace DirectX;

namespace
{
    static inline float Clamp01(const float v) noexcept
    {
        return std::clamp(v, 0.0f, 1.0f);
    }

    static inline float ClampCos(const float v) noexcept
    {
        return std::clamp(v, -1.0f, 1.0f);
    }

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

    static inline float GetOrthoSizeFromRange(const float range) noexcept
    {
        return std::max(0.1f, range);
    }
}

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
    return "Infinite light source with a single direction. Used for sun/moon and large scale lighting.";
}

bool kfe::KFEDirectionalLight::CanCullByDistance() const
{
    return false;
}

void kfe::KFEDirectionalLight::UpdateLight(const KFECamera* camera)
{
    if (!camera)
        return;

    {
        const XMFLOAT3 raw = GetDirectionWS();
        XMVECTOR d = XMVectorSet(raw.x, raw.y, raw.z, 0.0f);
        d = NormalizeSafe(d);

        XMFLOAT3 dn{};
        XMStoreFloat3(&dn, d);

        SetDirectionWS(dn);
    }

    const XMFLOAT3 dirNf = GetDirectionWSNormalized();
    const XMVECTOR dirN = XMVectorSet(dirNf.x, dirNf.y, dirNf.z, 0.0f);

    // Camera position / forward
    const XMFLOAT3 camPosF = camera->GetPosition();
    const XMVECTOR camPos = XMLoadFloat3(&camPosF);

    XMVECTOR camFwd = camera->GetForwardVector();
    camFwd = NormalizeSafe(camFwd);

    // Directional shadow params:
    const float nearZ = std::max(0.0001f, GetShadowNearZ());
    const float shadowDist = std::max(0.1f, GetShadowDistance()); // base maps to ShadowFarZ
    const float orthoSize = GetOrthoSizeFromRange(GetRange());       // convention: Range = ortho size

    // Place the shadow frustum centered in front of the camera.
    const XMVECTOR center = XMVectorAdd(camPos, XMVectorScale(camFwd, shadowDist * 0.5f));
    const XMVECTOR eye = XMVectorSubtract(center, XMVectorScale(dirN, shadowDist));

    const XMVECTOR up = ChooseUpVector(dirN);

    // View + Ortho proj
    const XMMATRIX V = XMMatrixLookAtLH(eye, center, up);
    const XMMATRIX P = XMMatrixOrthographicLH(
        orthoSize * 2.0f,
        orthoSize * 2.0f,
        nearZ,
        shadowDist);

    // Update base CPU matrices (and mark dirty)
    SetLightView(V);
    SetLightProj(P);

    {
        XMFLOAT3 eyeF{};
        XMStoreFloat3(&eyeF, eye);
        SetPositionWS(eyeF);
    }
}

void kfe::KFEDirectionalLight::ImguiView(float dt)
{
    if (!ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    // Name
    {
        char nameBuf[128]{};
        const std::string cur = GetLightName();

        strncpy_s(
            nameBuf,
            sizeof(nameBuf),
            cur.c_str(),
            _TRUNCATE
        );

        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
            SetLightName(std::string{ nameBuf });
    }

    // Enabled flag
    {
        bool enabled = IsEnable();
        if (ImGui::Checkbox("Enabled", &enabled))
        {
            if (enabled) Enable();
            else Disable();
        }
    }

    // Cast shadows flag
    {
        const std::uint32_t flags = GetFlags();
        bool castShadow = (flags & KFE_LIGHT_FLAG_CAST_SHADOW) != 0u;

        if (ImGui::Checkbox("Cast Shadow", &castShadow))
        {
            if (castShadow) AddFlags(KFE_LIGHT_FLAG_CAST_SHADOW);
            else RemoveFlags(KFE_LIGHT_FLAG_CAST_SHADOW);
        }
    }

    // Shadow technique (NONE / CSM)
    {
        int tech = static_cast<int>(GetShadowTech());
        const char* items[] = { "None", "Directional CSM", "Spot 2D (invalid here)", "Point Cube (invalid here)" };

        if (ImGui::Combo("Shadow Tech", &tech, items, IM_ARRAYSIZE(items)))
        {
            if (tech != static_cast<int>(KFE_SHADOW_NONE) && tech != static_cast<int>(KFE_SHADOW_DIR_CSM))
                tech = static_cast<int>(KFE_SHADOW_DIR_CSM);

            SetShadowTech(static_cast<KFE_SHADOW_TECH>(tech));
        }
    }

    // Intensity + color
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

    // Shadows
    {
        float strength = GetShadowStrength();
        if (ImGui::SliderFloat("Shadow Strength", &strength, 0.0f, 1.0f))
            SetShadowStrength(strength);

        float bias = GetShadowBias();
        if (ImGui::DragFloat("Shadow Bias", &bias, 0.00001f, 0.0f, 0.1f, "%.6f"))
            SetShadowBias(bias);

        float nb = GetNormalBias();
        if (ImGui::DragFloat("Normal Bias", &nb, 0.0001f, 0.0f, 10.0f, "%.6f"))
            SetNormalBias(nb);

        float nearZ = GetShadowNearZ();
        if (ImGui::DragFloat("Shadow NearZ", &nearZ, 0.01f, 0.0001f, 100000.0f))
            SetShadowNearZ(nearZ);

        float farZ = GetShadowDistance(); // maps to ShadowFarZ
        if (ImGui::DragFloat("Shadow Distance", &farZ, 0.1f, 0.1f, 100000.0f))
            SetShadowDistance(farZ);

        float filter = GetShadowFilterRadius();
        if (ImGui::DragFloat("Filter Radius", &filter, 0.01f, 0.0f, 50.0f))
            SetShadowFilterRadius(filter);
    }

    // Directional Ortho Size
    {
        float orthoSize = GetOrthoSizeFromRange(GetRange());
        if (ImGui::DragFloat("Ortho Size", &orthoSize, 0.1f, 0.1f, 100000.0f))
            SetRange(std::max(0.1f, orthoSize));
        ImGui::TextDisabled("(Directional convention: OrthoSize is stored in Range)");
    }

    // Shadow map size
    {
        int w = static_cast<int>(GetShadowMapWidth());
        int h = static_cast<int>(GetShadowMapHeight());

        if (w <= 0) w = 2048;
        if (h <= 0) h = 2048;

        if (ImGui::DragInt("ShadowMap Width", &w, 1.0f, 1, 16384))
            SetShadowMapSize(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h));

        if (ImGui::DragInt("ShadowMap Height", &h, 1.0f, 1, 16384))
            SetShadowMapSize(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h));

        const auto inv = GetLightData().InvShadowMapSize;
        ImGui::Text("InvShadowMapSize: %.6f, %.6f", inv.x, inv.y);
    }

    // Cascades
    {
        XMFLOAT4 splits = GetCascadeSplits();
        float s[4]{ splits.x, splits.y, splits.z, splits.w };

        if (ImGui::DragFloat4("Cascade Splits", s, 0.001f, 0.0f, 1.0f))
            SetCascadeSplits({ s[0], s[1], s[2], s[3] });

        ImGui::TextDisabled("(CSM matrices are computed in UpdateLight when implemented.)");
    }

    // Cull radius is irrelevant for directional
    {
        float cull = GetCullRadius();
        ImGui::BeginDisabled(true);
        ImGui::DragFloat("Cull Radius", &cull, 0.1f, 0.0f, 100000.0f);
        ImGui::EndDisabled();
        ImGui::Text("Directional lights are not distance-culled.");
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

    if (loader.Has("Flags"))
        SetFlags(loader["Flags"].AsUInt());

    if (loader.Has("ShadowTech"))
        SetShadowTech(static_cast<KFE_SHADOW_TECH>(loader["ShadowTech"].AsUInt()));

    if (loader.Has("ShadowMapId"))
        SetShadowMapId(loader["ShadowMapId"].AsUInt());

    if (loader.Has("DirectionWS"))
        SetDirectionWS(ReadFloat3(loader, "DirectionWS", { 0.0f, -1.0f, 0.0f }));
    else if (loader.Has("DirectionWSNormalized"))
        SetDirectionWS(ReadFloat3(loader, "DirectionWSNormalized", { 0.0f, -1.0f, 0.0f }));

    if (loader.Has("Color"))
        SetColor(ReadFloat3(loader, "Color", { 1.0f, 1.0f, 1.0f }));

    if (loader.Has("Intensity"))        SetIntensity(loader["Intensity"].AsFloat());
    if (loader.Has("ShadowStrength"))   SetShadowStrength(loader["ShadowStrength"].AsFloat());
    if (loader.Has("ShadowBias"))       SetShadowBias(loader["ShadowBias"].AsFloat());
    if (loader.Has("NormalBias"))       SetNormalBias(loader["NormalBias"].AsFloat());
    if (loader.Has("ShadowNearZ"))      SetShadowNearZ(loader["ShadowNearZ"].AsFloat());
    if (loader.Has("ShadowFarZ"))       SetShadowFarZ(loader["ShadowFarZ"].AsFloat());
    if (loader.Has("ShadowFilterRadius")) SetShadowFilterRadius(loader["ShadowFilterRadius"].AsFloat());

    // Directional OrthoSize convention stored in Range
    if (loader.Has("OrthoSize"))
    {
        const float ortho = std::max(0.1f, loader["OrthoSize"].AsFloat());
        SetRange(ortho);
    }

    if (loader.Has("ShadowMapSize"))
    {
        const std::uint32_t w = loader["ShadowMapSize"]["w"].AsUInt();
        const std::uint32_t h = loader["ShadowMapSize"]["h"].AsUInt();
        SetShadowMapSize(w, h);
    }

    if (loader.Has("CascadeSplits"))
    {
        XMFLOAT4 s{};
        s.x = loader["CascadeSplits"]["x"].AsFloat();
        s.y = loader["CascadeSplits"]["y"].AsFloat();
        s.z = loader["CascadeSplits"]["z"].AsFloat();
        s.w = loader["CascadeSplits"]["w"].AsFloat();
        SetCascadeSplits(s);
    }

    // Enforce directional invariants
    SetLightType(KFE_LIGHT_DIRECTIONAL);
    SetAttenuation(1.0f);
    SetSpotInnerAngle(0.0f);
    SetSpotOuterAngle(0.0f);
    SetCullRadius(0.0f);
}

JsonLoader kfe::KFEDirectionalLight::GetJsonData() const
{
    JsonLoader j{};

    j["Type"] = ToString(KFE_LIGHT_DIRECTIONAL);

    j["LightName"] = GetLightName();
    j["Enabled"] = IsEnable();
    j["Flags"] = std::to_string(GetFlags());
    j["ShadowTech"] = std::to_string(static_cast<std::uint32_t>(GetShadowTech()));
    j["ShadowMapId"] = std::to_string(GetShadowMapId());

    WriteFloat3(j, "DirectionWS", GetDirectionWSNormalized());
    WriteFloat3(j, "Color", GetColor());

    j["Intensity"] = GetIntensity();
    j["ShadowStrength"] = GetShadowStrength();
    j["ShadowBias"] = GetShadowBias();
    j["NormalBias"] = GetNormalBias();

    j["ShadowNearZ"] = GetShadowNearZ();
    j["ShadowFarZ"] = GetShadowFarZ();
    j["ShadowFilterRadius"] = GetShadowFilterRadius();

    // Directional OrthoSize stored in Range
    j["OrthoSize"] = GetOrthoSizeFromRange(GetRange());

    {
        const std::uint32_t w = GetShadowMapWidth();
        const std::uint32_t h = GetShadowMapHeight();
        j["ShadowMapSize"]["w"] = std::to_string(w);
        j["ShadowMapSize"]["h"] = std::to_string(h);
    }

    {
        const XMFLOAT4 s = GetCascadeSplits();
        j["CascadeSplits"]["x"] = s.x;
        j["CascadeSplits"]["y"] = s.y;
        j["CascadeSplits"]["z"] = s.z;
        j["CascadeSplits"]["w"] = s.w;
    }

    return j;
}

void kfe::KFEDirectionalLight::ResetToDefaults()
{
    SetLightType(KFE_LIGHT_DIRECTIONAL);

    // Common
    SetLightName("Directional");
    SetDirectionWS({ 0.0f, -1.0f, 0.0f });
    SetColor({ 1.0f, 1.0f, 1.0f });
    SetIntensity(1.0f);

    // Flags / tech
    SetFlags(KFE_LIGHT_FLAG_ENABLED | KFE_LIGHT_FLAG_CAST_SHADOW);
    SetShadowTech(KFE_SHADOW_NONE);
    SetShadowMapId(0u);

    // Shadows
    SetShadowStrength(1.0f);
    SetShadowBias(0.0005f);
    SetNormalBias(0.01f);

    SetShadowNearZ(0.1f);
    SetShadowDistance(60.0f); // base maps to ShadowFarZ

    SetShadowFilterRadius(1.0f);

    // Directional convention: Range == OrthoSize
    SetRange(25.0f);

    // Shadow map size
    SetShadowMapSize(2048u, 2048u);

    // Not used for directional but keep sane
    SetPositionWS({ 0.0f, 0.0f, 0.0f });
    SetAttenuation(1.0f);
    SetSpotInnerAngle(0.0f);
    SetSpotOuterAngle(0.0f);
    SetSpotSoftness(0.0f);

    // CSM splits default (tune later)
    SetCascadeSplits({ 0.05f, 0.15f, 0.35f, 1.0f });

    SetLightView(XMMatrixIdentity());
    SetLightProj(XMMatrixIdentity());

    SetCullRadius(0.0f);
}
