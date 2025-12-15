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

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

#include "engine/render_manager/api/components/device.h"
#include "engine/system/common_types.h"

namespace kfe
{
	class KFEResourceHeap;
	class KFEDSVHeap;
	class KFESamplerHeap;

	typedef struct _KFE_SHADOW_MAP_CREATE_DESC
	{
		KFEDevice* Device = nullptr;

		KFEDSVHeap*		 DSVHeap	  = nullptr;
		KFEResourceHeap* ResourceHeap = nullptr; 
		KFESamplerHeap*  SamplerHeap  = nullptr;

		std::uint32_t     Width  = 2048u;
		std::uint32_t     Height = 2048u;

		DXGI_FORMAT       DepthFormat = DXGI_FORMAT_D32_FLOAT;
		DXGI_FORMAT       SRVFormat   = DXGI_FORMAT_R32_FLOAT;

		const wchar_t* DebugName = L"KFEShadowMap";

	} KFE_SHADOW_MAP_CREATE_DESC;

	class KFE_API KFEShadowMap final : public IKFEObject
	{
	public:
		 KFEShadowMap() noexcept;
		~KFEShadowMap() override = default;

		NODISCARD bool Initialize(const KFE_SHADOW_MAP_CREATE_DESC& desc) noexcept;

		NODISCARD ID3D12Resource*			  GetResource() const noexcept;
		NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetDSV	 () const noexcept;
		NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE GetSRV	 () const noexcept;
		NODISCARD std::uint32_t GetHandle() const noexcept;

		NODISCARD std::uint32_t GetWidth() const noexcept;
		NODISCARD std::uint32_t GetHeight() const noexcept;

		NODISCARD bool CreateComparisonSampler(
			KFESamplerHeap* samplerHeap,
			D3D12_GPU_DESCRIPTOR_HANDLE& outGpuHandle) noexcept;

		//~ IKFEObject
		std::string GetName		  () const noexcept override;
		std::string GetDescription() const noexcept override;

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_shadowTex;

		KFEDevice* m_pDevice{ nullptr };
		D3D12_CPU_DESCRIPTOR_HANDLE m_dsv{};
		D3D12_GPU_DESCRIPTOR_HANDLE m_srv{};
		std::uint32_t m_handle{ KFE_INVALID_INDEX };

		std::uint32_t m_width = 0u;
		std::uint32_t m_height = 0u;

		DXGI_FORMAT m_depthFormat = DXGI_FORMAT_D32_FLOAT;
		DXGI_FORMAT m_srvFormat   = DXGI_FORMAT_R32_FLOAT;
	};
} // namespace kfe
