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
#include "engine/render_manager/shadow/shadow_map.h"
#include "engine/utils/helpers.h"

#include "engine/utils/logger.h"

#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"
#include "engine/render_manager/api/heap/heap_dsv.h"
#include "engine/render_manager/api/heap/heap_sampler.h"

#include <algorithm>

namespace kfe
{
	KFEShadowMap::KFEShadowMap() noexcept
	{
		m_dsv.ptr = 0u;
		m_srv.ptr = 0u;
	}

	std::string KFEShadowMap::GetName() const noexcept
	{
		return "KFEShadowMap";
	}

	std::string KFEShadowMap::GetDescription() const noexcept
	{
		return "Depth shadow map resource (D32) with DSV + SRV (R32_FLOAT).";
	}

	_Use_decl_annotations_
	ID3D12Resource* KFEShadowMap::GetResource() const noexcept
	{
		return m_shadowTex.Get();
	}

	_Use_decl_annotations_
	D3D12_CPU_DESCRIPTOR_HANDLE KFEShadowMap::GetDSV() const noexcept
	{
		return m_dsv;
	}

	_Use_decl_annotations_
	D3D12_GPU_DESCRIPTOR_HANDLE KFEShadowMap::GetSRV() const noexcept
	{
		return m_srv;
	}

	_Use_decl_annotations_
	std::uint32_t KFEShadowMap::GetWidth() const noexcept { return m_width; }

	_Use_decl_annotations_
	std::uint32_t KFEShadowMap::GetHeight() const noexcept { return m_height; }

    NODISCARD bool KFEShadowMap::Initialize(const KFE_SHADOW_MAP_CREATE_DESC& desc) noexcept
    {
        if (!desc.Device || !desc.DSVHeap || !desc.ResourceHeap)
        {
            LOG_ERROR("ShadowMap Initialize failed: Device/DSVHeap/ResourceHeap is null.");
            return false;
        }

        m_pDevice = desc.Device;

        m_width       = std::max(1u, desc.Width);
        m_height      = std::max(1u, desc.Height);
        m_depthFormat = desc.DepthFormat;
        m_srvFormat   = desc.SRVFormat;

        ID3D12Device* dev = m_pDevice->GetNative();
        if (!dev)
        {
            LOG_ERROR("ShadowMap Initialize failed: native device is null.");
            return false;
        }

        //~ Create depth texture
        D3D12_RESOURCE_DESC tex{};
        tex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        tex.Alignment = 0u;
        tex.Width = m_width;
        tex.Height = m_height;
        tex.DepthOrArraySize = 1;
        tex.MipLevels = 1;
        tex.Format = m_depthFormat; //~ DXGI_FORMAT_D32_FLOAT
        tex.SampleDesc.Count = 1;
        tex.SampleDesc.Quality = 0;
        tex.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        tex.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clear{};
        clear.Format = m_depthFormat;
        clear.DepthStencil.Depth = 1.0f;
        clear.DepthStencil.Stencil = 0;

        D3D12_HEAP_PROPERTIES heap{};
        heap.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap.CreationNodeMask = 1;
        heap.VisibleNodeMask = 1;

        HRESULT hr = dev->CreateCommittedResource(
            &heap,
            D3D12_HEAP_FLAG_NONE,
            &tex,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clear,
            IID_PPV_ARGS(m_shadowTex.ReleaseAndGetAddressOf()));

        if (FAILED(hr) || !m_shadowTex)
        {
            LOG_ERROR("CreateCommittedResource failed for ShadowMap.");
            return false;
        }

        if (desc.DebugName)
            m_shadowTex->SetName(desc.DebugName);

        //~ Allocate DSV
        std::uint32_t dsvIndex = desc.DSVHeap->Allocate();
        if (dsvIndex == KFE_INVALID_INDEX)
        {
            LOG_ERROR("ShadowMap DSV allocation failed.");
            return false;
        }

        m_dsv = desc.DSVHeap->GetHandle(dsvIndex);
        if (m_dsv.ptr == 0u)
        {
            LOG_ERROR("ShadowMap DSV handle is invalid.");
            return false;
        }

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = m_depthFormat; //~ DXGI_FORMAT_D32_FLOAT
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.Texture2D.MipSlice = 0;

        dev->CreateDepthStencilView(m_shadowTex.Get(), &dsvDesc, m_dsv);

        //~ Allocate SRV
        std::uint32_t srvIndex = desc.ResourceHeap->Allocate();
        if (srvIndex == KFE_INVALID_INDEX)
        {
            LOG_ERROR("ShadowMap SRV allocation failed.");
            return false;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE srvCpu = desc.ResourceHeap->GetHandle(srvIndex);
        m_srv = desc.ResourceHeap->GetGPUHandle(srvIndex);

        if (srvCpu.ptr == 0u || m_srv.ptr == 0u)
        {
            LOG_ERROR("ShadowMap SRV handle(s) invalid.");
            return false;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = m_srvFormat; //~ DXGI_FORMAT_R32_FLOAT
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        dev->CreateShaderResourceView(m_shadowTex.Get(), &srvDesc, srvCpu);

        LOG_SUCCESS("ShadowMap created: DSV + SRV.");
        return true;
    }

    NODISCARD bool KFEShadowMap::CreateComparisonSampler(
        KFESamplerHeap* samplerHeap,
        D3D12_GPU_DESCRIPTOR_HANDLE& outGpuHandle) noexcept
    {
        if (!samplerHeap)
            return false;

        const std::uint32_t sampIndex = samplerHeap->Allocate();
        if (sampIndex == KFE_INVALID_INDEX)
            return false;

        D3D12_CPU_DESCRIPTOR_HANDLE cpu = samplerHeap->GetHandle(sampIndex);
        outGpuHandle = samplerHeap->GetGPUHandle(sampIndex);

        if (cpu.ptr == 0u || outGpuHandle.ptr == 0u)
            return false;

        D3D12_SAMPLER_DESC samp{};
        samp.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        samp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.MipLODBias = 0.0f;
        samp.MaxAnisotropy = 1;
        samp.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        samp.BorderColor[0] = 0.0f;
        samp.BorderColor[1] = 0.0f;
        samp.BorderColor[2] = 0.0f;
        samp.BorderColor[3] = 0.0f;
        samp.MinLOD = 0.0f;
        samp.MaxLOD = D3D12_FLOAT32_MAX;

        ID3D12Device* dev = m_pDevice ? m_pDevice->GetNative() : nullptr;
        if (!dev)
            return false;

        dev->CreateSampler(&samp, cpu);
        return true;
    }

} // namespace kfe
