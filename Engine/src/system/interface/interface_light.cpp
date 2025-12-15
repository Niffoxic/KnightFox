#include "pch.h"
#include "engine/system/interface/interface_light.h"

#include "engine/render_manager/scene/cube_scene.h"
#include "engine/render_manager/components/render_queue.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

static inline float KFE_Clamp01(const float v) noexcept
{
    return std::clamp(v, 0.0f, 1.0f);
}

static inline float KFE_ClampCos(const float v) noexcept
{
    return std::clamp(v, -1.0f, 1.0f);
}

void kfe::IKFELight::EnsurePacked_() const noexcept
{
    if (!m_bDirty)
        return;

    m_lightData.LightType = GetLightTypeValue();

    {
        const XMVECTOR d = XMVectorSet(
            m_lightData.DirectionWS.x,
            m_lightData.DirectionWS.y,
            m_lightData.DirectionWS.z,
            0.0f);

        const XMVECTOR dn = NormalizeSafe(d);
        XMStoreFloat3(&m_lightData.DirectionWSNormalized, dn);
    }

    if (m_lightData.SpotInnerCos < m_lightData.SpotOuterCos)
        std::swap(m_lightData.SpotInnerCos, m_lightData.SpotOuterCos);

    m_bDirty = false;
}


kfe::IKFELight::IKFELight()
{

    m_lightData.LightType = 1.0f;
    m_lightData.Intensity = 1.0f;
    m_lightData.Range = 10.0f;
    m_lightData.Attenuation = 1.0f;

    m_lightData.PositionWS = { 0.0f, 0.0f, 0.0f };
    m_lightData.DirectionWS = { 0.0f, -1.0f, 0.0f };
    m_lightData.DirectionWSNormalized = { 0.0f, -1.0f, 0.0f };
    m_lightData.Color = { 1.0f, 1.0f, 1.0f };

    m_lightData.SpotInnerCos = std::cos(0.5f);
    m_lightData.SpotOuterCos = std::cos(0.75f);
    m_lightData.SpotSoftness = 0.0f;

    m_bDirty = true;
}

kfe::IKFELight::~IKFELight()
{
}

kfe::IKFELight::IKFELight(IKFELight&&) noexcept = default;
kfe::IKFELight& kfe::IKFELight::operator=(IKFELight&&) noexcept = default;

void kfe::IKFELight::Enable()
{
    if (m_enabled)
        return;

    m_enabled = true;
    m_bDirty = true;
}

void kfe::IKFELight::Disable()
{
    if (!m_enabled)
        return;

    m_enabled = false;
}

_Use_decl_annotations_
bool kfe::IKFELight::IsEnable() const
{
    return m_enabled;
}

void kfe::IKFELight::Update(const KFECamera* camera)
{
    if (!m_enabled)
        return;

    UpdateLight(camera);
    EnsurePacked_();
}

_Use_decl_annotations_
kfe::KFE_LIGHT_DATA_GPU kfe::IKFELight::GetLightData() const
{
    EnsurePacked_();
    return m_lightData;
}

void kfe::IKFELight::SetCullRadius(float r)
{
    m_cullRadius = std::max(0.0f, r);
}

float kfe::IKFELight::GetCullRadius() const
{
    return m_cullRadius;
}

_Use_decl_annotations_
float kfe::IKFELight::DistanceFromPoint(const DirectX::XMFLOAT3& p) const
{
    const float dx = p.x - m_lightData.PositionWS.x;
    const float dy = p.y - m_lightData.PositionWS.y;
    const float dz = p.z - m_lightData.PositionWS.z;
    return std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
}

_Use_decl_annotations_
float kfe::IKFELight::DistanceFromPointSq(const DirectX::XMFLOAT3& p) const
{
    const float dx = p.x - m_lightData.PositionWS.x;
    const float dy = p.y - m_lightData.PositionWS.y;
    const float dz = p.z - m_lightData.PositionWS.z;
    return (dx * dx) + (dy * dy) + (dz * dz);
}

_Use_decl_annotations_
bool kfe::IKFELight::IsInCullRadius(const DirectX::XMFLOAT3& p) const
{
    if (!CanCullByDistance())
        return true;

    const float r = m_cullRadius;
    if (r <= 0.0f)
        return false;

    return DistanceFromPointSq(p) <= (r * r);
}

_Use_decl_annotations_
bool kfe::IKFELight::IsInCullRadiusSq(const DirectX::XMFLOAT3& p) const
{
    if (!CanCullByDistance())
        return true;

    const float r = m_cullRadius;
    if (r <= 0.0f)
        return false;

    return DistanceFromPointSq(p) <= (r * r);
}

void kfe::IKFELight::SetIntensity(float v)
{
    m_lightData.Intensity = std::max(0.0f, v);
    m_bDirty = true;
}

float kfe::IKFELight::GetIntensity() const
{
    return m_lightData.Intensity;
}

void kfe::IKFELight::SetColor(const DirectX::XMFLOAT3& color)
{
    m_lightData.Color = color;
    m_bDirty = true;
}

DirectX::XMFLOAT3 kfe::IKFELight::GetColor() const
{
    return m_lightData.Color;
}

void kfe::IKFELight::SetPositionWS(const DirectX::XMFLOAT3& pos)
{
    m_lightData.PositionWS = pos;

    m_bDirty = true;
}

DirectX::XMFLOAT3 kfe::IKFELight::GetPositionWS() const
{
    return m_lightData.PositionWS;
}

void kfe::IKFELight::SetDirectionWS(const DirectX::XMFLOAT3& dir)
{
    m_lightData.DirectionWS = dir;

    const XMVECTOR d = XMVectorSet(dir.x, dir.y, dir.z, 0.0f);
    const XMVECTOR dn = NormalizeSafe(d);
    XMStoreFloat3(&m_lightData.DirectionWSNormalized, dn);

    m_bDirty = true;
}

DirectX::XMFLOAT3 kfe::IKFELight::GetDirectionWS() const
{
    return m_lightData.DirectionWS;
}

DirectX::XMFLOAT3 kfe::IKFELight::GetDirectionWSNormalized() const
{
    return m_lightData.DirectionWSNormalized;
}

void kfe::IKFELight::SetRange(float v)
{
    m_lightData.Range = std::max(0.0f, v);
    m_bDirty = true;
}

float kfe::IKFELight::GetRange() const
{
    return m_lightData.Range;
}

void kfe::IKFELight::SetAttenuation(float v)
{
    m_lightData.Attenuation = std::max(0.0f, v);
    m_bDirty = true;
}

float kfe::IKFELight::GetAttenuation() const
{
    return m_lightData.Attenuation;
}

void kfe::IKFELight::SetSpotInnerAngle(float radians)
{
    const float r = std::max(0.0f, radians);
    m_lightData.SpotInnerCos = std::cos(r);
    m_bDirty = true;
}

float kfe::IKFELight::GetSpotInnerAngle() const
{
    return std::acos(KFE_ClampCos(m_lightData.SpotInnerCos));
}

void kfe::IKFELight::SetSpotOuterAngle(float radians)
{
    const float r = std::max(0.0f, radians);
    m_lightData.SpotOuterCos = std::cos(r);
    m_bDirty = true;
}

float kfe::IKFELight::GetSpotOuterAngle() const
{
    return std::acos(KFE_ClampCos(m_lightData.SpotOuterCos));
}

void kfe::IKFELight::SetSpotSoftness(float v)
{
    m_lightData.SpotSoftness = KFE_Clamp01(v);
    m_bDirty = true;
}

float kfe::IKFELight::GetSpotSoftness() const
{
    return m_lightData.SpotSoftness;
}

DirectX::XMVECTOR kfe::IKFELight::NormalizeSafe(DirectX::FXMVECTOR v) noexcept
{
    const XMVECTOR lenSq = XMVector3LengthSq(v);
    const float ls = XMVectorGetX(lenSq);

    if (ls < 1e-12f)
        return XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

    return XMVector3Normalize(v);
}

DirectX::XMVECTOR kfe::IKFELight::ChooseUpVector(DirectX::FXMVECTOR dir) noexcept
{
    const float y = std::fabs(XMVectorGetY(dir));
    if (y > 0.99f)
        return XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    return XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
}

void kfe::IKFELight::SetLightName(const std::string& name)
{
    m_lightName = name;
}

std::string kfe::IKFELight::GetLightName() const
{
    return m_lightName;
}
