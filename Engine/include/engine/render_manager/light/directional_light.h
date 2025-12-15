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

#include "engine/system/interface/interface_light.h"

namespace kfe
{
    class KFE_API KFEDirectionalLight final : public IKFELight
    {
    public:
        KFEDirectionalLight();
        ~KFEDirectionalLight() override;

        // IKFEObject
        std::string GetName() const noexcept override;
        std::string GetDescription() const noexcept override;

        bool CanCullByDistance() const override;

        NODISCARD float GetLightTypeValue() const noexcept override { return 1.0f; }

        std::string GetLightType() const override;

        void ImguiView(float deltaTime) override;

        void LoadFromJson(const JsonLoader& loader) override;
        JsonLoader GetJsonData() const override;
        void ResetToDefaults() override;

    private:
        void UpdateLight(const KFECamera* camera) override;
    };
} // namespace kfe
