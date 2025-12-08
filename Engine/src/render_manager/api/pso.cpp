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
#include "engine/render_manager/api/pso.h"

#include <dxgi.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <algorithm>

#include "engine/render_manager/api/components/device.h"
#include "engine/utils/logger.h"


#pragma region Impl_Definition

class kfe::KFEPipelineState::Impl
{
public:
	 Impl();
	~Impl();

	NODISCARD bool Build(_In_ KFEDevice* device) noexcept;
	
	void Destroy() noexcept;

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

	void SetRasterizer	 (_In_ const D3D12_RASTERIZER_DESC& rs)	   noexcept;
	void SetBlend		 (_In_ const D3D12_BLEND_DESC& desc)	   noexcept;
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
	NODISCARD ID3D12RootSignature* GetRootSignature	() const noexcept;
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
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC			m_desc{};
	std::vector<D3D12_INPUT_ELEMENT_DESC>		m_inputLayout;
};

#pragma endregion

#pragma region POS_Body

kfe::KFEPipelineState::KFEPipelineState()
	: m_impl(std::make_unique<Impl>())
{
}

kfe::KFEPipelineState::~KFEPipelineState()
{
	if (m_impl)
	{
		m_impl->Destroy();
	}
}

kfe::KFEPipelineState::KFEPipelineState(KFEPipelineState&& other) noexcept
	: m_impl(std::move(other.m_impl))
{
}

kfe::KFEPipelineState& kfe::KFEPipelineState::operator=(KFEPipelineState&& other) noexcept
{
	if (this != &other)
	{
		m_impl = std::move(other.m_impl);
	}
	return *this;
}

_Use_decl_annotations_
std::string kfe::KFEPipelineState::GetName() const noexcept
{
	return "KFEPipelineState";
}

_Use_decl_annotations_
std::string kfe::KFEPipelineState::GetDescription() const noexcept
{
	return "DirectX 12 graphics pipeline state wrapper (PSO builder).";
}

_Use_decl_annotations_
bool kfe::KFEPipelineState::Build(KFEDevice* device) noexcept
{
	if (!m_impl)
	{
		LOG_ERROR("KFEPipelineState::Build: Impl is null.");
		return false;
	}
	return m_impl->Build(device);
}

_Use_decl_annotations_
bool  kfe::KFEPipelineState::IsInitialized() const noexcept
{
	return m_impl && m_impl->IsInitialized();
}

_Use_decl_annotations_
ID3D12PipelineState* kfe::KFEPipelineState::GetNative() const noexcept
{
	return m_impl ? m_impl->GetNative() : nullptr;
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetRootSignature(ID3D12RootSignature* rs) noexcept
{
	if (m_impl) m_impl->SetRootSignature(rs);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetVS(const D3D12_SHADER_BYTECODE& bc) noexcept
{
	if (m_impl) m_impl->SetVS(bc);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetPS(const D3D12_SHADER_BYTECODE& bc) noexcept
{
	if (m_impl) m_impl->SetPS(bc);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetGS(const D3D12_SHADER_BYTECODE& bc) noexcept
{
	if (m_impl) m_impl->SetGS(bc);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetHS(const D3D12_SHADER_BYTECODE& bc) noexcept
{
	if (m_impl) m_impl->SetHS(bc);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetDS(const D3D12_SHADER_BYTECODE& bc) noexcept
{
	if (m_impl) m_impl->SetDS(bc);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetInputLayout(const D3D12_INPUT_ELEMENT_DESC* elem, std::uint32_t count)
{
	if (m_impl) m_impl->SetInputLayout(elem, count);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetRasterizer(const D3D12_RASTERIZER_DESC& rs) noexcept
{
	if (m_impl) m_impl->SetRasterizer(rs);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetBlend(const D3D12_BLEND_DESC& desc) noexcept
{
	if (m_impl) m_impl->SetBlend(desc);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetDepthStencil(const D3D12_DEPTH_STENCIL_DESC& ds) noexcept
{
	if (m_impl) m_impl->SetDepthStencil(ds);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type) noexcept
{
	if (m_impl) m_impl->SetPrimitiveType(type);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetRTVFormat(std::uint32_t slot, DXGI_FORMAT format) noexcept
{
	if (m_impl) m_impl->SetRTVFormat(slot, format);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetRTVCount(std::uint32_t count) noexcept
{
	if (m_impl) m_impl->SetRTVCount(count);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetDSVFormat(DXGI_FORMAT format) noexcept
{
	if (m_impl) m_impl->SetDSVFormat(format);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetSampleDesc(std::uint32_t count, std::uint32_t quality) noexcept
{
	if (m_impl) m_impl->SetSampleDesc(count, quality);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetSampleMask(std::uint32_t mask) noexcept
{
	if (m_impl) m_impl->SetSampleMask(mask);
}

_Use_decl_annotations_
void kfe::KFEPipelineState::SetFlags(D3D12_PIPELINE_STATE_FLAGS flags) noexcept
{
	if (m_impl) m_impl->SetFlags(flags);
}

_Use_decl_annotations_
ID3D12RootSignature* kfe::KFEPipelineState::GetRootSignature() const noexcept
{
	return m_impl ? m_impl->GetRootSignature() : nullptr;
}

_Use_decl_annotations_
D3D12_SHADER_BYTECODE kfe::KFEPipelineState::GetVS() const noexcept
{
	return m_impl ? m_impl->GetVS() : D3D12_SHADER_BYTECODE{};
}

_Use_decl_annotations_
D3D12_SHADER_BYTECODE kfe::KFEPipelineState::GetPS() const noexcept
{
	return m_impl ? m_impl->GetPS() : D3D12_SHADER_BYTECODE{};
}

_Use_decl_annotations_
D3D12_SHADER_BYTECODE kfe::KFEPipelineState::GetGS() const noexcept
{
	return m_impl ? m_impl->GetGS() : D3D12_SHADER_BYTECODE{};
}

_Use_decl_annotations_
D3D12_SHADER_BYTECODE kfe::KFEPipelineState::GetHS() const noexcept
{
	return m_impl ? m_impl->GetHS() : D3D12_SHADER_BYTECODE{};
}

_Use_decl_annotations_
D3D12_SHADER_BYTECODE kfe::KFEPipelineState::GetDS() const noexcept
{
	return m_impl ? m_impl->GetDS() : D3D12_SHADER_BYTECODE{};
}

_Use_decl_annotations_
const D3D12_INPUT_ELEMENT_DESC* kfe::KFEPipelineState::GetInputLayoutElems() const noexcept
{
	return m_impl ? m_impl->GetInputLayoutElems() : nullptr;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEPipelineState::GetInputLayoutCount() const noexcept
{
	return m_impl ? m_impl->GetInputLayoutCount() : 0u;
}

_Use_decl_annotations_
D3D12_RASTERIZER_DESC kfe::KFEPipelineState::GetRasterizer() const noexcept
{
	return m_impl ? m_impl->GetRasterizer() : D3D12_RASTERIZER_DESC{};
}

_Use_decl_annotations_
D3D12_BLEND_DESC kfe::KFEPipelineState::GetBlend() const noexcept
{
	return m_impl ? m_impl->GetBlend() : D3D12_BLEND_DESC{};
}

_Use_decl_annotations_
D3D12_DEPTH_STENCIL_DESC kfe::KFEPipelineState::GetDepthStencil() const noexcept
{
	return m_impl ? m_impl->GetDepthStencil() : D3D12_DEPTH_STENCIL_DESC{};
}

_Use_decl_annotations_
D3D12_PRIMITIVE_TOPOLOGY_TYPE kfe::KFEPipelineState::GetPrimitiveType() const noexcept
{
	return m_impl ? m_impl->GetPrimitiveType() : D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFEPipelineState::GetRTVFormat(std::uint32_t slot) const noexcept
{
	return m_impl ? m_impl->GetRTVFormat(slot) : DXGI_FORMAT_UNKNOWN;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEPipelineState::GetRTVCount() const noexcept
{
	return m_impl ? m_impl->GetRTVCount() : 0u;
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFEPipelineState::GetDSVFormat() const noexcept
{
	return m_impl ? m_impl->GetDSVFormat() : DXGI_FORMAT_UNKNOWN;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEPipelineState::GetSampleCount() const noexcept
{
	return m_impl ? m_impl->GetSampleCount() : 0u;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEPipelineState::GetSampleQuality() const noexcept
{
	return m_impl ? m_impl->GetSampleQuality() : 0u;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEPipelineState::GetSampleMask() const noexcept
{
	return m_impl ? m_impl->GetSampleMask() : 0u;
}

_Use_decl_annotations_
D3D12_PIPELINE_STATE_FLAGS kfe::KFEPipelineState::GetFlags() const noexcept
{
	return m_impl ? m_impl->GetFlags() : D3D12_PIPELINE_STATE_FLAG_NONE;
}

#pragma endregion


#pragma region Impl_Body

kfe::KFEPipelineState::Impl::Impl()
{
	::ZeroMemory(&m_desc, sizeof(m_desc));

	// Basic defaults
	m_desc.pRootSignature		 = nullptr;
	m_desc.VS					 = {};
	m_desc.PS					 = {};
	m_desc.GS					 = {};
	m_desc.HS					 = {};
	m_desc.DS					 = {};
	m_desc.StreamOutput			 = {};
	m_desc.InputLayout			 = { nullptr, 0u };
	m_desc.IBStripCutValue		 = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	m_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_desc.NumRenderTargets		 = 1u;
	m_desc.RTVFormats[0]		 = DXGI_FORMAT_R8G8B8A8_UNORM;

	for (UINT i = 1; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		m_desc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	}

	m_desc.DSVFormat			= DXGI_FORMAT_D32_FLOAT;
	m_desc.SampleDesc.Count		= 1u;
	m_desc.SampleDesc.Quality	= 0u;
	m_desc.SampleMask			= UINT_MAX;
	m_desc.NodeMask				= 0u;
	m_desc.CachedPSO			= {};
	m_desc.Flags				= D3D12_PIPELINE_STATE_FLAG_NONE;

	// Rasterizer default
	D3D12_RASTERIZER_DESC rs{};
	rs.FillMode					= D3D12_FILL_MODE_SOLID;
	rs.CullMode					= D3D12_CULL_MODE_BACK;
	rs.FrontCounterClockwise	= FALSE;
	rs.DepthBias				= D3D12_DEFAULT_DEPTH_BIAS;
	rs.DepthBiasClamp			= D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rs.SlopeScaledDepthBias		= D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rs.DepthClipEnable			= TRUE;
	rs.MultisampleEnable		= FALSE;
	rs.AntialiasedLineEnable	= FALSE;
	rs.ForcedSampleCount		= 0u;
	rs.ConservativeRaster		= D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	m_desc.RasterizerState		= rs;

	// Blend default
	D3D12_BLEND_DESC blend{};
	blend.AlphaToCoverageEnable  = FALSE;
	blend.IndependentBlendEnable = FALSE;

	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		auto& rt				 = blend.RenderTarget[i];
		rt.BlendEnable			 = FALSE;
		rt.LogicOpEnable		 = FALSE;
		rt.SrcBlend				 = D3D12_BLEND_ONE;
		rt.DestBlend			 = D3D12_BLEND_ZERO;
		rt.BlendOp				 = D3D12_BLEND_OP_ADD;
		rt.SrcBlendAlpha		 = D3D12_BLEND_ONE;
		rt.DestBlendAlpha		 = D3D12_BLEND_ZERO;
		rt.BlendOpAlpha			 = D3D12_BLEND_OP_ADD;
		rt.LogicOp				 = D3D12_LOGIC_OP_NOOP;
		rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}
	m_desc.BlendState = blend;

	// Depth Stencil default
	D3D12_DEPTH_STENCIL_DESC ds{};
	ds.DepthEnable					= TRUE;
	ds.DepthWriteMask				= D3D12_DEPTH_WRITE_MASK_ALL;
	ds.DepthFunc					= D3D12_COMPARISON_FUNC_LESS_EQUAL;
	ds.StencilEnable				= FALSE;
	ds.StencilReadMask				= D3D12_DEFAULT_STENCIL_READ_MASK;
	ds.StencilWriteMask				= D3D12_DEFAULT_STENCIL_WRITE_MASK;
	ds.FrontFace.StencilFunc		= D3D12_COMPARISON_FUNC_ALWAYS;
	ds.FrontFace.StencilPassOp		= D3D12_STENCIL_OP_KEEP;
	ds.FrontFace.StencilFailOp		= D3D12_STENCIL_OP_KEEP;
	ds.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	ds.BackFace						= ds.FrontFace;
	m_desc.DepthStencilState		= ds;
}

kfe::KFEPipelineState::Impl::~Impl()
{
	Destroy();
}

_Use_decl_annotations_
bool kfe::KFEPipelineState::Impl::Build(KFEDevice* device) noexcept
{
	if (device == nullptr)
	{
		LOG_ERROR("KFEPipelineState::Impl::Build: device is nullptr.");
		return false;
	}

	ID3D12Device* native = device->GetNative();
	if (native == nullptr)
	{
		LOG_ERROR("KFEPipelineState::Impl::Build: native device is nullptr.");
		return false;
	}

	Destroy();

	const HRESULT hr = native->CreateGraphicsPipelineState(
		&m_desc,
		IID_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())
	);

	if (FAILED(hr))
	{
		LOG_ERROR("KFEPipelineState::Impl::Build: CreateGraphicsPipelineState failed. hr=0x{:08X}", hr);
		m_pipelineState.Reset();
		return false;
	}

	return true;
}

void kfe::KFEPipelineState::Impl::Destroy() noexcept
{
	if (m_pipelineState)
	{
		m_pipelineState.Reset();
	}
}

_Use_decl_annotations_
bool  kfe::KFEPipelineState::Impl::IsInitialized() const noexcept
{
	return m_pipelineState != nullptr;
}

_Use_decl_annotations_
ID3D12PipelineState* kfe::KFEPipelineState::Impl::GetNative() const noexcept
{
	return m_pipelineState.Get();
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetRootSignature(ID3D12RootSignature* rs) noexcept
{
	m_desc.pRootSignature = rs;
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetVS(const D3D12_SHADER_BYTECODE& bc) noexcept
{
	m_desc.VS = bc;
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetPS(const D3D12_SHADER_BYTECODE& bc) noexcept
{
	m_desc.PS = bc;
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetGS(const D3D12_SHADER_BYTECODE& bc) noexcept
{
	m_desc.GS = bc;
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetHS(const D3D12_SHADER_BYTECODE& bc) noexcept
{
	m_desc.HS = bc;
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetDS(const D3D12_SHADER_BYTECODE& bc) noexcept
{
	m_desc.DS = bc;
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetInputLayout(const D3D12_INPUT_ELEMENT_DESC* elem, std::uint32_t count)
{
	m_inputLayout.clear();

	if (elem != nullptr && count > 0)
	{
		m_inputLayout.assign(elem, elem + count);
		m_desc.InputLayout.pInputElementDescs = m_inputLayout.data();
		m_desc.InputLayout.NumElements = static_cast<UINT>(m_inputLayout.size());
	}
	else
	{
		m_desc.InputLayout.pInputElementDescs = nullptr;
		m_desc.InputLayout.NumElements = 0;
	}
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetRasterizer(const D3D12_RASTERIZER_DESC& rs) noexcept
{
	m_desc.RasterizerState = rs;
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetBlend(const D3D12_BLEND_DESC& desc) noexcept
{
	m_desc.BlendState = desc;
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetDepthStencil(const D3D12_DEPTH_STENCIL_DESC& ds) noexcept
{
	m_desc.DepthStencilState = ds;
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type) noexcept
{
	m_desc.PrimitiveTopologyType = type;
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetRTVFormat(std::uint32_t slot, DXGI_FORMAT format) noexcept
{
	if (slot < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)
	{
		m_desc.RTVFormats[slot] = format;
		if (slot + 1 > m_desc.NumRenderTargets)
		{
			m_desc.NumRenderTargets = slot + 1;
		}
	}
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetRTVCount(std::uint32_t count) noexcept
{
	m_desc.NumRenderTargets = static_cast<UINT>(
		std::min<std::uint32_t>(count, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT));
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetDSVFormat(DXGI_FORMAT format) noexcept
{
	m_desc.DSVFormat = format;
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetSampleDesc(std::uint32_t count, std::uint32_t quality) noexcept
{
	m_desc.SampleDesc.Count = count;
	m_desc.SampleDesc.Quality = quality;
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetSampleMask(std::uint32_t mask) noexcept
{
	m_desc.SampleMask = mask;
}

_Use_decl_annotations_
void kfe::KFEPipelineState::Impl::SetFlags(D3D12_PIPELINE_STATE_FLAGS flags) noexcept
{
	m_desc.Flags = flags;
}

_Use_decl_annotations_
ID3D12RootSignature* kfe::KFEPipelineState::Impl::GetRootSignature() const noexcept
{
	return m_desc.pRootSignature;
}

_Use_decl_annotations_
D3D12_SHADER_BYTECODE kfe::KFEPipelineState::Impl::GetVS() const noexcept
{
	return m_desc.VS;
}

_Use_decl_annotations_
D3D12_SHADER_BYTECODE kfe::KFEPipelineState::Impl::GetPS() const noexcept
{
	return m_desc.PS;
}

_Use_decl_annotations_
D3D12_SHADER_BYTECODE kfe::KFEPipelineState::Impl::GetGS() const noexcept
{
	return m_desc.GS;
}

_Use_decl_annotations_
D3D12_SHADER_BYTECODE kfe::KFEPipelineState::Impl::GetHS() const noexcept
{
	return m_desc.HS;
}

_Use_decl_annotations_
D3D12_SHADER_BYTECODE kfe::KFEPipelineState::Impl::GetDS() const noexcept
{
	return m_desc.DS;
}

_Use_decl_annotations_
const D3D12_INPUT_ELEMENT_DESC* kfe::KFEPipelineState::Impl::GetInputLayoutElems() const noexcept
{
	return m_inputLayout.empty() ? nullptr : m_inputLayout.data();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEPipelineState::Impl::GetInputLayoutCount() const noexcept
{
	return static_cast<std::uint32_t>(m_inputLayout.size());
}

_Use_decl_annotations_
D3D12_RASTERIZER_DESC kfe::KFEPipelineState::Impl::GetRasterizer() const noexcept
{
	return m_desc.RasterizerState;
}

_Use_decl_annotations_
D3D12_BLEND_DESC kfe::KFEPipelineState::Impl::GetBlend() const noexcept
{
	return m_desc.BlendState;
}

_Use_decl_annotations_
D3D12_DEPTH_STENCIL_DESC kfe::KFEPipelineState::Impl::GetDepthStencil() const noexcept
{
	return m_desc.DepthStencilState;
}

_Use_decl_annotations_
D3D12_PRIMITIVE_TOPOLOGY_TYPE kfe::KFEPipelineState::Impl::GetPrimitiveType() const noexcept
{
	return m_desc.PrimitiveTopologyType;
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFEPipelineState::Impl::GetRTVFormat(std::uint32_t slot) const noexcept
{
	if (slot < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)
	{
		return m_desc.RTVFormats[slot];
	}
	return DXGI_FORMAT_UNKNOWN;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEPipelineState::Impl::GetRTVCount() const noexcept
{
	return static_cast<std::uint32_t>(m_desc.NumRenderTargets);
}

_Use_decl_annotations_
DXGI_FORMAT kfe::KFEPipelineState::Impl::GetDSVFormat() const noexcept
{
	return m_desc.DSVFormat;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEPipelineState::Impl::GetSampleCount() const noexcept
{
	return static_cast<std::uint32_t>(m_desc.SampleDesc.Count);
}

_Use_decl_annotations_
std::uint32_t kfe::KFEPipelineState::Impl::GetSampleQuality() const noexcept
{
	return static_cast<std::uint32_t>(m_desc.SampleDesc.Quality);
}

_Use_decl_annotations_
std::uint32_t kfe::KFEPipelineState::Impl::GetSampleMask() const noexcept
{
	return m_desc.SampleMask;
}

_Use_decl_annotations_
D3D12_PIPELINE_STATE_FLAGS kfe::KFEPipelineState::Impl::GetFlags() const noexcept
{
	return m_desc.Flags;
}

#pragma endregion
