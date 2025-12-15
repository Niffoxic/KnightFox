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
#include "engine/system/interface/interface_scene.h"

#include <memory>

namespace kfe
{
    class KFE_API KFEMeshSceneObject final : public IKFESceneObject
    {
    public:
        KFEMeshSceneObject();
        ~KFEMeshSceneObject() override;

        // ~ Inherited via IKFESceneObject / IKFEObject
        std::string GetName() const noexcept override;
        std::string GetDescription() const noexcept override;

        //~ Inherited via IKFESceneObject
        void ChildMainPass(const KFE_RENDER_OBJECT_DESC& desc) override;
        void ChildShadowPass(const KFE_RENDER_OBJECT_DESC& desc) override;

        //~ Child Specifics 
        bool ChildBuild(const KFE_BUILD_OBJECT_DESC& desc) override;
        void ChildUpdate(const KFE_UPDATE_OBJECT_DESC& desc) override;
        bool ChildDestroy() override;

        //~ Imgui Specifics
        void ChildImguiViewHeader(float deltaTime) override;
        void ChildImguiViewBody(float deltaTime) override;

        //~ Serialization
        JsonLoader ChildGetJsonData() const override;
        void ChildLoadFromJson(const JsonLoader& loader) override;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;

    };
} // namespace kfe
