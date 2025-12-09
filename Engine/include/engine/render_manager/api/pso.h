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

#include <memory>
#include <cstdint>

struct ID3D12PipelineState;
struct ID3D12RootSignature;
struct D3D12_SHADER_BYTECODE;
struct D3D12_INPUT_ELEMENT_DESC;
struct D3D12_RASTERIZER_DESC;
struct D3D12_BLEND_DESC;
struct D3D12_DEPTH_STENCIL_DESC;
enum   D3D12_PRIMITIVE_TOPOLOGY_TYPE;
enum   D3D12_PIPELINE_STATE_FLAGS;
enum   DXGI_FORMAT;

namespace kfe
{
	class KFEDevice;

	class KFE_API KFEPipelineState final: public IKFEObject
	{
	public:
		KFEPipelineState ();
		~KFEPipelineState() override;

		KFEPipelineState(const KFEPipelineState&) noexcept = delete;
		KFEPipelineState(KFEPipelineState&&) noexcept;

		KFEPipelineState& operator=(const KFEPipelineState&) noexcept = delete;
		KFEPipelineState& operator=(KFEPipelineState&&) noexcept;

		NODISCARD std::string GetName		() const noexcept override;
		NODISCARD std::string GetDescription() const noexcept override;

		NODISCARD bool Build(_In_ KFEDevice* device) noexcept;
		void Destroy		() noexcept;

		NODISCARD bool				   IsInitialized() const noexcept;
		NODISCARD ID3D12PipelineState* GetNative	() const noexcept;

		//~ Setters
		void SetRootSignature(_In_opt_ ID3D12RootSignature* rs)		noexcept;
		void SetVS			 (_In_ const D3D12_SHADER_BYTECODE& bc) noexcept;
		void SetPS			 (_In_ const D3D12_SHADER_BYTECODE& bc) noexcept;
		void SetGS			 (_In_ const D3D12_SHADER_BYTECODE& bc) noexcept;
		void SetHS			 (_In_ const D3D12_SHADER_BYTECODE& bc) noexcept;
		void SetDS			 (_In_ const D3D12_SHADER_BYTECODE& bc) noexcept;

		void SetInputLayout(_In_reads_opt_(count) const D3D12_INPUT_ELEMENT_DESC* elem,
							_In_                  std::uint32_t                   count);

		void SetRasterizer   (_In_ const D3D12_RASTERIZER_DESC& rs)	   noexcept;
		void SetBlend	     (_In_ const D3D12_BLEND_DESC& desc)	   noexcept;
		void SetDepthStencil (_In_ const D3D12_DEPTH_STENCIL_DESC& ds) noexcept;
		void SetPrimitiveType(_In_ D3D12_PRIMITIVE_TOPOLOGY_TYPE type) noexcept;

		void SetRTVFormat(_In_ std::uint32_t slot,
						  _In_ DXGI_FORMAT format) noexcept;

		void SetRTVCount (_In_ std::uint32_t count) noexcept;
		void SetDSVFormat(_In_ DXGI_FORMAT format)  noexcept;

		void SetSampleDesc(_In_ std::uint32_t count,
						  _In_ std::uint32_t quality = 0u) noexcept;

		void SetSampleMask(_In_ std::uint32_t mask)				  noexcept;
		void SetFlags	  (_In_ D3D12_PIPELINE_STATE_FLAGS flags) noexcept;

		//~ Getters
		NODISCARD ID3D12RootSignature*  GetRootSignature() const noexcept;
		NODISCARD D3D12_SHADER_BYTECODE GetVS			() const noexcept;
		NODISCARD D3D12_SHADER_BYTECODE GetPS			() const noexcept;
		NODISCARD D3D12_SHADER_BYTECODE GetGS			() const noexcept;
		NODISCARD D3D12_SHADER_BYTECODE GetHS			() const noexcept;
		NODISCARD D3D12_SHADER_BYTECODE GetDS			() const noexcept;

		NODISCARD _Ret_opt_ 
		const D3D12_INPUT_ELEMENT_DESC* GetInputLayoutElems() const noexcept;
		
		NODISCARD std::uint32_t					GetInputLayoutCount	() const noexcept;
		NODISCARD D3D12_RASTERIZER_DESC			GetRasterizer		() const noexcept;
		NODISCARD D3D12_BLEND_DESC				GetBlend			() const noexcept;
		NODISCARD D3D12_DEPTH_STENCIL_DESC		GetDepthStencil		() const noexcept;
		NODISCARD D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveType	() const noexcept;
		NODISCARD DXGI_FORMAT					GetRTVFormat		(_In_ std::uint32_t slot) const noexcept;
		NODISCARD std::uint32_t					GetRTVCount			() const noexcept;
		NODISCARD DXGI_FORMAT					GetDSVFormat		() const noexcept;
		NODISCARD std::uint32_t					GetSampleCount		() const noexcept;
		NODISCARD std::uint32_t					GetSampleQuality	() const noexcept;
		NODISCARD std::uint32_t					GetSampleMask		() const noexcept;
		NODISCARD D3D12_PIPELINE_STATE_FLAGS	GetFlags			() const noexcept;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
} // namespace kfe
