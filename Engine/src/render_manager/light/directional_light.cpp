// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  File      : directional_light.cpp
 *  -----------------------------------------------------------------------------
 */

#include "pch.h"
#include "engine/render_manager/light/directional_light.h"

#include <algorithm>
#include <cmath>

namespace
{
	static inline float Clamp01(float v)
	{
		return std::max(0.0f, std::min(1.0f, v));
	}
}

using namespace DirectX;

kfe::KFEDirectionalLight::KFEDirectionalLight()
	: m_directionWS{ 0.0f, -1.0f, 0.0f }
	, m_intensity(1.0f)
	, m_color{ 1.0f, 1.0f, 1.0f }
	, m_shadowStrength(1.0f)
	, m_shadowBias(0.0005f)
	, m_normalBias(0.0025f)
	, m_shadowDistance(50.0f)
	, m_orthoSize(50.0f)
	, m_shadowMapW(2048u)
	, m_shadowMapH(2048u)
	, m_lightView(XMMatrixIdentity())
	, m_lightProj(XMMatrixIdentity())
	, m_lightViewProj(XMMatrixIdentity())
{
}

void kfe::KFEDirectionalLight::SetDirectionWS(const DirectX::XMFLOAT3& dir)
{
	m_directionWS = dir;
}

void kfe::KFEDirectionalLight::SetColor(const DirectX::XMFLOAT3& color)
{
	m_color = color;
}

void kfe::KFEDirectionalLight::SetIntensity(float intensity)
{
	m_intensity = std::max(0.0f, intensity);
}

void kfe::KFEDirectionalLight::SetShadowStrength(float v)
{
	m_shadowStrength = Clamp01(v);
}

void kfe::KFEDirectionalLight::SetShadowBias(float v)
{
	m_shadowBias = std::max(0.0f, v);
}

void kfe::KFEDirectionalLight::SetNormalBias(float v)
{
	m_normalBias = std::max(0.0f, v);
}

void kfe::KFEDirectionalLight::SetShadowDistance(float v)
{
	m_shadowDistance = std::max(0.01f, v);
}

void kfe::KFEDirectionalLight::SetOrthoSize(float v)
{
	m_orthoSize = std::max(0.01f, v);
}

void kfe::KFEDirectionalLight::SetShadowMapSize(std::uint32_t w, std::uint32_t h)
{
	m_shadowMapW = std::max(1u, w);
	m_shadowMapH = std::max(1u, h);
}

DirectX::XMFLOAT3 kfe::KFEDirectionalLight::GetDirectionWS() const
{
	return m_directionWS;
}

DirectX::XMFLOAT3 kfe::KFEDirectionalLight::GetColor() const
{
	return m_color;
}

float kfe::KFEDirectionalLight::GetIntensity() const
{
	return m_intensity;
}

float kfe::KFEDirectionalLight::GetShadowStrength() const
{
	return m_shadowStrength;
}

float kfe::KFEDirectionalLight::GetShadowBias() const
{
	return m_shadowBias;
}

float kfe::KFEDirectionalLight::GetNormalBias() const
{
	return m_normalBias;
}

float kfe::KFEDirectionalLight::GetShadowDistance() const
{
	return m_shadowDistance;
}

float kfe::KFEDirectionalLight::GetOrthoSize() const
{
	return m_orthoSize;
}

std::uint32_t kfe::KFEDirectionalLight::GetShadowMapWidth() const
{
	return m_shadowMapW;
}

std::uint32_t kfe::KFEDirectionalLight::GetShadowMapHeight() const
{
	return m_shadowMapH;
}

DirectX::XMMATRIX kfe::KFEDirectionalLight::GetLightView() const
{
	return m_lightView;
}

DirectX::XMMATRIX kfe::KFEDirectionalLight::GetLightProj() const
{
	return m_lightProj;
}

DirectX::XMMATRIX kfe::KFEDirectionalLight::GetLightViewProj() const
{
	return m_lightViewProj;
}

void kfe::KFEDirectionalLight::UpdateMatrices(
	const DirectX::XMVECTOR& cameraPosWS,
	const DirectX::XMVECTOR& cameraForwardWS)
{
	const XMVECTOR dir = NormalizeSafe(XMLoadFloat3(&m_directionWS));
	const XMVECTOR fwd = NormalizeSafe(cameraForwardWS);

	const float half = m_shadowDistance * 0.5f;
	const XMVECTOR center = cameraPosWS + fwd * half;

	const float back = m_shadowDistance;
	const XMVECTOR lightPos = center - dir * back;

	const XMVECTOR up = ChooseUpVector(dir);

	m_lightView = XMMatrixLookAtLH(lightPos, center, up);

	const float size = m_orthoSize;
	const float nearZ = 0.1f;
	const float farZ = m_shadowDistance * 2.0f;

	m_lightProj = XMMatrixOrthographicLH(size, size, nearZ, farZ);

	m_lightViewProj = XMMatrixMultiply(m_lightView, m_lightProj);
}

kfe::KFE_DIRECTIONAL_LIGHT_CB_DESC kfe::KFEDirectionalLight::GetCBDesc() const
{
	KFE_DIRECTIONAL_LIGHT_CB_DESC out{};

	out.DirectionWS = m_directionWS;
	out.Intensity = m_intensity;

	out.Color = m_color;
	out.ShadowStrength = m_shadowStrength;

	out.LightView = XMMatrixTranspose(m_lightView);
	out.LightProj = XMMatrixTranspose(m_lightProj);
	out.LightViewProj = XMMatrixTranspose(m_lightViewProj);

	out.ShadowBias = m_shadowBias;
	out.NormalBias = m_normalBias;
	out.ShadowDistance = m_shadowDistance;
	out.OrthoSize = m_orthoSize;

	out.InvShadowMapSize = XMFLOAT2(
		m_shadowMapW ? (1.0f / float(m_shadowMapW)) : 0.0f,
		m_shadowMapH ? (1.0f / float(m_shadowMapH)) : 0.0f);

	return out;
}

DirectX::XMVECTOR kfe::KFEDirectionalLight::NormalizeSafe(DirectX::FXMVECTOR v)
{
	const XMVECTOR lenSq = XMVector3LengthSq(v);
	const float ls = XMVectorGetX(lenSq);
	if (ls < 1e-12f)
		return XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
	return XMVector3Normalize(v);
}

DirectX::XMVECTOR kfe::KFEDirectionalLight::ChooseUpVector(DirectX::FXMVECTOR dir)
{
	const float y = std::abs(XMVectorGetY(dir));
	if (y > 0.99f)
		return XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	return XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
}
