// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  File      : directional_light.h
 *  -----------------------------------------------------------------------------
 */
#pragma once
#include "EngineAPI.h"
#include <DirectXMath.h>
#include <cstdint>

namespace kfe
{
#pragma pack(push, 16)

	typedef struct _KFE_DIRECTIONAL_LIGHT_CB_DESC
	{
		DirectX::XMFLOAT3 DirectionWS;
		float             Intensity;

		DirectX::XMFLOAT3 Color;
		float             ShadowStrength;

		DirectX::XMMATRIX LightView;
		DirectX::XMMATRIX LightProj;
		DirectX::XMMATRIX LightViewProj;

		float             ShadowBias;
		float             NormalBias;
		float             ShadowDistance;
		float             OrthoSize;

		DirectX::XMFLOAT2 InvShadowMapSize;
		float             _PaddingDL0;
		float             _PaddingDL1;

	} KFE_DIRECTIONAL_LIGHT_CB_DESC;

#pragma pack(pop)

	class KFE_API KFEDirectionalLight
	{
	public:
		 KFEDirectionalLight();
		~KFEDirectionalLight() = default;

		void SetDirectionWS(const DirectX::XMFLOAT3& dir);
		void SetColor(const DirectX::XMFLOAT3& color);
		void SetIntensity(float intensity);

		void SetShadowStrength(float v);
		void SetShadowBias(float v);
		void SetNormalBias(float v);

		void SetShadowDistance(float v);
		void SetOrthoSize(float v);

		void SetShadowMapSize(std::uint32_t w, std::uint32_t h);

		DirectX::XMFLOAT3 GetDirectionWS() const;
		DirectX::XMFLOAT3 GetColor() const;
		float             GetIntensity() const;

		float GetShadowStrength() const;
		float GetShadowBias() const;
		float GetNormalBias() const;

		float GetShadowDistance() const;
		float GetOrthoSize() const;

		std::uint32_t GetShadowMapWidth() const;
		std::uint32_t GetShadowMapHeight() const;

		DirectX::XMMATRIX GetLightView() const;
		DirectX::XMMATRIX GetLightProj() const;
		DirectX::XMMATRIX GetLightViewProj() const;

		void UpdateMatrices(
			const DirectX::XMVECTOR& cameraPosWS,
			const DirectX::XMVECTOR& cameraForwardWS);

		KFE_DIRECTIONAL_LIGHT_CB_DESC GetCBDesc() const;

	private:
		static DirectX::XMVECTOR NormalizeSafe(DirectX::FXMVECTOR v);
		static DirectX::XMVECTOR ChooseUpVector(DirectX::FXMVECTOR dir);

	private:
		DirectX::XMFLOAT3 m_directionWS;
		float             m_intensity;

		DirectX::XMFLOAT3 m_color;
		float             m_shadowStrength;

		float             m_shadowBias;
		float             m_normalBias;
		float             m_shadowDistance;
		float             m_orthoSize;

		std::uint32_t     m_shadowMapW;
		std::uint32_t     m_shadowMapH;

		DirectX::XMMATRIX m_lightView;
		DirectX::XMMATRIX m_lightProj;
		DirectX::XMMATRIX m_lightViewProj;
	};
} // namespace kfe
