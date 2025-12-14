// texture_library.cpp

#include "pch.h"

#include "engine/render_manager/assets_library/texture_library.h"

#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/commands/graphics_list.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"

#include "engine/render_manager/assets_library/shader_library.h"

#include <d3d12.h>
#include <dxgiformat.h>
#include <algorithm>
#include <wrl/client.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace kfe;
using Microsoft::WRL::ComPtr;

namespace
{
    static std::uint32_t CalcMipLevels(std::uint32_t w, std::uint32_t h) noexcept
    {
        std::uint32_t levels = 1u;
        while (w > 1u || h > 1u)
        {
            w = std::max(1u, w >> 1u);
            h = std::max(1u, h >> 1u);
            ++levels;
        }
        return levels;
    }

    struct MipGenConstants
    {
        UINT SrcWidth;
        UINT SrcHeight;
        UINT DstWidth;
        UINT DstHeight;
    };

    inline UINT CalcSubresourceIndex(
        UINT mipSlice,
        UINT arraySlice,
        UINT planeSlice,
        UINT mipLevels,
        UINT arraySize) noexcept
    {
        return mipSlice
            + arraySlice * mipLevels
            + planeSlice * mipLevels * arraySize;
    }
}

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
    m_bMipGenInitialized = false;
    m_pMipGenRootSignature.Reset();
    m_pMipGenPSO.Reset();

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
    ID3D12GraphicsCommandList* cmdList)
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

    if (!cmdList)
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
bool KFEImagePool::Reload(const std::string& path, ID3D12GraphicsCommandList* cmdList)
{
    if (!m_bInitialized)
    {
        LOG_ERROR("KFEImagePool::Reload: Image pool is not initialized.");
        return false;
    }

    if (!cmdList)
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
        if (!data.Staging->Destroy()) LOG_ERROR("HUGE LEAK!!!!!!!!!!!!!! ALREAT!!");
    }

    if (data.Staging && data.Staging->IsInitialized())
    {
        if (!data.Staging->Destroy()) LOG_ERROR("HUGE LEAK!!!!!!!!!!!!!! ALREAT!!");
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
            if (!data.Staging->Destroy()) LOG_ERROR("HUGE LEAK!!!!!!!!!!!!!! ALREAT!!");
        }

        if (data.Staging && data.Staging->IsInitialized())
        {
            if (!data.Staging->Destroy()) LOG_ERROR("HUGE LEAK!!!!!!!!!!!!!! ALREAT!!");
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

#pragma region Internal_Load

_Use_decl_annotations_
bool KFEImagePool::LoadTextureInternal(
    const std::string& path,
    ID3D12GraphicsCommandList* cmdList,
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

    if (!cmdList)
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

    const DXGI_FORMAT   format = DXGI_FORMAT_R8G8B8A8_UNORM;
    const std::uint32_t w = static_cast<std::uint32_t>(width);
    const std::uint32_t h = static_cast<std::uint32_t>(height);

    const std::uint32_t mipLevels = CalcMipLevels(w, h);

    auto staging = std::make_unique<KFEStagingTexture>();

    KFE_STAGING_TEXTURE_CREATE_DESC sdesc{};
    sdesc.Device = m_pDevice;
    sdesc.Width = w;
    sdesc.Height = h;
    sdesc.Format = format;
    sdesc.MipLevels = mipLevels; // texture will have full mip chain
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
        if (!staging->Destroy()) LOG_ERROR("HUGE LEAK!!!!!!!!!!!!!! ALREAT!!");
        stbi_image_free(pixels);
        return false;
    }

    stbi_image_free(pixels);

    ID3D12GraphicsCommandList* nativeCmd = cmdList;
    if (!staging->RecordUploadToTexture(nativeCmd))
    {
        LOG_ERROR("KFEImagePool::LoadTextureInternal: RecordUploadToTexture failed for '{}'.", path);
        if (!staging->Destroy()) LOG_ERROR("HUGE LEAK!!!!!!!!!!!!!! ALREAT!!");
        return false;
    }

    KFETexture* texResource = staging->GetTexture();
    if (!texResource || !texResource->GetNative())
    {
        LOG_ERROR("KFEImagePool::LoadTextureInternal: Staging texture's default resource is null for '{}'.", path);
        if (!staging->Destroy()) LOG_ERROR("HUGE LEAK!!!!!!!!!!!!!! ALREAT!!");
        return false;
    }

    // Generate mipmaps on the GPU
    if (!GenerateMips(texResource, w, h, cmdList))
    {
        LOG_WARNING("KFEImagePool::LoadTextureInternal: GenerateMips failed for '{}'. Using base level only.", path);
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
    srvDesc.MipLevels = mipLevels; // full chain
    srvDesc.FirstArraySlice = 0u;
    srvDesc.ArraySize = 1u;
    srvDesc.PlaneSlice = 0u;
    srvDesc.DescriptorIndex = KFE_INVALID_INDEX;

    if (!srv->Initialize(srvDesc))
    {
        LOG_ERROR("KFEImagePool::LoadTextureInternal: Failed to create SRV for '{}'.", path);
        if (!staging->Destroy()) LOG_ERROR("HUGE LEAK!!!!!!!!!!!!!! ALREAT!!");
        return false;
    }

    outData.Name = path;
    outData.Staging = std::move(staging);
    outData.Srv = std::move(srv);
    outData.Width = w;
    outData.Height = h;
    outData.Mips = mipLevels;

    LOG_SUCCESS("KFEImagePool::LoadTextureInternal: Loaded texture '{}': {}x{}, {} mips.",
        path, width, height, mipLevels);
    return true;
}

#pragma endregion

#pragma region Internal_MipGen

bool KFEImagePool::InitializeMipGenPipeline()
{
    if (m_bMipGenInitialized)
        return true;

    if (!m_pDevice || !m_pDevice->GetNative())
    {
        LOG_ERROR("KFEImagePool::InitializeMipGenPipeline: Device is null.");
        return false;
    }

    ID3D12Device* device = m_pDevice->GetNative();
    D3D12_ROOT_PARAMETER rootParams[3]{};

    // Constants
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParams[0].Constants.Num32BitValues = 4;
    rootParams[0].Constants.ShaderRegister = 0; // b0
    rootParams[0].Constants.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // SRV table
    D3D12_DESCRIPTOR_RANGE srvRange{};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0; // t0
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = 0;

    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &srvRange;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // UAV table
    D3D12_DESCRIPTOR_RANGE uavRange{};
    uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange.NumDescriptors = 1;
    uavRange.BaseShaderRegister = 0; // u0
    uavRange.RegisterSpace = 0;
    uavRange.OffsetInDescriptorsFromTableStart = 0;

    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[2].DescriptorTable.pDescriptorRanges = &uavRange;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = _countof(rootParams);
    rsDesc.pParameters = rootParams;
    rsDesc.NumStaticSamplers = 0;
    rsDesc.pStaticSamplers = nullptr;
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> rsBlob;
    ComPtr<ID3DBlob> rsError;

    HRESULT hr = D3D12SerializeRootSignature(
        &rsDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &rsBlob,
        &rsError);

    if (FAILED(hr))
    {
        if (rsError)
        {
            LOG_ERROR("KFEImagePool::InitializeMipGenPipeline: Root signature serialize failed: {}",
                static_cast<const char*>(rsError->GetBufferPointer()));
        }
        else
        {
            LOG_ERROR("KFEImagePool::InitializeMipGenPipeline: Root signature serialize failed (hr=0x{:08X}).",
                static_cast<unsigned int>(hr));
        }
        return false;
    }

    hr = device->CreateRootSignature(
        0,
        rsBlob->GetBufferPointer(),
        rsBlob->GetBufferSize(),
        IID_PPV_ARGS(m_pMipGenRootSignature.ReleaseAndGetAddressOf()));

    if (FAILED(hr))
    {
        LOG_ERROR("KFEImagePool::InitializeMipGenPipeline: CreateRootSignature failed (hr=0x{:08X}).",
            static_cast<unsigned int>(hr));
        return false;
    }

    // Compute shader
    ID3DBlob* csBlob = shaders::GetOrCompile("shaders/mipgen_cs.hlsl", "main", "cs_5_0");
    if (!csBlob)
    {
        LOG_ERROR("KFEImagePool::InitializeMipGenPipeline: shaders::GetOrCompile returned null for mipgen_cs.hlsl.");
        return false;
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = m_pMipGenRootSignature.Get();
    psoDesc.CS = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    hr = device->CreateComputePipelineState(
        &psoDesc,
        IID_PPV_ARGS(m_pMipGenPSO.ReleaseAndGetAddressOf()));

    if (FAILED(hr))
    {
        LOG_ERROR("KFEImagePool::InitializeMipGenPipeline: CreateComputePipelineState failed (hr=0x{:08X}).",
            static_cast<unsigned int>(hr));
        return false;
    }

    m_bMipGenInitialized = true;
    LOG_SUCCESS("KFEImagePool::InitializeMipGenPipeline: Mip generation pipeline initialized.");
    return true;
}

_Use_decl_annotations_
bool KFEImagePool::GenerateMips(
    KFETexture* texture,
    std::uint32_t width,
    std::uint32_t height,
    ID3D12GraphicsCommandList* cmdList)
{
    if (!texture || !texture->GetNative())
    {
        LOG_ERROR("KFEImagePool::GenerateMips: Texture is null.");
        return false;
    }

    if (!cmdList)
    {
        LOG_ERROR("KFEImagePool::GenerateMips: Command list is null.");
        return false;
    }

    if (!m_pResourceHeap)
    {
        LOG_ERROR("KFEImagePool::GenerateMips: Resource heap is null.");
        return false;
    }

    if (!InitializeMipGenPipeline())
    {
        LOG_ERROR("KFEImagePool::GenerateMips: Failed to initialize mip-gen pipeline.");
        return false;
    }

    ID3D12Resource* resource = texture->GetNative();
    const auto      desc = resource->GetDesc();

    if (desc.MipLevels <= 1)
    {
        return true;
    }

    ID3D12GraphicsCommandList* nativeCmd = cmdList;

    // Bind descriptor heap
    ID3D12DescriptorHeap* heaps[] =
    {
        m_pResourceHeap->GetNative()
    };
    nativeCmd->SetDescriptorHeaps(_countof(heaps), heaps);

    nativeCmd->SetComputeRootSignature(m_pMipGenRootSignature.Get());
    nativeCmd->SetPipelineState(m_pMipGenPSO.Get());

    const UINT totalMips = desc.MipLevels;

    const std::uint32_t descriptorCount = (totalMips - 1u) * 2u; // SRV and UAV per pass
    const std::uint32_t baseIndex =
        m_pResourceHeap->Allocate(descriptorCount);

    if (baseIndex == KFE_INVALID_INDEX)
    {
        LOG_ERROR("KFEImagePool::GenerateMips: Failed to allocate {} descriptors.", descriptorCount);
        return false;
    }

    UINT srcWidth  = width;
    UINT srcHeight = height;

    for (UINT mip = 1; mip < totalMips; ++mip)
    {
        const UINT dstWidth = std::max<UINT>(1u, srcWidth >> 1u);
        const UINT dstHeight = std::max<UINT>(1u, srcHeight >> 1u);

        const UINT srcMip = mip - 1u;
        const UINT dstMip = mip;

        const std::uint32_t srvIndex = baseIndex + (mip - 1u) * 2u;
        const std::uint32_t uavIndex = srvIndex + 1u;

        // SRV for source mip
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = desc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MostDetailedMip = srcMip;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

            const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle =
                m_pResourceHeap->GetHandle(srvIndex);

            m_pDevice->GetNative()->CreateShaderResourceView(
                resource,
                &srvDesc,
                cpuHandle);
        }

        // UAV for dest mip
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
            uavDesc.Format = desc.Format;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = dstMip;
            uavDesc.Texture2D.PlaneSlice = 0;

            const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle =
                m_pResourceHeap->GetHandle(uavIndex);

            m_pDevice->GetNative()->CreateUnorderedAccessView(
                resource,
                nullptr,
                &uavDesc,
                cpuHandle);
        }

        const UINT subresourceSrc = CalcSubresourceIndex(srcMip, 0, 0, totalMips, 1);
        const UINT subresourceDst = CalcSubresourceIndex(dstMip, 0, 0, totalMips, 1);

        // Transition:
        // src: PIXEL_SHADER_RESOURCE to NON_PIXEL_SHADER_RESOURCE
        // dst: PIXEL_SHADER_RESOURCE to UNORDERED_ACCESS
        {
            D3D12_RESOURCE_BARRIER barriers[2]{};

            barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers[0].Transition.pResource = resource;
            barriers[0].Transition.Subresource = subresourceSrc;
            barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

            barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers[1].Transition.pResource = resource;
            barriers[1].Transition.Subresource = subresourceDst;
            barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

            nativeCmd->ResourceBarrier(_countof(barriers), barriers);
        }

        // Root constants
        MipGenConstants constants{};
        constants.SrcWidth = srcWidth;
        constants.SrcHeight = srcHeight;
        constants.DstWidth = dstWidth;
        constants.DstHeight = dstHeight;

        nativeCmd->SetComputeRoot32BitConstants(
            0,  // root param index
            4,  // num values
            &constants,
            0);

        const D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle =
            m_pResourceHeap->GetGPUHandle(srvIndex);
        const D3D12_GPU_DESCRIPTOR_HANDLE uavGpuHandle =
            m_pResourceHeap->GetGPUHandle(uavIndex);

        nativeCmd->SetComputeRootDescriptorTable(1, srvGpuHandle); // SRV table
        nativeCmd->SetComputeRootDescriptorTable(2, uavGpuHandle); // UAV table

        const UINT threadsX = (dstWidth + 7u) / 8u;
        const UINT threadsY = (dstHeight + 7u) / 8u;

        nativeCmd->Dispatch(threadsX, threadsY, 1u);

        // UAV barrier
        {
            D3D12_RESOURCE_BARRIER uavBarrier{};
            uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            uavBarrier.UAV.pResource = resource;
            nativeCmd->ResourceBarrier(1, &uavBarrier);
        }

        // Back to PIXEL_SHADER_RESOURCE for both source and dest
        {
            D3D12_RESOURCE_BARRIER barriers[2]{};

            barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers[0].Transition.pResource = resource;
            barriers[0].Transition.Subresource = subresourceSrc;
            barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

            barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers[1].Transition.pResource = resource;
            barriers[1].Transition.Subresource = subresourceDst;
            barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

            nativeCmd->ResourceBarrier(_countof(barriers), barriers);
        }

        srcWidth = dstWidth;
        srcHeight = dstHeight;
    }
    return true;
}

#pragma endregion
