// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  File      : point_light.h
 *  -----------------------------------------------------------------------------
 */
#pragma once
#include "EngineAPI.h"
#include <DirectXMath.h>
#include <cstdint>

#include "engine/system/interface/interface_light.h"

namespace kfe
{
    class KFE_API KFEPointLight final : public IKFELight
    {
    public:
        KFEPointLight();
        ~KFEPointLight() override;

        std::string GetName() const noexcept override;
        std::string GetDescription() const noexcept override;

        void UpdateLight(const KFECamera* camera) override;

        bool CanCullByDistance() const override;
        void ImguiView(float deltaTime) override;

        void LoadFromJson(const JsonLoader& loader) override;
        JsonLoader GetJsonData() const override;
        void ResetToDefaults() override;
    };
} // namespace kfe
