#include "pch.h"
#include "engine/system/interface/interface_light.h"

#include "engine/render_manager/scene/cube_scene.h"
#include "engine/render_manager/components/render_queue.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

kfe::IKFELight::IKFELight()
{
    m_debugCube = std::make_unique<KEFCubeSceneObject>();
    m_debugCube->SetDrawMode(EDrawMode::WireFrame);
    KFERenderQueue::Instance().AddSceneObject(m_debugCube.get());

    m_lightData.Type = static_cast<float>(KFE_LIGHT_DIRECTIONAL);
    m_lightData.Intensity = 1.0f;
    m_lightData.Range = 10.0f;
    m_lightData.ShadowStrength = 1.0f;

    m_lightData.PositionWS = { 0.0f, 0.0f, 0.0f };
    m_lightData.DirectionWS = { 0.0f, -1.0f, 0.0f };
    m_lightData.Color = { 1.0f, 1.0f, 1.0f };

    m_lightData.NormalBias = 0.01f;
    m_lightData.ShadowBias = 0.0005f;

    m_lightData.Attenuation = 1.0f;
    m_lightData.SpotInnerAngle = 0.5f;
    m_lightData.SpotOuterAngle = 0.75f;

    m_lightData.ShadowDistance = 50.0f;
    m_lightData.OrthoSize = 20.0f;

    m_lightData.LightView = XMMatrixIdentity();
    m_lightData.LightProj = XMMatrixIdentity();
    m_lightData.LightViewProj = XMMatrixIdentity();

    m_lightData.InvShadowMapSize = { 0.0f, 0.0f };
    m_lightData.Padding0 = 0.0f;
    m_lightData.Padding1 = 0.0f;
}

kfe::IKFELight::~IKFELight()
{
    if (m_debugCube)
        KFERenderQueue::Instance().RemoveSceneObject(m_debugCube.get());
}

void kfe::IKFELight::Enable()
{
    if (m_enabled)
        return;

    m_enabled = true;

    if (m_debugCube)
        KFERenderQueue::Instance().AddSceneObject(m_debugCube.get());
}

void kfe::IKFELight::Disable()
{
    if (!m_enabled)
        return;

    m_enabled = false;

    if (m_debugCube)
        KFERenderQueue::Instance().RemoveSceneObject(m_debugCube.get());
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
}

_Use_decl_annotations_
kfe::KFE_LIGHT_DATA_DESC kfe::IKFELight::GetLightData() const
{
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

void kfe::IKFELight::SetLightType(KFE_LIGHT_TYPE type)
{
    m_lightData.Type = static_cast<float>(type);
}

_Use_decl_annotations_
kfe::KFE_LIGHT_TYPE kfe::IKFELight::GetLightType() const
{
    const float t = m_lightData.Type;
    const uint32_t ti = (t < 0.0f) ? 0u : static_cast<uint32_t>(t + 0.5f);

    switch (ti)
    {
    case 0u: return KFE_LIGHT_DIRECTIONAL;
    case 1u: return KFE_LIGHT_SPOT;
    case 2u: return KFE_LIGHT_POINT;
    default: break;
    }

    return KFE_LIGHT_DIRECTIONAL;
}

_Use_decl_annotations_
std::string kfe::IKFELight::GetLightTypeName() const
{
    return ToString(GetLightType());
}

void kfe::IKFELight::SetIntensity(float v)
{
    m_lightData.Intensity = std::max(0.0f, v);
}

float kfe::IKFELight::GetIntensity() const
{
    return m_lightData.Intensity;
}

void kfe::IKFELight::SetColor(const DirectX::XMFLOAT3& color)
{
    m_lightData.Color = color;
}

DirectX::XMFLOAT3 kfe::IKFELight::GetColor() const
{
    return m_lightData.Color;
}

void kfe::IKFELight::SetPositionWS(const DirectX::XMFLOAT3& pos)
{
    m_lightData.PositionWS = pos;

    if (m_debugCube && m_enabled)
        m_debugCube->SetPosition(pos);
}

DirectX::XMFLOAT3 kfe::IKFELight::GetPositionWS() const
{
    return m_lightData.PositionWS;
}

void kfe::IKFELight::SetDirectionWS(const DirectX::XMFLOAT3& dir)
{
    m_lightData.DirectionWS = dir;
}

DirectX::XMFLOAT3 kfe::IKFELight::GetDirectionWS() const
{
    return m_lightData.DirectionWS;
}

void kfe::IKFELight::SetRange(float v)
{
    m_lightData.Range = std::max(0.0f, v);
}

float kfe::IKFELight::GetRange() const
{
    return m_lightData.Range;
}

void kfe::IKFELight::SetAttenuation(float v)
{
    m_lightData.Attenuation = std::max(0.0f, v);
}

float kfe::IKFELight::GetAttenuation() const
{
    return m_lightData.Attenuation;
}

void kfe::IKFELight::SetSpotInnerAngle(float radians)
{
    m_lightData.SpotInnerAngle = std::max(0.0f, radians);
}

float kfe::IKFELight::GetSpotInnerAngle() const
{
    return m_lightData.SpotInnerAngle;
}

void kfe::IKFELight::SetSpotOuterAngle(float radians)
{
    m_lightData.SpotOuterAngle = std::max(0.0f, radians);
}

float kfe::IKFELight::GetSpotOuterAngle() const
{
    return m_lightData.SpotOuterAngle;
}

void kfe::IKFELight::SetShadowStrength(float v)
{
    m_lightData.ShadowStrength = std::clamp(v, 0.0f, 1.0f);
}

float kfe::IKFELight::GetShadowStrength() const
{
    return m_lightData.ShadowStrength;
}

void kfe::IKFELight::SetShadowBias(float v)
{
    m_lightData.ShadowBias = std::max(0.0f, v);
}

float kfe::IKFELight::GetShadowBias() const
{
    return m_lightData.ShadowBias;
}

void kfe::IKFELight::SetNormalBias(float v)
{
    m_lightData.NormalBias = std::max(0.0f, v);
}

float kfe::IKFELight::GetNormalBias() const
{
    return m_lightData.NormalBias;
}

void kfe::IKFELight::SetShadowDistance(float v)
{
    m_lightData.ShadowDistance = std::max(0.0f, v);
}

float kfe::IKFELight::GetShadowDistance() const
{
    return m_lightData.ShadowDistance;
}

void kfe::IKFELight::SetOrthoSize(float v)
{
    m_lightData.OrthoSize = std::max(0.0f, v);
}

float kfe::IKFELight::GetOrthoSize() const
{
    return m_lightData.OrthoSize;
}

void kfe::IKFELight::SetShadowMapSize(std::uint32_t w, std::uint32_t h)
{
    if (w == 0u || h == 0u)
    {
        m_lightData.InvShadowMapSize = { 0.0f, 0.0f };
        return;
    }

    m_lightData.InvShadowMapSize =
    {
        1.0f / static_cast<float>(w),
        1.0f / static_cast<float>(h)
    };
}

std::uint32_t kfe::IKFELight::GetShadowMapWidth() const
{
    const float inv = m_lightData.InvShadowMapSize.x;
    if (inv <= 0.0f)
        return 0u;

    const float w = 1.0f / inv;
    return (w <= 0.0f) ? 0u : static_cast<std::uint32_t>(w + 0.5f);
}

std::uint32_t kfe::IKFELight::GetShadowMapHeight() const
{
    const float inv = m_lightData.InvShadowMapSize.y;
    if (inv <= 0.0f)
        return 0u;

    const float h = 1.0f / inv;
    return (h <= 0.0f) ? 0u : static_cast<std::uint32_t>(h + 0.5f);
}

DirectX::XMMATRIX kfe::IKFELight::GetLightView() const
{
    return m_lightData.LightView;
}

DirectX::XMMATRIX kfe::IKFELight::GetLightProj() const
{
    return m_lightData.LightProj;
}

DirectX::XMMATRIX kfe::IKFELight::GetLightViewProj() const
{
    return m_lightData.LightViewProj;
}

void kfe::IKFELight::SetLightView(const DirectX::XMMATRIX& m)
{
    m_lightData.LightView = m;
}

void kfe::IKFELight::SetLightProj(const DirectX::XMMATRIX& m)
{
    m_lightData.LightProj = m;
}

void kfe::IKFELight::SetLightViewProj(const DirectX::XMMATRIX& m)
{
    m_lightData.LightViewProj = m;
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
