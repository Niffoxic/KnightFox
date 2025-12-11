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
#include "engine/core.h"

#include <string>
#include <cstdint>
#include <unordered_map>
#include <memory>

#include "engine/system/interface/interface_singleton.h"
#include "engine/render_manager/api/heap/heap_sampler.h"
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"
#include "engine/render_manager/api/texture/texture.h"
#include "engine/render_manager/api/texture/texture_srv.h"
#include "engine/render_manager/api/texture/staging_texture.h"

namespace kfe
{
    class KFEDevice;
    class KFEGraphicsCommandList;

    typedef struct _KFE_INIT_IMAGE_POOL
    {
        KFEDevice*       Device{ nullptr };
        KFEResourceHeap* ResourceHeap{ nullptr };
        KFESamplerHeap*  SamplerHeap{ nullptr };
    } KFE_INIT_IMAGE_POOL;

    class KFE_API KFEImagePool final : public ISingleton<KFEImagePool>
    {
        struct TextureData
        {
            std::string Name;
            std::unique_ptr<KFEStagingTexture> Staging;
            std::unique_ptr<KFETextureSRV>     Srv;
        };

        friend ISingleton<KFEImagePool>;

    public:
        KFEImagePool() noexcept;
        ~KFEImagePool() noexcept;

        KFEImagePool(const KFEImagePool&) = delete;
        KFEImagePool& operator=(const KFEImagePool&) = delete;
        KFEImagePool(KFEImagePool&&) = delete;
        KFEImagePool& operator=(KFEImagePool&&) = delete;

        NODISCARD bool Initialize(_In_ const KFE_INIT_IMAGE_POOL& desc);
        NODISCARD bool IsInitialized() const noexcept;

        NODISCARD KFETextureSRV* GetImageSrv(
            _In_ const std::string& path,
            _In_ KFEGraphicsCommandList* cmdList);

        NODISCARD KFETexture* GetTexture(
            _In_ const std::string& path) noexcept;

        NODISCARD bool Reload(
            _In_ const std::string& path,
            _In_ KFEGraphicsCommandList* cmdList);

        void Clear() noexcept;
        NODISCARD std::size_t GetTextureCount() const noexcept;

    private:
        bool LoadTextureInternal(
            _In_ const std::string& path,
            _In_ KFEGraphicsCommandList* cmdList,
            _Inout_ TextureData& outData);
    private:
        std::unordered_map<std::string, TextureData> m_imagePool{};

        KFEDevice*       m_pDevice{ nullptr };
        KFEResourceHeap* m_pResourceHeap{ nullptr };
        KFESamplerHeap*  m_pSamplerHeap{ nullptr };

        bool m_bInitialized{ false };
    };
} // namespace kfe
