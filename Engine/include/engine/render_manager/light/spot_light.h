// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  File      : spot_light.h
 *  -----------------------------------------------------------------------------
 */
#pragma once
#include "EngineAPI.h"
#include <DirectXMath.h>
#include <cstdint>

#include "engine/system/interface/interface_light.h"

namespace kfe
{
    class KFE_API KFESpotLight final : public IKFELight
    {
    public:
        KFESpotLight();
        ~KFESpotLight() override;

        // IKFEObject
        std::string GetName() const noexcept override;
        std::string GetDescription() const noexcept override;

        // IKFELight
        bool CanCullByDistance() const override;

        // 0.0 = Spot, 1.0 = Directional, 2.0 = Point
        NODISCARD float GetLightTypeValue() const noexcept override { return 0.0f; }

        std::string GetLightType() const override;

        void ImguiView(float deltaTime) override;

        void LoadFromJson(const JsonLoader& loader) override;
        JsonLoader GetJsonData() const override;
        void ResetToDefaults() override;

    private:
        void UpdateLight(const KFECamera* camera) override;
    };
} // namespace kfe
