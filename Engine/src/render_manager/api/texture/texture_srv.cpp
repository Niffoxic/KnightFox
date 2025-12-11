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
#include "engine/render_manager/api/texture/texture_srv.h"

 //~ Heaps / resources
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/texture/texture.h"

//~ Utils
#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"

#include <d3d12.h>

#pragma region Impl_Declaration

class kfe::KFETextureSRV::Impl
{
public:
    Impl() = default;

    ~Impl()
    {
        if (!Destroy())
        {
            LOG_ERROR("KFETextureSRV::Impl::~Impl: Failed to destroy SRV cleanly.");
        }
    }

    NODISCARD bool Initialize(_In_ const KFE_SRV_CREATE_DESC& desc);

    NODISCARD bool Destroy() noexcept;
    NODISCARD bool IsInitialize() const noexcept;

    NODISCARD KFEResourceHeap* GetAttachedHeap() const noexcept;
    NODISCARD KFETexture* GetTexture() const noexcept;

    NODISCARD std::uint32_t               GetDescriptorIndex() const noexcept;
    NODISCARD bool                        HasValidDescriptor() const noexcept;
    NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept;

    NODISCARD DXGI_FORMAT         GetFormat() const noexcept;
    NODISCARD D3D12_SRV_DIMENSION GetViewDimension() const noexcept;
    NODISCARD std::uint32_t       GetMostDetailedMip() const noexcept;
    NODISCARD std::uint32_t       GetMipLevels() const noexcept;
    NODISCARD std::uint32_t       GetFirstArraySlice() const noexcept;
    NODISCARD std::uint32_t       GetArraySize() const noexcept;
    NODISCARD std::uint32_t       GetPlaneSlice() const noexcept;
    NODISCARD float               GetResourceMinLODClamp() const noexcept;
    NODISCARD std::uint32_t       GetShader4ComponentMapping() const noexcept;

private:
    NODISCARD bool CacheValidateInput(const KFE_SRV_CREATE_DESC& desc);
    void Reset();

private:
    bool             m_bInitialized{ false };
    KFEDevice*       m_pDevice{ nullptr };
    KFEResourceHeap* m_pHeap{ nullptr };
    KFETexture*      m_pTexture{ nullptr };

    DXGI_FORMAT         m_format{ DXGI_FORMAT_UNKNOWN };
    D3D12_SRV_DIMENSION m_viewDimension{ D3D12_SRV_DIMENSION_UNKNOWN };
    std::uint32_t       m_mostDetailedMip{ 0u };
    std::uint32_t       m_mipLevels{ 0u }; // 0xFFFFFFFF == all mips at creation time
    std::uint32_t       m_firstArraySlice{ 0u };
    std::uint32_t       m_arraySize{ 0u };
    std::uint32_t       m_planeSlice{ 0u };
    float               m_resourceMinLODClamp{ 0.0f };
    std::uint32_t       m_shader4ComponentMapping{ D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING };

    std::uint32_t       m_descriptorIndex{ KFE_INVALID_INDEX };
};

#pragma endregion

#pragma region SRV_Implementation

kfe::KFETextureSRV::KFETextureSRV() noexcept
    : m_impl(std::make_unique<kfe::KFETextureSRV::Impl>())
{
}

kfe::KFETextureSRV::~KFETextureSRV() noexcept = default;

kfe::KFETextureSRV::KFETextureSRV(KFETextureSRV&&) noexcept = default;

kfe::KFETextureSRV& kfe::KFETextureSRV::operator=(KFETextureSRV&&) noexcept = default;

_Use_decl_annotations_
bool kfe::KFETextureSRV::Initialize(const KFE_SRV_CREATE_DESC& desc)
{
    return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFETextureSRV::Destroy() const noexcept
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFETextureSRV::IsInitialize() const noexcept
{
    return m_impl->IsInitialize();
}

_Use_decl_annotations_
kfe::KFEResourceHeap* kfe::KFETextureSRV::GetAttachedHeap() const noexcept
{
    return m_impl->GetAttachedHeap();
}

_Use_decl_annotations_
kfe::KFETexture* kfe::KFETextureSRV::GetTexture() const noexcept
{
    return m_impl->GetTexture();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureSRV::GetDescriptorIndex() const noexcept
{
    return m_impl->GetDescriptorIndex();
}

_Use_decl_annotations_
bool kfe::KFETextureSRV::HasValidDescriptor() const noexcept
{
    return m_impl->HasValidDescriptor();
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFETextureSRV::GetCPUHandle() const noexcept
{
    return m_impl->GetCPUHandle();
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFETextureSRV::GetFormat() const noexcept
{
    return m_impl->GetFormat();
}

_Use_decl_annotations_
D3D12_SRV_DIMENSION kfe::KFETextureSRV::GetViewDimension() const noexcept
{
    return m_impl->GetViewDimension();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureSRV::GetMostDetailedMip() const noexcept
{
    return m_impl->GetMostDetailedMip();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureSRV::GetMipLevels() const noexcept
{
    return m_impl->GetMipLevels();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureSRV::GetFirstArraySlice() const noexcept
{
    return m_impl->GetFirstArraySlice();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureSRV::GetArraySize() const noexcept
{
    return m_impl->GetArraySize();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureSRV::GetPlaneSlice() const noexcept
{
    return m_impl->GetPlaneSlice();
}

_Use_decl_annotations_
float kfe::KFETextureSRV::GetResourceMinLODClamp() const noexcept
{
    return m_impl->GetResourceMinLODClamp();
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureSRV::GetShader4ComponentMapping() const noexcept
{
    return m_impl->GetShader4ComponentMapping();
}

std::string kfe::KFETextureSRV::GetName() const noexcept
{
    return "KFETextureSRV";
}

std::string kfe::KFETextureSRV::GetDescription() const noexcept
{
    return "KFETextureSRV: SRV D3D12";
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFETextureSRV::Impl::Initialize(const KFE_SRV_CREATE_DESC& desc)
{
    // Validate + cache everything + allocate descriptor
    if (!CacheValidateInput(desc))
    {
        return false;
    }

    if (m_pDevice == nullptr)
    {
        LOG_ERROR("KFETextureSRV::Impl::Initialize: Cached device is null.");
        Reset();
        return false;
    }

    if (m_pTexture == nullptr)
    {
        LOG_ERROR("KFETextureSRV::Impl::Initialize: Cached texture is null.");
        Reset();
        return false;
    }

    if (m_pHeap == nullptr)
    {
        LOG_ERROR("KFETextureSRV::Impl::Initialize: Cached CBV/SRV/UAV heap is null.");
        Reset();
        return false;
    }

    auto* nativeDevice = m_pDevice->GetNative();
    if (nativeDevice == nullptr)
    {
        LOG_ERROR("KFETextureSRV::Impl::Initialize: Native device is null.");
        Reset();
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC view{};
    view.Format = m_format;
    view.ViewDimension = m_viewDimension;
    view.Shader4ComponentMapping = m_shader4ComponentMapping;

    switch (m_viewDimension)
    {
    case D3D12_SRV_DIMENSION_TEXTURE1D:
        view.Texture1D.MostDetailedMip = m_mostDetailedMip;
        view.Texture1D.MipLevels = m_mipLevels;
        view.Texture1D.ResourceMinLODClamp = m_resourceMinLODClamp;
        break;

    case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
        view.Texture1DArray.MostDetailedMip = m_mostDetailedMip;
        view.Texture1DArray.MipLevels = m_mipLevels;
        view.Texture1DArray.FirstArraySlice = m_firstArraySlice;
        view.Texture1DArray.ArraySize = m_arraySize;
        view.Texture1DArray.ResourceMinLODClamp = m_resourceMinLODClamp;
        break;

    case D3D12_SRV_DIMENSION_TEXTURE2D:
        view.Texture2D.MostDetailedMip = m_mostDetailedMip;
        view.Texture2D.MipLevels = m_mipLevels;
        view.Texture2D.PlaneSlice = m_planeSlice;
        view.Texture2D.ResourceMinLODClamp = m_resourceMinLODClamp;
        break;

    case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
        view.Texture2DArray.MostDetailedMip = m_mostDetailedMip;
        view.Texture2DArray.MipLevels = m_mipLevels;
        view.Texture2DArray.FirstArraySlice = m_firstArraySlice;
        view.Texture2DArray.ArraySize = m_arraySize;
        view.Texture2DArray.PlaneSlice = m_planeSlice;
        view.Texture2DArray.ResourceMinLODClamp = m_resourceMinLODClamp;
        break;

    case D3D12_SRV_DIMENSION_TEXTURE2DMS:
        // Nothing extra to set
        break;

    case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
        view.Texture2DMSArray.FirstArraySlice = m_firstArraySlice;
        view.Texture2DMSArray.ArraySize = m_arraySize;
        break;

    case D3D12_SRV_DIMENSION_TEXTURE3D:
        view.Texture3D.MostDetailedMip = m_mostDetailedMip;
        view.Texture3D.MipLevels = m_mipLevels;
        view.Texture3D.ResourceMinLODClamp = m_resourceMinLODClamp;
        break;

    case D3D12_SRV_DIMENSION_TEXTURECUBE:
        view.TextureCube.MostDetailedMip = m_mostDetailedMip;
        view.TextureCube.MipLevels = m_mipLevels;
        view.TextureCube.ResourceMinLODClamp = m_resourceMinLODClamp;
        break;

    case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
        view.TextureCubeArray.MostDetailedMip = m_mostDetailedMip;
        view.TextureCubeArray.MipLevels = m_mipLevels;
        view.TextureCubeArray.First2DArrayFace = m_firstArraySlice;
        view.TextureCubeArray.NumCubes = m_arraySize;
        view.TextureCubeArray.ResourceMinLODClamp = m_resourceMinLODClamp;
        break;

    case D3D12_SRV_DIMENSION_UNKNOWN:
    default:
        LOG_ERROR("KFETextureSRV::Impl::Initialize: Unsupported or UNKNOWN SRV view dimension.");
        Reset();
        return false;
    }

    if (!HasValidDescriptor())
    {
        LOG_ERROR("KFETextureSRV::Impl::Initialize: Descriptor index is not valid after CacheValidateInput.");
        Reset();
        return false;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_pHeap->GetHandle(m_descriptorIndex);

    nativeDevice->CreateShaderResourceView(
        m_pTexture->GetNative(),
        &view,
        cpuHandle
    );

    LOG_SUCCESS("KFETextureSRV::Impl::Initialize: SRV created. HeapIdx = {}, Format = {}, Dimension = {}",
        m_descriptorIndex,
        static_cast<int>(m_format),
        static_cast<int>(m_viewDimension));

    return true;
}

_Use_decl_annotations_
bool kfe::KFETextureSRV::Impl::Destroy() noexcept
{
    if (!m_bInitialized)
    {
        return true;
    }

    if (m_pHeap != nullptr &&
        m_descriptorIndex != KFE_INVALID_INDEX &&
        m_pHeap->IsValidIndex(m_descriptorIndex))
    {
        if (!m_pHeap->Free(m_descriptorIndex))
        {
            LOG_WARNING("KFETextureSRV::Impl::Destroy: Failed to free descriptor index {}.", m_descriptorIndex);
        }
    }

    Reset();

    LOG_SUCCESS("KFETextureSRV::Impl::Destroy: SRV destroyed and state reset.");
    return true;
}

_Use_decl_annotations_
bool kfe::KFETextureSRV::Impl::IsInitialize() const noexcept
{
    return m_bInitialized;
}

_Use_decl_annotations_
kfe::KFEResourceHeap* kfe::KFETextureSRV::Impl::GetAttachedHeap() const noexcept
{
    return m_pHeap;
}

_Use_decl_annotations_
kfe::KFETexture* kfe::KFETextureSRV::Impl::GetTexture() const noexcept
{
    return m_pTexture;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureSRV::Impl::GetDescriptorIndex() const noexcept
{
    return m_descriptorIndex;
}

_Use_decl_annotations_
bool kfe::KFETextureSRV::Impl::HasValidDescriptor() const noexcept
{
    if (m_pHeap == nullptr)
    {
        return false;
    }

    if (m_descriptorIndex == KFE_INVALID_INDEX)
    {
        return false;
    }

    return m_pHeap->IsValidIndex(m_descriptorIndex);
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFETextureSRV::Impl::GetCPUHandle() const noexcept
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};
    handle.ptr = 0;

    if (!HasValidDescriptor())
    {
        return handle;
    }

    return m_pHeap->GetHandle(m_descriptorIndex);
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFETextureSRV::Impl::GetFormat() const noexcept
{
    return m_format;
}

_Use_decl_annotations_
D3D12_SRV_DIMENSION kfe::KFETextureSRV::Impl::GetViewDimension() const noexcept
{
    return m_viewDimension;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureSRV::Impl::GetMostDetailedMip() const noexcept
{
    return m_mostDetailedMip;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureSRV::Impl::GetMipLevels() const noexcept
{
    return m_mipLevels;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureSRV::Impl::GetFirstArraySlice() const noexcept
{
    return m_firstArraySlice;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureSRV::Impl::GetArraySize() const noexcept
{
    return m_arraySize;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureSRV::Impl::GetPlaneSlice() const noexcept
{
    return m_planeSlice;
}

_Use_decl_annotations_
float kfe::KFETextureSRV::Impl::GetResourceMinLODClamp() const noexcept
{
    return m_resourceMinLODClamp;
}

_Use_decl_annotations_
std::uint32_t kfe::KFETextureSRV::Impl::GetShader4ComponentMapping() const noexcept
{
    return m_shader4ComponentMapping;
}

_Use_decl_annotations_
bool kfe::KFETextureSRV::Impl::CacheValidateInput(const KFE_SRV_CREATE_DESC& desc)
{
    if (m_bInitialized)
    {
        LOG_WARNING("KFETextureSRV::Impl::Initialize: Already initialized. Destroying previous state.");
        if (!Destroy())
        {
            LOG_ERROR("KFETextureSRV::Impl::Initialize: Failed to destroy previous state.");
            return false;
        }
    }

    if (desc.Device == nullptr)
    {
        LOG_ERROR("KFETextureSRV::Impl::Initialize: Device is null.");
        return false;
    }

    if (desc.Heap == nullptr)
    {
        LOG_ERROR("KFETextureSRV::Impl::Initialize: CBV/SRV/UAV heap is null.");
        return false;
    }

    if (desc.Texture == nullptr)
    {
        LOG_ERROR("KFETextureSRV::Impl::Initialize: Texture is null.");
        return false;
    }

    m_pDevice = desc.Device;
    m_pHeap = desc.Heap;
    m_pTexture = desc.Texture;

    // View parameters
    m_format = (desc.Format != DXGI_FORMAT_UNKNOWN)
        ? desc.Format
        : m_pTexture->GetFormat();

    m_viewDimension = desc.ViewDimension;
    m_mostDetailedMip = desc.MostDetailedMip;
    m_mipLevels = (desc.MipLevels == 0u) ? 0xFFFFFFFFu : desc.MipLevels; // 0 => all mips
    m_firstArraySlice = desc.FirstArraySlice;
    m_arraySize = desc.ArraySize;
    m_planeSlice = desc.PlaneSlice;
    m_resourceMinLODClamp = desc.ResourceMinLODClamp;
    m_shader4ComponentMapping =
        (desc.Shader4ComponentMapping == 0u)
        ? static_cast<std::uint32_t>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING)
        : desc.Shader4ComponentMapping;

    std::uint32_t index = desc.DescriptorIndex;

    if (index == KFE_INVALID_INDEX)
    {
        index = m_pHeap->Allocate();
        if (index == KFE_INVALID_INDEX)
        {
            LOG_ERROR("KFETextureSRV::Impl::Initialize: Failed to allocate descriptor from CBV/SRV/UAV heap.");
            Reset();
            return false;
        }
    }
    else
    {
        if (!m_pHeap->IsValidIndex(index))
        {
            LOG_ERROR("KFETextureSRV::Impl::Initialize: Provided descriptor index {} is invalid for this heap.", index);
            Reset();
            return false;
        }
    }

    m_descriptorIndex = index;
    m_bInitialized = true;

    LOG_SUCCESS("KFETextureSRV::Impl::Initialize: SRV cached. HeapIdx = {}, Format = {}, Dimension = {}",
        m_descriptorIndex,
        static_cast<int>(m_format),
        static_cast<int>(m_viewDimension));

    return true;
}

void kfe::KFETextureSRV::Impl::Reset()
{
    m_pDevice = nullptr;
    m_pHeap = nullptr;
    m_pTexture = nullptr;
    m_format = DXGI_FORMAT_UNKNOWN;
    m_viewDimension = D3D12_SRV_DIMENSION_UNKNOWN;
    m_mostDetailedMip = 0u;
    m_mipLevels = 0u;
    m_firstArraySlice = 0u;
    m_arraySize = 0u;
    m_planeSlice = 0u;
    m_resourceMinLODClamp = 0.0f;
    m_shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_descriptorIndex = KFE_INVALID_INDEX;
    m_bInitialized = false;
}

#pragma endregion
