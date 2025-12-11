// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */

#include "pch.h"

#include "engine/render_manager/assets_library/texture_library.h"

#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/commands/graphics_list.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"

#include <d3d12.h>
#include <dxgiformat.h>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace kfe;

#pragma region Ctor_Dtor

KFEImagePool::KFEImagePool() noexcept = default;
KFEImagePool::~KFEImagePool() noexcept
{
    Clear();
}

#pragma endregion

#pragma region Public_API

_Use_decl_annotations_
bool KFEImagePool::Initialize(const KFE_INIT_IMAGE_POOL& desc)
{
    if (m_bInitialized)
    {
        LOG_WARNING("KFEImagePool::Initialize: Already initialized. Reinitializing.");
        Clear();
    }

    if (!desc.Device || !desc.Device->GetNative())
    {
        LOG_ERROR("KFEImagePool::Initialize: Device or native device is null.");
        return false;
    }

    if (!desc.ResourceHeap)
    {
        LOG_ERROR("KFEImagePool::Initialize: Resource heap is null.");
        return false;
    }

    if (!desc.SamplerHeap)
    {
        LOG_ERROR("KFEImagePool::Initialize: Sampler heap is null.");
        return false;
    }

    m_pDevice = desc.Device;
    m_pResourceHeap = desc.ResourceHeap;
    m_pSamplerHeap = desc.SamplerHeap;

    m_imagePool.clear();
    m_bInitialized = true;

    LOG_SUCCESS("KFEImagePool::Initialize: Image pool initialized.");
    return true;
}

_Use_decl_annotations_
bool KFEImagePool::IsInitialized() const noexcept
{
    return m_bInitialized;
}

_Use_decl_annotations_
KFETextureSRV* KFEImagePool::GetImageSrv(
    const std::string& path,
    KFEGraphicsCommandList* cmdList)
{
    if (!m_bInitialized)
    {
        LOG_ERROR("KFEImagePool::GetImageSrv: Image pool is not initialized.");
        return nullptr;
    }

    if (path.empty())
    {
        LOG_ERROR("KFEImagePool::GetImageSrv: Path is empty.");
        return nullptr;
    }

    if (!cmdList || !cmdList->GetNative())
    {
        LOG_ERROR("KFEImagePool::GetImageSrv: Command list is null or has no native pointer.");
        return nullptr;
    }

    auto it = m_imagePool.find(path);
    if (it != m_imagePool.end())
    {
        if (it->second.Srv)
            return it->second.Srv.get();

        LOG_WARNING("KFEImagePool::GetImageSrv: Entry exists but SRV is null. Reloading: {}", path);
        if (!LoadTextureInternal(path, cmdList, it->second))
            return nullptr;

        return it->second.Srv.get();
    }

    TextureData data{};
    data.Name = path;

    if (!LoadTextureInternal(path, cmdList, data))
    {
        LOG_ERROR("KFEImagePool::GetImageSrv: Failed to load texture '{}'.", path);
        return nullptr;
    }

    auto [iter, inserted] = m_imagePool.emplace(path, std::move(data));
    if (!inserted)
    {
        LOG_WARNING("KFEImagePool::GetImageSrv: Emplace failed, updating existing entry for '{}'.", path);
    }

    return iter->second.Srv.get();
}

_Use_decl_annotations_
KFETexture* KFEImagePool::GetTexture(const std::string& path) noexcept
{
    auto it = m_imagePool.find(path);
    if (it == m_imagePool.end())
        return nullptr;

    if (!it->second.Staging)
        return nullptr;

    return it->second.Staging->GetTexture();
}

_Use_decl_annotations_
bool KFEImagePool::Reload(const std::string& path, KFEGraphicsCommandList* cmdList)
{
    if (!m_bInitialized)
    {
        LOG_ERROR("KFEImagePool::Reload: Image pool is not initialized.");
        return false;
    }

    if (!cmdList || !cmdList->GetNative())
    {
        LOG_ERROR("KFEImagePool::Reload: Command list is null or has no native pointer.");
        return false;
    }

    auto it = m_imagePool.find(path);
    if (it == m_imagePool.end())
    {
        return GetImageSrv(path, cmdList) != nullptr;
    }

    TextureData& data = it->second;

    if (data.Srv && data.Srv->IsInitialize())
    {
        data.Srv->Destroy();
    }

    if (data.Staging && data.Staging->IsInitialized())
    {
        data.Staging->Destroy();
    }

    data.Srv.reset();
    data.Staging.reset();
    data.Name = path;

    if (!LoadTextureInternal(path, cmdList, data))
    {
        LOG_ERROR("KFEImagePool::Reload: Failed to reload texture '{}'.", path);
        return false;
    }

    LOG_SUCCESS("KFEImagePool::Reload: Reloaded texture '{}'.", path);
    return true;
}

void KFEImagePool::Clear() noexcept
{
    for (auto& [key, data] : m_imagePool)
    {
        if (data.Srv && data.Srv->IsInitialize())
        {
            data.Srv->Destroy();
        }

        if (data.Staging && data.Staging->IsInitialized())
        {
            data.Staging->Destroy();
        }

        data.Srv.reset();
        data.Staging.reset();
    }

    m_imagePool.clear();
}

_Use_decl_annotations_
std::size_t KFEImagePool::GetTextureCount() const noexcept
{
    return m_imagePool.size();
}

#pragma endregion

#pragma region Internal

_Use_decl_annotations_
bool KFEImagePool::LoadTextureInternal(
    const std::string& path,
    KFEGraphicsCommandList* cmdList,
    TextureData& outData)
{
    if (!m_pDevice || !m_pDevice->GetNative())
    {
        LOG_ERROR("KFEImagePool::LoadTextureInternal: Device is null.");
        return false;
    }

    if (!m_pResourceHeap)
    {
        LOG_ERROR("KFEImagePool::LoadTextureInternal: Resource heap is null.");
        return false;
    }

    if (!cmdList || !cmdList->GetNative())
    {
        LOG_ERROR("KFEImagePool::LoadTextureInternal: Command list is null.");
        return false;
    }

    int width = 0;
    int height = 0;
    int comp = 0;

    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &comp, STBI_rgb_alpha);
    if (!pixels)
    {
        LOG_ERROR("KFEImagePool::LoadTextureInternal: stb_image failed to load '{}'.", path);
        return false;
    }

    const DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
    const std::uint32_t w = static_cast<std::uint32_t>(width);
    const std::uint32_t h = static_cast<std::uint32_t>(height);

    auto staging = std::make_unique<KFEStagingTexture>();

    KFE_STAGING_TEXTURE_CREATE_DESC sdesc{};
    sdesc.Device = m_pDevice;
    sdesc.Width = w;
    sdesc.Height = h;
    sdesc.Format = format;
    sdesc.MipLevels = 1u;
    sdesc.ArraySize = 1u;

    if (!staging->Initialize(sdesc))
    {
        LOG_ERROR("KFEImagePool::LoadTextureInternal: Failed to create staging texture for '{}'.", path);
        stbi_image_free(pixels);
        return false;
    }

    const std::uint32_t srcRowPitch = w * 4u;
    if (!staging->WritePixels(pixels, srcRowPitch))
    {
        LOG_ERROR("KFEImagePool::LoadTextureInternal: WritePixels failed for '{}'.", path);
        staging->Destroy();
        stbi_image_free(pixels);
        return false;
    }

    stbi_image_free(pixels);

    ID3D12GraphicsCommandList* nativeCmd = cmdList->GetNative();
    if (!staging->RecordUploadToTexture(nativeCmd))
    {
        LOG_ERROR("KFEImagePool::LoadTextureInternal: RecordUploadToTexture failed for '{}'.", path);
        staging->Destroy();
        return false;
    }

    KFETexture* texResource = staging->GetTexture();
    if (!texResource || !texResource->GetNative())
    {
        LOG_ERROR("KFEImagePool::LoadTextureInternal: Staging texture's default resource is null for '{}'.", path);
        staging->Destroy();
        return false;
    }

    auto srv = std::make_unique<KFETextureSRV>();

    KFE_SRV_CREATE_DESC srvDesc{};
    srvDesc.Device = m_pDevice;
    srvDesc.Heap = m_pResourceHeap;
    srvDesc.Texture = texResource;
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.MostDetailedMip = 0u;
    srvDesc.MipLevels = 1u;
    srvDesc.FirstArraySlice = 0u;
    srvDesc.ArraySize = 1u;
    srvDesc.PlaneSlice = 0u;
    srvDesc.DescriptorIndex = KFE_INVALID_INDEX;

    if (!srv->Initialize(srvDesc))
    {
        LOG_ERROR("KFEImagePool::LoadTextureInternal: Failed to create SRV for '{}'.", path);
        staging->Destroy();
        return false;
    }

    outData.Name = path;
    outData.Staging = std::move(staging);
    outData.Srv = std::move(srv);

    LOG_SUCCESS("KFEImagePool::LoadTextureInternal: Loaded texture '{}': {}x{}.", path, width, height);
    return true;
}

#pragma endregion
