#include "pch.h"
#include "graphics_queue.h"

#include <wrl.h>
#include <d3d12.h>

#include "engine/core/exception/dx_exception.h"
#include "engine/utils/logger/logger.h"
#include "engine/render_manager/components/device/device.h"

//~ Impl Decalaration
class kfe::KFEGraphicsCmdQ::Impl 
{
public:
	 Impl() = default;
	~Impl() = default;

	[[nodiscard]] bool Initialize(const KFEDevice* device);

	[[nodiscard]] bool				  Release	   ()		noexcept;
	[[nodiscard]] bool				  IsInitialized() const noexcept;
	[[nodiscard]] ID3D12CommandQueue* GetNative	   () const noexcept;

private:
	bool											m_bInitialized{ false };
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>	    m_pQueue{ nullptr };
	constexpr static inline D3D12_COMMAND_LIST_TYPE m_type  { D3D12_COMMAND_LIST_TYPE_DIRECT };
};

//~ Main Implementation
kfe::KFEGraphicsCmdQ::KFEGraphicsCmdQ()
	: m_impl(std::make_unique<kfe::KFEGraphicsCmdQ::Impl>())
{}

kfe::KFEGraphicsCmdQ::~KFEGraphicsCmdQ() = default;

bool kfe::KFEGraphicsCmdQ::Initialize(const KFEDevice* device)
{
	return m_impl->Initialize(device);
}

bool kfe::KFEGraphicsCmdQ::Release() noexcept
{
	return m_impl->Release();
}

bool kfe::KFEGraphicsCmdQ::IsInitialized() const noexcept
{
	return m_impl->IsInitialized();
}

ID3D12CommandQueue* kfe::KFEGraphicsCmdQ::GetNative() const noexcept
{
	return m_impl->GetNative();
}

//~ Impl Implementation
bool kfe::KFEGraphicsCmdQ::Impl::Initialize(const KFEDevice* device)
{
	if (!device || !device->GetNative())
	{
		LOG_ERROR("KFEGraphicsCmdQ::Impl::Initialize Failed Deivce is Null!");
		return false;
	}

	auto* native = device->GetNative();

	D3D12_COMMAND_QUEUE_DESC desc{};
	desc.Flags	  = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0u;
	desc.Type	  = m_type;

	HRESULT hr = native->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_pQueue));
	if (FAILED(hr))
	{
		LOG_ERROR("Failed to Create Graphics Command Queue");
#if defined(DEBUG) || defined(_DEBUG)
		THROW_DX_IF_FAILS(hr);
#endif
		return false;
	}

	m_bInitialized = true;
	return true;
}

bool kfe::KFEGraphicsCmdQ::Impl::Release() noexcept
{
	m_pQueue.Reset();
	m_bInitialized = false;
	return true;
}

ID3D12CommandQueue* kfe::KFEGraphicsCmdQ::Impl::GetNative() const noexcept
{
	return m_pQueue.Get();
}

bool kfe::KFEGraphicsCmdQ::Impl::IsInitialized() const noexcept
{
	return m_bInitialized;
}
