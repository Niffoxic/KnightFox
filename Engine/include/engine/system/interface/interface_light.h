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
    static constexpr std::uint32_t KFE_MAX_CASCADES = 4u;
    static constexpr std::uint32_t KFE_POINT_FACE_COUNT = 6u;

    enum KFE_LIGHT_TYPE : std::uint32_t
    {
        KFE_LIGHT_DIRECTIONAL = 0u,
        KFE_LIGHT_SPOT = 1u,
        KFE_LIGHT_POINT = 2u
    };

    enum KFE_SHADOW_TECH : std::uint32_t
    {
        KFE_SHADOW_NONE = 0u,
        KFE_SHADOW_DIR_CSM = 1u,
        KFE_SHADOW_SPOT_2D = 2u,
        KFE_SHADOW_POINT_CUBE = 3u
    };

    enum KFE_LIGHT_FLAGS : std::uint32_t
    {
        KFE_LIGHT_FLAG_ENABLED = (1u << 0),
        KFE_LIGHT_FLAG_CAST_SHADOW = (1u << 1),
        KFE_LIGHT_FLAG_VOLUMETRIC = (1u << 2),
        KFE_LIGHT_FLAG_RESERVED3 = (1u << 3)
    };

    inline static const char* ToString(KFE_LIGHT_TYPE type) noexcept
    {
        switch (type)
        {
        case KFE_LIGHT_DIRECTIONAL: return "DIRECTIONAL";
        case KFE_LIGHT_SPOT:        return "SPOT";
        case KFE_LIGHT_POINT:       return "POINT";
        default:                    return "UNKNOWN_LIGHT";
        }
    }

    struct alignas(16) KFE_LIGHT_DATA_GPU
    {
        //~ header
        std::uint32_t Type;          //~ KFE_LIGHT_TYPE
        std::uint32_t Flags;         //~ KFE_LIGHT_FLAGS
        std::uint32_t ShadowTech;    //~ KFE_SHADOW_TECH
        std::uint32_t ShadowMapId;   //~ index into shadow array

        //~ energy & range
        float Intensity;
        float Range;
        float Attenuation;
        float ShadowStrength;

        //~ position
        DirectX::XMFLOAT3 PositionWS;
        float             _PadPos;

        //~ direction (raw and normalized)
        DirectX::XMFLOAT3 DirectionWS;
        float             _PadDir0;

        DirectX::XMFLOAT3 DirectionWSNormalized;
        float             _PadDir1;

        //~ color
        DirectX::XMFLOAT3 Color;
        float             _PadColor;

        //~ spot params (cosines)
        float SpotInnerCos;
        float SpotOuterCos;
        float SpotSoftness;
        float _PadSpot;

        //~ shadow params
        float ShadowBias;
        float NormalBias;
        float ShadowNearZ;
        float ShadowFarZ;

        float ShadowFilterRadius;
        float ShadowTexelSize;
        float ShadowFadeStart;
        float ShadowFadeEnd;

        //~ atlas addressing
        DirectX::XMFLOAT2 ShadowUVScale;
        DirectX::XMFLOAT2 ShadowUVOffset;

        //~ directional cascades
        DirectX::XMFLOAT4   CascadeSplits;
        DirectX::XMFLOAT4X4 CascadeViewProjT[KFE_MAX_CASCADES];

        //~ single shadow (spot / simple directional)
        DirectX::XMFLOAT4X4 LightViewProjT;

        //~ point light cubemap shadows
        DirectX::XMFLOAT4X4 PointFaceViewProjT[KFE_POINT_FACE_COUNT];

        //~ shadow map size
        DirectX::XMFLOAT2 InvShadowMapSize;
        float             _PadSM0;
        float             _PadSM1;
    };
    static_assert((sizeof(KFE_LIGHT_DATA_GPU) % 16) == 0, "KFE_LIGHT_DATA_GPU must be 16-byte multiple.");

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

        //~ type
        void SetLightType(KFE_LIGHT_TYPE type);
        NODISCARD KFE_LIGHT_TYPE GetLightType() const;
        NODISCARD std::string GetLightTypeName() const;

        //~ flags & shadow tech
        void SetFlags(std::uint32_t flags);
        void AddFlags(std::uint32_t flags);
        void RemoveFlags(std::uint32_t flags);
        NODISCARD std::uint32_t GetFlags() const;

        void SetShadowTech(KFE_SHADOW_TECH tech);
        NODISCARD KFE_SHADOW_TECH GetShadowTech() const;

        void SetShadowMapId(std::uint32_t id);
        NODISCARD std::uint32_t GetShadowMapId() const;

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

        //~ shadow params
        void  SetShadowStrength(float v);
        float GetShadowStrength() const;

        void  SetShadowBias(float v);
        float GetShadowBias() const;

        void  SetNormalBias(float v);
        float GetNormalBias() const;

        void  SetShadowNearZ(float v);
        float GetShadowNearZ() const;

        void  SetShadowFarZ(float v);
        float GetShadowFarZ() const;

        void  SetShadowFilterRadius(float v);
        float GetShadowFilterRadius() const;

        void  SetShadowTexelSize(float v);
        float GetShadowTexelSize() const;

        void  SetShadowFadeStart(float v);
        float GetShadowFadeStart() const;

        void  SetShadowFadeEnd(float v);
        float GetShadowFadeEnd() const;

        void  SetShadowDistance(float v);
        float GetShadowDistance() const;

        void  SetOrthoSize(float v);
        float GetOrthoSize() const;

        void  SetShadowAtlasUV(const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT2& offset);
        DirectX::XMFLOAT2 GetShadowUVScale() const;
        DirectX::XMFLOAT2 GetShadowUVOffset() const;

        void  SetShadowMapSize(std::uint32_t w, std::uint32_t h);
        std::uint32_t GetShadowMapWidth()  const;
        std::uint32_t GetShadowMapHeight() const;

        //~ directional cascades
        void SetCascadeSplits(const DirectX::XMFLOAT4& splits);
        DirectX::XMFLOAT4 GetCascadeSplits() const;

        //~ matrices (cpu)
        DirectX::XMMATRIX GetLightView() const;
        DirectX::XMMATRIX GetLightProj() const;
        DirectX::XMMATRIX GetLightViewProj() const;

        void SetLightView(const DirectX::XMMATRIX& m);
        void SetLightProj(const DirectX::XMMATRIX& m);

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
        mutable DirectX::XMMATRIX       m_lightViewCPU{};
        mutable DirectX::XMMATRIX       m_lightProjCPU{};
        mutable DirectX::XMMATRIX       m_lightViewProjCPU{};
        mutable KFE_LIGHT_DATA_GPU      m_lightData{};
        float                           m_cullRadius = 30.0f;
        std::string                     m_lightName{};
        mutable bool                    m_bDirty{ true };

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
        bool m_enabled = true;
    };
} // namespace kfe
