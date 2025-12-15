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
    struct alignas(16) KFE_LIGHT_DATA_GPU
    {
        //~ header
        float LightType;        // 0 = Spot, 1 = Directional, 2 = Point
        float Intensity;
        float Range;
        float Attenuation;

        //~ position
        DirectX::XMFLOAT3 PositionWS;
        float             _PadPos;

        //~ direction
        DirectX::XMFLOAT3 DirectionWS;
        float             _PadDir0;

        DirectX::XMFLOAT3 DirectionWSNormalized;
        float             _PadDir1;

        //~ color
        DirectX::XMFLOAT3 Color;
        float             _PadColor;

        //~ spot params
        float SpotInnerCos;
        float SpotOuterCos;
        float SpotSoftness;
        float _PadSpot;
    };
    static_assert((sizeof(KFE_LIGHT_DATA_GPU) % 16) == 0);

    class KFE_API IKFELight : public IKFEObject
    {
    public:
        IKFELight();
        virtual ~IKFELight() override;

        IKFELight(const IKFELight&) = delete;
        IKFELight& operator=(const IKFELight&) = delete;

        IKFELight(IKFELight&&) noexcept;
        IKFELight& operator=(IKFELight&&) noexcept;

        //~ lifetime
        void Enable();
        void Disable();
        NODISCARD bool IsEnable() const;

        //~ update
        void Update(const KFECamera* camera);

        //~ gpu data
        NODISCARD KFE_LIGHT_DATA_GPU GetLightData() const;

        //~ cpu culling
        void  SetCullRadius(float r);
        float GetCullRadius() const;

        NODISCARD float DistanceFromPoint(const DirectX::XMFLOAT3& p) const;
        NODISCARD float DistanceFromPointSq(const DirectX::XMFLOAT3& p) const;

        NODISCARD bool  IsInCullRadius(const DirectX::XMFLOAT3& p) const;
        NODISCARD bool  IsInCullRadiusSq(const DirectX::XMFLOAT3& p) const;

        virtual NODISCARD bool CanCullByDistance() const = 0;

        // 0.0 = Spot, 1.0 = Directional, 2.0 = Point
        NODISCARD virtual float GetLightTypeValue() const noexcept = 0;
        NODISCARD virtual std::string GetLightType() const = 0;

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
        DirectX::XMFLOAT3 GetDirectionWSNormalized() const;

        //~ range & attenuation
        void  SetRange(float v);
        float GetRange() const;

        void  SetAttenuation(float v);
        float GetAttenuation() const;

        //~ spot params (radians)
        void  SetSpotInnerAngle(float radians);
        float GetSpotInnerAngle() const;

        void  SetSpotOuterAngle(float radians);
        float GetSpotOuterAngle() const;

        void  SetSpotSoftness(float v);
        float GetSpotSoftness() const;

        //~ imgui
        virtual void ImguiView(float deltaTime) = 0;

        //~ serialization
        virtual void LoadFromJson(const JsonLoader& loader) = 0;
        virtual JsonLoader GetJsonData() const = 0;

        //~ defaults
        virtual void ResetToDefaults() = 0;

        //~ configs
        void        SetLightName(const std::string& name);
        std::string GetLightName() const;

    protected:
        void EnsurePacked_() const noexcept;
        virtual void UpdateLight(const KFECamera* camera) = 0;

        static DirectX::XMVECTOR NormalizeSafe(DirectX::FXMVECTOR v) noexcept;
        static DirectX::XMVECTOR ChooseUpVector(DirectX::FXMVECTOR dir) noexcept;

    protected:
        mutable KFE_LIGHT_DATA_GPU  m_lightData{};
        float                       m_cullRadius = 30.0f;
        std::string                 m_lightName{};
        mutable bool                m_bDirty{ true };

    private:
        bool m_enabled = true;
    };
} // namespace kfe
