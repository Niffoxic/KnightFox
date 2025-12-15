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

static inline void KFE_StoreMatrixT(DirectX::XMFLOAT4X4& out, DirectX::FXMMATRIX m) noexcept
{
    DirectX::XMStoreFloat4x4(&out, DirectX::XMMatrixTranspose(m));
}

void kfe::IKFELight::EnsurePacked_() const noexcept
{
    if (!m_bDirty)
        return;

    m_lightViewProjCPU = DirectX::XMMatrixMultiply(m_lightViewCPU, m_lightProjCPU);
    KFE_StoreMatrixT(m_lightData.LightViewProjT, m_lightViewProjCPU);

    m_bDirty = false;
}

class kfe::IKFELight::Impl
{
public:
    Impl()
        : m_debugCube(std::make_unique<KEFCubeSceneObject>())
    {
    }
    ~Impl() = default;
    std::unique_ptr<KEFCubeSceneObject> m_debugCube;
};

kfe::IKFELight::IKFELight()
    : m_impl(std::make_unique<kfe::IKFELight::Impl>())
{
    m_impl->m_debugCube->Draw.DrawMode = EDrawMode::WireFrame;
    KFERenderQueue::Instance().AddSceneObject(m_impl->m_debugCube.get());

    //~ defaults
    m_lightData.Type = static_cast<std::uint32_t>(KFE_LIGHT_DIRECTIONAL);
    m_lightData.Flags = KFE_LIGHT_FLAG_ENABLED;
    m_lightData.ShadowTech = static_cast<std::uint32_t>(KFE_SHADOW_NONE);
    m_lightData.ShadowMapId = 0u;

    m_lightData.Intensity = 1.0f;
    m_lightData.Range = 10.0f;
    m_lightData.Attenuation = 1.0f;
    m_lightData.ShadowStrength = 1.0f;

    m_lightData.PositionWS = { 0.0f, 0.0f, 0.0f };
    m_lightData.DirectionWS = { 0.0f, -1.0f, 0.0f };
    m_lightData.DirectionWSNormalized = { 0.0f, -1.0f, 0.0f };
    m_lightData.Color = { 1.0f, 1.0f, 1.0f };

    m_lightData.SpotInnerCos = std::cos(0.5f);
    m_lightData.SpotOuterCos = std::cos(0.75f);
    m_lightData.SpotSoftness = 0.0f;

    m_lightData.ShadowBias = 0.0005f;
    m_lightData.NormalBias = 0.01f;
    m_lightData.ShadowNearZ = 0.1f;
    m_lightData.ShadowFarZ = 100.0f;

    m_lightData.ShadowFilterRadius = 1.0f;
    m_lightData.ShadowTexelSize = 0.0f;
    m_lightData.ShadowFadeStart = 0.0f;
    m_lightData.ShadowFadeEnd = 0.0f;

    m_lightData.ShadowUVScale = { 1.0f, 1.0f };
    m_lightData.ShadowUVOffset = { 0.0f, 0.0f };

    m_lightData.CascadeSplits = { 0.0f, 0.0f, 0.0f, 0.0f };

    m_lightViewCPU = XMMatrixIdentity();
    m_lightProjCPU = XMMatrixIdentity();
    m_lightViewProjCPU = XMMatrixIdentity();
    KFE_StoreMatrixT(m_lightData.LightViewProjT, m_lightViewProjCPU);

    for (std::uint32_t i = 0u; i < KFE_MAX_CASCADES; ++i)
        KFE_StoreMatrixT(m_lightData.CascadeViewProjT[i], XMMatrixIdentity());

    for (std::uint32_t f = 0u; f < KFE_POINT_FACE_COUNT; ++f)
        KFE_StoreMatrixT(m_lightData.PointFaceViewProjT[f], XMMatrixIdentity());

    m_lightData.InvShadowMapSize = { 0.0f, 0.0f };
    m_lightData._PadSM0 = 0.0f;
    m_lightData._PadSM1 = 0.0f;

    if (m_impl->m_debugCube && m_enabled)
        m_impl->m_debugCube->Transform.SetPosition(m_lightData.PositionWS);

    m_bDirty = true;
}

kfe::IKFELight::~IKFELight()
{
    if (m_impl->m_debugCube)
        KFERenderQueue::Instance().RemoveSceneObject(m_impl->m_debugCube.get());
}

kfe::IKFELight::IKFELight(IKFELight&&) noexcept = default;
kfe::IKFELight& kfe::IKFELight::operator=(IKFELight&&) noexcept = default;

void kfe::IKFELight::Enable()
{
    if (m_enabled)
        return;

    m_enabled = true;
    m_lightData.Flags |= KFE_LIGHT_FLAG_ENABLED;

    if (m_impl->m_debugCube)
        KFERenderQueue::Instance().AddSceneObject(m_impl->m_debugCube.get());

    m_bDirty = true;
}

void kfe::IKFELight::Disable()
{
    if (!m_enabled)
        return;

    m_enabled = false;
    m_lightData.Flags &= ~KFE_LIGHT_FLAG_ENABLED;

    if (m_impl->m_debugCube)
        KFERenderQueue::Instance().RemoveSceneObject(m_impl->m_debugCube.get());
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

void kfe::IKFELight::SetLightType(KFE_LIGHT_TYPE type)
{
    m_lightData.Type = static_cast<std::uint32_t>(type);
    m_bDirty = true;
}

_Use_decl_annotations_
kfe::KFE_LIGHT_TYPE kfe::IKFELight::GetLightType() const
{
    switch (m_lightData.Type)
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

void kfe::IKFELight::SetFlags(std::uint32_t flags)
{
    m_lightData.Flags = flags;
    m_bDirty = true;
}

void kfe::IKFELight::AddFlags(std::uint32_t flags)
{
    m_lightData.Flags |= flags;
    m_bDirty = true;
}

void kfe::IKFELight::RemoveFlags(std::uint32_t flags)
{
    m_lightData.Flags &= ~flags;
    m_bDirty = true;
}

_Use_decl_annotations_
std::uint32_t kfe::IKFELight::GetFlags() const
{
    return m_lightData.Flags;
}

void kfe::IKFELight::SetShadowTech(KFE_SHADOW_TECH tech)
{
    m_lightData.ShadowTech = static_cast<std::uint32_t>(tech);
    m_bDirty = true;
}

_Use_decl_annotations_
kfe::KFE_SHADOW_TECH kfe::IKFELight::GetShadowTech() const
{
    return static_cast<KFE_SHADOW_TECH>(m_lightData.ShadowTech);
}

void kfe::IKFELight::SetShadowMapId(std::uint32_t id)
{
    m_lightData.ShadowMapId = id;
    m_bDirty = true;
}

_Use_decl_annotations_
std::uint32_t kfe::IKFELight::GetShadowMapId() const
{
    return m_lightData.ShadowMapId;
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

    if (m_impl->m_debugCube && m_enabled)
        m_impl->m_debugCube->Transform.SetPosition(pos);

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

void kfe::IKFELight::SetShadowStrength(float v)
{
    m_lightData.ShadowStrength = KFE_Clamp01(v);
    m_bDirty = true;
}

float kfe::IKFELight::GetShadowStrength() const
{
    return m_lightData.ShadowStrength;
}

void kfe::IKFELight::SetShadowBias(float v)
{
    m_lightData.ShadowBias = std::max(0.0f, v);
    m_bDirty = true;
}

float kfe::IKFELight::GetShadowBias() const
{
    return m_lightData.ShadowBias;
}

void kfe::IKFELight::SetNormalBias(float v)
{
    m_lightData.NormalBias = std::max(0.0f, v);
    m_bDirty = true;
}

float kfe::IKFELight::GetNormalBias() const
{
    return m_lightData.NormalBias;
}

void kfe::IKFELight::SetShadowNearZ(float v)
{
    m_lightData.ShadowNearZ = std::max(0.0001f, v);
    m_bDirty = true;
}

float kfe::IKFELight::GetShadowNearZ() const
{
    return m_lightData.ShadowNearZ;
}

void kfe::IKFELight::SetShadowFarZ(float v)
{
    m_lightData.ShadowFarZ = std::max(0.0001f, v);
    m_bDirty = true;
}

float kfe::IKFELight::GetShadowFarZ() const
{
    return m_lightData.ShadowFarZ;
}

void kfe::IKFELight::SetShadowFilterRadius(float v)
{
    m_lightData.ShadowFilterRadius = std::max(0.0f, v);
    m_bDirty = true;
}

float kfe::IKFELight::GetShadowFilterRadius() const
{
    return m_lightData.ShadowFilterRadius;
}

void kfe::IKFELight::SetShadowTexelSize(float v)
{
    m_lightData.ShadowTexelSize = std::max(0.0f, v);
    m_bDirty = true;
}

float kfe::IKFELight::GetShadowTexelSize() const
{
    return m_lightData.ShadowTexelSize;
}

void kfe::IKFELight::SetShadowFadeStart(float v)
{
    m_lightData.ShadowFadeStart = std::max(0.0f, v);
    m_bDirty = true;
}

float kfe::IKFELight::GetShadowFadeStart() const
{
    return m_lightData.ShadowFadeStart;
}

void kfe::IKFELight::SetShadowFadeEnd(float v)
{
    m_lightData.ShadowFadeEnd = std::max(0.0f, v);
    m_bDirty = true;
}

float kfe::IKFELight::GetShadowFadeEnd() const
{
    return m_lightData.ShadowFadeEnd;
}

void kfe::IKFELight::SetShadowDistance(float v)
{
    m_lightData.ShadowFarZ = std::max(0.0001f, v);
    m_bDirty = true;
}

float kfe::IKFELight::GetShadowDistance() const
{
    return m_lightData.ShadowFarZ;
}

void kfe::IKFELight::SetOrthoSize(float v)
{
    (void)v;
    m_bDirty = true;
}

float kfe::IKFELight::GetOrthoSize() const
{
    return 0.0f;
}

void kfe::IKFELight::SetShadowAtlasUV(const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT2& offset)
{
    m_lightData.ShadowUVScale = scale;
    m_lightData.ShadowUVOffset = offset;
    m_bDirty = true;
}

DirectX::XMFLOAT2 kfe::IKFELight::GetShadowUVScale() const
{
    return m_lightData.ShadowUVScale;
}

DirectX::XMFLOAT2 kfe::IKFELight::GetShadowUVOffset() const
{
    return m_lightData.ShadowUVOffset;
}

void kfe::IKFELight::SetShadowMapSize(std::uint32_t w, std::uint32_t h)
{
    if (w == 0u || h == 0u)
    {
        m_lightData.InvShadowMapSize = { 0.0f, 0.0f };
        m_bDirty = true;
        return;
    }

    m_lightData.InvShadowMapSize =
    {
        1.0f / static_cast<float>(w),
        1.0f / static_cast<float>(h)
    };

    m_bDirty = true;
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

void kfe::IKFELight::SetCascadeSplits(const DirectX::XMFLOAT4& splits)
{
    m_lightData.CascadeSplits = splits;
    m_bDirty = true;
}

DirectX::XMFLOAT4 kfe::IKFELight::GetCascadeSplits() const
{
    return m_lightData.CascadeSplits;
}

DirectX::XMMATRIX kfe::IKFELight::GetLightView() const
{
    return m_lightViewCPU;
}

DirectX::XMMATRIX kfe::IKFELight::GetLightProj() const
{
    return m_lightProjCPU;
}

DirectX::XMMATRIX kfe::IKFELight::GetLightViewProj() const
{
    EnsurePacked_();
    return m_lightViewProjCPU;
}

void kfe::IKFELight::SetLightView(const DirectX::XMMATRIX& m)
{
    m_lightViewCPU = m;
    m_bDirty = true;
}

void kfe::IKFELight::SetLightProj(const DirectX::XMMATRIX& m)
{
    m_lightProjCPU = m;
    m_bDirty = true;
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
