// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */
#pragma once

#include "EngineAPI.h"
#include "engine/core.h"
#include <DirectXMath.h>
#include <cstdint>
#include <memory>
#include <string>

#include "engine/render_manager/components/camera.h"
#include "engine/utils/json_loader.h"

namespace kfe
{
    enum KFE_LIGHT_TYPE : uint32_t
    {
        KFE_LIGHT_DIRECTIONAL = 0,
        KFE_LIGHT_SPOT        = 1,
        KFE_LIGHT_POINT       = 2
    };

    inline static std::string ToString(KFE_LIGHT_TYPE type)
    {
        switch (type)
        {
        case KFE_LIGHT_DIRECTIONAL: return "DIRECTIONAL";
        case KFE_LIGHT_SPOT:        return "SPOT";
        case KFE_LIGHT_POINT:       return "POINT";
        default:                    break;
        }
        return "UNKNOWN_LIGHT";
    }

#pragma pack(push, 16)

    typedef struct _KFE_LIGHT_DATA_DESC
    {
        //~ light type & intensity
        float Type;
        float Intensity;
        float Range;
        float ShadowStrength;

        //~ spatial
        DirectX::XMFLOAT3 PositionWS;
        float             NormalBias;

        DirectX::XMFLOAT3 DirectionWS;
        float             ShadowBias;

        //~ color & attenuation
        DirectX::XMFLOAT3 Color;
        float             Attenuation;

        //~ spot params
        float SpotInnerAngle;
        float SpotOuterAngle;
        float ShadowDistance;
        float OrthoSize;

        //~ shadow matrices
        DirectX::XMMATRIX LightView;
        DirectX::XMMATRIX LightProj;
        DirectX::XMMATRIX LightViewProj;

        DirectX::XMFLOAT2 InvShadowMapSize;
        float             Padding0;
        float             Padding1;

    } KFE_LIGHT_DATA_DESC;

#pragma pack(pop)

    class KEFCubeSceneObject;

    class KFE_API IKFELight : public IKFEObject
    {
    public:
        IKFELight();
        virtual ~IKFELight() override;

        //~ lifetime
        void Enable();
        void Disable();
        NODISCARD bool IsEnable() const;

        //~ update
        void Update(const KFECamera* camera);

        //~ gpu data
        NODISCARD KFE_LIGHT_DATA_DESC GetLightData() const;

        //~ cpu culling
        void  SetCullRadius(float r);
        float GetCullRadius() const;

        NODISCARD float DistanceFromPoint(const DirectX::XMFLOAT3& p) const;
        NODISCARD float DistanceFromPointSq(const DirectX::XMFLOAT3& p) const;

        NODISCARD bool  IsInCullRadius(const DirectX::XMFLOAT3& p) const;
        NODISCARD bool  IsInCullRadiusSq(const DirectX::XMFLOAT3& p) const;

        virtual NODISCARD bool CanCullByDistance() const = 0;

        //~ type
        void SetLightType(KFE_LIGHT_TYPE type);
        NODISCARD KFE_LIGHT_TYPE GetLightType() const;
        NODISCARD std::string GetLightTypeName() const;

        //~ common params
        void  SetIntensity(float v);
        float GetIntensity() const;

        void  SetColor(const DirectX::XMFLOAT3& color);
        DirectX::XMFLOAT3 GetColor() const;

        //~ transform
        void  SetPositionWS(const DirectX::XMFLOAT3& pos);
        DirectX::XMFLOAT3 GetPositionWS() const;

        void  SetDirectionWS(const DirectX::XMFLOAT3& dir);
        DirectX::XMFLOAT3 GetDirectionWS() const;

        //~ range & attenuation
        void  SetRange(float v);
        float GetRange() const;

        void  SetAttenuation(float v);
        float GetAttenuation() const;

        //~ spot params
        void  SetSpotInnerAngle(float radians);
        float GetSpotInnerAngle() const;

        void  SetSpotOuterAngle(float radians);
        float GetSpotOuterAngle() const;

        //~ shadow params
        void  SetShadowStrength(float v);
        float GetShadowStrength() const;

        void  SetShadowBias(float v);
        float GetShadowBias() const;

        void  SetNormalBias(float v);
        float GetNormalBias() const;

        void  SetShadowDistance(float v);
        float GetShadowDistance() const;

        void  SetOrthoSize(float v);
        float GetOrthoSize() const;

        void  SetShadowMapSize(std::uint32_t w, std::uint32_t h);
        std::uint32_t GetShadowMapWidth()  const;
        std::uint32_t GetShadowMapHeight() const;

        //~ matrices
        DirectX::XMMATRIX GetLightView() const;
        DirectX::XMMATRIX GetLightProj() const;
        DirectX::XMMATRIX GetLightViewProj() const;

        void SetLightView(const DirectX::XMMATRIX& m);
        void SetLightProj(const DirectX::XMMATRIX& m);
        void SetLightViewProj(const DirectX::XMMATRIX& m);

        //~ imgui
        virtual void ImguiView(float deltaTime) = 0;

        //~ serialization
        virtual void LoadFromJson(const JsonLoader& loader) = 0;
        virtual JsonLoader GetJsonData() const = 0;

        //~ defaults
        virtual void ResetToDefaults() = 0;

        //~ Light Configs
        void        SetLightName(const std::string& name);
        std::string GetLightName() const;

    protected:
        virtual void UpdateLight(const KFECamera* camera) = 0;
        static DirectX::XMVECTOR NormalizeSafe(DirectX::FXMVECTOR v) noexcept;
        static DirectX::XMVECTOR ChooseUpVector(DirectX::FXMVECTOR dir) noexcept;

    protected:
            KFE_LIGHT_DATA_DESC m_lightData{};
            float               m_cullRadius = 30.0f;
            std::string         m_lightName{};

    private:
        bool m_enabled = true;
        std::unique_ptr<KEFCubeSceneObject> m_debugCube;
    };
} // namespace kfe
