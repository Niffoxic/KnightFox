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

        KFEMeshSceneObject(const KFEMeshSceneObject&) = delete;
        KFEMeshSceneObject& operator=(const KFEMeshSceneObject&) = delete;

        KFEMeshSceneObject(KFEMeshSceneObject&&);
        KFEMeshSceneObject& operator=(KFEMeshSceneObject&&);

        // ~ Inherited via IKFESceneObject / IKFEObject
        std::string GetName() const noexcept override;
        std::string GetDescription() const noexcept override;

        // Json / serialization
        JsonLoader GetJsonData() const override;
        void       LoadFromJson(const JsonLoader& loader) override;

        // ~ Shader Properties 
        void        SetVertexShader(const std::string& path) override;
        std::string VertexShader() const override;

        void        SetPixelShader(const std::string& path) override;
        std::string PixelShader() const override;

        void        SetGeometryShader(const std::string& path) override;
        std::string GeometryShader() const override;

        void        SetHullShader(const std::string& path) override;
        std::string HullShader() const override;

        void        SetDomainShader(const std::string& path) override;
        std::string DomainShader() const override;

        void        SetComputeShader(const std::string& path) override;
        std::string ComputeShader() const override;

        // ~ Draw Properties
        void        SetCullMode(const ECullMode mode) override;
        void        SetCullMode(const std::string& mode) override;
        void        SetDrawMode(const EDrawMode mode) override;
        void        SetDrawMode(const std::string& mode) override;
        ECullMode   GetCullMode() const override;
        std::string GetCullModeString() const override;
        EDrawMode   GetDrawMode() const override;
        std::string GetDrawModeString() const override;

        void ImguiView(float deltaTime) override;

        // ~ Lifecycle
        void Update(const KFE_UPDATE_OBJECT_DESC& desc) override;
        bool BuildChild(_In_ const KFE_BUILD_OBJECT_DESC& desc) override;
        bool Destroy() override;
        void Render(_In_ const KFE_RENDER_OBJECT_DESC& desc) override;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;

    };
} // namespace kfe
