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
#include "engine/render_manager/queue/copy_queue.h"

#include "engine/system/exception/dx_exception.h"
#include "engine/utils/logger.h"
#include "engine/render_manager/components/device.h"

//~ Impl Decalaration
class kfe::KFECopyCmdQ::Impl
{
public:
	 Impl() = default;
	~Impl() = default;

	NODISCARD bool Initialize(const KFEDevice* device);

	NODISCARD bool				  Release	   ()		noexcept;
	NODISCARD bool				  IsInitialized() const noexcept;
	NODISCARD ID3D12CommandQueue* GetNative	   () const noexcept;

private:
	bool											m_bInitialized{ false };
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>	    m_pQueue{ nullptr };
	constexpr static inline D3D12_COMMAND_LIST_TYPE m_type{ D3D12_COMMAND_LIST_TYPE_COPY };
};

//~ Main Implementation
kfe::KFECopyCmdQ::KFECopyCmdQ()
	: m_impl(std::make_unique<kfe::KFECopyCmdQ::Impl>())
{
}

kfe::KFECopyCmdQ::~KFECopyCmdQ() = default;

_Use_decl_annotations_
bool kfe::KFECopyCmdQ::Initialize(const KFEDevice* device)
{
	return m_impl->Initialize(device);
}

_Use_decl_annotations_
bool kfe::KFECopyCmdQ::Release() noexcept
{
	return m_impl->Release();
}

_Use_decl_annotations_
bool kfe::KFECopyCmdQ::IsInitialized() const noexcept
{
	return m_impl->IsInitialized();
}

_Use_decl_annotations_
ID3D12CommandQueue* kfe::KFECopyCmdQ::GetNative() const noexcept
{
	return m_impl->GetNative();
}

//~ Impl Implementation

_Use_decl_annotations_
bool kfe::KFECopyCmdQ::Impl::Initialize(const KFEDevice* device)
{
	if (!device || !device->GetNative())
	{
		LOG_ERROR("KFEGraphicsCmdQ::Impl::Initialize Failed Deivce is Null!");
		return false;
	}

	auto* native = device->GetNative();

	D3D12_COMMAND_QUEUE_DESC desc{};
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0u;
	desc.Type = m_type;

	HRESULT hr = native->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_pQueue));
	if (FAILED(hr))
	{
		LOG_ERROR("Failed to Create Graphics Command Queue");
#if defined(DEBUG) || defined(_DEBUG)
		THROW_DX_IF_FAILS(hr);
#endif
		return false;
	}

	LOG_SUCCESS("Copy Queue Initialized!");
	m_bInitialized = true;
	return true;
}

_Use_decl_annotations_
bool kfe::KFECopyCmdQ::Impl::Release() noexcept
{
	m_pQueue.Reset();
	m_bInitialized = false;
	return true;
}

_Use_decl_annotations_
ID3D12CommandQueue* kfe::KFECopyCmdQ::Impl::GetNative() const noexcept
{
	return m_pQueue.Get();
}

_Use_decl_annotations_
bool kfe::KFECopyCmdQ::Impl::IsInitialized() const noexcept
{
	return m_bInitialized;
}
