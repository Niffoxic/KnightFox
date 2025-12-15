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
#include <cstdint>
#include <string>
#include <d3d12.h>
#include "engine/utils/json_loader.h"
#include "engine/system/common_types.h"

namespace kfe
{
    class KFEDevice;
    class KFEResourceHeap;

    struct KFE_POST_EFFECT_INIT_DESC
    {
        KFEDevice*       Device{ nullptr };
        KFEResourceHeap* ResourceHeap{ nullptr };
        DXGI_FORMAT      OutputFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };
    };

    struct KFE_POST_EFFECT_RENDER_DESC
    {
        ID3D12GraphicsCommandList* Cmd{ nullptr };

        // Output
        D3D12_CPU_DESCRIPTOR_HANDLE OutputRTV{};

        // Input
        std::uint32_t InputSceneSRVIndex{ KFE_INVALID_INDEX };
        std::uint32_t RootParam_SceneSRV{ 1u };

        const D3D12_VIEWPORT* Viewport{ nullptr };
        const D3D12_RECT* Scissor{ nullptr };
    };

    class KFE_API IKFEPostEffect : public IKFEObject
    {
    public:
        virtual ~IKFEPostEffect() = default;

        virtual bool Initialize(_In_ const KFE_POST_EFFECT_INIT_DESC& desc) = 0;
        virtual void Destroy() noexcept = 0;

        virtual void Render(_In_ const KFE_POST_EFFECT_RENDER_DESC& desc) = 0;
        virtual void ImguiView(float deltaTime) = 0;

        virtual std::string GetPEClassName() const noexcept = 0;

        virtual JsonLoader GetJsonData() const = 0;
        virtual void LoadFromJson(const JsonLoader& loader) = 0;

        virtual bool IsInitialized() const noexcept = 0;

        void SetPostName(const std::string& name) { m_szName = name; }
        std::string GetPostName() const { return m_szName; }

        void Disable() { m_bEnabled = false; }
        void Enable() { m_bEnabled = true; }
        bool IsEnable() const { return m_bEnabled; }

    protected:
        std::string m_szName{ "No Name" };
        bool m_bEnabled{ true };
    };
}
