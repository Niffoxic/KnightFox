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

#include "engine/utils/json_loader.h"

#include <cstdint>
#include <memory>
#include <string_view>
#include <DirectXMath.h>

//~ light
#include "engine/render_manager/light/directional_light.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/buffer/constant_buffer.h"
#include "engine/utils/helpers.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/commands/graphics_list.h"
#include "engine/render_manager/api/queue/graphics_queue.h"
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"
#include "engine/render_manager/api/heap/heap_sampler.h"
#include "engine/render_manager/api/root_signature.h"
#include "engine/render_manager/light/light_manager.h"
#include <d3d12.h>
#include "engine/render_manager/shadow/shadow_map.h"

#include "engine/render_manager/scene/scene_types.h"
#include "engine/render_manager/api/frame_cb.h"

namespace kfe
{
	/// <summary>
	/// Base interface for any scene object
	/// </summary>
    class KFE_API IKFESceneObject : public IKFEObject
    {
    public:
        IKFESceneObject() = default;
        virtual ~IKFESceneObject() override = default;

        IKFESceneObject(const IKFESceneObject&) = default;
        IKFESceneObject(IKFESceneObject&&)      = default;

        IKFESceneObject& operator=(const IKFESceneObject&) = default;
        IKFESceneObject& operator=(IKFESceneObject&&)      = default;

        // Visibility
                  void SetVisible(bool visible);
        NODISCARD bool IsVisible    () const;
        NODISCARD bool IsInitialized() const noexcept;

        void Update(const KFE_UPDATE_OBJECT_DESC& desc);

        NODISCARD bool Build(_In_ const KFE_BUILD_OBJECT_DESC& desc);
        NODISCARD bool Destroy();

        //~ Passes
        void MainPass  (_In_ const KFE_RENDER_OBJECT_DESC& desc);
        void ShadowPass(_In_ const KFE_RENDER_OBJECT_DESC& desc);

        // Serialization
        JsonLoader GetJsonData() const;
        void       LoadFromJson(const JsonLoader& loader);

        // Draw properties
        void ImguiView(float deltaTime);

        NODISCARD virtual const DirectX::XMMATRIX& GetWorldMatrix() const;

        // Type name
        void SetTypeName  (const std::string& typeName);
        void SetObjectName(const std::string& typeName);
        
        std::string GetTypeName  () const;
        std::string GetObjectName() const;

        //~ Light Management
        void AttachLight(IKFELight* light);
        void DetachLight(IKFELight* light);
        void DetachLight(KID id);

    protected:
        //~ Passes
        virtual void ChildMainPass(_In_ const KFE_RENDER_OBJECT_DESC& desc)   = 0;
        virtual void ChildShadowPass(_In_ const KFE_RENDER_OBJECT_DESC& desc) = 0;
        
        //~ Building and views
        NODISCARD virtual bool ChildBuild    (_In_ const KFE_BUILD_OBJECT_DESC& desc) = 0;
                  virtual void ChildUpdate   (const KFE_UPDATE_OBJECT_DESC& desc)     = 0;
                  virtual void ChildImguiViewHeader(float deltaTime)                  = 0;
                  virtual void ChildImguiViewBody(float deltaTime)                    = 0;

        NODISCARD virtual bool ChildDestroy() = 0;

        NODISCARD virtual JsonLoader ChildGetJsonData() const                    = 0;
                  virtual  void      ChildLoadFromJson(const JsonLoader& loader) = 0;
    private:
        //~ Main Pass
        NODISCARD bool InitMainRootSignature    (_In_ const KFE_BUILD_OBJECT_DESC& desc);
        NODISCARD bool InitMainPipeline         (KFEDevice* device);
        NODISCARD bool InitPrimaryConstantBuffer(const KFE_BUILD_OBJECT_DESC& desc);
        NODISCARD bool InitMainSampler          (_In_ const KFE_BUILD_OBJECT_DESC& desc);

        //~ Shadow Pass
        NODISCARD bool InitShadowRootSignature          (_In_ const KFE_BUILD_OBJECT_DESC& desc);
        NODISCARD bool InitShadowPipeline               (KFEDevice* device);
        NODISCARD bool InitShadowLightMetaConstantbuffer(_In_ const KFE_BUILD_OBJECT_DESC& desc);
        NODISCARD bool InitShadowComparisionSampler     (_In_ const KFE_BUILD_OBJECT_DESC& desc);

        //~ Updates
        //~ Main Pass Updates
        void UpdatePrimaryConstantBuffer(const KFE_UPDATE_OBJECT_DESC& desc);

        //~ Shadow Update
        void UpdateLightConstantBuffer(_In_ const KFE_LIGHT_DATA_GPU& desc);

    public:
        //~ Pass Informations
        TransformInfo Transform       {};
        ShaderInfo    m_shaderInfo    {};
        DrawInfo      Draw            {};
        PassInfo      m_mainPassInfo  {};
        PassInfo      m_shadowPassInfo{};
        SceneInfo     m_sceneInfo     {};
        KFEDevice*    m_pDevice{ nullptr };

        //~ Primary Buffer
        const std::uint16_t    m_frameCount{ 2u };
        KFEFrameConstantBuffer m_primaryCBFrame{};

        //~ Light Management
        KFEFrameConstantBuffer m_lightCBFrame{};
        KFELightManager        m_lightManager{};
    };
}
